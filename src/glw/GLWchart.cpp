#include "glw/GLWchart.h"
#include "cmd.h"
#include "../Application.h"
#include "sqadapt.h"
#include "antiglut.h"
#include "glw/popup.h"
#include "GLWmenu.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>


static sqa::Initializer definer("GLWchart", GLWchart::sq_define);


GLWchart::GLWchart(const char *title, ChartSeries *_series) : st(title){
	flags = GLW_CLOSE | GLW_COLLAPSABLE;
	if(_series)
		series.push_back(_series);
}

int GLWchart::mouse(GLwindowState &ws, int button, int state, int x, int y){
	struct PopupMenuItemToggleShow : public PopupMenuItem{
		typedef PopupMenuItem st;
		WeakPtr<GLWchart> p;
		int index;
		PopupMenuItemToggleShow(gltestp::dstring title, GLWchart *p, int index) : st(title), p(p), index(index){}
		PopupMenuItemToggleShow(const PopupMenuItemToggleShow &o) : st(o), p(o.p), index(o.index){}
		virtual void execute(){
			if(p)
				p->setSeriesVisible(index, Toggle);
		}
		virtual PopupMenuItem *clone(void)const{return new PopupMenuItemToggleShow(*this);}
	};

	if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP){
		PopupMenu pm;
		for(int i = 0; i < series.size(); i++)
			pm.append(new PopupMenuItemToggleShow(gltestp::dstring(series[i]->visible ? "Hide " : "Show ") << series[i]->labelname(), this, i));

		glwPopupMenu(ws, pm);
	}
	return 1;
}

void GLWchart::setSeriesVisible(int i, Visibility v){
	if(i < 0 || series.size() <= i)
		return;
	switch(v){
		case Show: series[i]->visible = true; break;
		case Hide: series[i]->visible = false; break;
		case Toggle: series[i]->visible = !series[i]->visible; break;
	}
}

void GLWchart::anim(double dt){
	for(int i = 0; i < series.size(); i++){
		if(series[i])
			series[i]->anim(dt, this);
	}
}

void GLWchart::draw(GLwindowState &ws, double t){
	for(int i = 0; i < series.size(); i++){
		ChartSeries *cs = series[i];
		if(cs && cs->visible){
			GLWrect cr = clientRect();

			glColor4fv(cs->color());
			glwpos2d(cr.x0, cr.y0 + (i + 2) * GLwindow::glwfontwidth * glwfontscale);
			glwprintf(cs->labelstr());
			glBegin(GL_LINE_STRIP);
			int all = cs->count();
			for(int i = 0; i < all; i++){
				glVertex2d(cr.x0 * (all - i - 1) / all + cr.x1 * i / all, cr.y1 - (cr.height() - 2) * cs->value(i));
			}
			glEnd();
		}
	}
}

/// Define class GLWmessage for Squirrel
bool GLWchart::sq_define(HSQUIRRELVM v){
	st::sq_define(v);
	sq_pushstring(v, _SC("GLWchart"), -1);
	sq_pushstring(v, _SC("GLwindow"), -1);
	sq_get(v, -3);
	sq_newclass(v, SQTrue);
	sq_settypetag(v, -1, "GLWchart");
	sq_setclassudsize(v, -1, sizeof(WeakPtr<GLelement>));
	register_closure(v, _SC("constructor"), sqf_constructor);
	register_closure(v, _SC("addSeries"), sqf_addSeries);
	sq_createslot(v, -3);
	return true;
}

/// Squirrel constructor
SQInteger GLWchart::sqf_constructor(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);

	GLWchart *p = new GLWchart("Chart");

	sq_assignobj(v, p);
	glwAppend(p);
	return 0;
}

class FrameTimeChartSeries : public GLWchart::TimeChartSeries{
	virtual gltestp::dstring labelname()const{return "Frame Time";}
	virtual double timeProc(double dt){return dt;}
};
class FrameRateChartSeries : public GLWchart::TimeChartSeries{
	virtual gltestp::dstring labelname()const{return "Frame Rate";}
	virtual Vec4f color()const{return Vec4f(0,0,1,1);}
	virtual double timeProc(double dt){return dt == 0. ? 0. : 1. / dt;}
};
class RecvBytesChartSeries : public GLWchart::TimeChartSeries{
	virtual gltestp::dstring labelname()const{return "Received bytes";}
	virtual Vec4f color()const{return Vec4f(1,0,0,1);}
	virtual double timeProc(double dt){
		return application.mode & application.ServerBit ? application.server.sv->sendbufsiz : application.recvbytes;
	}
};

/// Squirrel add series
SQInteger GLWchart::sqf_addSeries(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	const SQChar *sstr;
	if(SQ_FAILED(sq_getstring(v, -1, &sstr))){
		return sq_throwerror(v, _SC("No string as addSeries arg"));
	}

	SQUserPointer up;
	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of GLWchart.");
	GLWchart *wnd = static_cast<GLWchart*>(static_cast<GLelement*>(*(WeakPtr<GLelement>*)up));
	if(!strcmp(sstr, _SC("framerate"))){
		wnd->addSeries(new FrameRateChartSeries());
	}
	else if(!strcmp(sstr, _SC("recvbytes")))
		wnd->addSeries(new RecvBytesChartSeries());
	return 0;
}


class CmdRegister{
	static int cmd_chart(int argc, char *argv[]){
		GLWchart *wnd = new GLWchart("Frame time", new FrameTimeChartSeries);
//		wnd->addSeries(new FrameRateChartSeries());
		wnd->addSeries(new RecvBytesChartSeries());
		glwAppend(wnd);
		return 0;
	}
public:
	CmdRegister(){
		CmdAdd("chart", cmd_chart);
	}
} cmdreg;