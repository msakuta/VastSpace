#ifndef YSSURF_H
#define YSSURF_H
#include <clib/suf/suf.h>
#include <string.h>

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
	sufcoord pos[3];
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
void DrawYSDNM(ysdnm_t *dnm, const char *bonenames[], double (*bonerot)[4], int bones, const char *skipnames[], int skips);
void TransYSDNM(ysdnm_t *dnm, const char *bonenames[], double (*bonerot)[4], int bones, const char *skipnames[], int skips, void (*callback)(const char *name, void *hint), void *hint);

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

/* variation info */
typedef struct ysdnm_var{
	int fcla;
	int fviscla;
	double cla[21];
	const char **bonenames;
	double (*bonerot)[7];
	int bones;
	const char **skipnames;
	int skips;
	const char *target;
	struct ysdnm_var *next;
	float *visible; ///< Visibility factor. Visible if > 0.
#ifdef __cplusplus
	ysdnm_var() : target(NULL){
		fcla = 0;
		fviscla = 0;
		bonenames = NULL;
		bonerot = NULL;
		bones = 0;
		skipnames = NULL;
		skips = 0;
		next = NULL;
		visible = NULL;
	}
	ysdnm_var(const ysdnm_var &o) : target(NULL){
		fcla = 0;
		fviscla = 0;
		bonenames = new const char*[o.bones];
		for(int i = 0; i < o.bones; i++){
			char *name = new char[strlen(o.bonenames[i])+1];
			strcpy(name, o.bonenames[i]);
			bonenames[i] = name;
		}
		if(o.bonerot){
			bonerot = new double [o.bones][7];
			memcpy(bonerot, o.bonerot, o.bones * sizeof *bonerot);
		}
		else
			bonerot = NULL;
		bones = o.bones;
		skipnames = NULL;
		skips = 0;
		next = NULL;
		if(o.visible){
			visible = new float[o.bones];
			memcpy(visible, o.visible, o.bones * sizeof *visible);
		}
		else
			visible = NULL;
	}
	~ysdnm_var(){
		if(bonenames){
			for(int i = 0; i < bones; i++)
				delete[] bonenames[i];
			delete[] bonenames;
			bonenames = NULL;
		}
		if(bonerot){
			delete[] bonerot;
			bonerot = NULL;
		}
		if(visible){
			delete[] visible;
			visible = NULL;
		}
		bones = 0;
	}
	ysdnm_var *dup()const;
	double *addBone(const char *name);
#endif
} ysdnmv_t;

void DrawYSDNM_V(ysdnm_t *dnm, ysdnmv_t *);
void TransYSDNM_V(ysdnm_t *dnm, ysdnmv_t *v, void (*callback)(const char *name, void *hint), void *hint);


struct ysdnm_motion *YSDNM_MotionLoad(const char *fname);
struct ysdnm_var *YSDNM_MotionInterpolate(struct ysdnm_motion **mot, double *time, int nmot);
void YSDNM_MotionInterpolateFree(struct ysdnm_var *mot);
void YSDNM_MotionSave(const char *fname, struct ysdnm_motion *mot);
void YSDNM_MotionAddKeyframe(struct ysdnm_motion *mot, double dt);



#endif
