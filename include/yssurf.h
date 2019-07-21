#ifndef YSSURF_H
#define YSSURF_H
#ifdef __cplusplus
extern "C"{
#endif
#include <clib/suf/suf.h>
#include <clib/avec3.h>
#include <clib/aquat.h>
#ifdef __cplusplus
}
#include <cpplib/vec3.h>
#include <cpplib/quat.h>
#endif

#include <string.h>

// Type definitions varies in languages
#ifdef __cplusplus
typedef Vec3d YSVec3;
typedef Quatd YSQuat;
typedef Vec3<sufcoord> sufvector;
#else
typedef avec3_t YSVec3;
typedef aquat_t YSQuat;
typedef sufcoord sufvector[3];
#endif

#ifdef __cplusplus
extern "C"{
#endif

suf_t *LoadYSSUF(const char *fname);

struct ysdnm_pck{
	char *name;
	suf_t *suf;
};

struct ysdnm_srfcld{/* children list */
	char *name;
	struct ysdnm_srf *srf;
};

struct ysdnm_srf{
	char *name;
	struct ysdnm_pck *pck;
	int cla, nst;
	sufvector pos;
	sufcoord sta[2][6];
	char stav[2];
	int nch; /* number of children */
	struct ysdnm_srfcld cld[1];
};

typedef struct ysdnm{
	int np;
	struct ysdnm_pck *p;
	int ns;
    struct ysdnm_srf **s;
} ysdnm_t;

ysdnm_t *LoadYSDNM(const char *fname);
void DrawYSDNM(ysdnm_t *dnm, const char *bonenames[], struct ysdnm_bone_var *bonerot, int bones, const char *skipnames[], int skips);
void TransYSDNM(ysdnm_t *dnm, const char *bonenames[], struct ysdnm_bone_var *bonerot, int bones, const char *skipnames[], int skips, void (*callback)(const char *name, void *hint), void *hint);


/*
CLA �̒l  	�p�[�c�̎��  	NST �̒l  	�e STA �s�ł̏��  	�Ή����鑀��  	����
0 	���� 	1 	STA �P�s�ځ^�펞 	�@ 	���삵�Ȃ��p�[�c�S�ʂɎg���܂��B
0 	�����f�B���O�M�A 	2 	STA �P�s�ځ^���[	STA �Q�s�ځ^�W�J 	Landing Gear 	CLA 0 �� NST �̒l�Ŏ�ނ��ς��܂��B
1 	�ϗ� 	2 	STA �P�s�ځ^��W�J	STA �Q�s�ځ^�W�J 	(Extend�^Retract)	Variable Geometry Wing 	�ʏ�� .dat �̐ݒ�ɂ���ē��삵�܂��B
2 	�A�t�^�[�o�[�i�[ 	2 	STA �P�s�ځ^�I�t	STA �Q�s�ځ^�I�� 	Afterburner, Throttle 	�@
3 	�X�s�i�[ 	1 	STA �P�s�ځ^�펞 	�@ 	��]���͂x���ł��B���ł��\��������]�����Ɏg���܂��B
4 	�X�|�C���[�E�G�A�u���[�L 	2 	STA �P�s�ځ^��W�J	STA �Q�s�ځ^�W�J 	AirBrake 	�@
5 	�t���b�v 	2 	STA �P�s�ځ^��W�J	STA �Q�s�ځ^�W�J 	Flap 	����ݒ�ɂ��S�T�i�K�œ������܂��B
6 	�G���x�[�^�[ 	3 	STA �P�s�ځ^�j���[�g����	STA �Q�s�ځ^�A�b�v	STA �R�s�ځ^�_�E�� 	Elevator (Trim) 	�@
7 	�G������ 	3 	STA �P�s�ځ^�j���[�g����	STA �Q�s�ځ^�����[��	STA �R�s�ځ^�E���[�� 	Aileron 	�@
8 	���_�[ 	3 	STA �P�s�ځ^�j���[�g����	STA �Q�s�ځ^��	STA �R�s�ځ^�E 	Rudder 	�@
9 	���e�q�h�A 	2 	STA �P�s�ځ^��	STA �Q�s�ځ^�J 	Open�^Close�@�@�@Bomb Bay Door,	Drop Bomb 	.dat �� BOMINBAY �� TRUE �ɂ���Ɣ��e�������Ɏ����ŊJ���܂��B
10 	VTOL�i���͕Ό����u�j 	2 	STA �P�s�ځ^�ʏ�	STA �Q�s�ځ^�g�p�� 	Nozzle Up,Nozzle Down 	�S�P�P�i�K�œ��삵�܂��B
11 	�X���X�g���o�[�T�[�i�t���ˑ��u�j 	2 	STA �P�s�ځ^�I�t	STA �Q�s�ځ^�I�� 	Reverse Thrust 	�L�[�������Ă���ԍ쓮���܂��B
12 	CLA 10 �A���P 	2 	STA �P�s�ځ^�I�t	STA �Q�s�ځ^�I�� 	Nozzle Up,Nozzle Down 	����� CLA 10 �Ɠ����ł��B
13 	CLA 10 �A���Q 	2 	STA �P�s�ځ^�I�t	STA �Q�s�ځ^�I�� 	Nozzle Up,Nozzle Down 	�S�T�i�K�ACLA 10�̂Q�{�̃y�[�X�œ��삵�܂��B
14 	�M�A�h�A 	2 	STA �P�s�ځ^��	STA �Q�s�ځ^�J 	Landing Gear 	�M�A����O�ɊJ���A�M�A�̓��삪�I���ƕ��܂��B�i���J���j
15 	�����f�B���O�M�A�Q 	2 	STA �P�s�ځ^��	STA �Q�s�ځ^�J 	Landing Gear 	CLA 0 �̃M�A��蓮�삪�x���A�܂��\���̃^�C�~���O�������Ⴂ�܂��B
16 	�M�A�u���[�L 	2 	STA �P�s�ځ^�I�t	STA �Q�s�ځ^�I�� 	Brake 	�A���X�e�B���O�E�t�b�N�ɂ��g�p�ł��܂��B
18 	�v���y���E���[�^�[ 	1 	STA �P�s�ځ^�펞 	Throttle 	��]���͂x���ł��B���]���ɂ̂݌�����u���[�h�ȂǂɎg���܂��B
20 	�v���y���O�Ձi�c���j 	1 	STA �P�s�ځ^�펞 	Throttle 	��]���͂x���ł��B����]���ɂ̂݌�����v���y���̋O�ՂȂǂ�\�����܂��B
21 	��]�e�� 	0 	�@ 	Rotate Turret 	��]���͂x���ł��B�蓮�̏ꍇ�̂ݑ���ł��܂��B
*/

/// Single bone modifier (variation).
struct ysdnm_bone_var{
	const char *name; ///< Name of the bone being matched against a ysdnm or Model.
	YSQuat rot; ///< Rotation modifier.
	YSVec3 pos; ///< Position modifier, applied after rotation.
	float visible; ///< Visibility factor. Visible if > 0.
};

/// YSFlight dynamic model modifier, also called variation information.
typedef struct ysdnm_var{
	int fcla; ///< Bitfield for available predefined modifier classes.
	int fviscla; ///< Bitfield for visible modifier classes.
	double cla[21]; ///< Amplitude of each modifier classes.
	struct ysdnm_bone_var *bonevar; ///< Arbitrary modifier.
	int bones; ///< Count of bonevar
	const char **skipnames;
	int skips;
	const char *target;
	struct ysdnm_var *next;
#ifdef __cplusplus
	ysdnm_var(){
		target = NULL;
		fcla = 0;
		fviscla = 0;
		bonevar = NULL;
		bones = 0;
		skipnames = NULL;
		skips = 0;
		next = NULL;
	}
	ysdnm_var(const ysdnm_var &o) : target(NULL){
		copy(o);
	}
	void copy(const ysdnm_var &o){
		fcla = 0;
		fviscla = 0;
		if(o.bonevar){
			bonevar = new ysdnm_bone_var [o.bones];
			for(int i = 0; i < o.bones; i++){
				char *name = new char[strlen(o.bonevar[i].name)+1];
				strcpy(name, o.bonevar[i].name);
				bonevar[i].name = name;
				bonevar[i].pos = o.bonevar[i].pos;
				bonevar[i].rot = o.bonevar[i].rot;
				bonevar[i].visible = o.bonevar[i].visible;
			}
		}
		else
			bonevar = NULL;
		bones = o.bones;
		skipnames = NULL;
		skips = 0;
		next = NULL;
	}
	~ysdnm_var(){
		if(bonevar){
			for(int i = 0; i < bones; i++)
				delete[] bonevar[i].name;
			delete[] bonevar;
			bonevar = NULL;
		}
		bones = 0;
	}
	ysdnm_var *dup()const;
	ysdnm_bone_var *findBone(const char *name);
	ysdnm_bone_var *addBone(const char *name);
	ysdnm_var &amplify(double f);
#endif
} ysdnmv_t;

void DrawYSDNM_V(ysdnm_t *dnm, ysdnmv_t *);
void TransYSDNM_V(ysdnm_t *dnm, ysdnmv_t *v, void (*callback)(const char *name, void *hint), void *hint);


struct ysdnm_motion *YSDNM_MotionLoad(const char *fname);
struct ysdnm_var *YSDNM_MotionInterpolate(struct ysdnm_motion **mot, double *time, int nmot);
struct ysdnm_var *YSDNM_MotionInterpolateAmp(struct ysdnm_motion **mot, double *time, int nmot, double *amplitude);
void YSDNM_MotionInterpolateFree(struct ysdnm_var *mot);
void YSDNM_MotionSave(const char *fname, struct ysdnm_motion *mot);
void YSDNM_MotionSave2(const char *fname, struct ysdnm_motion *mot);
void YSDNM_MotionAddKeyframe(struct ysdnm_motion *mot, double dt);


#ifdef __cplusplus
}
#endif


#endif
