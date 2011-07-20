#ifndef GLWCHART_H
#define GLWCHART_H

#include "glwindow.h"
#include <cpplib/vec4.h>


class EXPORT GLWchart : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;

	class ChartSeries{
	public:
		virtual double value(int index) = 0;
		virtual int count() const = 0;
		virtual Vec4f color()const{return Vec4f(1,1,1,1);}
		virtual void anim(double dt, GLWchart *){}
	};

	class TimeChartSeries;

	GLWchart(const char *title, ChartSeries *_series = NULL);

	void addSeries(ChartSeries *s){series.push_back(s);}

protected:
	std::vector<ChartSeries *> series;

	const char *classname()const{return "GLWchart";}
	virtual void anim(double dt);
	void draw(GLwindowState &ws, double t);
};


class GLWchart::TimeChartSeries : public GLWchart::ChartSeries{
	std::vector<double> chart;
	double normalizer;

	virtual double value(int index){
		return normalizer ? chart[index] / normalizer : chart[index];
	}
	virtual int count()const{return int(chart.size());}
	virtual void anim(double dt, GLWchart *wnd){
		if(wnd->clientRect().width() < chart.size())
			chart.erase(chart.begin());
		chart.push_back(timeProc(dt));

		normalizer = 0.;
		for(int i = 0; i < chart.size(); i++) if(normalizer < chart[i])
			normalizer = chart[i];
	}
protected:
	virtual double timeProc(double dt) = 0;
};


#endif
