/** \file
 * \brief Implementation of GLWbutton and its friends.
 */
#define NOMINMAX
#include "glw/GLWbutton.h"
#include "Application.h"
#include "cmd.h"
#include "../cmd_int.h"
#include "antiglut.h"
#include "draw/material.h"
#include "Player.h" // GLWmoveOrderButton
#include "sqadapt.h"
#include "glw/message.h"
#include "GLWtip.h"
extern "C"{
#include <clib/cfloat.h>
}





const char *GLWbutton::classname()const{return "GLWbutton";}

void GLWbutton::anim(double dt){
	flashTime = approach(flashTime, 0., dt, 0.);
}

void GLWbutton::mouseEnter(GLwindowState &){}
void GLWbutton::mouseLeave(GLwindowState &){}
int GLWbutton::mouse(GLwindowState &ws, int button, int state, int mousex, int mousey){
	if(!extentRect().include(mousex, mousey)){
		depress = false;
		if(glwtip->parent == this){
			glwtip->tips = NULL;
			glwtip->parent = NULL;
			glwtip->setVisible(false);
		}
		return 0;
	}
	else if(state == GLUT_KEEP_UP && tipstring){
		int xs, ys;
		glwGetSizeStringML(tipstring, GLwindow::getFontHeight(), &xs, &ys);
		GLWrect localrect = GLWrect(xpos, ypos - ys - 3 * GLwindow::margin, xpos + xs + 3 * GLwindow::margin, ypos);
		GLWrect parentrect = parent->clientRect();
		localrect.rmove(parentrect.x0, parentrect.y0);

		// Adjust rect to fit in the screen. No care is taken if tips window is larger than the screen.
		if(localrect.x0 < 0)
			localrect.move(0, localrect.y0);
		else if(ws.w < localrect.x1)
			localrect.rmove(ws.w - localrect.x1, 0);
		if(localrect.y0 < 0)
			localrect.move(localrect.x0, 0);
		else if(ws.h < localrect.y1)
			localrect.rmove(0, ws.h - localrect.y1);

		glwtip->setExtent(localrect);
		glwtip->tips = tipstring;
		glwtip->parent = this;
		glwtip->setVisible(true);
		glwActivate(glwFindPP(glwtip));
//		glwActivate(GLwindow::findpp(&glwlist, &GLWpointerCompar(glwtip)));
	}
	if(state == GLUT_UP){
		if(depress){
			depress = false;
			press();
		}
	}
	else if(state == GLUT_DOWN){
		depress = true;
	}
	return 0;
}

SQInteger GLWbutton::sqGet(HSQUIRRELVM v, const SQChar *wcs)const{
	if(!strcmp(wcs, _SC("depressed"))){
		sq_pushbool(v, depress);
		return 1;
	}
	else if(!strcmp(wcs, _SC("flashTime"))){
		sq_pushfloat(v, flashTime);
		return 1;
	}
	else if(!strcmp(wcs, _SC("tip"))){
		sq_pushstring(v, tipstring, -1);
		return 1;
	}
	else
		return GLelement::sqGet(v, wcs);
}

SQInteger GLWbutton::sqSet(HSQUIRRELVM v, const SQChar *wcs){
	if(!strcmp(wcs, _SC("depressed"))){
		SQBool b;
		if(SQ_FAILED(sq_getbool(v, 3, &b)))
			return SQ_ERROR;
		depress = !!b;
		return 0;
	}
	else if(!strcmp(wcs, _SC("flashTime"))){
		SQFloat f;
		if(SQ_FAILED(sq_getfloat(v, 3, &f)))
			return SQ_ERROR;
		flashTime = f;
		return 0;
	}
	else
		return GLelement::sqSet(v, wcs);
}



GLWcommandButton::GLWcommandButton(Game *game, const char *filename, const char *command, const char *tips) :
	st(game),
	texname(filename)
{
	xpos = ypos = 0;
	width = height = 32;
	if(command){
		this->command = new const char[strlen(command) + 1];
		strcpy(const_cast<char*>(this->command), command);
	}
	else
		this->command = NULL;

	if(tips){
		this->tipstring = new const char[strlen(tips) + 1];
		strcpy(const_cast<char*>(this->tipstring), tips);
	}
	else
		this->tipstring = NULL;
}

GLWcommandButton::~GLWcommandButton(){
	delete command;
	delete tipstring;
}

void GLWcommandButton::draw(GLwindowState &ws, double){
	GLubyte mod = depress ? 127 : 255;
	const gltestp::TexCacheBind *tcb = gltestp::FindTexture(texname);
	GLuint texlist;
	if(tcb)
		texlist = tcb->getList();
	else{
		suftexparam_t stp;
		stp.flags = STP_MINFIL | STP_ALPHA;
		stp.minfil = GL_LINEAR;
		texlist = CallCacheBitmap(texname, texname, &stp, NULL);
	}
	if(!texlist)
		return;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glColor4ub(mod,mod,mod,255);
	glCallList(texlist);
	glBegin(GL_QUADS);
	glTexCoord2i(0,1); glVertex2i(xpos, ypos);
	glTexCoord2i(1,1); glVertex2i(xpos + width, ypos);
	glTexCoord2i(1,0); glVertex2i(xpos + width, ypos + height);
	glTexCoord2i(0,0); glVertex2i(xpos, ypos + height);
	glEnd();
	glPopAttrib();
}

void GLWcommandButton::press(){
	CmdExec(command);
}

void GLWcommandButton::mouseLeave(GLwindowState &ws){
	depress = false;
	if(glwtip->parent == this){
		glwtip->tips = NULL;
		glwtip->parent = NULL;
		glwtip->setExtent(GLWrect(-10,-10,-10,-10));
	}
}



GLWstateButton::GLWstateButton(Game *game, const char *filename, const char *filename1, const char *tips) :
	st(game),
	texname(filename), texname1(filename1)
{
	xpos = ypos = 0;
	width = height = 32;
/*	if(command){
		this->command = new const char[strlen(command) + 1];
		strcpy(const_cast<char*>(this->command), command);
	}
	else
		this->command = NULL;*/

	if(tips){
		this->tipstring = new const char[strlen(tips) + 1];
		strcpy(const_cast<char*>(this->tipstring), tips);
	}
	else
		this->tipstring = NULL;
}

GLWstateButton::~GLWstateButton(){
//	delete command;
	delete tipstring;
}

void GLWstateButton::mouseLeave(GLwindowState &ws){
	depress = false;
	if(glwtip->parent == this){
		glwtip->tips = NULL;
		glwtip->parent = NULL;
		glwtip->setExtent(GLWrect(-10,-10,-10,-10));
	}
}

/// Draws button image. Button image 0 is drawn when not
/// active.
void GLWstateButton::draw(GLwindowState &ws, double){
	bool s = state(); // Store the value to the local variable to prevent multiple evaluation

	// We could have empty texname1, in which case we use the darkened version of the texture
	// of the active state.  The image can be darkened when the button is depressed, too.
	GLfloat mod = depress || this->texname1 == "" && !s ? 0.5 : 1.;
	gltestp::dstring texname = s || this->texname1 == "" ? this->texname : this->texname1;

	GLuint texlist;
	const gltestp::TexCacheBind *tcb = gltestp::FindTexture(texname);
	if(tcb)
		texlist = tcb->getList();
	else{
		suftexparam_t stp;
		stp.flags = STP_MINFIL | STP_ALPHA;
		stp.minfil = GL_LINEAR;
		texlist = CallCacheBitmap(texname, texname, &stp, NULL);
	}
	if(!texlist)
		return;
	glPushAttrib(GL_TEXTURE_BIT | GL_ENABLE_BIT);
	glColor4f(mod,mod,mod,1.f);
	glCallList(texlist);
	glBegin(GL_QUADS);
	glTexCoord2i(0,1); glVertex2i(xpos, ypos);
	glTexCoord2i(1,1); glVertex2i(xpos + width, ypos);
	glTexCoord2i(1,0); glVertex2i(xpos + width, ypos + height);
	glTexCoord2i(0,0); glVertex2i(xpos, ypos + height);
	glEnd();
	if(0 < flashTime){
		glEnable(GL_BLEND);

		// Reserve previous blendfunc
		GLint blendSrc, blendDst;
		glGetIntegerv(GL_BLEND_SRC, &blendSrc);
		glGetIntegerv(GL_BLEND_DST, &blendDst);

		// Set to add blend
		glBlendFunc(GL_ONE, GL_ONE);

		mod = fmod(flashTime, 1.f) * 4.f;
		if(2.f < mod)
			mod = 4.f - mod;
		glColor4f(mod, mod, mod, 1.f);
		glBegin(GL_QUADS);
		glTexCoord2i(0,1); glVertex2i(xpos, ypos);
		glTexCoord2i(1,1); glVertex2i(xpos + width, ypos);
		glTexCoord2i(1,0); glVertex2i(xpos + width, ypos + height);
		glTexCoord2i(0,0); glVertex2i(xpos, ypos + height);
		glEnd();

		// Restore previous values
		glBlendFunc(blendSrc, blendDst);
	}
	glPopAttrib();
}




bool GLWtoggleCvarButton::state()const{
	CVar *cv = CvarFind(var);
	if(!cv || cv->type != cvar_int)
		return 0;
	else
		return *cv->v.i;
}

void GLWtoggleCvarButton::press(){
	CVar *cv = CvarFind(var);
	if(!cv || cv->type != cvar_int)
		;
	else
		*cv->v.i = !*cv->v.i;
}






GLWbuttonMatrix::GLWbuttonMatrix(Game *game, int x, int y, int xsize, int ysize) :
	st(game, "Button Matrix"),
	xbuttons(x), ybuttons(y),
	xbuttonsize(xsize), ybuttonsize(ysize),
	buttons(new (GLWbutton*[x * y]))
{
	flags |= GLW_COLLAPSABLE | GLW_CLOSE | GLW_PINNABLE;
	GLWrect r = adjustRect(GLWrect(0, 0, x * xsize, y * ysize));
	width = r.x1 - r.x0;
	height = r.y1 - r.y0;
	memset(buttons, 0, x * y * sizeof(GLWbutton*));
}

GLWbuttonMatrix::~GLWbuttonMatrix(){
	// Delete contained controls
	for(int y = 0; y < ybuttons; y++) for(int x = 0; x < xbuttons; x++){
		if(buttons[x + y * xbuttons])
			delete buttons[x + y * xbuttons];
	}
}

void GLWbuttonMatrix::anim(double dt){
	for(int y = 0; y < ybuttons; y++) for(int x = 0; x < xbuttons; x++){
		if(buttons[x + y * xbuttons])
			buttons[x + y * xbuttons]->anim(dt);
	}
}

void GLWbuttonMatrix::draw(GLwindowState &ws, double dt){
	GLWrect r = clientRect();
	glPushMatrix();
	glTranslatef(r.x0, r.y0, 0);
	glColor4f(.5,.5,.5,1);
	glBegin(GL_LINES);
	for(int y = 1; y < ybuttons; y++){
		glVertex2i(0, y * ybuttonsize);
		glVertex2i(xbuttons * xbuttonsize, y * ybuttonsize);
	}
	for(int x = 1; x < xbuttons; x++){
		glVertex2i(x * xbuttonsize, 0);
		glVertex2i(x * xbuttonsize, ybuttons * ybuttonsize);
	}
	glEnd();
	for(int y = 0; y < ybuttons; y++) for(int x = 0; x < xbuttons; x++){
		if(buttons[y * xbuttons + x])
			buttons[y * xbuttons + x]->draw(ws, dt);
	}
	glPopMatrix();
}

bool GLWbuttonMatrix::focusable()const{
	return false;
}

int GLWbuttonMatrix::mouse(GLwindowState &ws, int button, int state, int mousex, int mousey){
	// You shouldn't be able to press or show hints for buttons in a collapsed window.
	if(flags & GLW_COLLAPSE)
		return 0;

	if(state == GLUT_KEEP_DOWN || state == GLUT_KEEP_UP){
		for(int i = 0; i < xbuttons * ybuttons; i++) if(buttons[i])
			buttons[i]->mouse(ws, button, state, mousex, mousey);
	}
	else{
		int x = (mousex) / xbuttonsize;
		int y = (mousey) / ybuttonsize;
		GLWrect cr = clientRect();
		if(0 <= x && x < xbuttons && 0 <= y && y < ybuttons){
			GLWbutton *p = buttons[y * xbuttons + x];
			if(p)
				return p->mouse(ws, button, state, mousex, mousey);
		}
	}
	return 1;
}

void GLWbuttonMatrix::mouseEnter(GLwindowState &ws){
	for(int i = 0; i < xbuttons * ybuttons; i++) if(buttons[i])
		buttons[i]->mouseEnter(ws);
}

void GLWbuttonMatrix::mouseLeave(GLwindowState &ws){
	for(int i = 0; i < xbuttons * ybuttons; i++) if(buttons[i])
		buttons[i]->mouseLeave(ws);
}

bool GLWbuttonMatrix::addButton(GLWbutton *b, int x, int y){
	int i;
	if(x < 0 || y < 0){
		for(i = 0; i < xbuttons * ybuttons; i++) if(!buttons[i])
			break;
		if(xbuttons * ybuttons <= i)
			return false;
	}
	else if(0 <= x && x < xbuttons && 0 <= y && y < ybuttons)
		i = x + y * xbuttons;
	else
		return false;
	b->parent = this;
	this->buttons[i] = b;
	b->setExtent(GLWrect(i % xbuttons * xbuttonsize, i / xbuttons * ybuttonsize, i % xbuttons * xbuttonsize + xbuttonsize, i / xbuttons * ybuttonsize + ybuttonsize));
	return true;
}

static SQInteger sqf_GLWbuttonMatrix_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = (Game*)sq_getforeignptr(v);
	if(!game)
		return sq_throwerror(v, _SC("The game object is not assigned"));
	SQInteger x, y, sx, sy;
	if(argc <= 1 || SQ_FAILED(sq_getinteger(v, 2, &x)))
		x = 3;
	if(argc <= 2 || SQ_FAILED(sq_getinteger(v, 3, &y)))
		y = 3;
	if(argc <= 3 || SQ_FAILED(sq_getinteger(v, 4, &sx)))
		sx = 64;
	if(argc <= 4 || SQ_FAILED(sq_getinteger(v, 5, &sy)))
		sy = 64;
	GLWbuttonMatrix *p = new GLWbuttonMatrix(game, x, y, sx, sy);
	GLelement::sq_assignobj(v, p);
	glwAppend(p);
	return 0;
}

static SQInteger sqf_GLWbuttonMatrix_addButton(HSQUIRRELVM v){
	GLWbuttonMatrix *p = GLWbuttonMatrix::sq_refobj(v);
	const SQChar *cmd, *path, *tips;
	if(SQ_FAILED(sq_getstring(v, 2, &cmd))){
		// The addButton function is polymorphic, it can take GLWbutton object as argument too.
		GLelement *element = GLelement::sq_refobj(v, 2);
		if(!element)
			return SQ_ERROR;
		// TODO: Type unsafe! element can be object other than GLWbutton.
		if(!p->addButton(static_cast<GLWbutton*>(element)))
			return sq_throwerror(v, _SC("Could not add button"));
		element->sq_pushobj(v, element);
		return 1;
	}
	if(SQ_FAILED(sq_getstring(v, 3, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 4, &tips)))
		tips = NULL;
	GLWcommandButton *b = new GLWcommandButton(p->getGame(), path, cmd, tips);
	if(!p->addButton(b)){
		delete b;
		return sq_throwerror(v, _SC("Could not add button"));
	}
	// Returning created GLWbutton instance is a bit costly, but it won't be frequent operation.
	b->sq_pushobj(v, b);
	return 1;
}

static SQInteger sqf_GLWbuttonMatrix_addToggleButton(HSQUIRRELVM v){
	GLWbuttonMatrix *p = GLWbuttonMatrix::sq_refobj(v);
	const SQChar *cvarname, *path, *path1, *tips;
	if(SQ_FAILED(sq_getstring(v, 2, &cvarname)))
		return SQ_ERROR;
/*	int value;
	cvar *cv = CvarFind(cvarname);
	if(!cv || cv->type != cvar_int)
		value = 0;
	else
		value = *cv->v.i;*/
	if(SQ_FAILED(sq_getstring(v, 3, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 4, &path1)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 5, &tips)))
		tips = NULL;

	// Temporary!! the third param must be a reference to persistent object, which is not in this case!
	GLWtoggleCvarButton *b = new GLWtoggleCvarButton(p->getGame(), path, path1, cvarname, tips);
	if(!p->addButton(b)){
		delete b;
		return sq_throwerror(v, _SC("Could not add button"));
	}
	b->sq_pushobj(v, b);
	return 1;
}

static SQInteger sqf_GLWbuttonMatrix_addMoveOrderButton(HSQUIRRELVM v){
	GLWbuttonMatrix *p = GLWbuttonMatrix::sq_refobj(v);
	const SQChar *path, *path1, *tips;
	if(SQ_FAILED(sq_getstring(v, 2, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 3, &path1)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 4, &tips)))
		tips = NULL;
	GLWstateButton *b = Player::newMoveOrderButton(p->getGame(), path, path1, tips);
	if(!p->addButton(b)){
		delete b;
		return sq_throwerror(v, _SC("Could not add button"));
	}
	b->sq_pushobj(v, b);
	return 1;
}

/// \brief getButton(pos)
/// \param pos zero-based index for target button.
static SQInteger sqf_GLWbuttonMatrix_getButton(HSQUIRRELVM v){
	GLWbuttonMatrix *p = GLWbuttonMatrix::sq_refobj(v);
	const SQChar *path, *path1, *tips;
	SQInteger i;
	if(SQ_FAILED(sq_getinteger(v, 2, &i)))
		return SQ_ERROR;
	GLWbutton *b = p->getButton(i);
	if(!b){
		sq_pushnull(v);
		return 1;
	}
	b->sq_pushobj(v, b);
	return 1;
}

class GLWsqStateButton : public GLWstateButton{
public:
	HSQOBJECT hstatefunc;
	HSQOBJECT hpressfunc;
	GLWsqStateButton(Game *game, const char *filename, const char *filename1, const HSQOBJECT &hstatefunc, const HSQOBJECT &hpressfunc, const char *tip = NULL) :
		GLWstateButton(game, filename, filename1, tip), hstatefunc(hstatefunc), hpressfunc(hpressfunc){}
	virtual bool state()const;
	virtual void press();
	virtual const SQChar *sqClassName()const{return _SC("GLWsqStateButton");}
	static SQInteger sqf_constructor(HSQUIRRELVM v);
	static bool sq_define(HSQUIRRELVM v);
	~GLWsqStateButton(){
		HSQUIRRELVM v = game->sqvm;
		sq_release(v, &hstatefunc);
		sq_release(v, &hpressfunc);
	}
};

bool GLWsqStateButton::state()const{
	if(hstatefunc._type == OT_NULL)
		return false;
	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushobject(v, hstatefunc);
	sq_pushroottable(v);
	if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
		return false;
	SQBool b;
	if(SQ_FAILED(sq_getbool(v, -1, &b)))
		return false;
	return b;
}

void GLWsqStateButton::press(){
	if(hpressfunc._type == OT_NULL)
		return;
	HSQUIRRELVM v = game->sqvm;
	StackReserver sr(v);
	sq_pushobject(v, hpressfunc);
	sq_pushroottable(v);
	if(SQ_FAILED(sq_call(v, 1, SQTrue, SQTrue)))
		return;
}

SQInteger GLWsqStateButton::sqf_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	Game *game = (Game*)sq_getforeignptr(v);
	if(!game)
		return sq_throwerror(v, _SC("The game object is not assigned"));
	const SQChar *path, *path1, *tips, *statefunc, *pressfunc;
	if(SQ_FAILED(sq_getstring(v, 2, &path)))
		return SQ_ERROR;
	if(SQ_FAILED(sq_getstring(v, 3, &path1)))
		path1 = ""; // We allow null for the second argument.

	// Assign hstatefunc member variable
	HSQOBJECT hstatefunc;
	if(SQ_FAILED(sq_getstackobj(v, 4, &hstatefunc)))
		return SQ_ERROR;
	sq_addref(v, &hstatefunc);

	// Assign hpressfunc member variable
	HSQOBJECT hpressfunc;
	if(SQ_FAILED(sq_getstackobj(v, 5, &hpressfunc)))
		return SQ_ERROR;
	sq_addref(v, &hpressfunc);

	if(SQ_FAILED(sq_getstring(v, 6, &tips)))
		tips = NULL;
	GLWsqStateButton *b = new GLWsqStateButton(game, path, path1, hstatefunc, hpressfunc, tips);
	GLelement::sq_assignobj(v, b);
	return 1;
}

bool GLWsqStateButton::sq_define(HSQUIRRELVM v){
	// Define class GLWsqStateButton
	GLwindow::sq_define(v);
	sq_pushstring(v, _SC("GLWsqStateButton"), -1);
	sq_pushstring(v, _SC("GLelement"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLWsqStateButton");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("constructor"), sqf_constructor);
	sq_createslot(v, -3);
	return true;
}

static sqa::Initializer GLWsqStateButton_init("GLWsqStateButton", GLWsqStateButton::sq_define);



GLWbuttonMatrix *GLWbuttonMatrix::sq_refobj(HSQUIRRELVM v, SQInteger idx){
	// It's crucial to static_cast in this order, not just reinterpret_cast.
	// It'll make difference when multiple inheritance is involved.
	// Either way, virtual inheritances are prohibited, or use dynamic_cast.
	return static_cast<GLWbuttonMatrix*>(GLwindow::sq_refobj(v, idx));
}



static bool GLWbuttonMatrix_sq_define(HSQUIRRELVM v){
	// Define class GLWbuttonMatrix
	GLwindow::sq_define(v);
	sq_pushstring(v, _SC("GLWbuttonMatrix"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, 1);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLWbuttonMatrix");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("constructor"), sqf_GLWbuttonMatrix_constructor);
	register_closure(v, _SC("addButton"), sqf_GLWbuttonMatrix_addButton);
	register_closure(v, _SC("addToggleButton"), sqf_GLWbuttonMatrix_addToggleButton);
	register_closure(v, _SC("addMoveOrderButton"), sqf_GLWbuttonMatrix_addMoveOrderButton);
	register_closure(v, _SC("getButton"), sqf_GLWbuttonMatrix_getButton);
	sq_createslot(v, -3);
	return true;
}

static sqa::Initializer init_GLWbuttonMatrix("GLWbuttonMatrix", GLWbuttonMatrix_sq_define);

