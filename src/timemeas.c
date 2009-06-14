#include "clib/timemeas.h"

int timemeas_highres = -1;
LARGE_INTEGER timemeas_frequency;

int TimeMeasStart(timemeas_t *ptm){
	if(timemeas_highres == -1){
		timemeas_highres = QueryPerformanceFrequency(&timemeas_frequency) ? 1 : 0;
	}
	if(timemeas_highres == 0){
		ptm->c = clock();
		return 1;
	}
	else
		return QueryPerformanceCounter(&ptm->li);
}

double TimeMeasLap(timemeas_t *ptm){
	if(timemeas_highres == 0)
		return (double)(clock() - ptm->c) / CLOCKS_PER_SEC;
	else{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (double)(now.QuadPart - ptm->li.QuadPart) / timemeas_frequency.QuadPart;
	}
}
