#include "material.h"
#include "bitmap.h"
//extern "C"{
#include <clib/zip/UnZip.h>
#include <clib/suf/sufdraw.h>
#include <clib/dstr.h>
//}



struct material{
	char *name;
	char *texname[2];
	suftexparam_t stp[2];
};

static struct material *g_mats = NULL;
static int g_nmats = 0;

static const suftexparam_t defstp = {
	STP_MAGFIL, // unsigned long flags;
	NULL, NULL, // const BITMAPINFO *bmi, *bmiMask;
	0, // GLuint env;
	GL_LINEAR, // GLuint magfil, minfil, wraps, wrapt;
};

void AddMaterial(const char *name, const char *texname1, suftexparam_t *stp1, const char *texname2, suftexparam_t *stp2){
	struct material *m;
	int i;
	for(i = 0; i < g_nmats; i++) if(!strcmp(name, g_mats[i].name)){
		m = &g_mats[i];
		m->texname[0] = realloc(m->texname[0], texname1 ? strlen(texname1) + 1 : 0);
		if(texname1)
			strcpy(m->texname[0], texname1);
		m->texname[1] = realloc(m->texname[1], texname2 ? strlen(texname2) + 1 : 0);
		if(texname2)
			strcpy(m->texname[1], texname2);
		m->stp[0] = stp1 ? *stp1 : defstp;
		m->stp[1] = stp2 ? *stp2 : defstp;
		return;
	}
	m = &(g_mats = realloc(g_mats, (++g_nmats) * sizeof*g_mats))[g_nmats - 1];
	m->name = malloc(strlen(name) + 1);
	strcpy(m->name, name);
	m->texname[0] = texname1 ? malloc(strlen(texname1) + 1) : NULL;
	if(texname1)
		strcpy(m->texname[0], texname1);
	m->texname[1] = texname2 ? malloc(strlen(texname2) + 1) : NULL;
	if(texname2)
		strcpy(m->texname[1], texname2);
	m->stp[0] = stp1 ? *stp1 : defstp;
	m->stp[1] = stp2 ? *stp2 : defstp;
}

GLuint CacheMaterial(const char *name){
	struct material *m;
	int i;
	for(i = 0; i < g_nmats; i++) if(!strcmp(name, g_mats[i].name)){
		m = &g_mats[i];
		return CallCacheBitmap5(name, m->texname[0], &m->stp[0], m->texname[1], &m->stp[1]);
	}
	return CallCacheBitmap(name, name, NULL, NULL);
}

void CacheSUFMaterials(const suf_t *suf){
	int i, j;
	for(i = 0; i < suf->na; i++) if(suf->a[i].colormap){

		/* an unreasonable filtering to exclude default mapping. */
		if(!strcmp(suf->a[i].colormap, "Mapping1.bmp"))
			continue;

		/* skip the same texture */
		for(j = 0; j < i; j++) if(suf->a[j].colormap && !strcmp(suf->a[i].colormap, suf->a[j].colormap))
			break;
		if(j != i)
			continue;
		CacheMaterial(suf->a[i].colormap);
	}
}

GLuint CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2){
	const struct suftexcache *stc;
	suftexparam_t stp, stp2;
	BITMAPFILEHEADER *bfh, *bfh2;
	GLuint ret;

	stc = FindTexCache(entry);
	if(stc)
		return stc->list;
	stp = pstp1 ? *pstp1 : defstp;
	bfh = ZipUnZip("rc.zip", fname1, NULL);
	stp.bmi = !bfh ? ReadBitmap(fname1) : (BITMAPINFO*)&bfh[1];
	if(!stp.bmi)
		return 0;
	if(stp.flags & STP_ALPHATEX){
		dstr_t ds = dstr0;
		dstrcat(&ds, fname1);
		dstrcat(&ds, ".a.bmp");
		bfh = ZipUnZip("rc.zip", dstr(&ds), NULL);
		stp.bmiMask = !bfh ? ReadBitmap(dstr(&ds)) : (BITMAPINFO*)&bfh[1];
		dstrfree(&ds);
	}

	stp2 = pstp2 ? *pstp2 : defstp;
	if(fname2){
		bfh2 = ZipUnZip("rc.zip", fname2, NULL);
		stp2.bmi = !bfh2 ? ReadBitmap(fname2) : (BITMAPINFO*)&bfh2[1];
		if(!stp2.bmi)
			return 0;
	}

	ret = CacheSUFMTex(entry, &stp, fname2 ? (BITMAPINFO*)&stp2 : NULL);
	if(bfh)
		ZipFree(bfh);
	else
		LocalFree(stp.bmi);
	if(fname2){
		if(bfh)
			ZipFree(bfh2);
		else
			LocalFree(stp2.bmi);
	}
	return ret;
}

GLuint CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2){
	return CallCacheBitmap5(entry, fname1, pstp, fname2, pstp);
}

suf_t *CallLoadSUF(const char *fname){
	suf_t *ret;
	ret = ZipUnZip("rc.zip", fname, NULL);
	if(!ret){
		char *p;
		p = strrchr(fname, '.');
		if(p && !strcmp(p, ".bin")){
			FILE *fp;
			long size;
			fp = fopen(fname, "rb");
			if(!fp)
				return NULL;
			fseek(fp, 0, SEEK_END);
			size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			ret = malloc(size);
			fread(ret, size, 1, fp);
			fclose(fp);
		}
		else
			ret = LoadSUF(fname);
	}
	if(!ret)
		return NULL;
	return RelocateSUF(ret);
}
