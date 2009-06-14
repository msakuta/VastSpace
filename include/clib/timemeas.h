#ifndef TIMEMEAS_H
#define TIMEMEAS_H

#include <windows.h>
#include <time.h>

typedef union time_meas{
	LARGE_INTEGER li;
	clock_t c;
} timemeas_t;

int TimeMeasStart(timemeas_t *);
double TimeMeasLap(timemeas_t *);

/* privates */
extern int timemeas_highres;
extern LARGE_INTEGER timemeas_frequency;

#endif /* TIMEMEAS_H */
