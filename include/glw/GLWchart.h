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
		virtual double value(int index, GLWchart *w) = 0;
		virtual double getMax()const{return 1;}
		virtual double getMin()const{return 0;}
		virtual int getYGroup()const{return -1;}
		virtual int count() const = 0;
		virtual Vec4f color()const{return Vec4f(1,1,1,1);}
		virtual void anim(double dt, GLWchart *){}
		virtual gltestp::dstring labelname()const{return "none";}
		virtual gltestp::dstring labelstr()const{return "";}
		virtual void addSample(double){}
		bool visible;
	};

	/// \brief A flag modification code that specifies how the flag should be changed from the previous state.
	///
	/// It's merely a single-bit truth table. The value's bit 0 designates the resulting state when the
	/// previous state is off. In contrast, bit 1 indicates the resulting state when it was on.
	enum FlagMod{Set = 3, Clear = 0, Toggle = 1, Keep = 2};

	class NormalizedChartSeries;
	class TimeChartSeries;
	class SampledChartSeries;

	GLWchart(const char *title, ChartSeries *_series = NULL);

	virtual int mouse(GLwindowState &ws, int button, int state, int mx, int my);

	void addSeries(ChartSeries *s){series.push_back(s);}
	void setSeriesVisible(int i, FlagMod v);

	void addSample(gltestp::dstring label, double value)const;

	static bool sq_define(HSQUIRRELVM v);
	static SQInteger sqf_constructor(HSQUIRRELVM v);
	static SQInteger sqf_addSeries(HSQUIRRELVM v);

	static bool addSampleToCharts(gltestp::dstring label, double value);
	static bool addSamplesToCharts(const gltestp::dstring label[], const double value[], int count);

protected:
	std::vector<ChartSeries *> series;

	SampledChartSeries *sampled;

	const char *classname()const{return "GLWchart";}
	virtual void anim(double dt);
	void draw(GLwindowState &ws, double t);
};

/// \brief The normalized chart, which will fit the range of sample values to the window.
///
/// There's a property named ygroup to control which chart series will have the same Y-axis scales.
class GLWchart::NormalizedChartSeries : public GLWchart::ChartSeries{
public:
	/// \param ygroup The Y-axis scale group number. -1 won't share scale.
	NormalizedChartSeries(int ygroup = -1, const Vec4f &color = Vec4f(1,1,1,1)) : ygroup(ygroup), colorValue(color){}
protected:
	Vec4f colorValue;
	int ygroup;
	virtual int getYGroup()const{return ygroup;}
	virtual double value(int index, GLWchart *w){
		double theMax = getMax();
		if(ygroup != -1){
			for(int i = 0; i < w->series.size(); i++){
				ChartSeries *c = w->series[i];
				if(c->getYGroup() == ygroup && theMax < c->getMax())
					theMax = c->getMax();
			}
		}
		double val = valueBeforeNormalize(index);
		return theMax != 0. ? val / theMax : val;
	}
	virtual Vec4f color()const{return colorValue;}

	/// \brief Derived classes should override this function to return a value instead of value().
	virtual double valueBeforeNormalize(int index) = 0;
};

/// \brief The series that records track of a value in history.
class GLWchart::TimeChartSeries : public GLWchart::NormalizedChartSeries{
protected:
	std::vector<double> chart;
	double normalizer;

	TimeChartSeries(int ygroup, const Vec4f &color) : NormalizedChartSeries(ygroup, color){}

	double getMax()const{
		return normalizer;
	}
	double valueBeforeNormalize(int index){
		return chart[index];
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
	virtual gltestp::dstring labelstr()const{return gltestp::dstring() << chart.back();}
	/// \brief The derived classes must override this function to mark a value in time line.
	virtual double timeProc(double dt) = 0;
};


//-----------------------------------------------------------------------------
//    Inline Implementation for WarField
//-----------------------------------------------------------------------------

inline bool GLWchart::addSampleToCharts(gltestp::dstring label, double value){
	return addSamplesToCharts(&label, &value, 1);
}



#endif
