// SqScripter scripting widget Program

#include "sqscripter.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/stc/stc.h>
#include <wx/splitter.h>
#include "wx/dynlib.h"
#include <wx/file.h>
#include <wx/aui/auibook.h>
#include <wx/filename.h>

#include <process.h> // for _beginthreadex()


struct ScripterWindowImpl : ScripterWindow{
};


class AddErrorEvent : public wxThreadEvent{
public:
	wxString desc;
	wxString source;
	int line;
	int column;

	AddErrorEvent(wxEventType type, int id, const char *desc, const char *source, int line, int column) : wxThreadEvent(type, id),
		desc(desc), source(source), line(line), column(column)
	{
	}
};

class SqScripterFrame;
class StyledFileTextCtrl;

class SqScripterApp: public wxApp
{
public:
	SqScripterApp() : frame(NULL), handle(NULL){}
	virtual bool OnInit();
	void OnShowWindow(wxThreadEvent& event);
	void OnTerminate(wxThreadEvent&);
	void OnSetLexer(wxThreadEvent&);
	void OnAddError(wxThreadEvent&);
	void OnClearError(wxThreadEvent&);
	void OnPrint(wxThreadEvent&);

	wxBitmap LoadBitmap(const wxString& name);

	SqScripterFrame *frame;
	ScripterWindowImpl *handle;
	wxString resourcePath;
};


class SqScripterFrame: public wxFrame
{
public:
	SqScripterFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	~SqScripterFrame()override;
private:
	void OnRun(wxCommandEvent& event);
	void OnNew(wxCommandEvent& event);
	void OnOpen(wxCommandEvent& event);
	void OnSave(wxCommandEvent& event);
	void OnExit(wxCommandEvent& event);
	void OnAbout(wxCommandEvent& event);
	void OnClose(wxCloseEvent&);
	void OnClear(wxCommandEvent& event);
	void OnWhiteSpaces(wxCommandEvent&);
	void OnLineNumbers(wxCommandEvent&);
	void OnEnterCmd(wxCommandEvent&);
	void OnCmdChar(wxKeyEvent&);
	void OnPageChange(wxAuiNotebookEvent&);
	void OnPageClose(wxAuiNotebookEvent&);
	void OnSavePointReached(wxStyledTextEvent&);
	wxDECLARE_EVENT_TABLE();

	void SetLexer();
	void SetStcLexer(wxStyledTextCtrl *stc);
	void AddError(AddErrorEvent&);
	void ClearError();
	void RecalcLineNumberWidth(int pageIndex = -1);
	void LoadScriptFile(const wxString& fileName);
	void SaveScriptFile(const wxString& fileName);
	void UpdateTitle();
	void SetFileName(const wxString& fileName, bool dirty = false);
	StyledFileTextCtrl *GetPage(size_t i){
		wxWindow *w = note->GetPage(i);
		if(!w)
			return NULL;
		return wxStaticCast(w, StyledFileTextCtrl);
	}
	StyledFileTextCtrl *GetCurrentPage(){
		wxWindow *w = note->GetCurrentPage();
		if(!w)
			return NULL;
		return wxStaticCast(w, StyledFileTextCtrl);
	}

	wxAuiNotebook *note;
	wxFont stcFont;
	wxTextCtrl *log;
	wxLog *logger;
	wxTextCtrl *cmd;
	wxString fileName;

	/// Command history buffer
	wxArrayString cmdHistory; // wxArrayString is (hopefully) more efficient than std::vector<wxString> or such.
	int currentHistory;

	enum{LexUnknown, LexSquirrel} lex;
	int pageIndexGenerator;
	bool dirty;

	friend class SqScripterApp;
};

/// Inherit to create a new class for edit control only because we want to remember the name of file
/// along with edit control.
/// We could have a list of strings in the same order as the wxAuiNotebook's children, but wxAuiNotebook
/// allow the user to change order or even close the pages with GUI, which makes tracking and synchronizing
/// the order between two lists be so difficult.
/// Inheriting and adding a member variable is one of the easiest ways to ensure that additional information
/// always moves along with the page.
class StyledFileTextCtrl : public wxStyledTextCtrl{
public:
	StyledFileTextCtrl(wxWindow *parent, const wxString &fileName, int pageIndex = -1) :
		wxStyledTextCtrl(parent), fileName(fileName), pageIndex(pageIndex), dirty(false){
	}

	wxString GetName()const{
		if(fileName.empty())
			return wxString("(New ") << pageIndex << ")";
		else
			return fileName;
	}

	wxString fileName;
	int pageIndex; ///< For newly ceated, nameless buffers
	bool dirty;
};


enum
{
	ID_Run = 1,
	ID_New,
	ID_Open,
	ID_Save,
	ID_SaveAs,
	ID_Clear,
	ID_WhiteSpaces,
	ID_LineNumbers,
	ID_Command,
	ID_NoteBook
};


wxBEGIN_EVENT_TABLE(SqScripterFrame, wxFrame)
EVT_MENU(ID_Run,   SqScripterFrame::OnRun)
EVT_MENU(ID_New,  SqScripterFrame::OnNew)
EVT_MENU(ID_Open,  SqScripterFrame::OnOpen)
EVT_MENU(ID_Save,  SqScripterFrame::OnSave)
EVT_MENU(ID_SaveAs,  SqScripterFrame::OnSave)
EVT_MENU(ID_Clear,  SqScripterFrame::OnClear)
EVT_MENU(ID_WhiteSpaces, SqScripterFrame::OnWhiteSpaces)
EVT_MENU(ID_LineNumbers, SqScripterFrame::OnLineNumbers)
EVT_MENU(wxID_EXIT,  SqScripterFrame::OnExit)
EVT_MENU(wxID_ABOUT, SqScripterFrame::OnAbout)
EVT_CLOSE(SqScripterFrame::OnClose)
EVT_TEXT_ENTER(ID_Command, SqScripterFrame::OnEnterCmd)
EVT_AUINOTEBOOK_PAGE_CHANGED(ID_NoteBook, SqScripterFrame::OnPageChange)
EVT_AUINOTEBOOK_PAGE_CLOSE(ID_NoteBook, SqScripterFrame::OnPageClose)
EVT_STC_SAVEPOINTREACHED(wxID_ANY, SqScripterFrame::OnSavePointReached)
EVT_STC_SAVEPOINTLEFT(wxID_ANY, SqScripterFrame::OnSavePointReached)
wxEND_EVENT_TABLE()




#ifdef _DLL
wxIMPLEMENT_APP_NO_MAIN(SqScripterApp);

static const int CMD_SHOW_WINDOW = wxNewId();
static const int CMD_TERMINATE = wxNewId();
static const int CMD_SETLEXER = wxNewId();
static const int CMD_ADDERROR = wxNewId();
static const int CMD_CLEARERROR = wxNewId();
static const int CMD_PRINT = wxNewId();


// Critical section that guards everything related to wxWidgets "main" thread
// startup or shutdown.
wxCriticalSection gs_wxStartupCS;
// Handle of wx "main" thread if running, NULL otherwise
HANDLE gs_wxMainThread = NULL;

//  wx application startup code -- runs from its own thread
unsigned wxSTDCALL MyAppLauncher(void* event)
{
	// Note: The thread that called run_wx_gui_from_dll() holds gs_wxStartupCS
	//       at this point and won't release it until we signal it.

	// We need to pass correct HINSTANCE to wxEntry() and the right value is
	// HINSTANCE of this DLL, not of the main .exe, use this MSW-specific wx
	// function to get it. Notice that under Windows XP and later the name is
	// not needed/used as we retrieve the DLL handle from an address inside it
	// but you do need to use the correct name for this code to work with older
	// systems as well.
	const HINSTANCE
		hInstance = wxDynamicLibrary::MSWGetModuleHandle("my_dll",
			&gs_wxMainThread);
	if ( !hInstance )
		return 0; // failed to get DLL's handle

				  // wxIMPLEMENT_WXWIN_MAIN does this as the first thing
	wxDISABLE_DEBUG_SUPPORT();

	// We do this before wxEntry() explicitly, even though wxEntry() would
	// do it too, so that we know when wx is initialized and can signal
	// run_wx_gui_from_dll() about it *before* starting the event loop.
	wxInitializer wxinit;
	if ( !wxinit.IsOk() )
		return 0; // failed to init wx

				  // Signal run_wx_gui_from_dll() that it can continue
	HANDLE hEvent = *(static_cast<HANDLE*>(event));
	if ( !SetEvent(hEvent) )
		return 0; // failed setting up the mutex

				  // Run the app:
	wxEntry(hInstance);

	return 1;
}

static void PrintProc(ScripterWindow *, const char *text){
	// Send a message to wx thread to show a new frame:
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_PRINT);
	event->SetString(text);
	wxQueueEvent(wxApp::GetInstance(), event);
}

ScripterWindow *scripter_init(const ScripterConfig *sc){
	// In order to prevent conflicts with hosting app's event loop, we
	// launch wx app from the DLL in its own thread.
	//
	// We can't even use wxInitializer: it initializes wxModules and one of
	// the modules it handles is wxThread's private module that remembers
	// ID of the main thread. But we need to fool wxWidgets into thinking that
	// the thread we are about to create now is the main thread, not the one
	// from which this function is called.
	//
	// Note that we cannot use wxThread here, because the wx library wasn't
	// initialized yet. wxCriticalSection is safe to use, though.

	wxCriticalSectionLocker lock(gs_wxStartupCS);

	if ( !gs_wxMainThread )
	{
		HANDLE hEvent = CreateEvent
			(
				NULL,  // default security attributes
				FALSE, // auto-reset
				FALSE, // initially non-signaled
				NULL   // anonymous
				);
		if ( !hEvent )
			return NULL; // error

					// NB: If your compiler doesn't have _beginthreadex(), use CreateThread()
		gs_wxMainThread = (HANDLE)_beginthreadex
			(
				NULL,           // default security
				0,              // default stack size
				&MyAppLauncher,
				&hEvent,        // arguments
				0,              // create running
				NULL
				);

		if ( !gs_wxMainThread )
		{
			CloseHandle(hEvent);
			return NULL; // error
		}

		// Wait until MyAppLauncher signals us that wx was initialized. This
		// is because we use wxMessageQueue<> and wxString later and so must
		// be sure that they are in working state.
		WaitForSingleObject(hEvent, INFINITE);
		CloseHandle(hEvent);
	}

	// The scripting window object does mean nothing now since the wnidow is managed by
	// global object in the DLL, but return it anyway in order to keep the API unchanged.
	ScripterWindowImpl *ret = new ScripterWindowImpl;
	ret->config = *sc;
	wxGetApp().handle = ret;
	*sc->printProc = PrintProc;

	return ret;
}

bool scripter_set_resource_path(ScripterWindow *, const char *path){
	if(!wxTheApp)
		return false; // Call scripter_init() first!
	wxGetApp().resourcePath = path;
	return true;
}

int scripter_show(ScripterWindow *){
	// Send a message to wx thread to show a new frame:
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_SHOW_WINDOW);
	wxQueueEvent(wxApp::GetInstance(), event);

	return 0;
}

int scripter_lexer_squirrel(ScripterWindow *){
	// Send a message to wx thread to show a new frame:
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_SETLEXER);
	wxQueueEvent(wxApp::GetInstance(), event);

	return 0;
}

/// \brief Add a script error indicator to specified line in a specified source file.
void scripter_adderror(ScripterWindow*, const char *desc, const char *source, int line, int column){
	wxThreadEvent *event =
		new AddErrorEvent(wxEVT_THREAD, CMD_ADDERROR, desc, source, line, column);
	wxQueueEvent(wxApp::GetInstance(), event);
}

void scripter_clearerror(ScripterWindow*){
	wxThreadEvent *event =
		new wxThreadEvent(wxEVT_THREAD, CMD_CLEARERROR);
	wxQueueEvent(wxApp::GetInstance(), event);
}

void scripter_delete(ScripterWindow *sw){
	SqScripterApp& app = wxGetApp();
	if(sw == app.handle)
		app.handle = NULL;
	delete sw;
}
#else
wxIMPLEMENT_APP(SqScripterApp);
#endif


/// Try to load a file with given name from resource path.
/// Also does some error handling for failure on loading image.
wxBitmap SqScripterApp::LoadBitmap(const wxString& name){
	wxImage im = wxFileName(resourcePath, name).GetFullPath();
	if(im.IsOk())
		return im;
	else // Return a placeholder bitmap even if the image fails to load
		return wxBitmap(32, 32);
}

bool SqScripterApp::OnInit()
{
	wxInitAllImageHandlers();
#ifdef _DLL
	// Keep the wx "main" thread running even without windows. This greatly
	// simplifies threads handling, because we don't have to correctly
	// implement wx-thread restarting.
	//
	// Note that this only works if you don't explicitly call ExitMainLoop(),
	// except in reaction to wx_dll_cleanup()'s message. wx_dll_cleanup()
	// relies on the availability of wxApp instance and if the event loop
	// terminated, wxEntry() would return and wxApp instance would be
	// destroyed.
	//
	// Also note that this is efficient, because if there are no windows, the
	// thread will sleep waiting for a new event. We could safe some memory
	// by shutting the thread down when it's no longer needed, though.
	SetExitOnFrameDelete(false);

	Connect(CMD_SHOW_WINDOW,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnShowWindow));
	Connect(CMD_TERMINATE,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnTerminate));
	Connect(CMD_SETLEXER,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnSetLexer));
	Connect(CMD_ADDERROR,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnAddError));
	Connect(CMD_CLEARERROR,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnClearError));
	Connect(CMD_PRINT,
		wxEVT_THREAD,
		wxThreadEventHandler(SqScripterApp::OnPrint));
#else
	frame = new SqScripterFrame( "SqScripter", wxPoint(50, 50), wxSize(450, 540) );
	frame->Show( true );
#endif
	return true;
}

void SqScripterApp::OnShowWindow(wxThreadEvent&){
	// Postpone creation of the frame since the caller may call scripter_set_resource_path() after SqScripter::OnInit().
	// The safe time to create is when the user of the library calls scripter_show().
	if(!frame)
		frame = new SqScripterFrame( "SqScripter", wxPoint(50, 50), wxSize(450, 540) );
	frame->Show(true);
}

void SqScripterApp::OnTerminate(wxThreadEvent&){
	ExitMainLoop();
}

void SqScripterApp::OnSetLexer(wxThreadEvent&){
	if(frame)
		frame->SetLexer();
}

void SqScripterApp::OnAddError(wxThreadEvent& evt){
	AddErrorEvent& ae = static_cast<AddErrorEvent&>(evt);
	if(frame)
		frame->AddError(ae);
}

void SqScripterApp::OnClearError(wxThreadEvent&){
	if(frame)
		frame->ClearError();
}

void SqScripterApp::OnPrint(wxThreadEvent& evt){
	if(frame)
		*frame->log << evt.GetString();
}

SqScripterFrame::SqScripterFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(NULL, wxID_ANY, title, pos, size),
	log(NULL), logger(NULL), dirty(false), pageIndexGenerator(0)
{
	wxMenu *menuFile = new wxMenu;
	menuFile->Append(ID_Run, "&Run\tCtrl-R", "Run the program");
	menuFile->AppendSeparator();
	menuFile->Append(ID_New, "&New\tCtrl-N", "Create a new buffer");
	menuFile->Append(ID_Open, "&Open\tCtrl-O", "Open a file");
	menuFile->Append(ID_Save, "&Save\tCtrl-S", "Save and overwrite the file");
	menuFile->Append(ID_SaveAs, "&Save As..\tCtrl-Shift-S", "Save to a file with a different name");
	menuFile->AppendSeparator();
	menuFile->Append(ID_Clear, "&Clear Log\tCtrl-C", "Clear output log");
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	wxMenu *menuView = new wxMenu;
	menuView->AppendCheckItem(ID_WhiteSpaces, "Toggle Whitespaces\tCtrl-W", "Toggles show state of whitespaces");
	menuView->AppendCheckItem(ID_LineNumbers, "Toggle Line Numbers\tCtrl-L", "Toggles show state of line numbers");
	menuView->Check(ID_LineNumbers, true); // Default is true
	wxMenu *menuHelp = new wxMenu;
	menuHelp->Append(wxID_ABOUT);
	wxMenuBar *menuBar = new wxMenuBar;
	menuBar->Append( menuFile, "&File" );
	menuBar->Append( menuView, "&View" );
	menuBar->Append( menuHelp, "&Help" );
	SetMenuBar( menuBar ); // Menu bar must be set before first call to SetStcLexer()

	wxSplitterWindow *splitter = new wxSplitterWindow(this);
	// Script editing pane almost follows the window size, while the log pane occupies surplus area.
	splitter->SetSashGravity(0.75);
	splitter->SetMinimumPaneSize(40);

	note = new wxAuiNotebook(splitter, ID_NoteBook);

	// Set default font attributes and tab width.  They should really be configurable.
	stcFont = wxFont(wxFontInfo(10).Family(wxFONTFAMILY_TELETYPE));

	OnNew(wxCommandEvent());

	log = new wxTextCtrl(splitter, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	log->SetFont(stcFont);
	splitter->SplitHorizontally(note, log, 200);

	cmd = new wxTextCtrl(this, ID_Command, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	cmd->Bind(wxEVT_CHAR_HOOK, &SqScripterFrame::OnCmdChar, this);
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(splitter, 1, wxEXPAND | wxALL);
	sizer->Add(cmd, 0, wxEXPAND | wxBOTTOM);
	SetSizer(sizer);

	SqScripterApp &app = wxGetApp();

	wxToolBar *toolbar = CreateToolBar();
	toolbar->AddTool(ID_Run, "Run", app.LoadBitmap("run.png"), "Run the program");
	toolbar->AddTool(ID_New, "New", app.LoadBitmap("new.png"), "Create a new buffer");
	toolbar->AddTool(ID_Open, "Open", app.LoadBitmap("open.png"), "Open a file");
	toolbar->AddTool(ID_Save, "Save", app.LoadBitmap("save.png"), "Save a file");
	toolbar->AddTool(ID_SaveAs, "SaveAs", app.LoadBitmap("saveas.png"), "Save with a new name");
	toolbar->AddTool(ID_Clear, "Clear", app.LoadBitmap("clear.png"), "Clear output log");
	toolbar->Realize();
	CreateStatusBar();
	SetStatusText( "" );

	logger = new wxLogStream(&std::cout);
}

SqScripterFrame::~SqScripterFrame(){
	delete note;
	delete log;
	delete logger;
}

void SqScripterFrame::SetLexer(){
	lex = LexSquirrel;
	wxWindowList stcList = GetChildren();
	for(size_t i = 0; i < note->GetPageCount(); i++){
		StyledFileTextCtrl *stc = GetPage(i);
		if(!stc)
			continue;
		SetStcLexer(stc);
	}
}

void SqScripterFrame::SetStcLexer(wxStyledTextCtrl *stc){
	stc->SetFont(stcFont);
	stc->StyleSetFont(wxSTC_STYLE_DEFAULT, stcFont);
	stc->SetTabWidth(4);
	stc->SetViewWhiteSpace(GetMenuBar()->IsChecked(ID_WhiteSpaces) ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
	stc->SetWhitespaceForeground(true, wxColour(0x7f, 0xbf, 0xbf));
	if(lex != LexSquirrel)
		return;
	stc->SetLexer(wxSTC_LEX_CPP);
	stc->StyleSetForeground(wxSTC_C_COMMENT, wxColour(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTLINE, wxColour(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOC, wxColour(0,127,0));
	stc->StyleSetForeground(wxSTC_C_COMMENTLINEDOC, wxColour(0,127,63));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORD, wxColour(0,63,127));
	stc->StyleSetForeground(wxSTC_C_COMMENTDOCKEYWORDERROR, wxColour(255,0,0));
	stc->StyleSetForeground(wxSTC_C_NUMBER, wxColour(0,127,255));
	stc->StyleSetForeground(wxSTC_C_WORD, wxColour(0,0,255));
	stc->StyleSetForeground(wxSTC_C_STRING, wxColour(127,0,0));
	stc->StyleSetForeground(wxSTC_C_CHARACTER, wxColour(127,0,127));
	stc->SetKeyWords(0,
		"base break case catch class clone "
		"continue const default delete else enum "
		"extends for foreach function if in "
		"local null resume return switch this "
		"throw try typeof while yield constructor "
		"instanceof true false static ");
	stc->SetKeyWords(2,
		"a addindex addtogroup anchor arg attention author authors b brief bug "
		"c callgraph callergraph category cite class code cond copybrief copydetails "
		"copydoc copyright date def defgroup deprecated details diafile dir docbookonly "
		"dontinclude dot dotfile e else elseif em endcode endcond enddocbookonly enddot "
		"endhtmlonly endif endinternal endlatexonly endlink endmanonly endmsc endparblock "
		"endrtfonly endsecreflist endverbatim enduml endxmlonly enum example exception extends "
		"f$ f[ f] f{ f} file fn headerfile hideinitializer htmlinclude htmlonly idlexcept if "
		"ifnot image implements include includelineno ingroup internal invariant interface "
		"latexinclude latexonly li line link mainpage manonly memberof msc mscfile n name "
		"namespace nosubgrouping note overload p package page par paragraph param parblock "
		"post pre private privatesection property protected protectedsection protocol public "
		"publicsection pure ref refitem related relates relatedalso relatesalso remark remarks "
		"result return returns retval rtfonly sa secreflist section see short showinitializer "
		"since skip skipline snippet startuml struct subpage subsection subsubsection "
		"tableofcontents test throw throws todo tparam typedef union until var verbatim "
		"verbinclude version vhdlflow warning weakgroup xmlonly xrefitem $ @ \\ & ~ < > # %");
}

void SqScripterFrame::AddError(AddErrorEvent &ae){
	wxWindow *w = note->GetCurrentPage();
	if(!w)
		return;
	wxStyledTextCtrl *stc = static_cast<wxStyledTextCtrl*>(w);
	if(stc){
		if(strcmp(ae.source, "scriptbuf") /*&& strcmp(ae.source, p->GetCurrentBuffer().fileName.c_str())*/)
			LoadScriptFile(ae.source);

		int pos = stc->PositionFromLine(ae.line - 1);
		stc->IndicatorSetStyle(0, wxSTC_INDIC_SQUIGGLE);
		stc->IndicatorSetForeground(0, wxColour(255,0,0));
		stc->SetIndicatorCurrent(0);
		stc->IndicatorFillRange(pos + ae.column - 1, stc->LineLength(0) - (ae.column - 1));
		stc->MarkerDefine(0, wxSTC_MARK_CIRCLE);
		stc->MarkerSetForeground(0, wxColour(255,0,0));
		stc->MarkerAdd(ae.line - 1, 0);
	}
}

void SqScripterFrame::ClearError(){
	wxWindow *w = note->GetCurrentPage();
	if(!w)
		return;
	wxStyledTextCtrl *stc = static_cast<wxStyledTextCtrl*>(w);
	if(stc){
		stc->IndicatorClearRange(0, stc->GetLength());
		stc->MarkerDeleteAll(0);
	}
}

/// Calculate width of line numbers margin by contents
void SqScripterFrame::RecalcLineNumberWidth(int pageIndex){
	StyledFileTextCtrl *stc = pageIndex < 0 ? GetCurrentPage() : GetPage(pageIndex);
	if(!stc)
		return;
	bool showState = GetMenuBar()->IsChecked(ID_LineNumbers);

	// Make sure to set type of the first margin to be line numbers
	stc->SetMarginType(0, wxSTC_MARGIN_NUMBER);

	// Obtain width needed to display all line count in the buffer
	int lineCount = stc->GetLineCount();
	wxString lineText = "_";
	for(int i = 0; pow(10, i) <= lineCount; i++)
		lineText += "9";

	int width = showState ? stc->TextWidth(wxSTC_STYLE_LINENUMBER, lineText) : 0;
	stc->SetMarginWidth(0, width);
}

void SqScripterFrame::LoadScriptFile(const wxString& fileName){

	// Reset status to create an empty buffer if file name is absent
	if(fileName.empty()){
		SetFileName("");
		RecalcLineNumberWidth();
		return;
	}

	// Find a buffer with the same file name and select it instead of creating a new buffer.
	int existingPage = -1;
	for(int i = 0; i < note->GetPageCount(); i++){
		StyledFileTextCtrl *stc = GetPage(i);
		if(stc && stc->fileName == fileName){
			existingPage = i;
			break;
		}
	}

	wxFile file(fileName, wxFile::read);
	if(file.IsOpened()){
		wxString str;
		if(file.ReadAll(&str)){
			wxStyledTextCtrl *stc;
			if(existingPage < 0){
				stc = new StyledFileTextCtrl(note, fileName);
				SetStcLexer(stc);

				note->AddPage(stc, wxFileName(fileName).GetFullName(), true, 0);
			}
			else{
				stc = GetPage(size_t(existingPage));
				note->SetSelection(size_t(existingPage));
			}

			stc->SetText(str);

			// Clear undo buffer instead of setting a savepoint because we don't want to undo to empty document
			// if it's opened from a file.
			stc->EmptyUndoBuffer();
			// For some reason, emptying the undo buffer does not trigger the savepoint reached event, so
			// we have to manually invoke it.
			stc->SetSavePoint();

			RecalcLineNumberWidth();
		}
	}
}

void SqScripterFrame::SaveScriptFile(const wxString& fileName){
	StyledFileTextCtrl *stc = GetCurrentPage();
	if(!stc)
		return;
	wxFile hFile(fileName, wxFile::write);
	if(hFile.IsOpened()){
		wxString text = stc->GetText();
		hFile.Write(text);
		// Set savepoint for buffer dirtiness management
		stc->SetSavePoint();
		SetFileName(fileName); // Update the title string
		stc->fileName = fileName; // Remember the file name for the next save operation
		// Update the tab text
		note->SetPageText(note->GetPageIndex(stc), wxFileName(fileName).GetFullName());
	}
}


void SqScripterFrame::UpdateTitle(){
	wxString title = "Scripting Window ";
	StyledFileTextCtrl *stc = GetCurrentPage();
	if(stc){
		if(stc->dirty)
			title += "* ";
		if(!stc->fileName.empty())
			title += "(" + stc->fileName + ")";
	}
	SetTitle(title);
}

void SqScripterFrame::SetFileName(const wxString& fileName, bool dirty){
	this->fileName = fileName;
	this->dirty = dirty;

	UpdateTitle();
}

void SqScripterFrame::OnExit(wxCommandEvent& event)
{
	Close( true );
}

void SqScripterFrame::OnAbout(wxCommandEvent& event)
{
	wxMessageBox( "SqScripter powered by wxWidgets & Scintilla",
		"About SqScripter", wxOK | wxICON_INFORMATION );
}

void SqScripterFrame::OnClose(wxCloseEvent& event){
	bool canceled = false;
	// Confirm all dirty buffers before closing
	for(size_t i = 0; i < note->GetPageCount(); i++){
		StyledFileTextCtrl *stc = GetPage(i);
		if(stc && stc->dirty && wxMessageBox(wxString("Changes to ") << stc->GetName() << " is not saved. OK to close?", "Scripting Window", wxOK | wxCANCEL) != wxOK){
			canceled = true;
			break;
		}
	}
	if(!canceled){
		ScripterWindowImpl *handle = wxGetApp().handle;
		if(handle->config.onClose)
			handle->config.onClose(handle);
	}
	else
		event.Veto();
}


void SqScripterFrame::OnClear(wxCommandEvent& event){
	log->Clear();
}

void SqScripterFrame::OnWhiteSpaces(wxCommandEvent& event){
	// Update all editor control in all pages
	for(int i = 0; i < note->GetPageCount(); i++){
		StyledFileTextCtrl *stc = GetPage(i);
		if(stc)
			stc->SetViewWhiteSpace(event.IsChecked() ? wxSTC_WS_VISIBLEALWAYS : wxSTC_WS_INVISIBLE);
	}
}

void SqScripterFrame::OnLineNumbers(wxCommandEvent& event){
	// Update all editor control in all pages
	for(int i = 0; i < note->GetPageCount(); i++)
		RecalcLineNumberWidth(i);
}

void SqScripterFrame::OnEnterCmd(wxCommandEvent& event){
	wxLog* oldLogger = wxLog::SetActiveTarget(logger);
	wxStreamToTextRedirector redirect(log);
	wxString cmdStr = cmd->GetValue();
	if(wxGetApp().handle && wxGetApp().handle->config.commandProc)
		wxGetApp().handle->config.commandProc(cmdStr);
	else
		wxLogMessage("Execute the command: " + cmdStr + "\n");
	// Remember issued command in the history buffer only if the command is not repeated.
	if(cmdHistory.empty() || cmdHistory.back() != cmdStr){
		cmdHistory.push_back(cmdStr);
		currentHistory = -1;
	}
	cmd->Clear();
	wxLog::SetActiveTarget(oldLogger);
}

/// Event handler for Command line control character input.
void SqScripterFrame::OnCmdChar(wxKeyEvent& event){
	if(event.GetEventObject() == cmd){
		if((event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_UP) && cmdHistory.size() != 0){
			if(currentHistory == -1)
				currentHistory = (int)cmdHistory.size() - 1;
			else
				currentHistory = (int)(currentHistory + (event.GetKeyCode() == WXK_DOWN ? -1 : 1) + cmdHistory.size()) % cmdHistory.size();
			cmd->SetValue(cmdHistory[currentHistory]);
			// Set the caret to the end, because DOS or ssh command line users usually expect this behavior.
			cmd->SetInsertionPointEnd();
			return; // Return without skipping
		}
	}
	event.Skip();
}

void SqScripterFrame::OnRun(wxCommandEvent& event)
{
	wxWindow *w = note->GetCurrentPage();
	if(!w)
		return;
	wxStyledTextCtrl *stc = static_cast<wxStyledTextCtrl*>(w);
	if(!stc)
		return;

	wxLog* oldLogger = wxLog::SetActiveTarget(logger);
	wxStreamToTextRedirector redirect(log);
	if(wxGetApp().handle && wxGetApp().handle->config.runProc)
		wxGetApp().handle->config.runProc(fileName, stc->GetText());
	else
		wxLogMessage("Run the program");
	wxLog::SetActiveTarget(oldLogger);
}

void SqScripterFrame::OnNew(wxCommandEvent&)
{
	wxStyledTextCtrl *stc = new StyledFileTextCtrl(note, "", pageIndexGenerator);
	SetStcLexer(stc);
	note->AddPage(stc, wxString("(New ") << pageIndexGenerator++ << ")", true, 0);
	SetFileName("");
	RecalcLineNumberWidth();
}

void SqScripterFrame::OnOpen(wxCommandEvent& event)
{
	ScripterWindowImpl *handle = wxGetApp().handle;
	wxFileDialog openFileDialog(this, _("Open NUT file"), "", "",
		handle && handle->config.sourceFilters ? handle->config.sourceFilters : "Squirrel source files (*.nut)|*.nut", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;

	LoadScriptFile(openFileDialog.GetPath());
}

void SqScripterFrame::OnSave(wxCommandEvent& event)
{
	StyledFileTextCtrl *stc = GetCurrentPage();
	ScripterWindowImpl *handle = wxGetApp().handle;
	wxFileDialog openFileDialog(this, _("Save NUT file"), "", stc ? stc->fileName : "",
		handle && handle->config.sourceFilters ? handle->config.sourceFilters : "Squirrel source files (*.nut)|*.nut", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

	// Query a file name only if the buffer is a new buffer or the user selected "Save As..."
	if(stc->fileName.empty() || event.GetId() == ID_SaveAs){
		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return;
		SaveScriptFile(openFileDialog.GetPath());
	}
	else
		SaveScriptFile(stc->fileName);
}

void SqScripterFrame::OnPageChange(wxAuiNotebookEvent& ){
	UpdateTitle();
}

void SqScripterFrame::OnPageClose(wxAuiNotebookEvent& event){
	StyledFileTextCtrl *stc = GetCurrentPage();
	if(stc && stc->dirty && wxMessageBox(wxString("Changes to ") << stc->GetName() << " is not saved. OK to close?", "Scripting Window", wxOK | wxCANCEL) != wxOK){
		event.Veto();
	}
}

/// Update save point status (whether the document is dirty i.e. need to be saved before closing)
void SqScripterFrame::OnSavePointReached(wxStyledTextEvent& event){
	// This event handler is shared among "reached" and "left" events, so
	// query the event type and change the title accordingly.
	bool reached = event.GetEventType() == wxEVT_STC_SAVEPOINTREACHED;
	StyledFileTextCtrl *stc = wxStaticCast(event.GetEventObject(), StyledFileTextCtrl);
	stc->dirty = !reached;

	wxWindow *w = note->GetCurrentPage();
	// Sometimes controls in hidden pages can send the event, but we do not want to update
	// the main window title by events from them, so update only if the event is sent from current page.
	// Note that StyledText controls have indeterminant IDs (wxID_ANY) because there can be arbitrary
	// number of controls, so we cannot rely on them.
	if(w && w == stc){
		UpdateTitle();
	}
}
