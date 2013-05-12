#ifndef MASALIB_COLOR_H
#define MASALIB_COLOR_H /* might collide to something */
/* (hopefully) portable definition of RGB and Alpha colors */
/* unsigned long has enough storage size to hold all intencities of
  Red, Green and Blue components, and optionally, alpha channel value.*/
typedef unsigned long COLOR32;

/* these macros should _not_ depend on byte order. */
#define COLOR32R(c) ((unsigned char)((c)&0xff))
#define COLOR32G(c) ((unsigned char)((c)>>8&0xff))
#define COLOR32B(c) ((unsigned char)((c)>>16&0xff))
#define COLOR32A(c) ((unsigned char)((c)>>24&0xff))
#define COLOR32RGB(r,g,b) ((COLOR32)(r)&0xff|((COLOR32)(g)&0xff)<<8|((COLOR32)(b)&0xff)<<16)
#define COLOR32RGBA(r,g,b,a) ((COLOR32)(r)&0xff|((COLOR32)(g)&0xff)<<8|((COLOR32)(b)&0xff)<<16|((COLOR32)(a)&0xff)<<24)
#define COLOR32UNPACK(a,c) ((a)[0]=COLOR32R(c),(a)[1]=COLOR32G(c),(a)[2]=COLOR32B(c))
#define COLOR32UNPACK4(a,c) ((a)[0]=COLOR32R(c),(a)[1]=COLOR32G(c),(a)[2]=COLOR32B(c),(a)[3]=COLOR32A(c))

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#define COLOR32SCALE(c,a) COLOR32RGBA(COLOR32R(c)*(a)/256,COLOR32G(c)*(a)/256,COLOR32B(c)*(a)/256,COLOR32A(c)*(a)/256)
#define COLOR32SCALE3(c,a) COLOR32RGB(COLOR32R(c)*(a)/256,COLOR32G(c)*(a)/256,COLOR32B(c)*(a)/256)
#define COLOR32ADD(c1,c2) COLOR32RGBA(MIN(255,COLOR32R(c1)+COLOR32R(c2)),MIN(255,COLOR32G(c1)+COLOR32G(c2)),MIN(255,COLOR32B(c1)+COLOR32B(c2)),MIN(255,COLOR32A(c1)+COLOR32A(c2)))
#define COLOR32AADD(c1,c2) COLOR32RGBA(MIN(255,COLOR32R(c1)+COLOR32A(c2)*COLOR32R(c2)/256),MIN(255,COLOR32G(c1)+COLOR32A(c2)*COLOR32G(c2)/256),MIN(255,COLOR32B(c1)+COLOR32A(c2)*COLOR32B(c2)/256),COLOR32A(c1))
#define COLOR32MIX(c1,c2,f) COLOR32RGBA(COLOR32R(c2)*(f)+COLOR32R(c1)*(1-(f)),COLOR32G(c2)*(f)+COLOR32G(c1)*(1-(f)),COLOR32B(c2)*(f)+COLOR32B(c1)*(1-(f)),COLOR32A(c2)*(f)+COLOR32A(c1)*(1-(f)))
#define COLOR32MUL(c1,c2) COLOR32RGBA(COLOR32R(c1)*COLOR32R(c2)/256,COLOR32G(c1)*COLOR32G(c2)/256,COLOR32B(c1)*COLOR32B(c2)/256,COLOR32A(c1)*COLOR32A(c2)/256)

#endif
