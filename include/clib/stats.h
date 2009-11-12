#ifndef MOD_STATS_H
#define MOD_STATS_H
#include <math.h>

#ifdef __cplusplus

namespace ns_modification{ namespace ns_mathlib{
class CStatistician{ // hard spell
	double avg; // sum(value) == average
	double m20; // sum(value^2) == 2nd moment around 0 == variance - average^2
	double m30; // sum(value^3) == 3rd moment around 0
	unsigned cnt; // count of samples so far
public:
	CStatistician(): avg(0), m20(0), m30(0){cnt = 0;}
	void put(double value){
		avg = (cnt * avg + value) / (cnt + 1);
		m20 = (cnt * m20 + value * value) / (cnt + 1);
		m30 = (cnt * m30 + value * value * value) / (cnt + 1);
		cnt++;
	}
	unsigned getCount(void)const{ return cnt; }
	double getAvg(void)const{ return avg; } // average
	double getVar(void)const{ return m20 - avg * avg; } // variance
	double getDev(void)const{ return ::sqrt(getVar()); } // deviation
	double getMu3(void)const{ return m30 + avg * (2 * avg * avg - 3 * m20); }
	double getSkw(void)const{ double var = getVar(); return getMu3() / var / ::sqrt(var); } // skewness

	class parser{
		const CStatistician &t;
		bool avar, adev, amu3, askw;
		double var; // variance
		double dev; // deviation
		double mu3; // 3rd moment around average
		double skw; // skewness
		parser &operator=(parser&);
	public:
		parser(const CStatistician &target) : t(target){avar = amu3 = adev = askw = false;}
		unsigned getCount(void){ return t.cnt; }
		double getAvg(void){ return t.avg; } // direct
		double getVar(void){ return avar ? var : avar = true, var = t.m20 - t.avg * t.avg; }
		double getDev(void){ return adev ? dev : adev = true, dev = ::sqrt(getVar()); }
		double getMu3(void){ return amu3 ? mu3 : amu3 = true, mu3 = t.m30 + t.avg * (2 * t.avg * t.avg - 3 * t.m20); }
		double getSkw(void){
			if(askw) return skw;
			else{
				askw = true;
				return getVar() ? getMu3() / getVar() / getDev() : 0;
			}
		}
	};
};
}}
using namespace ns_modification::ns_mathlib;

#endif

typedef struct statistician_s{
	double avg;
	double m20;
	double m30;
	unsigned cnt;
} statistician_t;
static const statistician_t statistician_0 = {0., 0., 0., 0};

#define PutStats(s,value) ((s)->avg = ((s)->cnt * (s)->avg + (value)) / ((s)->cnt + 1),\
		(s)->m20 = ((s)->cnt * (s)->m20 + (value) * (value)) / ((s)->cnt + 1),\
		(s)->m30 = ((s)->cnt * (s)->m30 + (value) * (value) * (value)) / ((s)->cnt + 1),\
		(s)->cnt++)
#define GetStatsCount(s) ((s)->cnt)
#define GetStatsAvg(s) ((s)->avg)
#define GetStatsVar(s) ((s)->m20 - (s)->avg * (s)->avg)
#define GetStatsDev(s) sqrt(GetStatsVar(s))
#define GetStatsMu3(s) ((s)->m30 + (s)->avg * (2 * (s)->avg * (s)->avg - 3 * (s)->m20))
#define GetStatsSkw(s) (GetStatsVar(s) == 0 ? 0 : GetStatsMu3(s) / GetStatsVar(s) / GetStatsDev(s))


#endif /* MOD_STATS_H */
