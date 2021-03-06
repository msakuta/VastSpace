#include "yssurf.h"
#include "motion.h"
#include "mqo.h"
extern "C"{
#include <clib/amat4.h>
#include <clib/gl/gldraw.h>
#include <clib/suf/sufdraw.h>
}
#include <cpplib/vec3.h>
#include <cpplib/vec4.h>
#include <cpplib/mat4.h>
#include <cpplib/quat.h>

#define g_gldcache NULL

static int nestlevel = 0;
static int dumped = 0;
static void draw_mqo_node(const Model *model, const ysdnmv_t *v0, Bone *bone, const Quatd &srot, const Vec3d &spos){
	int i;
	int nodraw = 0;
	int target = 0;
	const ysdnmv_t *v;

	if(!dumped){
//		for(i = 0; i < nestlevel; i++)
//			putchar('|');
//		printf("\"%s\" \"%s\" %d (%lg,%lg,%lg)\n", srf->name, srf->pck->name, srf->cla, srf->pos[0], srf->pos[1], srf->pos[2]);
	}

	/* ignore afterburners */
/*	if(!strncmp(srf->name, "AB", 2) && srf->name[2] != 'b')
		return;*/

/*	if(dumped) for(v = v0; v; v = v->next){
		const char **skipnames = v->skipnames;
		int skips = v->skips;
		for(i = 0; i < skips; i++) if(!strcmp(skipnames[i], srf->name)){
			return;
		}
	}*/
/*	gldTranslate3dv(srf->pos);*/

	Vec3d apos = spos;
	Quatd arot = srot;

	// Normally, '1' means visible.
	float avisible = 1.;

	for(v = v0; v; v = v->next){
		ysdnm_bone_var *bonevar = v->bonevar;
		int bones = min(v->bones, model->n);
		for(i = 0; i < bones; i++) if(!strcmp(bonevar[i].name, bone->name)){
/*			Mat4d rotmat = Quatd(bonerot[i][0], bonerot[i][1], bonerot[i][2], bonerot[i][3]).tomat4();
			gldTranslate3dv(bone->joint);
			gldTranslate3dv(&bonerot[i][4]);
			glMultMatrixd(rotmat);
			gldTranslaten3dv(bone->joint);*/
			apos += arot.trans(bone->joint);
			apos += arot.trans(v->bonevar[i].pos);
			arot *= bonevar[i].rot;
			apos -= arot.trans(bone->joint);
			avisible = v->bonevar[i].visible;
		}
#if 0
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
#endif
	}

	// Visibility factor can be any value, but only relation to 0 matters.
	// Magnitude of visibility factor come into play when interpolation between keyframes involves.
	if(0. < avisible /*srf->pck && !nodraw*/){
		glPushMatrix();
		Mat4d rotmat = arot.tomat4();
		rotmat.vec3(3) = apos;
		glMultMatrixd(rotmat);

		if(target){
			glPushAttrib(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
		}
		bone->suf->decalDraw(SUF_ATR, NULL, bone->suftex, NULL, NULL);
		if(target){
			glPopAttrib();
		}
		glPopMatrix();
	}

	nestlevel++;
	for(Bone *nextbone = bone->children; nextbone; nextbone = nextbone->nextSibling){
//		if(!srf->cld[i].srf)
//			srf->cld[i].srf = find_node(dnm, srf->cld[i].name);
		if(nextbone->suf){
			draw_mqo_node(model, v0, nextbone, arot, apos);
		}
	}
	nestlevel--;
}

/*void DrawMQO(suf_t **sufs, const char *bonenames[], double (*bonerot)[7], int bones, const char *skipnames[], int skips){
	ysdnmv_t v;
	v.fcla = 0;
	v.bonenames = bonenames;
	v.bonerot = bonerot;
	v.bones = bones;
	v.skipnames = skipnames;
	v.skips = skips;
	v.target = NULL;
	v.next = NULL;
	if(!dnm->ns)
		return;
	nestlevel = 0;
	draw_mqo_node(sufs, &v);
	if(!dumped)
		dumped = 1;
}*/


void DrawMQO_V(const Model *model, const ysdnmv_t *v){
	nestlevel = 0;
	for(int i = 0; i < model->n; i++) if(model->bones[i]->depth == 0)
		draw_mqo_node(model, v, model->bones[i], quat_u, vec3_000);
	if(!dumped)
		dumped = 1;
}




static void draw_mqo_node(const Model *model, const MotionPose *v0, Bone *bone, const Quatd &srot, const Vec3d &spos){
	int i;
	int nodraw = 0;
	int target = 0;
	const MotionPose *v;

	if(!dumped){
//		for(i = 0; i < nestlevel; i++)
//			putchar('|');
//		printf("\"%s\" \"%s\" %d (%lg,%lg,%lg)\n", srf->name, srf->pck->name, srf->cla, srf->pos[0], srf->pos[1], srf->pos[2]);
	}

	/* ignore afterburners */
/*	if(!strncmp(srf->name, "AB", 2) && srf->name[2] != 'b')
		return;*/

/*	if(dumped) for(v = v0; v; v = v->next){
		const char **skipnames = v->skipnames;
		int skips = v->skips;
		for(i = 0; i < skips; i++) if(!strcmp(skipnames[i], srf->name)){
			return;
		}
	}*/
/*	gldTranslate3dv(srf->pos);*/

	Vec3d apos = spos;
	Quatd arot = srot;

	// Normally, '1' means visible.
	float avisible = 1.;

	for(v = v0; v; v = v->next){
		int bones = min(v->nodes.size(), model->n);
		MotionPose::const_iterator it = v->nodes.find(bone->name);
		if(it != v->nodes.end()){
/*			Mat4d rotmat = Quatd(bonerot[i][0], bonerot[i][1], bonerot[i][2], bonerot[i][3]).tomat4();
			gldTranslate3dv(bone->joint);
			gldTranslate3dv(&bonerot[i][4]);
			glMultMatrixd(rotmat);
			gldTranslaten3dv(bone->joint);*/
			apos += arot.trans(bone->joint);
			apos += arot.trans(it->second.pos);
			arot *= it->second.rot;
			apos -= arot.trans(bone->joint);

			// Here is the magic: applying one or more motions with visibility value 1 to the same node
			// does not change the value of avisible.  Instead, if the value is less than 1, avisible
			// approaches zero.  In this way, we can stack multiple motions to a model witout worrying
			// about interference if only one of them has visibility value customized.
			avisible += it->second.visible - 1;
		}
#if 0
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
#endif
	}

	// Visibility factor can be any value, but only relation to 0 matters.
	// Magnitude of visibility factor come into play when interpolation between keyframes involves.
	if(0. < avisible /*srf->pck && !nodraw*/){
		glPushMatrix();
		Mat4d rotmat = arot.tomat4();
		rotmat.vec3(3) = apos;
		glMultMatrixd(rotmat);

		if(target){
			glPushAttrib(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);
		}
		bone->suf->decalDraw(Mesh::Attribs, NULL, bone->suftex, NULL, NULL);
		if(target){
			glPopAttrib();
		}
		glPopMatrix();
	}

	nestlevel++;
	for(Bone *nextbone = bone->children; nextbone; nextbone = nextbone->nextSibling){
//		if(!srf->cld[i].srf)
//			srf->cld[i].srf = find_node(dnm, srf->cld[i].name);
		if(nextbone->suf){
			draw_mqo_node(model, v0, nextbone, arot, apos);
		}
	}
	nestlevel--;
}

void DrawMQOPose(const Model *model, const MotionPose *v){
	nestlevel = 0;
	for(int i = 0; i < model->n; i++) if(model->bones[i]->depth == 0)
		draw_mqo_node(model, v, model->bones[i], quat_u, vec3_000);
	if(!dumped)
		dumped = 1;
}
