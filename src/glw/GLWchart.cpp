#include "glw/GLWchart.h"
#include "cmd.h"

#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>


GLWchart::GLWchart(const char *title, ChartSeries *_series) : st(title){
	flags = GLW_CLOSE | GLW_COLLAPSABLE;
	if(_series)
		series.push_back(_series);
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
		if(cs){
			GLWrect cr = clientRect();
			int i, all = cs->count();

			glColor4fv(cs->color());
			glBegin(GL_LINE_STRIP);
			for(i = 0; i < all; i++){
				glVertex2d(cr.x0 * (all - i - 1) / all + cr.x1 * i / all, cr.y1 - (cr.height() - 2) * cs->value(i));
			}
			glEnd();
		}
	}
}


class CmdRegister{
	class FrameTimeChartSeries : public GLWchart::TimeChartSeries{
		virtual double timeProc(double dt){return dt;}
	};
	class FrameRateChartSeries : public GLWchart::TimeChartSeries{
		virtual Vec4f color()const{return Vec4f(0,0,1,1);}
		virtual double timeProc(double dt){return 1. / dt;}
	};

	static int cmd_chart(int argc, char *argv[]){
		GLWchart *wnd = new GLWchart("Frame time", new FrameTimeChartSeries);
		wnd->addSeries(new FrameRateChartSeries());
		glwAppend(wnd);
		return 0;
	}
public:
	CmdRegister(){
		CmdAdd("chart", cmd_chart);
	}
} cmdreg;