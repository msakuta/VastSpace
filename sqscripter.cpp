#include "sqscripter.h"
#include "scintilla/include/Scintilla.h"
#include "scintilla/include/SciLexer.h"

#include <windows.h>
#include "resource.h"
#include <CommCtrl.h>

#include <string>
#include <sstream>
#include <vector>
#include <deque>


// Because ResEdit erases unused IDs from resource.h automatically, we cannot define
// the dynamic (runtime generated) control IDs in there, unless we use a placeholder
// control in the resource for the sole purpose of reserving the ID, which would be
// counterintuitive. So we define the value here.
// This ID is only used for Scintilla component notification messages, so multiple
// controls can share this ID (individual control is identified by window handles).
// The problem is that it's hard to keep it unique from all the other IDs in resource.h.
#define IDC_SCRIPTEDIT_DYNAMIC 1998


struct ScripterWindowImpl : ScripterWindow{
	/// Window handle for the scripting window.
	HWND hwndScriptDlg;

	/// Remembered default edit control's window procedure
	WNDPROC defEditWndProc;

	/// Command history buffer
	std::vector<char*> cmdHistory;
	int currentHistory;

	struct Buffer{
		std::string fileName;
		std::string contents;
		HWND hEdit;
		bool dirty;
		Buffer() : dirty(false){}
	};

	std::deque<Buffer> buffers;
	int activeBuffer;

	ScripterWindowImpl() : activeBuffer(0){}

	void print(const char *line);

	INT_PTR ScriptCommandProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void UpdateTitle(){
		std::string title = "Scripting Window ";
		if(0 <= activeBuffer && activeBuffer < buffers.size()){
			Buffer &buf = buffers[activeBuffer];
			if(buf.dirty)
				title += "* ";
			if(!buf.fileName.empty())
				title += "(" + buf.fileName + ")";
		}
		SetWindowTextA(hwndScriptDlg, title.c_str());
	}

	void SetFileName(const char *fileName, bool dirty = false){
		if(0 <= activeBuffer && activeBuffer < buffers.size()){
			Buffer &buf = buffers[activeBuffer];
			buf.fileName = fileName;
			buf.dirty = dirty;

			// Update current buffer's tab title string, too
			TCITEMA tcitem;
			tcitem.mask = TCIF_TEXT;
			const char *pstr = strrchr(fileName, '\\');
			std::string bufName = GetBufferName(activeBuffer);
			tcitem.pszText = const_cast<LPSTR>(bufName.c_str());
			SendMessageA(GetDlgItem(hwndScriptDlg, IDC_TABBUFFER), TCM_SETITEMA, activeBuffer, (LPARAM)&tcitem);
		}
		UpdateTitle();
	}

	/// Update save point status (whether the document is dirty i.e. need to be saved before closing)
	void SavePoint(bool reached){
		if(0 <= activeBuffer && activeBuffer < buffers.size()){
			Buffer &buf = buffers[activeBuffer];
			buf.dirty = !reached;
		}
		UpdateTitle();
	}

	Buffer &GetCurrentBuffer(){
		if(0 <= activeBuffer && activeBuffer < buffers.size()){
			Buffer &buf = buffers[activeBuffer];
		}
		else
			throw std::exception("No active buffer");
	}

	void SetCurrentBuffer(int index);
	std::string GetBufferName(int index);

	Buffer &AddBuffer(const char *fileName);

	void SetLexerSingle(HWND hEdit);
	void SetLexer();
};

static void PrintProc(ScripterWindow *p, const char *s){
	static_cast<ScripterWindowImpl*>(p)->print(s);
}

ScripterWindow *scripter_init(const ScripterConfig *sc){
	*sc->printProc = PrintProc;
	ScripterWindowImpl *ret = new ScripterWindowImpl;
	ret->config = *sc;
	ret->hwndScriptDlg = NULL;
	ret->defEditWndProc = NULL;
	ret->currentHistory = 0;
	return ret;
}


/// Event handler function to receive printed messages in the console for displaying on scripting window.
void ScripterWindowImpl::print(const char *line){
	if(IsWindow(hwndScriptDlg)){
		HWND hEdit = GetDlgItem(hwndScriptDlg, IDC_CONSOLE);
		size_t buflen = GetWindowTextLengthA(hEdit);
		SendMessageA(hEdit, EM_SETSEL, buflen, buflen);
		std::string s = line;
		s += "\r\n";
		SendMessageA(hEdit, EM_REPLACESEL, 0, (LPARAM) s.c_str());
	}
}

void ScripterWindowImpl::SetCurrentBuffer(int index){
	if(0 <= index && index < buffers.size()){
		// Hide old buffer
		if(0 <= activeBuffer && activeBuffer < buffers.size()){
			ShowWindow(buffers[activeBuffer].hEdit, SW_HIDE);
		}

		activeBuffer = index;
		Buffer &buf = buffers[index];
		UpdateTitle();
		ShowWindow(buf.hEdit, SW_SHOW); // Show new buffer
	}
}

std::string ScripterWindowImpl::GetBufferName(int index){
	if(0 <= index && index < buffers.size()){
		if(buffers[index].fileName.empty()){
			std::stringstream ss;
			ss << "(New " << index << ")";
			return ss.str();
		}
		else{
			size_t pos = buffers[index].fileName.rfind('\\');
			return pos == std::string::npos ? buffers[index].fileName : buffers[index].fileName.substr(pos + 1);
		}
	}
	else
		throw std::exception("No active buffer");
}

ScripterWindowImpl::Buffer &ScripterWindowImpl::AddBuffer(const char *fileName){
	buffers.push_back(ScripterWindowImpl::Buffer());
	Buffer *buf = &buffers.back();
	buf->fileName = fileName;

	HWND hRun = GetDlgItem(hwndScriptDlg, IDOK);
	HWND hScr = GetDlgItem(hwndScriptDlg, IDC_SCRIPTEDIT);
	RECT cr;
	GetClientRect(hwndScriptDlg, &cr);
	RECT scr;
	GetWindowRect(hScr, &scr);
	POINT posScr = {scr.left, scr.top};
	ScreenToClient(hwndScriptDlg, &posScr);
	RECT runr;
	GetWindowRect(hRun, &runr);
	buf->hEdit = CreateWindowExW(0,
		L"Scintilla", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,
		posScr.x, posScr.y, posScr.x + scr.right - scr.left, posScr.y + scr.bottom - scr.top, hwndScriptDlg,
		(HMENU)IDC_SCRIPTEDIT_DYNAMIC, GetModuleHandle(NULL), NULL);
	if(!buf->hEdit)
		throw std::exception("Couldn't create editor component");

	// Adjust position as in WM_SIZE
	SetWindowPos(buf->hEdit, NULL, 0, 0, cr.right - posScr.x - (runr.right - runr.left) - 10,
		scr.bottom - scr.top, SWP_NOMOVE);

	SetLexerSingle(buf->hEdit);

	TCITEMA tcitem;
	tcitem.mask = TCIF_TEXT;
	const char *pstr = strrchr(fileName, '\\');
	tcitem.pszText = const_cast<LPSTR>(fileName[0] == '\0' ? "(New)" : pstr ? pstr+1 : fileName);
	SendMessageA(GetDlgItem(hwndScriptDlg, IDC_TABBUFFER), TCM_INSERTITEMA, buffers.size() - 1, (LPARAM)&tcitem);
	SetCurrentBuffer(buffers.size() - 1);
	SendMessageA(GetDlgItem(hwndScriptDlg, IDC_TABBUFFER), TCM_SETCURSEL, buffers.size() - 1, 0);

	return *buf;
}

inline int SciRGB(unsigned char r, unsigned char g, unsigned char b){
	return r | (g << 8) | (b << 16);
}

void ScripterWindowImpl::SetLexerSingle(HWND hScriptEdit){
	SendMessage(hScriptEdit, SCI_SETLEXER, SCLEX_CPP, 0);
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_COMMENT, SciRGB(0,127,0));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_COMMENTLINE, SciRGB(0,127,0));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_COMMENTDOC, SciRGB(0,127,0));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_COMMENTLINEDOC, SciRGB(0,127,63));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_COMMENTDOCKEYWORD, SciRGB(0,63,127));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_COMMENTDOCKEYWORDERROR, SciRGB(255,0,0));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_NUMBER, SciRGB(0,127,255));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_WORD, SciRGB(0,0,255));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_STRING, SciRGB(127,0,0));
	SendMessage(hScriptEdit, SCI_STYLESETFORE, SCE_C_CHARACTER, SciRGB(127,0,127));
	SendMessage(hScriptEdit, SCI_SETKEYWORDS, 0,
		(LPARAM)"base break case catch class clone "
		"continue const default delete else enum "
		"extends for foreach function if in "
		"local null resume return switch this "
		"throw try typeof while yield constructor "
		"instanceof true false static ");
	SendMessage(hScriptEdit, SCI_SETKEYWORDS, 2,
		(LPARAM)"a addindex addtogroup anchor arg attention author authors b brief bug "
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

	bool state = (GetMenuState(GetMenu(hwndScriptDlg), IDM_WHITESPACES, MF_BYCOMMAND) & MF_CHECKED);
	SendMessage(hScriptEdit, SCI_SETVIEWWS, state ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
	SendMessage(hScriptEdit, SCI_SETWHITESPACEFORE, 1, SciRGB(0x7f, 0xbf, 0xbf));
}

void ScripterWindowImpl::SetLexer(){
	for(int i = 0; i < buffers.size(); i++)
		SetLexerSingle(buffers[i].hEdit);
}


static HMODULE hSciLexer = NULL;

/// \brief Window procedure to replace edit control for subclassing command edit control
///
/// Handles enter key and up/down arrow keys to constomize behavior of default edit control.
static INT_PTR CALLBACK ScriptCommandProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	ScripterWindowImpl *p = (ScripterWindowImpl*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	return p->ScriptCommandProc(hWnd, message, wParam, lParam);
}


INT_PTR ScripterWindowImpl::ScriptCommandProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	switch (message)
	{
	case WM_GETDLGCODE:
		return (DLGC_WANTALLKEYS |
				CallWindowProc(defEditWndProc, hWnd, message,
								wParam, lParam));

	case WM_CHAR:
		//Process this message to avoid message beeps.
		if ((wParam == VK_RETURN) || (wParam == VK_TAB))
			return 0;
		else
			return (CallWindowProc(defEditWndProc, hWnd,
								message, wParam, lParam));

	case WM_KEYDOWN:
		// Enter key issues command
		if(wParam == VK_RETURN){
			int buflen = GetWindowTextLengthA(hWnd);
			char *buf = (char*)malloc(buflen+1);
			GetWindowTextA(hWnd, buf, buflen+1);
			config.commandProc(buf);
			// Remember issued command in the history buffer only if the command is not repeated.
			if(cmdHistory.empty() || strcmp(cmdHistory.back(), buf)){
				cmdHistory.push_back(buf);
				currentHistory = -1;
			}
			else // Forget repeated command
				free(buf);
			SetWindowTextA(hWnd, "");
			return FALSE;
		}

		// TODO: command completion
		if(wParam == VK_TAB){
		}

		// Up/down arrow keys navigate through command history.
		if(wParam == VK_UP || wParam == VK_DOWN){
			if(cmdHistory.size() != 0){
				if(currentHistory == -1)
					currentHistory = (int)cmdHistory.size() - 1;
				else
					currentHistory = (int)(currentHistory + (wParam == VK_UP ? -1 : 1) + cmdHistory.size()) % cmdHistory.size();
				SetWindowTextA(hWnd, cmdHistory[currentHistory]);
			}
			return FALSE;
		}

		return CallWindowProc(defEditWndProc, hWnd, message, wParam, lParam);
		break ;

	default:
		break;

	} /* end switch */
	return CallWindowProc(defEditWndProc, hWnd, message, wParam, lParam);
}

/// Calculate width of line numbers margin by contents
static void RecalcLineNumberWidth(HWND hDlg){
	ScripterWindowImpl *p = (ScripterWindowImpl*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	for(int i = 0; i < p->buffers.size(); i++){
		HWND hScriptEdit = p->buffers[i].hEdit;
		if(!hScriptEdit)
			return;
		bool showState = (GetMenuState(GetMenu(hDlg), IDM_LINENUMBERS, MF_BYCOMMAND) & MF_CHECKED) != 0;

		// Make sure to set type of the first margin to be line numbers
		SendMessage(hScriptEdit, SCI_SETMARGINTYPEN, 0, SC_MARGIN_NUMBER);

		// Obtain width needed to display all line count in the buffer
		int lineCount = SendMessage(hScriptEdit, SCI_GETLINECOUNT, 0, 0);
		std::string lineText = "_";
		for(int i = 0; pow(10, i) <= lineCount; i++)
			lineText += "9";

		int width = showState ? SendMessage(hScriptEdit, SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)lineText.c_str()) : 0;
		SendMessage(hScriptEdit, SCI_SETMARGINWIDTHN, 0, width);
	}
}

static void LoadScriptFile(HWND hDlg, const char *fileName){
	ScripterWindowImpl *p = (ScripterWindowImpl*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if(!p)
		return;

	ScripterWindowImpl::Buffer *buf = NULL;
	// Find a buffer with the same file name and select it instead of creating a new buffer.
	if(fileName[0]){
		for(int i = 0; i < p->buffers.size(); i++){
			if(p->buffers[i].fileName == fileName){
				buf = &p->buffers[i];
				p->SetCurrentBuffer(i);
				SendMessageA(GetDlgItem(hDlg, IDC_TABBUFFER), TCM_SETCURSEL, i, 0);
				break;
			}
		}
	}
	if(!buf){
		buf = &p->AddBuffer(fileName);
	}

	// Reset status to create an empty buffer if file name is absent
	if(fileName[0] == '\0'){
		p->SetFileName("");
		RecalcLineNumberWidth(hDlg);
		return;
	}

	HANDLE hFile = CreateFileA(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		DWORD textLen = GetFileSize(hFile, NULL);
		char *text = (char*)malloc((textLen+1) * sizeof(char));
		ReadFile(hFile, text, textLen, &textLen, NULL);
		wchar_t *wtext = (wchar_t*)malloc((textLen+1) * sizeof(wchar_t));
		text[textLen] = '\0';
		::MultiByteToWideChar(CP_UTF8, 0, text, textLen, wtext, textLen);
		// SetWindowTextA() seems to convert given string into unicode string prior to calling message handler.
//						SetWindowTextA(GetDlgItem(hDlg, IDC_SCRIPTEDIT), text);
		// SCI_SETTEXT seems to pass the pointer verbatum to the message handler.
		SendMessageA(p->GetCurrentBuffer().hEdit, SCI_SETTEXT, 0, (LPARAM)text);
		CloseHandle(hFile);
		free(text);
		free(wtext);

		// Clear undo buffer instead of setting a savepoint because we don't want to undo to empty document
		// if it's opened from a file.
		SendMessageA(buf->hEdit, SCI_EMPTYUNDOBUFFER, 0, 0);

		p->SetFileName(fileName);
		RecalcLineNumberWidth(hDlg);
	}
}

static void SaveScriptFile(HWND hDlg, const char *fileName){
	ScripterWindowImpl *p = (ScripterWindowImpl*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	if(!p)
		return;
	HANDLE hFile = CreateFileA(fileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile != INVALID_HANDLE_VALUE){
		DWORD textLen = SendMessageA(p->GetCurrentBuffer().hEdit, SCI_GETTEXTLENGTH, 0, 0);
		char *text = (char*)malloc((textLen+1) * sizeof(char));
		SendMessageA(p->GetCurrentBuffer().hEdit, SCI_GETTEXT, textLen + 1, (LPARAM)text);
		WriteFile(hFile, text, textLen, &textLen, NULL);
		CloseHandle(hFile);
		free(text);
		// Set savepoint for buffer dirtiness management
		SendMessageA(p->GetCurrentBuffer().hEdit, SCI_SETSAVEPOINT, 0, 0);
		p->SetFileName(fileName); // Remember the file name for the next save operation
	}
}

/// Dialog handler for scripting window
static INT_PTR CALLBACK ScriptDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam){
	ScripterWindowImpl *p = (ScripterWindowImpl*)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	static HWND hwndScintilla = NULL;
	switch(message){
	case WM_INITDIALOG:
		SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
		p = (ScripterWindowImpl*)lParam;
		p->hwndScriptDlg = hDlg;
		{
			HWND hCommand = GetDlgItem(hDlg, IDC_COMMAND);
			p->defEditWndProc = (WNDPROC)GetWindowLongPtr(hCommand, GWLP_WNDPROC);
			SetWindowLongPtr(hCommand, GWLP_WNDPROC, (LONG_PTR)ScriptCommandProc);
			SetWindowLongPtr(hCommand, GWLP_USERDATA, (LONG_PTR)lParam);
		}
		if(hSciLexer){
			// Hide the placeholder control instead of destroying because we need it to
			// get position of dynamically created controls.
			ShowWindow(GetDlgItem(hDlg, IDC_SCRIPTEDIT), SW_HIDE);
			LoadScriptFile(hDlg, ""); // Initialize with empty buffer
		}
		break;
	case WM_DESTROY:
		if(p)
			p->hwndScriptDlg = NULL;
		break;
	case WM_SIZE:
		{
			HWND hEdit = GetDlgItem(hDlg, IDC_CONSOLE);
			HWND hScr = GetDlgItem(hDlg, IDC_SCRIPTEDIT);
			HWND hRun = GetDlgItem(hDlg, IDOK);
			HWND hTab = GetDlgItem(hDlg, IDC_TABBUFFER);
			RECT scr, runr, rect, cr, wr, comr;
			GetClientRect(hDlg, &cr);
			GetWindowRect(hRun, &runr);

			// Adjust tabs bar size
			RECT tabr;
			GetWindowRect(hTab, &tabr);
			POINT posTab = {tabr.left, tabr.top};
			ScreenToClient(hDlg, &posTab);
			SetWindowPos(hTab, NULL, 0, 0, cr.right - posTab.x - (runr.right - runr.left) - 10,
				tabr.bottom - tabr.top, SWP_NOMOVE);

			GetWindowRect(hScr, &scr);
			POINT posScr = {scr.left, scr.top};
			ScreenToClient(hDlg, &posScr);
			SetWindowPos(hScr, NULL, 0, 0, cr.right - posScr.x - (runr.right - runr.left) - 10,
				scr.bottom - scr.top, SWP_NOMOVE);
			// Update all Scintilla components including hidden ones
			for(int i = 0; i < p->buffers.size(); i++)
				SetWindowPos(p->buffers[i].hEdit, NULL, 0, 0, cr.right - posScr.x - (runr.right - runr.left) - 10,
					scr.bottom - scr.top, SWP_NOMOVE);
			SetWindowPos(hRun, NULL, cr.right - (runr.right - runr.left) - 5, 10, 0, 0, SWP_NOSIZE);
			SetWindowPos(GetDlgItem(hDlg, IDCANCEL), NULL, cr.right - (runr.right - runr.left) - 5, 10 + (runr.bottom - runr.top + 10), 0, 0, SWP_NOSIZE);
			SetWindowPos(GetDlgItem(hDlg, IDC_CLEARCONSOLE), NULL, cr.right - (runr.right - runr.left) - 5, 10 + (runr.bottom - runr.top + 10) * 2, 0, 0, SWP_NOSIZE);

			GetWindowRect(hEdit, &rect);
			POINT posEdit = {rect.left, rect.top};
			ScreenToClient(hDlg, &posEdit);
			GetWindowRect(GetDlgItem(hDlg, IDC_COMMAND), &comr);
			GetWindowRect(hDlg, &wr);
			SetWindowPos(hEdit, NULL, 0, 0, cr.right - posEdit.x - 5,
				cr.bottom - posEdit.y - (comr.bottom - comr.top) - 10, SWP_NOMOVE);
			POINT posCom = {comr.left, comr.top};
			ScreenToClient(hDlg, &posCom);
			SetWindowPos(GetDlgItem(hDlg, IDC_COMMAND), NULL, posCom.x, cr.bottom - (comr.bottom - comr.top) - 5,
				cr.right - posCom.x - 5, comr.bottom - comr.top, 0);
		}
		break;
	case WM_COMMAND:
		if(p){
			UINT id = LOWORD(wParam);
			if(id == IDOK){
				HWND hEdit = p->GetCurrentBuffer().hEdit;
				int buflen = GetWindowTextLengthW(hEdit)+1;
				char *buf = (char*)malloc(buflen * 3 * sizeof*buf);
				wchar_t *wbuf = (wchar_t*)malloc(buflen * sizeof*wbuf);
				SendMessageW(hEdit, WM_GETTEXT, buflen, (LPARAM)wbuf);
				// Squirrel is not compiled with unicode, so we must convert text into utf-8, which is ascii transparent.
				::WideCharToMultiByte(CP_UTF8, 0, wbuf, buflen, buf, buflen * 3, NULL, NULL);
				p->config.runProc(p->GetCurrentBuffer().fileName.c_str(), buf);
				free(buf);
				free(wbuf);
				return TRUE;
			}
			else if(id == IDCANCEL || id == IDCLOSE)
			{
				bool canceled = false;
				// Confirm all dirty buffers before closing
				for(int i = 0; i < p->buffers.size(); i++){
					if(p->buffers[i].dirty && MessageBoxA(hDlg, (p->GetBufferName(i) + " is not saved. OK to close?").c_str(), "Scripting Window", MB_OKCANCEL) != IDOK){
						canceled = true;
						break;
					}
				}
				if(!canceled){
					EndDialog(hDlg, LOWORD(wParam));
					if(p->config.onClose)
						p->config.onClose(p);
				}
				return TRUE;
			}
			else if(id == IDC_CLEARCONSOLE){
				SetDlgItemText(p->hwndScriptDlg, IDC_CONSOLE, TEXT(""));
				return TRUE;
			}
			else if(id == IDM_NEW){
				LoadScriptFile(hDlg, "");
				return TRUE;
			}
			else if(id == IDM_SCRIPT_OPEN){
				static char fileBuf[MAX_PATH];
				OPENFILENAMEA ofn = {
					sizeof(OPENFILENAME), //  DWORD         lStructSize;
					hDlg, //  HWND          hwndOwner;
					NULL, // HINSTANCE     hInstance; ignored
					p->config.sourceFilters, //  LPCTSTR       lpstrFilter;
					NULL, //  LPTSTR        lpstrCustomFilter;
					0, // DWORD         nMaxCustFilter;
					0, // DWORD         nFilterIndex;
					fileBuf, // LPTSTR        lpstrFile;
					sizeof fileBuf, // DWORD         nMaxFile;
					NULL, //LPTSTR        lpstrFileTitle;
					0, // DWORD         nMaxFileTitle;
					".", // LPCTSTR       lpstrInitialDir;
					"Open File", // LPCTSTR       lpstrTitle;
					0, // DWORD         Flags;
					0, // WORD          nFileOffset;
					0, // WORD          nFileExtension;
					NULL, // LPCTSTR       lpstrDefExt;
					0, // LPARAM        lCustData;
					NULL, // LPOFNHOOKPROC lpfnHook;
					NULL, // LPCTSTR       lpTemplateName;
				};
				if(GetOpenFileNameA(&ofn)){
					LoadScriptFile(hDlg, ofn.lpstrFile);
				}
			}
			else if(id == IDM_SCRIPT_SAVE || id == IDM_SCRIPT_SAVEAS){
				if(p->GetCurrentBuffer().fileName.empty() || id == IDM_SCRIPT_SAVEAS){
					static char fileBuf[MAX_PATH];
					OPENFILENAMEA ofn = {
						sizeof(OPENFILENAME), //  DWORD         lStructSize;
						hDlg, //  HWND          hwndOwner;
						NULL, // HINSTANCE     hInstance; ignored
						p->config.sourceFilters, //  LPCTSTR       lpstrFilter;
						NULL, //  LPTSTR        lpstrCustomFilter;
						0, // DWORD         nMaxCustFilter;
						0, // DWORD         nFilterIndex;
						fileBuf, // LPTSTR        lpstrFile;
						sizeof fileBuf, // DWORD         nMaxFile;
						NULL, //LPTSTR        lpstrFileTitle;
						0, // DWORD         nMaxFileTitle;
						".", // LPCTSTR       lpstrInitialDir;
						"Save File", // LPCTSTR       lpstrTitle;
						OFN_OVERWRITEPROMPT, // DWORD         Flags;
						0, // WORD          nFileOffset;
						0, // WORD          nFileExtension;
						NULL, // LPCTSTR       lpstrDefExt;
						0, // LPARAM        lCustData;
						NULL, // LPOFNHOOKPROC lpfnHook;
						NULL, // LPCTSTR       lpTemplateName;
					};
					if(GetSaveFileNameA(&ofn)){
						SaveScriptFile(hDlg, ofn.lpstrFile);
					}
				}
				else
					SaveScriptFile(hDlg, p->GetCurrentBuffer().fileName.c_str());
			}
			else if(id == IDM_WHITESPACES){
				bool newState = !(GetMenuState(GetMenu(hDlg), IDM_WHITESPACES, MF_BYCOMMAND) & MF_CHECKED);
				CheckMenuItem(GetMenu(hDlg), IDM_WHITESPACES,
					(newState ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
				for(int i = 0; i < p->buffers.size(); i++){
					HWND hScriptEdit = p->buffers[i].hEdit;
					if(hScriptEdit){
						SendMessage(hScriptEdit, SCI_SETVIEWWS, newState ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE, 0);
						SendMessage(hScriptEdit, SCI_SETWHITESPACEFORE, 1, SciRGB(0x7f, 0xbf, 0xbf));
					}
				}
			}
			else if(id == IDM_LINENUMBERS){
				bool newState = !(GetMenuState(GetMenu(hDlg), IDM_LINENUMBERS, MF_BYCOMMAND) & MF_CHECKED);
				CheckMenuItem(GetMenu(hDlg), IDM_LINENUMBERS,
					(newState ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
				RecalcLineNumberWidth(hDlg);
			}
		}
		break;
	case WM_NOTIFY:
		if(p){
			UINT id = LOWORD(wParam);
			NMHDR *nmh = (NMHDR*)lParam;
			if(nmh->idFrom == IDC_SCRIPTEDIT_DYNAMIC && nmh->code == SCN_SAVEPOINTREACHED)
				p->SavePoint(true);
			else if(nmh->idFrom == IDC_SCRIPTEDIT_DYNAMIC && nmh->code == SCN_SAVEPOINTLEFT)
				p->SavePoint(false);

			if(nmh->idFrom == IDC_TABBUFFER && nmh->code == TCN_SELCHANGE){
				p->SetCurrentBuffer(SendMessageA(GetDlgItem(hDlg, IDC_TABBUFFER), TCM_GETCURSEL, 0, 0));
			}
		}
		break;
	}
	return FALSE;
}

/// Toggle scripting window
int scripter_show(ScripterWindow *sc){
	if(!hSciLexer){
		hSciLexer = LoadLibrary(TEXT("SciLexer.DLL"));
		if(hSciLexer == NULL){
			MessageBox(NULL,
			TEXT("The Scintilla DLL could not be loaded."),
			TEXT("Error loading Scintilla"),
			MB_OK | MB_ICONERROR);
		}
	}
	ScripterWindowImpl *p = (ScripterWindowImpl*)sc;
	if(!IsWindow(p->hwndScriptDlg)){
		p->hwndScriptDlg = CreateDialogParam(hSciLexer, TEXT("ScriptWin"), NULL, ScriptDlg, (LPARAM)p);
	}
	else
		ShowWindow(p->hwndScriptDlg, SW_SHOW);
	return 0;
}

int scripter_lexer_squirrel(ScripterWindow *sc){
	ScripterWindowImpl *p = static_cast<ScripterWindowImpl*>(sc);
	p->SetLexer();
	return 0;
}

void scripter_adderror(ScripterWindow *sc, const char *desc, const char *source, int line, int column){
	ScripterWindowImpl *p = static_cast<ScripterWindowImpl*>(sc);
	HWND hScriptEdit = p->GetCurrentBuffer().hEdit;
	if(hScriptEdit){
		if(strcmp(source, "scriptbuf") && strcmp(source, p->GetCurrentBuffer().fileName.c_str()))
			LoadScriptFile(p->hwndScriptDlg, source);

		int pos = SendMessage(hScriptEdit, SCI_POSITIONFROMLINE, line - 1, 0);
		SendMessage(hScriptEdit, SCI_INDICSETSTYLE, 0, INDIC_SQUIGGLE);
		SendMessage(hScriptEdit, SCI_INDICSETFORE, 0, SciRGB(255,0,0));
		SendMessage(hScriptEdit, SCI_SETINDICATORCURRENT, 0, 0);
		SendMessage(hScriptEdit, SCI_INDICATORFILLRANGE, pos + column - 1, SendMessage(hScriptEdit, SCI_LINELENGTH, 0, 0) - (column - 1));
		SendMessage(hScriptEdit, SCI_MARKERDEFINE, 0, SC_MARK_CIRCLE);
		SendMessage(hScriptEdit, SCI_MARKERSETFORE, 0, SciRGB(255,0,0));
		SendMessage(hScriptEdit, SCI_MARKERADD, line - 1, 0);
	}
}

void scripter_clearerror(ScripterWindow *sc){
	ScripterWindowImpl *p = static_cast<ScripterWindowImpl*>(sc);
	HWND hScriptEdit = p->GetCurrentBuffer().hEdit;
	if(hScriptEdit){
		SendMessage(hScriptEdit, SCI_INDICATORCLEARRANGE, 0, SendMessage(hScriptEdit, SCI_GETLENGTH, 0, 0));
		SendMessage(hScriptEdit, SCI_MARKERDELETEALL, 0, 0);
	}
}

void scripter_delete(ScripterWindow *sc){
	delete sc;
}
