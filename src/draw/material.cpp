/** \file
 * \brief Texture image handling.
 *
 * Materials are defined to suggest multi-texturing, lighting, blending,
 * and programmable shaders to OpenGL for specific textures.
 *
 * Material definitions must precede image loading.
 *
 * JPEG and PNG reading methods are defined here too.
 */
#include "draw/material.h"
#include "bitmap.h"
#include "dstring.h"
#include "draw/OpenGLState.h"
#include "draw/ShaderBind.h"
#include "draw/ShadowMap.h"
#include "glsl.h"
#include "sqadapt.h"
#include <vector>
extern "C"{
#include <clib/c.h>
#include <clib/cfloat.h>
#include <clib/zip/UnZip.h>
#include <clib/suf/sufdraw.h>
#include <clib/suf/sufbin.h>
#include <clib/gl/multitex.h>
#include <clib/timemeas.h>
#include <jpeglib.h>
#include <jerror.h>
#include <png.h>
}
#include <sqstdio.h>
#include <sqstdaux.h>
#include <sqstdmath.h>
#include <setjmp.h>
#include <gl/glu.h>
#include <map>


static const suftexparam_t defstp = {
	STP_MAGFIL, // unsigned long flags;
	NULL, NULL, // const BITMAPINFO *bmi, *bmiMask;
	0, // GLuint env;
	GL_LINEAR, // GLuint magfil, minfil, wraps, wrapt;
};

TexParam::TexParam(const suftexparam_t &o) : suftexparam_t(o){
}


/// Material object.
struct MaterialBind{
	gltestp::dstring name;
	gltestp::dstring texname[2];
	TexParam stp[2];
	MaterialBind(const gltestp::dstring &name = "", const gltestp::dstring &texname0 = "", const gltestp::dstring &texname1 = "", const TexParam &stp0 = TexParam(defstp), const TexParam &stp1 = TexParam(defstp)) :
	name(name)
	{
		texname[0] = texname0;
		texname[1] = texname1;
		stp[0] = stp0;
		stp[1] = stp1;
	}
};

typedef std::map<gltestp::dstring, MaterialBind> Materials;
static Materials g_mats;

void AddMaterial(const char *name, const char *texname1, TexParam *stp1, const char *texname2, TexParam *stp2){
	MaterialBind *m;
	gltestp::dstring aname = name;
	Materials::iterator i = g_mats.find(aname);
	if(i != g_mats.end()){
//	for(i = g_mats.begin(); i != g_mats.end(); i++) if(!strcmp(name, i->name)){
		m = &i->second;
		if(texname1)
			m->texname[0] = texname1;
		if(texname2)
			m->texname[1] = texname2;
		m->stp[0] = stp1 ? *stp1 : defstp;
		m->stp[1] = stp2 ? *stp2 : defstp;
		return;
	}
	g_mats[aname] = MaterialBind(aname, texname1, texname2, stp1 ? *stp1 : defstp, stp2 ? *stp2 : defstp);
}

GLuint CacheMaterial(const char *name){
	MaterialBind *m;
	GLuint ret;
	gltestp::dstring aname = name;
	Materials::iterator i = g_mats.find(aname);
	if(i != g_mats.end()){
//	for(i = g_mats.begin(); i != g_mats.end(); i++) if(name == i->name){
		return CallCacheBitmap5(name, i->second.texname[0], &i->second.stp[0], i->second.texname[1], &i->second.stp[1]);
	}
	ret = CallCacheBitmap(name, name, NULL, NULL);
/*	if(!ret){
		gltestp::dstring ds = "models/";
		ds << aname;
		ret = CallCacheBitmap(name, ds, NULL, NULL);
	}*/
	return ret;
}

void CacheSUFMaterials(const suf_t *suf, const char *path){
	int i, j;
	if(!suf)
		return;
	for(i = 0; i < suf->na; i++) if(suf->a[i].colormap){
		gltestp::dstring textureName = gltestp::dstring(path) << suf->a[i].colormap;

		if(gltestp::FindTexture(textureName))
			continue;

		/* an unreasonable filtering to exclude default mapping. */
//		if(!strcmp(suf->a[i].colormap, "Mapping1.bmp"))
//			continue;

		/* skip the same texture */
		for(j = 0; j < i; j++) if(suf->a[j].colormap && !strcmp(suf->a[i].colormap, suf->a[j].colormap))
			break;
		if(j != i)
			continue;
		CacheMaterial(textureName);
	}
}



/*#define BUFFER_SIZE 2048*/

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_source_mgr{
	struct jpeg_source_mgr pub; /* public fields */
	int source_size;
	JOCTET* source_data;
	boolean start_of_file;
/*	JOCTET buffer[BUFFER_SIZE];*/
} my_source_mgr;

typedef my_source_mgr * my_src_ptr;
typedef struct my_error_mgr * my_error_ptr;

static void my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Disable the message if the image is not a JPEG, to avoid a message
	 when PNG formatted file is about to be read. */
	/* We could postpone this until after returning, if we chose. */
	if(cinfo->err->msg_code != JERR_NO_SOI)
		(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr) cinfo ->src;
	src->start_of_file = TRUE; 
}

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;
#if 1
	src->pub.next_input_byte = src->source_data;
	src->pub.bytes_in_buffer = src->source_size;
#else
	size_t nbytes = 0;	
	/* Create a fake EOI marker */
	if(src->source_size > BUFFER_SIZE){
		nbytes = BUFFER_SIZE;
	}
	else
	{
		nbytes = src->source_size;
	}

	if(nbytes <= 0)
	{
		if(src->start_of_file)
        {
			exit(1); // treat empty input file as fatal error
		}
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}
	else
	{
		memcpy(src->buffer, src->source_data,nbytes);
		src->source_data += nbytes;
		src->source_size -= nbytes;
	}
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;
#endif
	return TRUE;
}

METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr src = (my_src_ptr) cinfo->src;
	if (num_bytes <= src->pub.bytes_in_buffer ){
		src->pub.bytes_in_buffer -= num_bytes;
		src->pub.next_input_byte += num_bytes;
	}
	else{
		num_bytes -= src->pub.bytes_in_buffer;
		src->pub.bytes_in_buffer = 0;
		src->source_data += num_bytes;
		src->source_size -= num_bytes;
	}
}

METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
	(void)cinfo;
}

static void jpeg_memory_src (j_decompress_ptr cinfo, const JOCTET * buffer, size_t bufsize)
{
	my_src_ptr src;
	
	if (cinfo->src == NULL) 
	{   /* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(my_source_mgr));
		src = (my_src_ptr) cinfo -> src;
	}
	src = (my_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;
	src->pub.bytes_in_buffer = 0; // forces fill_input_buffer on first read
	src->pub.next_input_byte = NULL; // until buffer loaded
	src->source_data = const_cast<JOCTET*>(buffer);
	src->source_size = bufsize;
	src->start_of_file = 0;
}


BITMAPINFO *ReadJpeg(const char *fname, void (**freeproc)(BITMAPINFO*)){
	BITMAPINFO *bmi;
	FILE * infile;		/* source file */
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in image buffer */
	int src_row_stride;
	BYTE *image_buffer = NULL;
	unsigned long size;

	infile = fopen(fname, "rb");

	if(!infile){
		image_buffer = (BYTE*)ZipUnZip("rc.zip", fname, &size);
		if(!image_buffer)
			return NULL;
	}

	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		if(infile)
			fclose(infile);
		else
			ZipFree(image_buffer);
		return NULL;
	}
	jpeg_create_decompress(&cinfo);

	/* Choose source manager */
	if(!image_buffer)
		jpeg_stdio_src(&cinfo, infile);
	else
		jpeg_memory_src(&cinfo, (JOCTET*)image_buffer, size);

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

	/* If the on-memory source is available, free it, otherwise the source is a file. */
	if(image_buffer)
		free(image_buffer);
	else
		fclose(infile);

	if(freeproc)
		*freeproc = (void(*)(BITMAPINFO*))free;
	return bmi;
}

struct ReadPNGData{
	BYTE *image_buffer, *current;
	unsigned long size;
};

static void ReadPNG_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	struct ReadPNGData *prpd;

	prpd = (struct ReadPNGData*)png_get_io_ptr(png_ptr);
	if (prpd != NULL)
	{
		memcpy(data, prpd->current, length);
		prpd->current += length;
	}

	if (0 < prpd->image_buffer - prpd->current)
		png_error(png_ptr, "Read Error!");
}

BITMAPINFO *ReadPNG(const char *fname, void (**freeproc)(BITMAPINFO*)){
	FILE * infile;		/* source file */
	struct ReadPNGData rpd;
	png_structp png_ptr;
	png_infop info_ptr = NULL, end_info = NULL;
	png_bytepp ret;

	infile = fopen(fname, "rb");

	if(!infile){
		rpd.image_buffer = (BYTE*)ZipUnZip("rc.zip", fname, &rpd.size);
		if(!rpd.image_buffer)
			return NULL;
	}

	{
		unsigned char header[8];
		if(infile)
			fread(header, sizeof header, 1, infile);
		else{
			memcpy(header, rpd.image_buffer, sizeof header);
			rpd.current = &rpd.image_buffer[sizeof header];
		}
		if(png_sig_cmp(header, 0, sizeof header)){
			fclose(infile);
			return NULL;
		}
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!png_ptr){
		goto shortjmp;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr){
		goto shortjmp;
	}

	end_info = png_create_info_struct(png_ptr);
	if(!end_info){
		goto shortjmp;
	}

	if(setjmp(png_jmpbuf(png_ptr))){
shortjmp:
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		if(infile)
			fclose(infile);
		else
			ZipFree(rpd.image_buffer);
		return NULL;
	}

	if(infile)
		png_init_io(png_ptr, infile);
	else
		png_set_read_fn(png_ptr, (png_voidp)&rpd, ReadPNG_read_data);
	png_set_sig_bytes(png_ptr, 8);

	{
		BITMAPINFO *bmi;
		png_uint_32 width, height;
		int bit_depth, color_type, interlace_type, comps;
		int i;

		/* The native order of RGB components differs in order against Windows bitmaps,
		 * so we must instruct libpng to convert it. */
		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, NULL);

		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
			&interlace_type, NULL, NULL);

		/* Grayscale images are not supported.
		 * TODO: alpha channel? */
		if(bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA && color_type != PNG_COLOR_TYPE_PALETTE)
			goto shortjmp;

		/* Calculate number of components. */
		comps = (color_type == PNG_COLOR_TYPE_PALETTE ? 1 : color_type == PNG_COLOR_TYPE_RGB ? 3 : 4);

		// Supports paletted images
		png_colorp pal;
		int npal = 0;
		if(color_type == PNG_COLOR_TYPE_PALETTE){
			png_get_PLTE(png_ptr, info_ptr, &pal, &npal);
		}

		/* png_get_rows returns array of pointers to rows allocated by the library,
		 * which must be copied to single bitmap buffer. */
		ret = png_get_rows(png_ptr, info_ptr);

		bmi = (BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER) + npal * sizeof(*bmi->bmiColors) + (width * comps + 3) / 4 * 4 * height);
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biWidth = width;
		bmi->bmiHeader.biHeight = height;
		bmi->bmiHeader.biPlanes = 1;
		bmi->bmiHeader.biBitCount = bit_depth * comps;
		bmi->bmiHeader.biCompression = BI_RGB;
		bmi->bmiHeader.biSizeImage = 0;
		bmi->bmiHeader.biXPelsPerMeter = 0;
		bmi->bmiHeader.biYPelsPerMeter = 0;
		bmi->bmiHeader.biClrUsed = npal;
		bmi->bmiHeader.biClrImportant = npal;
		for(i = 0; i < npal; i++){
			bmi->bmiColors[i].rgbBlue = pal[i].blue;
			bmi->bmiColors[i].rgbGreen = pal[i].green;
			bmi->bmiColors[i].rgbRed = pal[i].red;
			bmi->bmiColors[i].rgbReserved = 0;
		}
		for(i = 0; i < height; i++)
			memcpy(&((unsigned char*)&bmi->bmiColors[npal])[(height - i - 1) * width * comps], ret[i], width * comps);

		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		if(infile)
			fclose(infile);
		else
			ZipFree(rpd.image_buffer);
		if(freeproc)
			*freeproc = (void(*)(BITMAPINFO*))free;
		return bmi;
	}
}

static void LocalFreer(BITMAPINFO *bmi){
	LocalFree(bmi);
}

/// \brief Makes a texture bitmap name known as a material that have given characteristics.
///
/// I have tried to make the arguments dstring, but no luck.
/// The reason is that NULL pointers to be treated as integral values and converted to "0".
///
/// If we could make the arguments dstring, the same strings would share memory to improve space
/// performance, but it's not much drastic.
GLuint CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2){
	const struct suftexcache *stc;
	suftexparam_t stp, stp2;
	BITMAPFILEHEADER *bfh = NULL, *bfhMask = NULL;
	GLuint ret;
	int mask = 0, maskjpeg = 0;
	void (*freeproc)(BITMAPINFO*) = LocalFreer;
	void (*freeproc2)(BITMAPINFO*) = LocalFreer;
	stp.bmiMask = NULL;

	// We must not rely onto sufdraw's global variables.
//	stc = FindTexCache(entry);
//	if(stc)
//		return stc->list;
	stp = pstp1 ? *pstp1 : defstp;

	/* Try loading from plain bitmap first */
	stp.bmi = ReadBitmap(fname1);

	if(!stp.bmi){
		/* If no luck yet, try jpeg decoding. */
		stp.bmi = ReadJpeg(fname1, &freeproc);
	}

	if(!stp.bmi){
		/* If still not, try PNG. */
		stp.bmi = ReadPNG(fname1, &freeproc);
	}

	/* If all above fail, give up loading image. */
	if(!stp.bmi)
		return 0;

	/* Alpha channel texture */
	if(stp.flags & STP_ALPHATEX){
		gltestp::dstring ds = fname1;
		ds << ".a.bmp";
		bfhMask = (BITMAPFILEHEADER*)ZipUnZip("rc.zip", ds, NULL);
		stp.bmiMask = !bfhMask ? ReadBitmap(ds) : (BITMAPINFO*)&bfhMask[1];
		mask = !!stp.bmiMask;
		if(!stp.bmiMask){
			ds = fname1;
			ds << ".a.jpg";
			stp.bmiMask = ReadJpeg(ds, NULL);
			mask = maskjpeg = !!stp.bmiMask;
		}
	}

	stp2 = pstp2 ? *pstp2 : defstp;
	if(fname2 && *fname2){
		/* Try loading from plain bitmap first */
		stp2.bmi = ReadBitmap(fname2);
		if(!stp2.bmi)
			stp2.bmi = ReadJpeg(fname2, &freeproc2);
		if(!stp2.bmi)
			stp2.bmi = ReadPNG(fname2, &freeproc2);
		if(!stp2.bmi)
			return 0;
	}

	ret = gltestp::CacheSUFMTex(entry, &gltestp::TextureKey(&stp), fname2 && *fname2 ? &gltestp::TextureKey(&stp2) : NULL);

	freeproc(const_cast<BITMAPINFO*>(stp.bmi));

	if(mask){
		if(bfhMask)
			ZipFree(bfhMask);
		else if(maskjpeg)
			free((BITMAPINFO*)stp.bmiMask);
		else
			LocalFree((BITMAPINFO*)stp.bmiMask);
	}
	if(fname2 && *fname2){
		freeproc2(const_cast<BITMAPINFO*>(stp2.bmi));
	}
	return ret;
}

/// \brief Makes a texture bitmap name known as a material that have given characteristics.
///
/// I have tried to make the arguments dstring, but no luck.
///
/// Simply calls CallCacheBitmap5 with the last argument NULL.
/// With C++ function definition, this function is no longer necessary.
GLuint CallCacheBitmap(const char *entry, const char *fname1, suftexparam_t *pstp, const char *fname2){
	return CallCacheBitmap5(entry, fname1, pstp, fname2, pstp);
}

suf_t *CallLoadSUF(const char *fname){
	suf_t *ret = NULL;
	do{
		const char *p;
		p = strrchr(fname, '.');
		if(p && !strcmp(p, ".bin")){
			FILE *fp;
			long size;
			void *src;
			fp = fopen(fname, "rb");
			if(!fp)
				break;
			fseek(fp, 0, SEEK_END);
			size = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			src = malloc(size);
			fread(src, size, 1, fp);
			fclose(fp);
			ret = UnserializeSUF(src, size);
			free(src);
		}
		else
			ret = LoadSUF(fname);
	} while(0);
	if(!ret){
		void *src;
		unsigned long size;
		src = ZipUnZip("rc.zip", fname, &size);
		if(src){
			ret = UnserializeSUF(src, size);
			free(src);
		}
	}
	if(!ret)
		return NULL;
	return (ret);
}


namespace gltestp{

//static std::map<TextureKey, TextureBind> textures;
typedef std::map<gltestp::dstring, OpenGLState::weak_ptr<TexCacheBind> > TCBMap;
static TCBMap gstc;

TexCacheBind::TexCacheBind(const gltestp::dstring &aname, bool additive) : name(aname), list(0), additive(additive){
	tex[0] = tex[1] = NULL;
}

TexCacheBind::~TexCacheBind(){
	if(tex[0])
		glDeleteTextures(2, tex);
	if(list)
		glDeleteLists(list, 1);
}

/* if we have already compiled the texture into a list, reuse it */
const TexCacheBind *FindTexture(const gltestp::dstring &name){
	return gstc[name];
}


static void cachemtex(const suftexparam_t *stp){
	if(stp->flags & STP_ALPHA_TEST /*stp->mipmap & 0x80*/){
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER, .5f);
	}
	else
		glDisable(GL_ALPHA_TEST);
	glEnable(GL_TEXTURE_2D);
}

static GLuint cachetex(const suftexparam_t *stp){
	GLuint ret;
	const BITMAPINFO *bmi = stp->bmi;
	GLint env = stp->env;
	unsigned char *head;
	int alpha = stp->flags & STP_ALPHA;
	int mipmap = stp->flags & STP_MIPMAP;
#if 1
	GLubyte (*tex)[3], (*tex4)[4];

	head = &((unsigned char*)bmi)[offsetof(BITMAPINFO, bmiColors) + bmi->bmiHeader.biClrUsed * sizeof(RGBQUAD)];

	switch(bmi->bmiHeader.biBitCount){
	case 4:
	{
		int x, y;
		int cols = bmi->bmiHeader.biClrUsed ? bmi->bmiHeader.biClrUsed : 16;
		const unsigned char *src = (const unsigned char*)&bmi->bmiColors[cols];
		if(alpha){
			tex4 = (GLubyte(*)[4])malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex4);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++){
				unsigned char *buf = head + (bmi->bmiHeader.biWidth * bmi->bmiHeader.biBitCount + 31) / 32 * 4 * y;
				for(x = 0; x < bmi->bmiHeader.biWidth; x++){
					int pos = x + y * bmi->bmiHeader.biWidth, idx = 0xf & (buf[x / 2] >> ((x+1) % 2 * 4));
					tex4[pos][0] = bmi->bmiColors[idx].rgbRed;
					tex4[pos][1] = bmi->bmiColors[idx].rgbGreen;
					tex4[pos][2] = bmi->bmiColors[idx].rgbBlue;
//					tex4[pos][3] = stp->mipmap & 0x80 ? idx ? 255 : 0 : 128 == bmi->bmiColors[idx].rgbRed && 128 == bmi->bmiColors[idx].rgbGreen && 128 == bmi->bmiColors[idx].rgbBlue ? 0 : 255;
					tex4[pos][3] = stp->transparentColor == idx ? 0 : 255;
				}
			}
		}
		else{
			tex = (GLubyte(*)[3])malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++){
				unsigned char *buf = head + (bmi->bmiHeader.biWidth * bmi->bmiHeader.biBitCount + 31) / 32 * 4 * y;
				for(x = 0; x < bmi->bmiHeader.biWidth; x++){
					int pos = x + y * bmi->bmiHeader.biWidth, idx = 0xf & (buf[x / 2] >> ((x+1) % 2 * 4));
					tex[pos][0] = bmi->bmiColors[idx].rgbRed;
					tex[pos][1] = bmi->bmiColors[idx].rgbGreen;
					tex[pos][2] = bmi->bmiColors[idx].rgbBlue;
				}
			}
		}
		break;
	}
	case 8:
	{
		int x, y;
		int cols = bmi->bmiHeader.biClrUsed;
		const unsigned char *src = (const unsigned char*)&bmi->bmiColors[cols];
		if(alpha){
			tex4 = (GLubyte(*)[4])malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex4);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth, idx = src[pos];
				tex4[pos][0] = bmi->bmiColors[idx].rgbRed;
				tex4[pos][1] = bmi->bmiColors[idx].rgbGreen;
				tex4[pos][2] = bmi->bmiColors[idx].rgbBlue;
//				tex4[pos][3] = 128 == bmi->bmiColors[idx].rgbRed && 128 == bmi->bmiColors[idx].rgbGreen && 128 == bmi->bmiColors[idx].rgbBlue ? 0 : 255;
				tex4[pos][3] = stp->transparentColor == idx ? 0 : 255;
			}
		}
		else{
			int w = bmi->bmiHeader.biWidth, h = bmi->bmiHeader.biHeight, wb = (bmi->bmiHeader.biWidth + 3) / 4 * 4;
			tex = (GLubyte(*)[3])malloc(w * h * sizeof*tex);
			if(stp->flags & STP_NORMALMAP) for(y = 0; y < h; y++) for(x = 0; x < w; x++){
				int pos = x + y * w;
				tex[pos][0] = (unsigned char)rangein(127 + (double)stp->normalfactor * (src[x + y * wb] - src[(x - 1 + w) % w + y * wb]) / 4, 0, 255);
				tex[pos][1] = (unsigned char)rangein(127 + stp->normalfactor * (src[x + y * wb] - src[x + (y - 1 + h) % h * wb]) / 4, 0, 255);
				tex[pos][2] = 255;
			}
			else  for(y = 0; y < h; y++) for(x = 0; x < w; x++){
				int pos = x + y * w, idx = src[x + y * wb];
				tex[pos][0] = bmi->bmiColors[idx].rgbRed;
				tex[pos][1] = bmi->bmiColors[idx].rgbGreen;
				tex[pos][2] = bmi->bmiColors[idx].rgbBlue;
			}
		}
		break;
	}
	case 24:
	{
		int x, y;
		int cols = bmi->bmiHeader.biClrUsed;
		const unsigned char (*src)[3] = (const unsigned char(*)[3])&bmi->bmiColors[cols];
		if(alpha){
			int alphatex = stp->flags & STP_ALPHATEX && stp->bmiMask->bmiHeader.biBitCount == 8 && stp->bmi->bmiHeader.biWidth == stp->bmiMask->bmiHeader.biWidth && stp->bmi->bmiHeader.biHeight == stp->bmiMask->bmiHeader.biHeight;
			BYTE *alphasrc;
			if(alphatex)
				alphasrc = (BYTE*)&stp->bmiMask->bmiColors[stp->bmiMask->bmiHeader.biClrUsed];
			tex4 = (GLubyte(*)[4])malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex4);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				tex4[pos][0] = src[pos][2];
				tex4[pos][1] = src[pos][1];
				tex4[pos][2] = src[pos][0];
				tex4[pos][3] = alphatex ? alphasrc[pos] : stp->transparentColor == (tex4[pos][0] | (tex4[pos][1] << 8) | (tex4[pos][2] << 16)) ? 0 : 255;
			}
		}
		else{
			tex = (GLubyte(*)[3])malloc(bmi->bmiHeader.biWidth * bmi->bmiHeader.biHeight * sizeof*tex);
			for(y = 0; y < bmi->bmiHeader.biHeight; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				tex[pos][0] = src[pos][2];
				tex[pos][1] = src[pos][1];
				tex[pos][2] = src[pos][0];
			}
		}
		break;
	}
	case 32:
	{
		int x, y, h = ABS(bmi->bmiHeader.biHeight);
		int cols = bmi->bmiHeader.biClrUsed;
		const unsigned char (*src)[4] = (const unsigned char(*)[4])&bmi->bmiColors[cols], *mask;
		if(stp->flags & STP_MASKTEX)
			mask = (const unsigned char*)&stp->bmiMask->bmiColors[stp->bmiMask->bmiHeader.biClrUsed];
		if((stp->flags & (STP_ALPHA | STP_RGBA32)) == (STP_ALPHA | STP_RGBA32)){
			tex4 = (unsigned char(*)[4])src;
		}
		else if(alpha /*stp->alphamap & (3 | STP_MASKTEX)*/){
			tex4 = (GLubyte(*)[4])malloc(bmi->bmiHeader.biWidth * h * sizeof*tex4);
			for(y = 0; y < h; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				int pos1 = x + (bmi->bmiHeader.biHeight < 0 ? h - y - 1 : y) * bmi->bmiHeader.biWidth;
				tex4[pos1][0] = src[pos][2];
				tex4[pos1][1] = src[pos][1];
				tex4[pos1][2] = src[pos][0];
				tex4[pos1][3] = stp->alphamap & STP_MASKTEX ?
					((mask[x / 8 + (stp->bmiMask->bmiHeader.biWidth + 31) / 32 * 4 * (bmi->bmiHeader.biHeight < 0 ? h - y - 1 : y)] >> (7 - x % 8)) & 1) * 127 + 127 :
					stp->alphamap == 2 ? src[pos][3] : 255;
			}
		}
		else{
			tex = (GLubyte(*)[3])malloc(bmi->bmiHeader.biWidth * h * sizeof*tex);
			for(y = 0; y < h; y++) for(x = 0; x < bmi->bmiHeader.biWidth; x++){
				int pos = x + y * bmi->bmiHeader.biWidth;
				int pos1 = x + (bmi->bmiHeader.biHeight < 0 ? h - y - 1 : y) * bmi->bmiHeader.biWidth;
				tex[pos1][0] = src[pos][2];
				tex[pos1][1] = src[pos][1];
				tex[pos1][2] = src[pos][0];
			}
		}
		break;
	}
	default:
		return 0;
	}
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);

	// The texture parameters and environments belong to a texture unit; they'll restore if you call glBindTexture later on.
	// Therefore, we do not set them in the display list, but just before loading content.
	// At least magnify and minify filter parameters must be specified prior to actually load the image anyway.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, stp->flags & STP_WRAP_S ? stp->wraps : GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, stp->flags & STP_WRAP_T ? stp->wrapt : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, stp->flags & STP_MAGFIL ? stp->magfil : GL_NEAREST); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, stp->flags & STP_MINFIL ? stp->minfil : stp->flags & STP_MIPMAP ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, stp->flags & STP_ENV ? stp->env : GL_MODULATE);

	if(mipmap)
		gluBuild2DMipmaps(GL_TEXTURE_2D, alpha ? GL_RGBA : GL_RGB, bmi->bmiHeader.biWidth, ABS(bmi->bmiHeader.biHeight),
			alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, alpha ? (void*)tex4 : (void*)tex);
	else
		glTexImage2D(GL_TEXTURE_2D, 0, alpha ? GL_RGBA : GL_RGB, bmi->bmiHeader.biWidth, ABS(bmi->bmiHeader.biHeight), 0,
			alpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, alpha ? (void*)tex4 : (void*)tex);
	if(!(bmi->bmiHeader.biBitCount == 32 && (stp->flags & (STP_ALPHA | STP_RGBA32)) == (STP_ALPHA | STP_RGBA32)))
		free(stp->flags & STP_ALPHA ? (void*)tex4 : (void*)tex);
#else
	{
		int i;
		static const GLenum target[3] = {GL_PIXEL_MAP_I_TO_B, GL_PIXEL_MAP_I_TO_G, GL_PIXEL_MAP_I_TO_R};
		GLushort *map;
		map = malloc(cols * sizeof*map);
		for(i = 0; i < 3; i++){
			int j;
			for(j = 0; j < cols; j++)
				map[j] = (GLushort)(*(unsigned long*)&bmi->bmiColors[j] >> i * 8 << 8);
			glPixelMapusv(target[i], cols, map);
		}
		free(map);
	}
	glGenTextures(1, &ret);
	glBindTexture(GL_TEXTURE_2D, ret);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB4, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight, 0,
		GL_COLOR_INDEX, GL_UNSIGNED_BYTE, &bmi->bmiColors[cols]);
/*	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, bmi->bmiHeader.biWidth, bmi->bmiHeader.biHeight,
		GL_COLOR_INDEX, GL_UNSIGNED_BYTE, &bmi->bmiColors[cols]);*/
#endif
/*	cachemtex(stp);*/
	return ret;
}

unsigned long CacheSUFMTex(const char *name, const TextureKey *tex1, const TextureKey *tex2){

	/* allocate cache list space */
	std::map<gltestp::dstring, OpenGLState::weak_ptr<TexCacheBind> >::iterator it = gstc.find(name);
	if(it != gstc.end() && it->second)
		return it->second->getList();
	OpenGLState::weak_ptr<TexCacheBind> &wp = gstc[name];
	wp.create(*openGLState);
	TexCacheBind &tcb(*wp);
//	gstc = realloc(gstc, ++nstc * sizeof *gstc);
//	gstc[nstc-1].name = malloc(strlen(name)+1);
//	strcpy(gstc[nstc-1].name, name);

	tcb.setAdditive(tex1->flags & STP_ENV && tex1->env == GL_ADD);

/*	cachetex(bmi);*/
	glPushAttrib(GL_TEXTURE_BIT);
/*	glGetIntegerv(GL_TEXTURE_2D_BINDING, &prevtex);*/
	if(!strcmp(name, "Mapping1.bmp")){
		return 0;
	}
/*	glNewList(gstc[nstc-1].list = glGenLists(1), GL_COMPILE);*/
	tcb.setTex(1, 0);
	if(!glActiveTextureARB){
//		TextureBind tb = textures[*tex1];
//		if(!tb){
			TextureBind tb = cachetex(tex1);
//			textures[*tex1] = tb;
//		}
		tcb.setTex(0, tb);
	}
	else{
		if(tex2){
/*			glActiveTextureARB(GL_TEXTURE0_ARB);*/
			TextureBind tb = cachetex(tex1);
			tcb.setTex(0, tb);
/*			glActiveTextureARB(GL_TEXTURE1_ARB);*/
/*			if(tex1->bmi == tex2->bmi){
				gstc[nstc-1].tex[1] = gstc[nstc-1].tex[0];
				glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[1] = gstc[nstc-1].tex[0]);
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
			}
			else*/
				tb = cachetex(tex2);
				tcb.setTex(1, tb);
/*			glActiveTextureARB(GL_TEXTURE0_ARB);*/
		}
		else{
/*			glActiveTextureARB(GL_TEXTURE0_ARB);*/
			tcb.setTex(0, cachetex(tex1));
/*			glActiveTextureARB(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);*/
		}
	}
/*	glBindTexture(GL_TEXTURE_2D, prevtex);*/
	glPopAttrib();
/*	glEndList();*/

	tcb.setList(glGenLists(1));
	glNewList(tcb.getList(), GL_COMPILE);
	if(!glActiveTextureARB){
		cachemtex(tex1);
		glBindTexture(GL_TEXTURE_2D, tcb.getTex(0));
		{
			GLfloat envcolor[4] = {1., 0., 0., 1.};
			glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
		}
	}
	else{
		if(tex2){
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, tcb.getTex(0));
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, tcb.getTex(1));
			cachemtex(tex2);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			{
				GLfloat envcolor[4] = {1., 1., 0., 1.};
				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
			}
		}
		else{
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, tcb.getTex(0));
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			{
				GLfloat envcolor[4] = {1., 0., 0., 1.};
				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, envcolor);
			}
		}
	}
	glEndList();
#if 0
	glNewList(gstc[nstc-1].list = glGenLists(1), GL_COMPILE);
	if(!glActiveTextureARB){
		glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
		cachemtex(tex1);
	}
	else{
		if(tex2){
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[1]);
			cachemtex(tex2);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
		else{
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_2D, gstc[nstc-1].tex[0]);
			cachemtex(tex1);
			glActiveTextureARB(GL_TEXTURE1_ARB);
			glDisable(GL_TEXTURE_2D);
			glActiveTextureARB(GL_TEXTURE0_ARB);
		}
	}
	glEndList();
#endif

	return tcb.getList();
}

unsigned long CacheSUFTex(const char *name, const BITMAPINFO *bmi, int mipmap){
	suftexparam_t stp;
	if(!bmi)
		return 0;
	stp.bmi = bmi;
	stp.flags = STP_ENV | STP_ALPHA | (mipmap ? STP_MIPMAP : 0);
	stp.env = GL_MODULATE;
	stp.mipmap = mipmap;
	stp.magfil = GL_NEAREST;
	stp.alphamap = 1;
	return CacheSUFMTex(name, &stp, NULL);
}

suftex_t *AllocSUFTex(const suf_t *suf, const char *path){
	return gltestp::AllocSUFTexScales(suf, NULL, 0, NULL, 0, path);
}

static void shaderOnBeginTexture(void *){
	if(g_shader_enable && g_currentShaderBind){
		(*g_currentShaderBind)->enableTextures(false, false);
	}
}

static void shaderOnEndTexture(void *){
	if(g_shader_enable && g_currentShaderBind){
		(*g_currentShaderBind)->enableTextures(true, false);
	}
}

static void additiveShaderOnBeginTexture(void *){
	if(g_shader_enable && g_currentShadowMap){
		(*g_currentShadowMap)->setAdditive(true);
	}
}

static void additiveShaderOnEndTexture(void *){
	if(g_shader_enable && g_currentShadowMap){
		(*g_currentShadowMap)->setAdditive(false);
	}
}

suftex_t *AllocSUFTexScales(const suf_t *suf, const double *scales, int nscales, const char **texes, int ntexes, const char *path){
	suftex_t *ret;
	int i, n, k;
/*	for(i = n = 0; i < suf->na; i++) if(suf->a[i].colormap)
		n++;*/
	if(!suf)
		return NULL;
	n = suf->na;
	ret = (suftex_t*)malloc(offsetof(suftex_t, a) + n * sizeof *ret->a);
	ret->n = n;
	for(i = k = 0; i < n; i++){
		unsigned j;
		const char *colormap = texes && i < ntexes && texes[i] ? texes[i] : suf->a[i].colormap;

		k = i;
		if(!(suf->a[i].valid & SUF_TEX) || !colormap){
			struct suftexlist *s = &ret->a[k];
			s->list = 0;
			s->tex[0] = 0;
			s->tex[1] = 0;
			s->scale = 1.;
			s->onBeginTexture = shaderOnBeginTexture; // Tell the shader that the texture is disabled.
			s->onBeginTextureData = NULL;
			s->onInitedTexture = NULL;
			s->onInitedTextureData = NULL;
			s->onEndTexture = shaderOnEndTexture; // Tell The shader to restore texture usage.
			s->onEndTextureData = NULL;
			continue;
		}

		// Allocate dynamic string to concatenate strings.
		gltestp::dstring name = !path && !colormap ? "" : !path ? colormap : !colormap ? path : gltestp::dstring(path) << colormap;

		/* if we have already compiled the texture into a list, reuse it */
		std::map<gltestp::dstring, OpenGLState::weak_ptr<TexCacheBind> >::iterator it = gstc.find(name);
		if(it != gstc.end() && it->second){
			TexCacheBind &tcb = *gstc[name];
			struct suftexlist *s = &ret->a[k];
			s->list = tcb.getList();
			s->tex[0] = tcb.getTex(0);
			s->tex[1] = tcb.getTex(1);
			s->scale = 1.;
			s->onBeginTexture = it->second->getAdditive() ? additiveShaderOnBeginTexture : NULL;
			s->onBeginTextureData = NULL;
			s->onInitedTexture = NULL;
			s->onInitedTextureData = NULL;
			s->onEndTexture = it->second->getAdditive() ? additiveShaderOnEndTexture :NULL;
			s->onEndTextureData = NULL;
			continue;
		}
		else{
			/* otherwise, compile it */
#if 1
			CallCacheBitmap(name, name, NULL, NULL);
			it = gstc.find(name);
			if(it != gstc.end() && it->second){
				TexCacheBind &tcb = *gstc[name];
				struct suftexlist *s = &ret->a[k];
				s->list = tcb.getList();
				s->tex[0] = tcb.getTex(0);
				s->tex[1] = tcb.getTex(1);
				s->scale = 1.;
				s->onBeginTexture = NULL;
				s->onBeginTextureData = NULL;
				s->onInitedTexture = NULL;
				s->onInitedTextureData = NULL;
				s->onEndTexture = NULL;
				s->onEndTextureData = NULL;
				continue;
			}
			else{
				TexCacheBind &tcb = *gstc[name];
				struct suftexlist *s = &ret->a[k];
				s->list = 0;
				s->tex[0] = 0;
				s->tex[1] = 0;
				s->scale = 1.;
				s->onBeginTexture = shaderOnBeginTexture; // Tell the shader that the texture is disabled.
				s->onBeginTextureData = NULL;
				s->onInitedTexture = NULL;
				s->onInitedTextureData = NULL;
				s->onEndTexture = shaderOnEndTexture; // Tell The shader to restore texture usage.
				s->onEndTextureData = NULL;
				continue;
			}
#else
			HANDLE hbm;
			BITMAP bm;
			BITMAPINFOHEADER dbmi;
			FILE *fp;
			LPVOID pbuf;
			LONG lines, liens;
			HDC hdc;
			int err;

			/* allocate cache list space */
			TexCacheBind tcb(name);
//			gstc = realloc(gstc, ++nstc * sizeof *gstc);
//			gstc[nstc-1].name = malloc(strlen(name)+1);
//			strcpy(gstc[nstc-1].name, name);

			fp = fopen(suf->a[i].colormap, "rb");
			if(!fp){
				struct suftexlist *s = &ret->a[k];
				s->list = 0;
				s->tex[0] = s->tex[1] = 0;
				s->scale = 1.;
				s->onBeginTexture = NULL;
				s->onBeginTextureData = NULL;
				s->onInitedTexture = NULL;
				s->onInitedTextureData = NULL;
				s->onEndTexture = NULL;
				s->onEndTextureData = NULL;
				continue;
			}
/*			glNewList(ret->a[k].list = gstc[nstc-1].list = glGenLists(1), GL_COMPILE);*/
			hbm = LoadImage(0, suf->a[i].colormap, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE/* | LR_CREATEDIBSECTION*/);
			GetObject(hbm, sizeof bm, &bm);
			pbuf = malloc(bm.bmWidth * bm.bmHeight * 4);
			lines = bm.bmHeight;
			dbmi.biSize = sizeof dbmi;
			dbmi.biWidth = bm.bmWidth;
			dbmi.biHeight = bm.bmHeight;
			dbmi.biPlanes = 1;
			dbmi.biBitCount = 24;
			dbmi.biCompression = BI_RGB;
			dbmi.biSizeImage = 0;
			dbmi.biClrUsed = dbmi.biClrImportant = 0;
			hdc = GetDC(GetDesktopWindow());
			if(lines != (liens = GetDIBits(hdc, (HBITMAP)hbm, 0, bm.bmHeight, pbuf, (LPBITMAPINFO)&dbmi, DIB_RGB_COLORS))){
/*				glEndList();
				glDeleteLists(ret->a[k].list, 1);*/
				ret->a[k].list = 0;
			}
			ReleaseDC(GetDesktopWindow(), hdc);
			if(lines == liens){
				GLint align;
				GLboolean swapbyte;
				glGenTextures(1, ret->a[k].tex);
				tcb.setTex(0, ret->a[k].tex[0]);
				tcb.setTex(1, ret->a[k].tex[1] = 0);
				glBindTexture(GL_TEXTURE_2D, ret->a[k].tex[0]);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &align);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 2);
				glGetBooleanv(GL_UNPACK_SWAP_BYTES, &swapbyte);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_TRUE);
				err = gluBuild2DMipmaps(GL_TEXTURE_2D, 3, dbmi.biWidth, dbmi.biHeight, GL_RGB, GL_UNSIGNED_BYTE, pbuf);
				glPixelStorei(GL_UNPACK_ALIGNMENT, align);
				glPixelStorei(GL_UNPACK_SWAP_BYTES, swapbyte);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
				glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				tcb.setList(glGenLists(1));
				glNewList(ret->a[k].list = tcb.getList(), GL_COMPILE);
				glBindTexture(GL_TEXTURE_2D, ret->a[k].tex[0]);
				glEnable(GL_TEXTURE_2D);
				if(glActiveTextureARB){
					glActiveTextureARB(GL_TEXTURE1_ARB);
					glDisable(GL_TEXTURE_2D);
					glActiveTextureARB(GL_TEXTURE0_ARB);
				}
				glEndList();
			}
			fclose(fp);
			DeleteObject(hbm);
			free(pbuf);
			if(k < nscales)
				ret->a[k].scale = scales[k];
#endif
		}
	}
	return ret;
}


class SQMaterial{
	HSQUIRRELVM v;
public:
	SQMaterial();
	~SQMaterial();
protected:
	static void interpretTexParam(HSQUIRRELVM v, SQInteger idx, TexParam &stp);
	static SQInteger sqf_addMaterial(HSQUIRRELVM v);
};

SQMaterial::SQMaterial(){
	v = sq_open(1024);

	sqstd_seterrorhandlers(v);

//	sq_setprintfunc(v, sqa::sqf_print); //sets the print function
	sqa::register_global_func(v, sqf_addMaterial, "addMaterial");

    sq_pushroottable(v);

	timemeas_t tm;
	TimeMeasStart(&tm);
	if(SQ_SUCCEEDED(sqstd_dofile(v, _SC("scripts/materials.nut"), 0, 1))) // also prints syntax errors if any 
	{
		double d = TimeMeasLap(&tm);
//		CmdPrint(cpplib::dstring() << "materials.nut total: " << d << " sec");
	}
//	else
//		CmdPrint("scripts/init.nut failed.");

	sq_poptop(v);
	sq_close(v);
}

SQMaterial::~SQMaterial(){
}

void SQMaterial::interpretTexParam(HSQUIRRELVM v, SQInteger idx, TexParam &stp){
	const SQChar *env;
	sq_pushstring(v, _SC("env"), -1);
	if(SQ_SUCCEEDED(sq_get(v, idx)) && SQ_SUCCEEDED(sq_getstring(v, -1, &env))){
		if(!strcmp(env, "GL_MODULATE")){
			stp.flags |= STP_ENV;
			stp.env = GL_MODULATE;
		}
		else if(!strcmp(env, "GL_ADD")){
			stp.flags |= STP_ENV;
			stp.env = GL_ADD;
		}
		sq_poptop(v);
	}

	sq_pushstring(v, _SC("magfil"), -1);
	if(SQ_SUCCEEDED(sq_get(v, idx)) && SQ_SUCCEEDED(sq_getstring(v, -1, &env))){
		if(!strcmp(env, "GL_NEAREST")){
			stp.flags |= STP_MAGFIL;
			stp.magfil = GL_NEAREST;
		}
		else if(!strcmp(env, "GL_LINEAR")){
			stp.flags |= STP_MAGFIL;
			stp.magfil = GL_LINEAR;
		}
		sq_poptop(v);
	}

	sq_pushstring(v, _SC("minfil"), -1);
	if(SQ_SUCCEEDED(sq_get(v, idx)) && SQ_SUCCEEDED(sq_getstring(v, -1, &env))){
		if(!strcmp(env, "GL_NEAREST")){
			stp.flags |= STP_MINFIL;
			stp.minfil = GL_NEAREST;
		}
		else if(!strcmp(env, "GL_LINEAR")){
			stp.flags |= STP_MINFIL;
			stp.minfil = GL_LINEAR;
		}
		sq_poptop(v);
	}

	SQBool b;
	sq_pushstring(v, _SC("alpha"), -1);
	if(SQ_SUCCEEDED(sq_get(v, idx)) && SQ_SUCCEEDED(sq_getbool(v, -1, &b))){
		if(b)
			stp.flags |= STP_ALPHA;
		else
			stp.flags &= ~STP_ALPHA;
		sq_poptop(v);
	}

	sq_pushstring(v, _SC("alphaTest"), -1);
	if(SQ_SUCCEEDED(sq_get(v, idx)) && SQ_SUCCEEDED(sq_getbool(v, -1, &b))){
		if(b)
			stp.flags |= STP_ALPHA_TEST;
		else
			stp.flags &= ~STP_ALPHA_TEST;
		sq_poptop(v);
	}

	SQInteger i;
	sq_pushstring(v, _SC("transparentColor"), -1);
	if(SQ_SUCCEEDED(sq_get(v, idx)) && SQ_SUCCEEDED(sq_getinteger(v, -1, &i))){
		stp.flags |= STP_TRANSPARENTCOLOR;
		stp.transparentColor = i;
		sq_poptop(v);
	}
}

SQInteger SQMaterial::sqf_addMaterial(HSQUIRRELVM v){
	try{
		const SQChar *name;
		if(SQ_FAILED(sq_getstring(v, 2, &name)))
			return SQ_ERROR;

		const SQChar *texname;
		if(SQ_FAILED(sq_getstring(v, 3, &texname)))
			return SQ_ERROR;

		TexParam stp;
		stp.flags = 0;
		interpretTexParam(v, 4, stp);

		const SQChar *texname2;
		if(SQ_FAILED(sq_getstring(v, 5, &texname2)))
			texname2 = NULL;

		TexParam stp2;
		stp2.flags = 0;
		interpretTexParam(v, 6, stp2);

		AddMaterial(name, texname, &stp, texname2, &stp2);
		return 1;
	}
	catch(SQIntrinsicError){
		return SQ_ERROR;
	}
}


static SQMaterial sqm;

}
