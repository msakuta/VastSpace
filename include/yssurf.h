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
CLA の値  	パーツの種類  	NST の値  	各 STA 行での状態  	対応する操作  	説明
0 	胴体 	1 	STA １行目／常時 	　 	動作しないパーツ全般に使います。
0 	ランディングギア 	2 	STA １行目／収納	STA ２行目／展開 	Landing Gear 	CLA 0 は NST の値で種類が変わります。
1 	可変翼 	2 	STA １行目／非展開	STA ２行目／展開 	(Extend／Retract)	Variable Geometry Wing 	通常は .dat の設定によって動作します。
2 	アフターバーナー 	2 	STA １行目／オフ	STA ２行目／オン 	Afterburner, Throttle 	　
3 	スピナー 	1 	STA １行目／常時 	　 	回転軸はＹ軸です。いつでも表示される回転部分に使います。
4 	スポイラー・エアブレーキ 	2 	STA １行目／非展開	STA ２行目／展開 	AirBrake 	　
5 	フラップ 	2 	STA １行目／非展開	STA ２行目／展開 	Flap 	操作設定により全５段階で動かせます。
6 	エレベーター 	3 	STA １行目／ニュートラル	STA ２行目／アップ	STA ３行目／ダウン 	Elevator (Trim) 	　
7 	エルロン 	3 	STA １行目／ニュートラル	STA ２行目／左ロール	STA ３行目／右ロール 	Aileron 	　
8 	ラダー 	3 	STA １行目／ニュートラル	STA ２行目／左	STA ３行目／右 	Rudder 	　
9 	爆弾倉ドア 	2 	STA １行目／閉	STA ２行目／開 	Open／Close　　　Bomb Bay Door,	Drop Bomb 	.dat の BOMINBAY を TRUE にすると爆弾投下時に自動で開きます。
10 	VTOL（推力偏向装置） 	2 	STA １行目／通常	STA ２行目／使用時 	Nozzle Up,Nozzle Down 	全１１段階で動作します。
11 	スラストリバーサー（逆噴射装置） 	2 	STA １行目／オフ	STA ２行目／オン 	Reverse Thrust 	キーを押している間作動します。
12 	CLA 10 連動１ 	2 	STA １行目／オフ	STA ２行目／オン 	Nozzle Up,Nozzle Down 	動作は CLA 10 と同じです。
13 	CLA 10 連動２ 	2 	STA １行目／オフ	STA ２行目／オン 	Nozzle Up,Nozzle Down 	全５段階、CLA 10の２倍のペースで動作します。
14 	ギアドア 	2 	STA １行目／閉	STA ２行目／開 	Landing Gear 	ギア動作前に開き、ギアの動作が終わると閉じます。（閉→開→閉）
15 	ランディングギア２ 	2 	STA １行目／閉	STA ２行目／開 	Landing Gear 	CLA 0 のギアより動作が遅く、また表示のタイミングが少し違います。
16 	ギアブレーキ 	2 	STA １行目／オフ	STA ２行目／オン 	Brake 	アレスティング・フックにも使用できます。
18 	プロペラ・ローター 	1 	STA １行目／常時 	Throttle 	回転軸はＹ軸です。低回転時にのみ見えるブレードなどに使います。
20 	プロペラ軌跡（残像） 	1 	STA １行目／常時 	Throttle 	回転軸はＹ軸です。高回転時にのみ見えるプロペラの軌跡などを表現します。
21 	回転銃座 	0 	　 	Rotate Turret 	回転軸はＹ軸です。手動の場合のみ操作できます。
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
