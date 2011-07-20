/** \file
 * \brief Declares GLWmenu.
 */
#ifndef GLW_GLWMENU_H
#define GLW_GLWMENU_H
#include "glw/glwindow.h"
#include "glw/popup.h"
extern "C"{
#include <clib/c.h>
#include <string.h>
}

/// Window that lists menus. Menu items are bound to console commands.
class GLWmenu : public GLwindow{
public:
	int count;
#if 0
	struct glwindowmenuitem{
		const char *title;
		int key;
		const char *cmd;
//		int allocated; /* whether this item need to be freed */
	} *menus;
#else
	typedef PopupMenuItem MenuItem;
	PopupMenu *menus;
#endif
	GLWmenu(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
	GLWmenu(const char *title, const PopupMenu &, unsigned flags = 0);
	void draw(GLwindowState &,double);
	int mouse(GLwindowState &ws, int button, int state, int x, int y);
	int key(int key);
	~GLWmenu();
	virtual GLWmenu *addItem(MenuItem *item);
	GLWmenu *addItem(const char *title, int key, const char *cmd);
	GLWmenu *addItem(const char *title, int key, HSQOBJECT ho);
	static int cmd_addcmdmenuitem(int argc, char *argv[], void *p);
	static GLWmenu *newBigMenu();
	static void sq_define(HSQUIRRELVM v);
};

extern const int glwMenuAllAllocated[];
extern const char glwMenuSeparator[];
GLWmenu *glwMenu(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
GLWmenu *glwMenu(const char *title, const PopupMenu &, unsigned flags);
GLwindow *glwPopupMenu(GLwindowState &, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky);
GLWmenu *glwPopupMenu(GLwindowState &, const PopupMenu &);

#endif
