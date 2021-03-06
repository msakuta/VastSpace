/** \file
 * \brief Implementation of dynamic string for gltestplus
 */
#define _CRT_SECURE_NO_WARNINGS // We are aware of ANSI C's weaknesses.
#include "dstring.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>

#define alunit 0x10 /* allocation unit */
#define alsize(s) ((((s) + alunit - 1) / alunit) * alunit) /* allocated size */

namespace gltestp{

#ifdef _DEBUG
int dstring::cats = 0;
int dstring::allocs = 0;
int dstring::reallocs = 0;
int dstring::frees = 0;
#endif

void dstring::init(const char *a, long len){
	p = NULL;
	if(a){
		if(len < 0)
			strcat(a);
		else
			strncat(a, len);
	}
}

void dstring::init(long src){
	p = NULL;
	char buf[1 + 8*sizeof src]; /* n-bit long's maximum string expression width is far less than 8*n, plus null */
	sprintf(buf, "%ld", src);
	init(buf);
}

void dstring::initu(unsigned long src){
	p = NULL;
	char buf[1 + 8*sizeof src]; /* n-bit long's maximum string expression width is far less than 8*n, plus null */
	sprintf(buf, "%lu", src);
	init(buf);
}

void dstring::initd(double src){
	p = NULL;
	char buf[1 + 8*sizeof src]; // double expression is mystery
	sprintf(buf, "%lg", src);
	init(buf);
}

void dstring::initp(const void *a){
	char buf[32]; // OK with 128-bit machine or something?
	sprintf(buf, "%p", a);
	init(buf);
}

dstring &dstring::strncat(const char *src, unsigned long len){
	size_t sl;
	assert(this && src);
	if(!src)
		return *this;
#if 0
	__asm{
		mov eax,src
		mov ecx,len
loopback:
		loop loopback
	}
#elif 1
	{// thing like strnlen
		unsigned length = 0;
		const char *str = src;
		while(length < len && *str++ )
			++length;
		sl = length;
	}
#else
	sl = strlen(src);
#endif
	if(len < sl)
		sl = len;
	if(!sl)
		return *this;

#ifdef _DEBUG
	cats++;
#endif

	// Allocate if null
	if(!p){
		size_t sz = offsetof(in, s) + alsize(sl) + 1;
		p = (in*)malloc(sz);
		p->refs = 1;
		p->size = (unsigned long)sl;
		memcpy(p->s, src, sl);
		p->s[p->size] = '\0';
#ifdef _DEBUG
		allocs++;
#endif
		return *this;
	}

	// Copy on write
	if(1 < p->refs){
		size_t sz = offsetof(in, s) + alsize(p->size + sl) + 1;
		in *newp = (in*)malloc(sz);
		memcpy(newp->s, p->s, p->size);
		memcpy(&newp->s[p->size], src, sl);
		newp->refs = 1;
		newp->size = p->size + (unsigned long)sl;
		newp->s[newp->size] = '\0';
		p->refs--;
		p = newp;
#ifdef _DEBUG
		allocs++;
#endif
		return *this;
	}
	if(alsize(p->size) < p->size + sl){ /* expand if necessary */
		p = (in*)realloc(p, offsetof(in, s) + alsize(p->size + sl) + 1);
		if(p->size && !p->s){
			this->dstring::~dstring();
			return *this;
		}
#ifdef _DEBUG
		reallocs++;
#endif
	}
	if(p->size)
		::strncat(p->s, src, sl);
	else
		::strncpy(p->s, src, sl + 1);
	p->size += (unsigned long)sl;
	return *this;
}

dstring &dstring::strcat(const char *src){
	return strncat(src, (unsigned long)::strlen(src) + 1);
}

dstring &dstring::strcat(const dstring &src){
	return strncat(src, src.len());
}

long dstring::strncpy(const char *src, unsigned long len){
	assert(this && src);
	this->dstring::~dstring();
	strncat(src, len);
	return this->len();
}

long dstring::strcpy(const char *src){
	return strncpy(src, (unsigned long)::strlen(src) + 1);
}

bool dstring::operator ==(const dstring &o)const{
	return this->p == o.p ? true : !!this->p ^ !o.p && !::strcmp(this->p->s, o.p->s);
}

bool dstring::operator ==(const char *o)const{
	return this->p ? !::strcmp(this->p->s, o) : !*o;
}

bool dstring::operator <(const dstring &o)const{
	return !p || o.p && ::strcmp(p->s, o.p->s) < 0;
}

dstring::~dstring(){
	if(!p)
		return;
	if(!--p->refs){
		::free(p);
#ifdef _DEBUG
		frees++;
#endif
	}
	p = NULL;
}

typedef uint32_t Fnv32_t;

#define FNV1_32_INIT ((Fnv32_t)0x811c9dc5)
#define FNV1_32A_INIT FNV1_32_INIT

/// @brief Hash function for hash tables (slightly modified version of FNV-1a algorithm)
/// @returns The hash value in 32 bits. The type size_t may have size wider than 32 bits
///          (especially on 64 bit platforms), but the algorithm only makes use of lower 32.
///
/// See http://isthe.com/chongo/tech/comp/fnv/ for FNV-1a hash function algorithm.
size_t dstring::hash()const{
	Fnv32_t hval = FNV1_32_INIT;
	if(!p)
		return (size_t)hval;
	unsigned char *bp = (unsigned char *)p->s;	/* start of buffer */
	unsigned char *be = bp + p->size;		/* beyond end of buffer */

										/*
										* FNV-1a hash each octet in the buffer
										*/
	while (bp < be) {

		/* xor the bottom with the current octet */
		hval ^= (Fnv32_t)*bp++;

		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV_32_PRIME;
#else
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
	}
	/* return our new hash value */
	return (size_t)hval;
}

}

