#include "clib/timemeas.h"

int timemeas_highres = -1;
#ifdef _WIN32
LARGE_INTEGER timemeas_frequency;
#endif

int TimeMeasStart(timemeas_t *ptm){
#ifdef _WIN32
	if(timemeas_highres == -1){
		timemeas_highres = QueryPerformanceFrequency(&timemeas_frequency) ? 1 : 0;
	}
	if(timemeas_highres == 0){
		ptm->c = clock();
		return 1;
	}
	else
		return QueryPerformanceCounter(&ptm->li);
#elif _POSIX_C_SOURCE >= 199309L
	clock_gettime(CLOCK_REALTIME, &ptm->ts);
	return 1;
#else
	// clock() returns the process's CPU time, which is not real time.
	// For example, you'll get less value if you sleep the process.
	// Therefore it's inappropriate to measure time interval with clock()
	// from the start, but if we do lack both Windows' high-resolution
	// timer and clock_gettime(), we have no other option.
	ptm->c = clock();
	return 1;
#endif
}

double TimeMeasLap(timemeas_t *ptm){
#ifdef _WIN32
	if(timemeas_highres == 0)
		return (double)(clock() - ptm->c) / CLOCKS_PER_SEC;
	else{
		LARGE_INTEGER now;
		QueryPerformanceCounter(&now);
		return (double)(now.QuadPart - ptm->li.QuadPart) / timemeas_frequency.QuadPart;
	}
#elif _POSIX_C_SOURCE >= 199309L
	struct timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	return (double)(now.tv_sec - ptm->ts.tv_sec) + (now.tv_nsec - ptm->ts.tv_nsec) / 1e9;
#else
	return (double)(clock() - ptm->c) / CLOCKS_PER_SEC;
#endif
}
