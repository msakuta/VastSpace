#ifndef GLWINDOW_H
#define GLWINDOW_H

/* flags */
#define GLW_CLOSE		1
#define GLW_SIZEABLE	2
#define GLW_COLLAPSABLE 4
#define GLW_COLLAPSE	8
#define GLW_PINNABLE	0x10
#define GLW_PINNED		0x20
#define GLW_SIZEPROP	0x40 /* proportional size change, i.e. ratio is reserved */
#define GLW_POPUP		0x80 /* hidden as soon as defocused */
#define GLW_TIP			0x100 /* hidden as soon as the cursor moves out */
#define GLW_TODELETE	0x8000

struct viewport;

typedef class GLwindow{
public:
	typedef GLwindow st;
	static int mouseFunc(int button, int state, int x, int y, viewport &gvp);
	static friend GLwindow **glwAppend(GLwindow *wnd);
	static friend void glwActivate(GLwindow **ppwnd);
	void glwDraw(viewport &, double t, int *);
	void glwDrawMinimized(viewport &gvp, double t, int *pp);
	static friend GLwindow **glwFindPP(GLwindow *);
	void mouseDrag(int x, int y);
	int getX()const{return x;}
	int getY()const{return y;}
	virtual int mouse(int key, int state, int x, int y);
	virtual int key(int key); /* returns nonzero if processed */
protected:
	GLwindow(const char *title = "");
	int x, y;
	int w, h;
	char *title;
	GLwindow *next;
	unsigned int flags;
	GLwindow *modal; /* The window that must be closed prior to proceed this window's process. */
	virtual void draw(int mousex, int mousey, double t);
	virtual ~GLwindow(); /* destructor method, NULL permitted */
private:
	void drawInt(viewport &vp, double t, int mousex, int mousey, int, int);
	void glwFree();
} glwindow;

extern glwindow *glwlist;
extern glwindow *glwfocus;
extern glwindow *glwdrag;
extern int glwdragpos[2];


/* UI strings are urged to be printed by this function. */
void glwpos2d(double x, double y);
int glwprintf(const char *f, ...);
void glwVScrollBarDraw(glwindow *wnd, int x0, int y0, int w, int h, int range, int iy);
void glwHScrollBarDraw(glwindow *wnd, int x0, int y0, int w, int h, int range, int ix);
int glwVScrollBarMouse(glwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int iy);
int glwHScrollBarMouse(glwindow *wnd, int mousex, int mousey, int x0, int y0, int w, int h, int range, int ix);

class GLwindowMenu : public GLwindow{
public:
	int count;
	struct glwindowmenuitem{
		const char *title;
		int key;
		const char *cmd;
		int allocated; /* whether this item need to be freed */
	} *menus;
	GLwindowMenu(const char *title, int count, const char *menutitles[], const int keys[], const char *cmd[], int sticky);
	void draw(int,int,double);
	int mouse(int button, int state, int x, int y);
	int key(int key);
	~GLwindowMenu();
	GLwindowMenu *addItem(const char *title, int key, const char *cmd);
	static int cmd_addcmdmenuitem(int argc, char *argv[], void *p);
};

extern const int glwMenuAllAllocated[];
extern const char glwMenuSeparator[];
GLwindowMenu *glwMenu(const char *title, int count, const char *menutitles[], const int keys[], const char *cmd[], int sticky);
glwindow *glwPopupMenu(viewport &, int count, const char *menutitles[], const int keys[], const char *cmd[], int sticky);


#if 0
struct glwindowsizeable{
	glwindow st;
	float ratio; /* ratio of window size if it is to be reserved */
	int sizing; /* edge flags of changing borders */
	int minw, minh, maxw, maxh;
};

void glwsizeable_init(struct glwindowsizeable *p);
int glwsizeable_mouse(glwindow *wnd, int button, int state, int x, int y);
#endif
#endif
