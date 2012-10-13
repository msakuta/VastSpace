/** \file
 * \brief Definition of GLWchart class.
 */
#ifndef GLWCHART_H
#define GLWCHART_H

#include "glwindow.h"
#include <cpplib/vec4.h>

/// \brief The window that is capable of showing multiple series of values as polygonal lines.
///
/// The embedded series is represented by ChartSeries derived classes.
class EXPORT GLWchart : public GLwindowSizeable{
public:
	typedef GLwindowSizeable st;

	/// \brief The internal base class that represents a series in a chart.
	class ChartSeries{
	public:
		virtual double value(int index) = 0;
		virtual int count() const = 0;
		virtual Vec4f color()const{return Vec4f(1,1,1,1);}
		virtual void anim(double dt, GLWchart *){}
		virtual gltestp::dstring labelname()const{return "none";}
		virtual gltestp::dstring labelstr()const{return "";}
		bool visible;
	};

	/// \brief A flag modification code that specifies how the flag should be changed from the previous state.
	///
	/// It's merely a single-bit truth table. The value's bit 0 designates the resulting state when the
	/// previous state is off. In contrast, bit 1 indicates the resulting state when it was on.
	enum FlagMod{Set = 3, Clear = 0, Toggle = 1, Keep = 2};

	class TimeChartSeries;
	class SampledChartSeries;

	GLWchart(const char *title, ChartSeries *_series = NULL);

	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);

	void addSeries(ChartSeries *s){series.push_back(s);}
	void setSeriesVisible(int i, FlagMod v);

	void addSample(double value)const;

	static bool sq_define(HSQUIRRELVM v);
	static SQInteger sqf_constructor(HSQUIRRELVM v);
	static SQInteger sqf_addSeries(HSQUIRRELVM v);

protected:
	std::vector<ChartSeries *> series;

	SampledChartSeries *sampled;

	const char *classname()const{return "GLWchart";}
	virtual void anim(double dt);
	void draw(GLwindowState &ws, double t);
};


/// \brief The series that records track of a value in history.
class GLWchart::TimeChartSeries : public GLWchart::ChartSeries{
public:
	double getNormalizer()const{
		return normalizer;
	}
protected:
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
	virtual gltestp::dstring labelstr()const{return gltestp::dstring() << normalizer;}
	/// \breif The derived classes must override this function to mark a value in time line.
	virtual double timeProc(double dt) = 0;
};


#endif
