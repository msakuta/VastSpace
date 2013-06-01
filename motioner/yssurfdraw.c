#include "yssurf.h"
#include <clib/amat4.h>
#include <clib/aquatrot.h>
#include <clib/gl/gldraw.h>
#include <clib/suf/sufdraw.h>

#define g_gldcache NULL

static struct ysdnm_srf *find_node(ysdnm_t *dnm, char *name){
	int i;
	for(i = 0; i < dnm->ns; i++) if(!strcmp(dnm->s[i]->name, name))
		return dnm->s[i];
	return NULL;
}

static int nestlevel = 0;
static int dumped = 0;
static void draw_ysdnm_node(ysdnm_t *dnm, struct ysdnm_srf *srf, ysdnmv_t *v0){
	int i;
	int nodraw = 0;
	int target = 0;
	ysdnmv_t *v;

	if(!dumped){
		for(i = 0; i < nestlevel; i++)
			putchar('|');
		printf("\"%s\" \"%s\" %d (%lg,%lg,%lg)\n", srf->name, srf->pck->name, srf->cla, srf->pos[0], srf->pos[1], srf->pos[2]);
	}

	/* ignore afterburners */
/*	if(!strncmp(srf->name, "AB", 2) && srf->name[2] != 'b')
		return;*/

	if(dumped) for(v = v0; v; v = v->next){
		const char **skipnames = v->skipnames;
		int skips = v->skips;
		for(i = 0; i < skips; i++) if(!strcmp(skipnames[i], srf->name)){
			return;
		}
	}
	glPushMatrix();
/*	gldTranslate3dv(srf->pos);*/

	for(v = v0; v; v = v->next){
		struct ysdnm_bone_var *bonerot = v->bonevar;
		int bones = v->bones;
		for(i = 0; i < bones; i++) if(!strcmp(bonerot[i].name, srf->name)){
			amat4_t rotmat;
			quat2mat(&rotmat, bonerot[i].rot);
			gldTranslate3dv(srf->pos);
			gldTranslate3dv(bonerot[i].pos);
			glMultMatrixd(rotmat);
			gldTranslaten3dv(srf->pos);
		}
		if(v->fcla & (1 << srf->cla) && 2 <= srf->nst){
			double f = v->cla[srf->cla];
			avec3_t pos;
			if(f == 0. && !(v->fviscla & (1 << srf->cla)) && !srf->stav[0]){
				nodraw = 1;
			}
			if(f == 1. && !(v->fviscla & (1 << srf->cla)) && !srf->stav[1]){
				nodraw = 1;
			}
			gldTranslate3dv(srf->pos);
			for(i = 0; i < 3; i++)
				pos[i] = srf->sta[1][i] * f + srf->sta[0][i] * (1. - f);
			gldTranslate3dv(pos);
			glRotated(srf->sta[1][3] * f + srf->sta[0][3] * (1. - f), 0, -1, 0); /* heading */
			glRotated(srf->sta[1][4] * f + srf->sta[0][4] * (1. - f), -1, 0, 0); /* pitch */
			glRotated(srf->sta[1][5] * f + srf->sta[0][5] * (1. - f), 0, 0, 1); /* bank */
			gldTranslaten3dv(srf->pos);
		}
		if(v->target && !strcmp(srf->name, v->target))
			target = 1;
	}

	if(srf->pck && !nodraw){
		if(target){
			glPushAttrib(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
		}
		DrawSUF(srf->pck->suf, SUF_ATR, NULL);
		if(target){
			glPopAttrib();
		}
	}
	nestlevel++;
	for(i = 0; i < srf->nch; i++){
		if(!srf->cld[i].srf)
			srf->cld[i].srf = find_node(dnm, srf->cld[i].name);
		if(srf->cld[i].srf){
			draw_ysdnm_node(dnm, srf->cld[i].srf, v0);
		}
	}
	nestlevel--;
	glPopMatrix();
}

void DrawYSDNM(ysdnm_t *dnm, const char *bonenames[], struct ysdnm_bone_var *bonevar, int bones, const char *skipnames[], int skips){
	ysdnmv_t v;
	v.fcla = 0;
	v.bonevar = bonevar;
	v.bones = bones;
	v.skipnames = skipnames;
	v.skips = skips;
	v.target = NULL;
	v.next = NULL;
	if(!dnm->ns)
		return;
	nestlevel = 0;
	draw_ysdnm_node(dnm, dnm->s[0], &v);
	if(!dumped)
		dumped = 1;
}


void DrawYSDNM_V(ysdnm_t *dnm, ysdnmv_t *v){
	if(!dnm->ns)
		return;
	nestlevel = 0;
	draw_ysdnm_node(dnm, dnm->s[0], v);
	if(!dumped)
		dumped = 1;
}









static void trans_ysdnm_node(ysdnm_t *dnm, struct ysdnm_srf *srf, ysdnmv_t *v0, void (*callback)(const char *name, void *hint), void *hint){
	int i;
	ysdnmv_t *v;

	for(v = v0; v; v = v->next){
		const char **skipnames = v->skipnames;
		int skips = v->skips;
		for(i = 0; i < skips; i++) if(!strcmp(skipnames[i], srf->name)){
			return;
		}
	}

	glPushMatrix();

	for(v = v0; v; v = v->next){
		struct ysdnm_bone_var *bonevar = v->bonevar;
		int bones = v->bones;
		for(i = 0; i < bones; i++) if(!strcmp(bonevar[i].name, srf->name)){
			amat4_t rotmat;
			quat2mat(&rotmat, bonevar[i].rot);
			gldTranslate3dv(srf->pos);
			gldTranslate3dv(bonevar[i].pos);
			glMultMatrixd(rotmat);
			gldTranslaten3dv(srf->pos);
		}
		if(v->fcla & (1 << srf->cla) && 2 <= srf->nst){
			double f = v->cla[srf->cla];
			avec3_t pos;
			gldTranslate3dv(srf->pos);
			for(i = 0; i < 3; i++)
				pos[i] = srf->sta[1][i] * f + srf->sta[0][i] * (1. - f);
			gldTranslate3dv(pos);
			glRotated(srf->sta[1][3] * f + srf->sta[0][3] * (1. - f), 0, -1, 0); /* heading */
			glRotated(srf->sta[1][4] * f + srf->sta[0][4] * (1. - f), -1, 0, 0); /* pitch */
			glRotated(srf->sta[1][5] * f + srf->sta[0][5] * (1. - f), 0, 0, 1); /* bank */
			gldTranslaten3dv(srf->pos);
		}
	}

	gldTranslate3dv(srf->pos);
	callback(srf->name, hint);
	gldTranslaten3dv(srf->pos);
	for(i = 0; i < srf->nch; i++){
		if(!srf->cld[i].srf)
			srf->cld[i].srf = find_node(dnm, srf->cld[i].name);
		if(srf->cld[i].srf){
			trans_ysdnm_node(dnm, srf->cld[i].srf, v0, callback, hint);
		}
	}
	glPopMatrix();
}

void TransYSDNM(ysdnm_t *dnm, const char *bonenames[], struct ysdnm_bone_var *bonevar, int bones, const char *skipnames[], int skips, void (*callback)(const char *name, void *hint), void *hint){
	ysdnmv_t v;
	v.fcla = 0;
	v.bonevar = bonevar;
	v.bones = bones;
	v.skipnames = skipnames;
	v.skips = skips;
	v.target = NULL;
	v.next = NULL;
	if(!dnm->ns)
		return;
	trans_ysdnm_node(dnm, dnm->s[0], &v, callback, hint);
}

void TransYSDNM_V(ysdnm_t *dnm, ysdnmv_t *v, void (*callback)(const char *name, void *hint), void *hint){
	if(!dnm->ns)
		return;
	trans_ysdnm_node(dnm, dnm->s[0], v, callback, hint);
}


