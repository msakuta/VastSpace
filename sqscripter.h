#ifndef SQSCRIPTER_H
#define SQSCRIPTER_H

#ifdef __cplusplus
extern "C"{
#endif

#ifdef SCI_LEXER
#define SCRIPTER_EXPORT __declspec(dllexport)
#else
#define SCRIPTER_EXPORT __declspec(dllimport)
#endif

struct ScripterWindow;

/// \brief Parameter set for a scripter window
struct ScripterConfig{
	void (*commandProc)(const char*);
	void (**printProc)(ScripterWindow *, const char*);
	void (*runProc)(const char *fileName, const char *content);
	void (*onClose)(ScripterWindow*);
	const char *sourceFilters;
};

/// \brief A Scripter Window handle.  Don't rely on this internal structure.
struct ScripterWindow{
	ScripterConfig config;
};

/// \brief Initialize the scripter system with command and print procedures
SCRIPTER_EXPORT ScripterWindow *scripter_init(const ScripterConfig *);


/// \brief Show the scripter window
SCRIPTER_EXPORT int scripter_show(ScripterWindow *);


/// \brief Set Squirrel lexical coloring, must be called after scripter_show().
/// \returns Nonzero if succeeded
SCRIPTER_EXPORT int scripter_lexer_squirrel(ScripterWindow *);


/// \brief Add a script error indicator to specified line in a specified source file.
SCRIPTER_EXPORT void scripter_adderror(ScripterWindow*, const char *desc, const char *source, int line, int column);


/// \brief Clear all error indicators in the window.
SCRIPTER_EXPORT void scripter_clearerror(ScripterWindow*);


/// \brief Delete the scripter window
SCRIPTER_EXPORT void scripter_delete(ScripterWindow *);


#ifdef __cplusplus
}
#endif

#endif
