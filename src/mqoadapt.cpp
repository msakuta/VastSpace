/** \file
 * \brief Implementation of functions to adapt mqo.h to VastSpace project.
 */
#include "draw/mqoadapt.h"
#include "draw/material.h"
extern "C"{
#include <clib/zip/UnZip.h>
}
#include <streambuf>

struct MQOTextureLoad : MQOTextureCallback{
	const char *fpath;
	MQOTextureLoad(const char *fpath) : fpath(fpath){}
	void operator()(suf_t *suf, suftex_t **ret){
		CacheSUFMaterials(suf, fpath);
		*ret = gltestp::AllocSUFTex(suf, fpath);
	}
};

/// \brief An adaptor class for std::streambuf that feeds data from raw pointer source.
///
/// It does not use internal buffer at all to cache contents, because the source data
/// is already expanded to memory.
class srcbuf : public std::streambuf{
	char *src;
	std::streamsize size;
public:
	srcbuf(void *pv, std::streamsize size) : src((char*)pv), size(size){
		setg(src, src, &src[size]);
	}
	/// Probably not necessary to override.
	std::streamsize showmanyc(){
		return size;
	}
	/// Probably not necessary to override.
	std::streamsize xsgetn(char *s, std::streamsize n){
		if(size < n)
			n = size;
		memcpy(s, src, n);
		src += n;
		size -= n;
		return n;
	}
	/// Returns EOF when the source is exhausted.
	int underflow(){
		if(size <= gptr() - src)
			return EOF;
		setg(src, src, &src[size]);
		return *gptr();
	}
};

Model *LoadMQOModel(const char *fname, double scale){
	std::ifstream ifs(fname);

	// Retrieve file path delimiter
	const char *p = strrchr(fname, '\\');
	if(!p)
		p = strrchr(fname, '/');
	gltestp::dstring fpath;
	if(p){
		fpath.strncpy(fname, p - fname);
		fpath << '/';
	}

	if(ifs.good())
		return LoadMQOModelSource(ifs, scale, &MQOTextureLoad(fpath));
	else{
		unsigned long size;
		void *buffer = (BYTE*)ZipUnZip("rc.zip", fname, &size);
		if(!buffer)
			return NULL;
		srcbuf sb(buffer, size);
		std::istream is(&sb);
		Model *ret = LoadMQOModelSource(is, scale, &MQOTextureLoad(fpath));
		ZipFree(buffer);
		return ret;
	}
}

Motion *LoadMotion(const char *fname){
	std::ifstream ifs(fname);
	if(ifs.good())
		return new Motion(ifs);
	else{
		unsigned long size;
		void *buffer = (BYTE*)ZipUnZip("rc.zip", fname, &size);
		if(!buffer)
			return NULL;
		srcbuf sb(buffer, size);
		std::istream is(&sb);
		Motion *ret = new Motion(is);
		ZipFree(buffer);
		return ret;
	}
}
