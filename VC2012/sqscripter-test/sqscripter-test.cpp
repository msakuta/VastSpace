/** \file 
 * \brief A demonstration program for SqScripter GUI window
 *
 * It does not demonstrate anything like scripting.
 * It just shows how to setup and run the program to enable SqScripter window.
 */

#include "sqscripter.h"

#include <windows.h>

#include <string>

/// Buffer to hold the function to print something to the console (the lower half of the window).
/// Set by scripter_init().
static void (*PrintProc)(ScripterWindow *, const char *) = NULL;

/// ScripterWindow object handle.  It represents single SqScripter window.
static ScripterWindow *sw = NULL;

/// Function to handle commands come from the command line (the single line edit at the bottom of the window).
/// "Commands" mean anything, but it would be good to use for interactive scripting like Python or irb
void CmdProc(const char *cmd){
	// In this test program, "Commands" are just echoed back to user.
	MessageBox(NULL, cmd, "Command Executed", MB_ICONERROR);
	PrintProc(sw, (std::string("Echo: ") + cmd).c_str());
}

/// Function to handle batch exection of a script file.  File name is provided as the first argument if available.
static void RunProc(const char *fileName, const char *content){
	// In this test program, running a script file just dumps all its content to the console.
	MessageBox(NULL, (std::string("Source file run: ") + fileName).c_str(), "Source File Run", MB_ICONINFORMATION);
	PrintProc(sw, (std::string("Content: \r\n") + content).c_str());
}

static void OnClose(ScripterWindow *sc){
	scripter_delete(sc);
	PostQuitMessage(0);
}

/// The starting point for this demonstration program.
int main(int argc, char *argv[])
{
	// Fill in the Scripter's config parameters.
	const char *filters = "All (*.*)\0*.*\0Squirrel Scripts (*.nut)\0*.nut";
	ScripterConfig sc;
	sc.commandProc = CmdProc;
	sc.printProc = &PrintProc;
	sc.runProc = RunProc;
	sc.onClose = OnClose;
	sc.sourceFilters = filters;

	// Initialize a Scripter window
	sw = scripter_init(&sc);

	// Show the window on screen
	scripter_show(sw);

	// Set the lexer to squirrel
	scripter_lexer_squirrel(sw);

	// Windows message loop
	do{
		MSG msg;
		if(PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(GetMessage(&msg, NULL, 0, 0) <= 0)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}while (true);

	return 0;
}
