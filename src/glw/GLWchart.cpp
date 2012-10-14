/** \file
 * \brief Implementation of GLWchart class and its related classes.
 */
#include "glw/GLWchart.h"
#include "cmd.h"
#include "Application.h"
#include "sqadapt.h"
#include "antiglut.h"
#include "glw/popup.h"
#include "GLWmenu.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>


class FrameTimeChartSeries : public GLWchart::TimeChartSeries{
public:
	FrameTimeChartSeries(int ygroup = -1, const Vec4f *color = NULL)
		: TimeChartSeries(ygroup, color ? *color : Vec4f(1,1,1,1)){}
	virtual gltestp::dstring labelstr()const{return gltestp::dstring("Frame Time: ") << TimeChartSeries::labelstr();}
	virtual gltestp::dstring labelname()const{return "Frame Time";}
	virtual double timeProc(double dt){return dt;}
};
class FrameRateChartSeries : public GLWchart::TimeChartSeries{
public:
	FrameRateChartSeries(int ygroup = -1, const Vec4f *color = NULL)
		: TimeChartSeries(ygroup, color ? *color : Vec4f(0,0,1,1)){}
	virtual gltestp::dstring labelstr()const{return gltestp::dstring("Frame Rate: ") << TimeChartSeries::labelstr();}
	virtual gltestp::dstring labelname()const{return "Frame Rate";}
	virtual double timeProc(double dt){return dt == 0. ? 0. : 1. / dt;}
};
class RecvBytesChartSeries : public GLWchart::TimeChartSeries{
public:
	RecvBytesChartSeries(int ygroup = -1, const Vec4f *color = NULL)
		: TimeChartSeries(ygroup, color ? *color : Vec4f(1,0,0,1)){}
	virtual gltestp::dstring labelstr()const{return gltestp::dstring("Received bytes: ") << TimeChartSeries::labelstr();}
	virtual gltestp::dstring labelname()const{return "Received bytes";}
	virtual double timeProc(double dt){
		return application.mode & application.ServerBit ? application.server.sv->sendbufsiz : application.recvbytes;
	}
};
class GLWchart::SampledChartSeries : public GLWchart::TimeChartSeries{
	virtual gltestp::dstring labelstr()const{return gltestp::dstring() << label << ": " << TimeChartSeries::labelstr();}
	virtual gltestp::dstring labelname()const{return label;}
	virtual double timeProc(double dt){
		return lastValue;
	}
	virtual void addSample(double v){
		lastValue = v;
	}
public:
	double lastValue;
	gltestp::dstring label;
	SampledChartSeries(int ygroup = -1, gltestp::dstring label = "", const Vec4f *color = NULL)
		: TimeChartSeries(ygroup, color ? *color : Vec4f(1,0,1,1)), label(label){}
};

/// \brief The base class for a histogram chart.
class HistogramChartSeries : public GLWchart::NormalizedChartSeries{
public:
	HistogramChartSeries(int ygroup, const Vec4f &color)
		: maxvalue(1), NormalizedChartSeries(ygroup, color){}
protected:
	std::vector<int> values;
	int maxvalue;
	virtual double getMax()const{return maxvalue;}
	virtual int minThreshold()const{return 0;}
	virtual int getValue(double dt)const = 0;
	void anim(double dt, GLWchart *c){
		int v = getValue(dt);
		if(v <= minThreshold())
			return;
		if(values.size() <= v)
			values.resize(v+1);
		values[v]++;
		if(maxvalue < values[v])
			maxvalue = values[v];
	}
	double valueBeforeNormalize(int index){
		return (double)values[index];
	}
	int count()const{return values.size();}
	virtual gltestp::dstring labelname()const{return "histogram";}
	virtual gltestp::dstring labelstr()const{return gltestp::dstring(maxvalue) << "/" << values.size();}
};

/// \brief Histogram of frame time.
class FrameTimeHistogramChartSeries : public HistogramChartSeries{
public:
	FrameTimeHistogramChartSeries(int ygroup = -1, const Vec4f *color = NULL)
		: HistogramChartSeries(ygroup, color ? *color : Vec4f(0,1,1,1)){}
	static const double cellsize;
	int getValue(double dt)const{return dt / cellsize;}
	virtual gltestp::dstring labelstr()const{return gltestp::dstring("Frame time histogram: ") << HistogramChartSeries::labelstr();}
	virtual gltestp::dstring labelname()const{return "Frame time histogram";}
};
const double FrameTimeHistogramChartSeries::cellsize = 0.001;

/// \brief Histogram of frame rate [fps].
class FrameRateHistogramChartSeries : public HistogramChartSeries{
public:
	FrameRateHistogramChartSeries(int ygroup = -1, const Vec4f *color = NULL)
		: HistogramChartSeries(ygroup, color ? *color : Vec4f(1,0,1,1)){}
	static const double cellsize;
	int getValue(double dt)const{return dt == 0. ? 0 : 1. / dt / cellsize;}
	virtual gltestp::dstring labelstr()const{return gltestp::dstring("Frame rate histogram: ") << HistogramChartSeries::labelstr();}
	virtual gltestp::dstring labelname()const{return "Frame rate histogram";}
};
const double FrameRateHistogramChartSeries::cellsize = 0.1;

/// \brief Histogram of received bytes in the frame.
class RecvBytesHistogramChartSeries : public HistogramChartSeries{
public:
	RecvBytesHistogramChartSeries(int ygroup = -1, const Vec4f *color = NULL)
		: HistogramChartSeries(ygroup, color ? *color : Vec4f(1,1,0,1)){}
	static const int cellsize = 32;
	int minThreshold()const{return 4;}
	int getValue(double)const{return (application.mode & application.ServerBit ? application.server.sv->sendbufsiz : application.recvbytes) / cellsize;}
	virtual gltestp::dstring labelstr()const{return gltestp::dstring("Received bytes histogram: ") << HistogramChartSeries::labelstr();}
	virtual gltestp::dstring labelname()const{return "Received bytes histogram";}
};


static sqa::Initializer definer("GLWchart", GLWchart::sq_define);


GLWchart::GLWchart(const char *title, ChartSeries *_series) : st(title), sampled(NULL){
	flags = GLW_CLOSE | GLW_COLLAPSABLE;
	if(_series)
		series.push_back(_series);
}

template<typename T>
struct PopupMenuItemAddSeries : public PopupMenuItem{
	typedef PopupMenuItem st;
	typedef PopupMenuItemAddSeries tt;
	WeakPtr<GLWchart> p;
	tt(gltestp::dstring title, GLWchart *p) : st(title), p(p){}
	tt(const tt &o) : st(o), p(o.p){}
	virtual void execute(){
		if(p)
			p->addSeries(new T());
	}
	virtual PopupMenuItem *clone(void)const{return new tt(*this);}
};

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

	struct PopupMenuItemDelete : public PopupMenuItem{
		typedef PopupMenuItem st;
		typedef PopupMenuItemDelete tt;
		WeakPtr<GLWchart> p;
		int index;
		tt(gltestp::dstring title, GLWchart *p, int index) : st(title), p(p), index(index){}
		tt(const tt &o) : st(o), p(o.p), index(o.index){}
		virtual void execute(){
			if(p){
				delete p->series[index];
				p->series.erase(p->series.begin() + index);
			}
		}
		virtual PopupMenuItem *clone(void)const{return new tt(*this);}
	};

	if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP){
		PopupMenu pm;
		for(int i = 0; i < series.size(); i++)
			pm.append(new PopupMenuItemToggleShow(gltestp::dstring(series[i]->visible ? "Hide " : "Show ") << "[" << i << "] " << series[i]->labelname(), this, i));
		pm.appendSeparator();
		for(int i = 0; i < series.size(); i++)
			pm.append(new PopupMenuItemDelete(gltestp::dstring("Delete ") << "[" << i << "] " << series[i]->labelname(), this, i));
		pm.appendSeparator();
		pm.append(new PopupMenuItemAddSeries<FrameTimeChartSeries>("Add frame time", this));
		pm.append(new PopupMenuItemAddSeries<FrameRateChartSeries>("Add frame rate", this));
		pm.append(new PopupMenuItemAddSeries<RecvBytesChartSeries>("Add received bytes", this));
		pm.append(new PopupMenuItemAddSeries<FrameTimeHistogramChartSeries>("Add frame time histogram", this));
		pm.append(new PopupMenuItemAddSeries<FrameRateHistogramChartSeries>("Add frame rate histogram", this));
		pm.append(new PopupMenuItemAddSeries<RecvBytesHistogramChartSeries>("Add received bytes histogram", this));

		glwPopupMenu(ws, pm);
	}
	return 1;
}

void GLWchart::setSeriesVisible(int i, FlagMod v){
	if(i < 0 || series.size() <= i)
		return;
	series[i]->visible = !!(v & (1 << !!series[i]->visible));
}

void GLWchart::anim(double dt){
	for(int i = 0; i < series.size(); i++){
		if(series[i])
			series[i]->anim(dt, this);
	}
}

void GLWchart::draw(GLwindowState &ws, double t){
	GLWrect cr = clientRect();

	// The first pass draws series lines.
	for(int i = 0; i < series.size(); i++){
		ChartSeries *cs = series[i];
		if(cs && cs->visible){
			glColor4fv(cs->color());
			glBegin(GL_LINE_STRIP);
			int all = cs->count();
			for(int i = 0; i < all; i++){
				glVertex2d(cr.x0 * (all - i - 1) / all + cr.x1 * i / all, cr.y1 - (cr.height() - 2) * cs->value(i, this));
			}
			glEnd();
		}
	}

	int lineHeight = GLwindow::glwfontheight * glwfontscale;

	// The second pass draws the labels.
	for(int i = 0; i < series.size(); i++){
		ChartSeries *cs = series[i];
		if(cs && cs->visible){

			int y = cr.y0 + (i + 2) * lineHeight;
			gltestp::dstring label = cs->labelstr();
			int labelWidth = glwsizef(label);

			glColor4f(0,0,0,0.75f);
			glBegin(GL_QUADS);
			glVertex2d(cr.x0, y);
			glVertex2d(cr.x0 + labelWidth, y);
			glVertex2d(cr.x0 + labelWidth, y - lineHeight);
			glVertex2d(cr.x0, y - lineHeight);
			glEnd();

			glColor4fv(cs->color());
			glwpos2d(cr.x0, y);
			glwprintf(label);
		}
	}

}

void GLWchart::addSample(gltestp::dstring label, double value)const{
	for(int i = 0; i < series.size(); i++){
		if(series[i]->labelname() == label)
			series[i]->addSample(value);
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

/// Squirrel add series
SQInteger GLWchart::sqf_addSeries(HSQUIRRELVM v){
	SQInteger argc = sq_gettop(v);
	const SQChar *sstr;
	if(SQ_FAILED(sq_getstring(v, 2, &sstr))){
		return sq_throwerror(v, _SC("No string as addSeries arg"));
	}

	SQInteger ygroup;
	if(argc < 3 || SQ_FAILED(sq_getinteger(v, 3, &ygroup)))
		ygroup = -1;

	const SQChar *label;
	if(argc < 4 || SQ_FAILED(sq_getstring(v, 4, &label)))
		label = "";

	Vec4f color;
	Vec4f *pcolor = NULL;
	if(5 <= argc){
		for(int j = 0; j < 4; j++){
			static const SQFloat defaultColor[4] = {1., 1., 1., 1.};
			sq_pushinteger(v, j); // color j
			if(SQ_FAILED(sq_get(v, 5))){ // color color[j]
				color[j] = defaultColor[j];
				continue;
			}
			SQFloat f;
			if(SQ_FAILED(sq_getfloat(v, -1, &f)))
				color[j] = defaultColor[j];
			else
				color[j] = f;
			sq_poptop(v); // color
		}
		pcolor = &color;
	}

	SQUserPointer up;
	// If the instance does not have a user pointer, it's a serious exception that might need some codes fixed.
	if(SQ_FAILED(sq_getinstanceup(v, 1, &up, NULL)) || !up)
		throw SQFError("Something's wrong with Squirrel Class Instace of GLWchart.");
	GLWchart *wnd = static_cast<GLWchart*>(static_cast<GLelement*>(*(WeakPtr<GLelement>*)up));
	if(!strcmp(sstr, _SC("frametime")))
		wnd->addSeries(new FrameTimeChartSeries(ygroup, pcolor));
	else if(!strcmp(sstr, _SC("framerate")))
		wnd->addSeries(new FrameRateChartSeries(ygroup, pcolor));
	else if(!strcmp(sstr, _SC("recvbytes")))
		wnd->addSeries(new RecvBytesChartSeries(ygroup, pcolor));
	else if(!strcmp(sstr, _SC("frametimehistogram")))
		wnd->addSeries(new FrameTimeHistogramChartSeries(ygroup, pcolor));
	else if(!strcmp(sstr, _SC("frameratehistogram")))
		wnd->addSeries(new FrameRateHistogramChartSeries(ygroup, pcolor));
	else if(!strcmp(sstr, _SC("recvbyteshistogram")))
		wnd->addSeries(new RecvBytesHistogramChartSeries(ygroup, pcolor));
	else if(!strcmp(sstr, _SC("sampled")))
		wnd->addSeries(wnd->sampled = new SampledChartSeries(ygroup, label, pcolor));
	return 0;
}

