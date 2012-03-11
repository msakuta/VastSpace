#include "draw/mqoadapt.h"
#include "draw/material.h"

static void tex_callback(suf_t *suf, suftex_t **ret){
	CacheSUFMaterials(suf);
	*ret = gltestp::AllocSUFTex(suf);
}

Model *LoadMQOModel(const char *fname, double scale){
	return LoadMQOModel(fname, scale, tex_callback);
}
