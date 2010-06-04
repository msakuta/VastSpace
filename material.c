#include "material.h"
#include "bitmap.h"
//extern "C"{
#include <clib/zip/UnZip.h>
#include <clib/suf/sufdraw.h>
#include <clib/dstr.h>
#include <jpeglib.h>
//}
#include <setjmp.h>



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
	GLuint ret;
	int i;
	for(i = 0; i < g_nmats; i++) if(!strcmp(name, g_mats[i].name)){
		m = &g_mats[i];
		return CallCacheBitmap5(name, m->texname[0], &m->stp[0], m->texname[1], &m->stp[1]);
	}
	ret = CallCacheBitmap(name, name, NULL, NULL);
	if(!ret){
		dstr_t ds = dstr0;
		dstrcat(&ds, "models/");
		dstrcat(&ds, name);
		ret = CallCacheBitmap(name, dstr(&ds), NULL, NULL);
		dstrfree(&ds);
	}
	return ret;
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

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

static void my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

static BITMAPINFO *ReadJpeg(const char *fname){
	BITMAPINFO *bmi;
	FILE * infile;		/* source file */
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in image buffer */
	int src_row_stride;

	infile = fopen(fname, "rb");

	if(!infile)
		return NULL;

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, infile);
	(void) jpeg_read_header(&cinfo, TRUE);
	(void) jpeg_start_decompress(&cinfo);
	row_stride = cinfo.output_width * cinfo.output_components;
	src_row_stride = cinfo.output_width * cinfo.output_components;
	bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + cinfo.output_width * cinfo.output_height * cinfo.output_components);
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = cinfo.output_width; 
	bmi->bmiHeader.biHeight = cinfo.output_height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = cinfo.output_components * 8;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = cinfo.output_width * cinfo.output_height * cinfo.output_components;
	bmi->bmiHeader.biXPelsPerMeter = 0;
	bmi->bmiHeader.biYPelsPerMeter = 0;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, src_row_stride, 1);
	while (cinfo.output_scanline < cinfo.output_height) {
		unsigned j;
		(void) jpeg_read_scanlines(&cinfo, buffer, 1);

		// We cannot just memcpy it because of byte ordering.
		if(cinfo.output_components == 3) for(j = 0; j < cinfo.output_width; j++){
			JSAMPLE *dst = &((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride + j * cinfo.output_components];
			JSAMPLE *src = &buffer[0][j * cinfo.output_components];
			dst[0] = src[2];
			dst[1] = src[1];
			dst[2] = src[0];
		}
		else if(cinfo.output_components == 1) for(j = 0; j < cinfo.output_width; j++){
			JSAMPLE *dst = &((JSAMPLE*)bmi->bmiColors)[(cinfo.output_scanline-1) * row_stride + j * cinfo.output_components];
			JSAMPLE *src = &buffer[0][j * cinfo.output_components];
			dst[0] = src[0];
		}
	}
	(void) jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	fclose(infile);
	return bmi;
}

GLuint CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2){
	const struct suftexcache *stc;
	suftexparam_t stp, stp2;
	BITMAPFILEHEADER *bfh = NULL, *bfhMask = NULL, *bfh2;
	GLuint ret;
	int mask = 0, jpeg = 0, maskjpeg = 0, jpeg2 = 0;
	stp.bmiMask = NULL;

	stc = FindTexCache(entry);
	if(stc)
		return stc->list;
	stp = pstp1 ? *pstp1 : defstp;

	/* Try loading from plain bitmap first */
	stp.bmi = ReadBitmap(fname1);

	if(!stp.bmi){
		/* If couldn't, search through resource zip file. */
		bfh = ZipUnZip("rc.zip", fname1, NULL);
		stp.bmi = (BITMAPINFO*)&bfh[1];

		if(!bfh){
			/* If no luck yet, try jpeg decoding. */
			stp.bmi = ReadJpeg(fname1);
			jpeg = 1;
		}
	}

	/* If all above fail, give up loading image. */
	if(!stp.bmi)
		return 0;

	/* Alpha channel texture */
	if(stp.flags & STP_ALPHATEX){
		dstr_t ds = dstr0;
		dstrcat(&ds, fname1);
		dstrcat(&ds, ".a.bmp");
		bfhMask = ZipUnZip("rc.zip", dstr(&ds), NULL);
		stp.bmiMask = !bfhMask ? ReadBitmap(dstr(&ds)) : (BITMAPINFO*)&bfhMask[1];
		mask = !!stp.bmiMask;
		dstrfree(&ds);
		if(!stp.bmiMask){
			ds = dstr0;
			dstrcat(&ds, fname1);
			dstrcat(&ds, ".a.jpg");
			stp.bmiMask = ReadJpeg(dstr(&ds));
			dstrfree(&ds);
			mask = maskjpeg = !!stp.bmiMask;
		}
	}

	stp2 = pstp2 ? *pstp2 : defstp;
	if(fname2){
		bfh2 = ZipUnZip("rc.zip", fname2, NULL);
		stp2.bmi = !bfh2 ? ReadBitmap(fname2) : (BITMAPINFO*)&bfh2[1];
		if(!stp2.bmi)
			return 0;
//		jpeg2 = 1;
	}

	ret = CacheSUFMTex(entry, &stp, fname2 ? (BITMAPINFO*)&stp2 : NULL);
	if(bfh)
		ZipFree(bfh);
	else if(jpeg)
		free(stp.bmi);
	else
		LocalFree(stp.bmi);
	if(mask){
		if(bfhMask)
			ZipFree(bfhMask);
		else if(maskjpeg)
			free(stp.bmiMask);
		else
			LocalFree(stp.bmiMask);
	}
	if(fname2){
		if(bfh2)
			ZipFree(bfh2);
		else if(jpeg2)
			free(stp2.bmi);
		else
			LocalFree(stp2.bmi);
	}
	return ret;
}

GLuint CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2){
	return CallCacheBitmap5(entry, fname1, pstp, fname2, pstp);
}

suf_t *CallLoadSUF(const char *fname){
	suf_t *ret = NULL;
	do{
		char *p;
		p = strrchr(fname, '.');
		if(p && !strcmp(p, ".bin")){
			FILE *fp;
			long size;
			fp = fopen(fname, "rb");
			if(!fp)
				break;
			fseek(fp, 0, SEEK_END);
			size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			ret = malloc(size);
			fread(ret, size, 1, fp);
			fclose(fp);
		}
		else
			ret = LoadSUF(fname);
	} while(0);
	if(!ret){
		ret = ZipUnZip("rc.zip", fname, NULL);
	}
	if(!ret)
		return NULL;
	return RelocateSUF(ret);
}
