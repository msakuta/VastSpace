#ifndef TIMEMEAS_H
#define TIMEMEAS_H

#ifdef _WIN32
#include <windows.h>
#endif
#include <time.h>

typedef union time_meas{
#ifdef _WIN32
	LARGE_INTEGER li;
#endif
	clock_t c;
} timemeas_t;

int TimeMeasStart(timemeas_t *);
double TimeMeasLap(timemeas_t *);

/* privates */
extern int timemeas_highres;
#ifdef _WIN32
extern LARGE_INTEGER timemeas_frequency;
#endif

#endif /* TIMEMEAS_H */
