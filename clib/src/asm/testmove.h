#ifndef TESTMOVE_H
#define TESTMOVE_H
#ifdef __cplusplus
extern "C"{
#endif
extern int CPUIDavailable(void);
extern int MMXavailable(void);
extern void testmove(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw, DWORD test);
extern void addmove(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw);
extern void addmoveMMX(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw);
extern void addtestmove(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw, DWORD test);
extern void displacemove(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw, DWORD test);
extern void reversemove(DWORD *dst, const DWORD *src, size_t w, size_t h);
extern void reversetestmove(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw, DWORD test);
extern void reverseaddmove(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw);
extern void reverseaddmoveMMX(DWORD *dst, const DWORD *src, size_t w, size_t h, size_t dstw, size_t srcw);
#ifdef __cplusplus
}
#endif
#endif
