/** \file
 * \brief Implementation of GLWmenu.
 */
#include "glwindow.h"
#include "../cmd.h"
#include "../antiglut.h"
#include "../viewer.h"
#include "../material.h"
#include "../player.h" // GLWmoveOrderButton
#include "../sqadapt.h"
#include "GLWmenu.h"
extern "C"{
#include <clib/c.h>
#include <clib/GL/gldraw.h>
}
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define projection(e) glMatrixMode(GL_PROJECTION); e; glMatrixMode(GL_MODELVIEW);

#define fontwidth (GLwindow::glwfontwidth * GLwindow::glwfontscale)
#define fontheight (GLwindow::glwfontheight * GLwindow::glwfontscale)


const int glwMenuAllAllocated[] = {1};
const char glwMenuSeparator[] = "-";

/// Draws menu items.
void GLWmenu::draw(GLwindowState &ws, double t){
	GLWrect r = clientRect();
	int mx = ws.mousex, my = ws.mousey;
	GLWmenu *p = this;
	GLwindow *wnd = this;
	int i, len, maxlen = 1;
	int ind = 0 <= my && 0 <= mx && mx <= width ? (my) / fontheight : -1;
	MenuItem *item = menus->get();
	for(i = 0; i < count; i++, item = item->next) if(item->isSeparator()){
		glBegin(GL_LINES);
		glVertex2d(r.x0 + fontwidth / 2, r.y0 + (0 + i) * fontheight + 6);
		glVertex2d(r.x1 - fontwidth / 2, r.y0 + (0 + i) * fontheight + 6);
		glEnd();
	}
	else{
		if(i == ind){
			glColor4ub(0,0,255,128);
			glBegin(GL_QUADS);
			glVertex2d(r.x0 + 1, r.y0 + (0 + i) * fontheight);
			glVertex2d(r.x1    , r.y0 + (0 + i) * fontheight);
			glVertex2d(r.x1    , r.y0 + (1 + i) * fontheight);
			glVertex2d(r.x0 + 1, r.y0 + (1 + i) * fontheight);
			glEnd();
		}
		glColor4ub(255,255,255,255);
		glwpos2d(r.x0, r.y0 + (1 + i) * fontheight);
		glwprintf(item->title);
		len = item->title.len();
		if(maxlen < len)
			maxlen = len;
	}
//	wnd->w = maxlen * fontwidth + 2;
}

int GLWmenu::mouse(GLwindowState &, int button, int state, int x, int y){
	int ind = (y) / fontheight;
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && 0 <= ind && ind < count){
		MenuItem *item = menus->get();
		for(; 0 < ind; ind--)
			item = item->next;
		item->execute();
		if((flags & (GLW_CLOSE | GLW_POPUP)) && !(flags & GLW_PINNED))
			flags |= GLW_TODELETE;
		return 1;
	}
	return 0;
}

int GLWmenu::key(int key){
	MenuItem *item = menus->get();
	for(; item; item = item->next) if(item->key == key){
		item->execute();
		if(flags & GLW_CLOSE && !(flags & GLW_PINNED))
			flags |= GLW_TODELETE;
		return 1;
	}
	return 0;
}

GLWmenu::~GLWmenu(){
	delete menus;
}

/// \brief Constructs a GLWmenu object of given items.
/// \param name Title string.
/// \param count Number of items.
/// \param menutitles Pointer to array of strings for displaying menu items. \param keys Pointer to array of keyboard shortcuts. \param cmd Pointer to array of strings for console commands that are executed when menu item is selected.
/// \param stickey ???
/// \return Constructed GLWmenu object.
/// \relates GLWmenu
GLWmenu *glwMenu(const char *name, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky){
	return new GLWmenu(name, count, menutitles, keys, cmd, sticky);
}

/// \brief Constructs a GLWmenu object of given items.
/// \param name Title string.
/// \param list PopupMenu object that contains list of menu items.
/// \params flags ???
/// \relates GLWmenu
GLWmenu *glwMenu(const char *name, const PopupMenu &list, unsigned flags){
	return new GLWmenu(name, list, flags);
}

/// \brief Constructs a GLWmenu object of given items.
/// \par It's effectively same as the glwMenu function.
/// \param name Title string.
/// \param count Number of items.
/// \param menutitles Pointer to array of strings for displaying menu items. \param keys Pointer to array of keyboard shortcuts. \param cmd Pointer to array of strings for console commands that are executed when menu item is selected.
/// \param stickey ???
/// \return Constructed GLWmenu object.
GLWmenu::GLWmenu(const char *title, int acount, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky) : st(title), count(acount), menus(NULL){
	GLwindow *ret = this;
	int i, len, maxlen = 0;
//	menus = (glwindowmenuitem*)malloc(count * sizeof(*menus));
	xpos = !sticky * 50;
	ypos = 75;
	width = 150;
	height = (1 + count) * fontheight;
	title = NULL;
	flags = (sticky ? 0 : GLW_CLOSE | GLW_PINNABLE) | GLW_COLLAPSABLE;
	modal = NULL;

	/* To ensure no memory leaks, we need to allocate all strings though if it's obvious
	  that the given parameters are static const. The caller also must disallocate
	  the parameters if they are allocated memory.
	   This feels ridiculous, but no other way can clearly ensure memory safety. For
	  short-lived programs, memory leaks wouldn't be a big problem, but this work is
	  going to have sustained life. */
	menus = new PopupMenu;
	for(i = 0; i < count; i++){
		if(menutitles[i] == glwMenuSeparator){
			menus->appendSeparator();
		}
		else{
			menus->append(menutitles[i], keys ? keys[i] : '\0', cmd && cmd[i] ? cmd[i] : "");
			len = ::strlen(menutitles[i]);
			if(maxlen < len)
				maxlen = len;
		}
	}
	height = (1 + count) * fontheight;

/*	if(ret->w < maxlen * fontwidth + 2)
		ret->w = maxlen * fontwidth + 2;*/
}

GLWmenu::GLWmenu(const char *title, const PopupMenu &list, unsigned aflags) : st(title), count(list.count()), menus(new PopupMenu(list)){
	flags = aflags & (GLW_CLOSE | GLW_SIZEABLE | GLW_COLLAPSABLE | GLW_PINNABLE); // filter the bits
	width = 150;
	for(MenuItem *mi = menus->get(); mi; mi = mi->next){
		int itemwidth = glwsizef(mi->title);
		if(width < itemwidth)
			width = itemwidth;
	}
	height = (1 + count) * fontheight;
}

/// \brief Adds an item to the menu.
/// Window size is expanded.
GLWmenu *GLWmenu::addItem(PopupMenuItem *item){
	menus->append(item);
	GLWrect cr = clientRect();
	count = menus->count(); // Count is not necessarily increase, so we cannot just ++count.
	cr.y1 = cr.y0 + count * fontheight;
	cr = adjustRect(cr);
	height = cr.y1 - cr.y0;
	return this;
}

/// Adds console command item.
GLWmenu *GLWmenu::addItem(const char *title, int key, const char *cmd){
	PopupMenuItemCmd *item = new PopupMenuItemCmd;
	item->title = title;
	item->key = key;
	item->cmd = cmd;
	return addItem(item);
}

/// Adds Squirrel closure item.
GLWmenu *GLWmenu::addItem(const char *title, int key, HSQOBJECT ho){
	PopupMenuItemClosure *item = new PopupMenuItemClosure(ho, g_sqvm);
	item->title = title;
	item->key = key;
	return addItem(item);
}

int GLWmenu::cmd_addcmdmenuitem(int argc, char *argv[], void *p){
	GLWmenu *wnd = (GLWmenu*)p;
	if(argc < 3){
		CmdPrint("addcmdmenuitem title command [key]");
		return 0;
	}
	if(!wnd){
		int key = argc < 4 ? 0 : argv[3][0];
		wnd = glwMenu("Command Menu", 0, NULL, NULL, NULL, 1);
		return 0;
	}
	wnd->addItem(argv[1], argc < 4 ? 0 : argv[3][0], argv[2]);
	return 0;
}

static SQInteger sqf_GLwindowMenu_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	const SQChar *title;
	SQBool sticky;
	if(argc <= 1 || SQ_FAILED(sq_getstring(v, 2, &title)))
		title = " ";
	if(argc <= 2 || SQ_FAILED(sq_getbool(v, 2, &sticky)))
		sticky = false;
	GLWmenu *p = new GLWmenu(title, 0, NULL, NULL, NULL, sticky);
	if(!sqa_newobj(v, p, 1))
		return SQ_ERROR;
	glwAppend(p);
	return 0;
}

static SQInteger sqf_GLwindowMenu_addItem(HSQUIRRELVM v){
	GLWmenu *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
	const SQChar *title, *cmd;
	if(SQ_FAILED(sq_getstring(v, 2, &title)))
		return SQ_ERROR;
	HSQOBJECT ho;
	if(SQ_SUCCEEDED(sq_getstring(v, 3, &cmd)))
		p->addItem(title, 0, cmd);
	else if(SQ_SUCCEEDED(sq_getstackobj(v, 3, &ho)))
		p->addItem(title, 0, ho);
	else
		return SQ_ERROR;
	return 0;
}

static SQInteger sqf_GLwindowMenu_hide(HSQUIRRELVM v){
	GLWmenu *p;
	if(!sqa_refobj(v, (SQUserPointer*)&p))
		return SQ_ERROR;
	const SQChar *title, *cmd;
	p->setVisible(false);
	return 0;
}

static SQInteger sqf_GLWbigMenu_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	const SQChar *title;
	SQBool sticky;
	if(argc <= 1 || SQ_FAILED(sq_getstring(v, 2, &title)))
		title = " ";
	if(argc <= 2 || SQ_FAILED(sq_getbool(v, 2, &sticky)))
		sticky = false;
	GLWmenu *p = GLWmenu::newBigMenu();
	if(!sqa_newobj(v, p, 1))
		return SQ_ERROR;
	glwAppend(p);
	return 0;
}


/// Define class GLWmenu and GLWbigMenu for Squirrel
void GLWmenu::sq_define(HSQUIRRELVM v){
	sq_pushstring(v, _SC("GLWmenu"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	register_closure(v, _SC("constructor"), sqf_GLwindowMenu_constructor);
	register_closure(v, _SC("addItem"), sqf_GLwindowMenu_addItem);
	register_closure(v, _SC("hide"), sqf_GLwindowMenu_hide);
	sq_createslot(v, -3);

	sq_pushstring(v, _SC("GLWbigMenu"), -1);
	sq_pushstring(v, _SC("GLWmenu"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	register_closure(v, _SC("constructor"), sqf_GLWbigMenu_constructor);
	sq_createslot(v, -3);
}






class GLWbigMenu : public GLWmenu{
public:
	typedef GLWmenu st;
	static const int bigfontheight = 24;
	int *widths;
	GLWbigMenu(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky)
		: st(title, count, menutitles, keys, cmd, sticky), widths(NULL){
		flags &= ~(GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE);
	}
	virtual GLWmenu *addItem(MenuItem *item){
		menus->append(item, false);
		GLWrect cr = clientRect();
		count = menus->count();
		cr.y1 = cr.y0 + count * (bigfontheight + 40) + 0;
		int strwidth = glwGetSizeTextureString(item->title, bigfontheight);
		cr.x1 = cr.x0 + 20 + strwidth + 20;
		cr = adjustRect(cr);
		height = cr.y1 - cr.y0;
		if(width < cr.x1 - cr.x0)
			width = cr.x1 - cr.x0;
		widths = (int*)realloc(widths, count * sizeof*widths);
		widths[count-1] = strwidth;
		return this;
	}
	int mouse(GLwindowState &, int button, int state, int x, int y);
	virtual void draw(GLwindowState &ws, double t);
	virtual ~GLWbigMenu(){
		free(widths);
	}
};

int GLWbigMenu::mouse(GLwindowState &, int button, int state, int x, int y){
	int ind = (y) / (bigfontheight + 40);
	if(button == GLUT_LEFT_BUTTON && state == GLUT_UP && 0 <= ind && ind < count){
		MenuItem *item = menus->get();
		for(; 0 < ind; ind--)
			item = item->next;
		item->execute();
		if((flags & (GLW_CLOSE | GLW_POPUP)) && !(flags & GLW_PINNED))
			flags |= GLW_TODELETE;
		return 1;
	}
	return 0;
}

/// Draws big menu items.
void GLWbigMenu::draw(GLwindowState &ws, double t){
	GLWrect r = clientRect();
	int mx = ws.mousex, my = ws.mousey;
	GLWmenu *p = this;
	GLwindow *wnd = this;
	int i, len, maxlen = 1;
	int ind = 0 <= my && 0 <= mx && mx <= width ? (my) / (bigfontheight + 40) : -1;
	MenuItem *item = menus->get();
	for(i = 0; i < count; i++, item = item->next){
		if(i == ind){
			glColor4ub(0,0,255,128);
			glBegin(GL_QUADS);
			glVertex2d(r.x0 + 10, r.y0 + (0 + i) * (bigfontheight + 40) + 10);
			glVertex2d(r.x1 - 10, r.y0 + (0 + i) * (bigfontheight + 40) + 10);
			glVertex2d(r.x1 - 10, r.y0 + (1 + i) * (bigfontheight + 40) - 10);
			glVertex2d(r.x0 + 10, r.y0 + (1 + i) * (bigfontheight + 40) - 10);
			glEnd();
		}

		// Draw borders
		glColor4ub(255,255,127,255);
		glBegin(GL_LINE_LOOP);
		glVertex2d(r.x0 + 10, r.y0 + (0 + i) * (bigfontheight + 40) + 10);
		glVertex2d(r.x1 - 10, r.y0 + (0 + i) * (bigfontheight + 40) + 10);
		glVertex2d(r.x1 - 10, r.y0 + (1 + i) * (bigfontheight + 40) - 10);
		glVertex2d(r.x0 + 10, r.y0 + (1 + i) * (bigfontheight + 40) - 10);
		glEnd();

		glColor4ub(255,255,255,255);
		glPushMatrix();
		glTranslated(r.x0 + (r.x1 - r.x0) / 2 - widths[i] / 2, r.y0 + (i) * (bigfontheight + 40) + 10 + bigfontheight + 10, 0);
		glwPutTextureStringN(item->title, strlen(item->title), bigfontheight);
		glPopMatrix();
		len = item->title.len();
		if(maxlen < len)
			maxlen = len;
	}
//	wnd->w = maxlen * fontwidth + 2;
}




GLWmenu *GLWmenu::newBigMenu(){
	return new GLWbigMenu("", 0, NULL, NULL, NULL, 1);
}

class GLWpopup : public GLWmenu{
public:
	typedef GLWmenu st;
	GLWpopup(const char *title, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky, GLwindowState &gvp)
		: st(title, count, menutitles, keys, cmd, sticky){
		init(gvp);
	}
	GLWpopup(const char *title, GLwindowState &gvp, const PopupMenu &list)
		: st(title, list, 0){
		init(gvp);
	}
	void init(GLwindowState &gvp){
		flags &= ~(GLW_CLOSE | GLW_COLLAPSABLE | GLW_PINNABLE);
		flags |= GLW_POPUP;
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(WindowFromDC(wglGetCurrentDC()), &point);
		this->xpos = point.x + this->width < gvp.w ? point.x : gvp.w - this->width;
		this->ypos = point.y + this->height < gvp.h ? point.y : gvp.h - this->height;
	}
	~GLWpopup(){
		if(glwfocus == this)
			glwfocus = NULL;
	}
};

GLwindow *glwPopupMenu(GLwindowState &gvp, int count, const char *const menutitles[], const int keys[], const char *const cmd[], int sticky){
	GLwindow *ret;
	GLWmenu *p;
	int i, len, maxlen = 0;
	ret = new GLWpopup(NULL, count, menutitles, keys, cmd, sticky, gvp);
	glwAppend(ret);
	return ret;
}

GLWmenu *glwPopupMenu(GLwindowState &gvp, const PopupMenu &list){
	GLWmenu *ret = new GLWpopup(NULL, gvp, list);
	glwAppend(ret);
	return ret;
}








PopupMenu::PopupMenu(const PopupMenu &o) : list(NULL), end(&list){
	const PopupMenuItem *src = o.list;
	int count;
	for(count = 0; src; count++, src = src->next){
		PopupMenuItem *dst = *end = src->clone();
		end = &dst->next;
	}
}

PopupMenu::~PopupMenu(){
	for(PopupMenuItem *mi = list; mi;){
		PopupMenuItem *minext = mi->next;
		delete mi;
		mi = minext;
	}
}

PopupMenu &PopupMenu::append(cpplib::dstring title, int key, cpplib::dstring cmd, bool unique){
	if(unique){
		// Returns unchanged if duplicate menu item is present. This determination is done by title, not command.
		for(PopupMenuItem *p = list; p; p = p->next) if(p->title == title)
			return *this;
	}
	PopupMenuItemCmd *i = new PopupMenuItemCmd;
	i->title = title;
	i->key = key;
	i->cmd = cmd;
	i->next = NULL;
	*end = i;
	end = &i->next;
	return *this;
}

PopupMenu &PopupMenu::appendSeparator(bool mergenext){
	// Returns unchanged if preceding menu item is a separator too.
	if(mergenext && list){
		PopupMenuItem *p;
		for(p = list; p->next; p = p->next);
		if(p->key == PopupMenuItem::separator_key)
			return *this;
	}
	PopupMenuItem *i = new PopupMenuItemSeparator;
	i->key = PopupMenuItem::separator_key;
	*end = i;
	i->next = NULL;
	end = &i->next;
	return *this;
}

PopupMenu &PopupMenu::append(PopupMenuItem *item, bool unique){
	if(unique){
		// Returns unchanged if duplicate menu item is present. This determination is done by title, not command.
		for(PopupMenuItem *p = list; p; p = p->next) if(p->title == item->title)
			return *this;
	}
	*end = item;
	item->next = NULL;
	end = &item->next;
	return *this;
}

int PopupMenu::count()const{
	int ret = 0;
	for(const PopupMenuItem *item = list; item; item = item->next)
		ret++;
	return ret;
}

