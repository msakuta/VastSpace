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
#include "material.h"
#include "bitmap.h"
//extern "C"{
#include <clib/c.h>
#include <clib/zip/UnZip.h>
#include <clib/suf/sufdraw.h>
#include <clib/dstr.h>
#include <clib/gl/multitex.h>
#include <jpeglib.h>
#include <jerror.h>
#include <png.h>
//}
#include <setjmp.h>
#include <gl/glu.h>



/// Material object.
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
	src->source_data = buffer;
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
		image_buffer = ZipUnZip("rc.zip", fname, &size);
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
		*freeproc = free;
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
	int row_stride;		/* physical row width in image buffer */
	int src_row_stride;
	struct ReadPNGData rpd;
	png_structp png_ptr;
	png_infop info_ptr = NULL, end_info = NULL;
	png_bytepp ret;

	infile = fopen(fname, "rb");

	if(!infile){
		rpd.image_buffer = ZipUnZip("rc.zip", fname, &rpd.size);
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
		int width, height, bit_depth, color_type, interlace_type, comps;
		int i;

		/* The native order of RGB components differs in order against Windows bitmaps,
		 * so we must instruct libpng to convert it. */
		png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, NULL);

		png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
			&interlace_type, NULL, NULL);

		/* Grayscale images are not supported.
		 * TODO: alpha channel? */
		if(bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGB && color_type != PNG_COLOR_TYPE_RGBA)
			goto shortjmp;

		/* Calculate number of components. */
		comps = (color_type == PNG_COLOR_TYPE_RGB ? 3 : 4);

		/* png_get_rows returns array of pointers to rows allocated by the library,
		 * which must be copied to single bitmap buffer. */
		ret = png_get_rows(png_ptr, info_ptr);

		bmi = malloc(sizeof(BITMAPINFOHEADER) + width * height * 4);
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biWidth = width;
		bmi->bmiHeader.biHeight = height;
		bmi->bmiHeader.biPlanes = 1;
		bmi->bmiHeader.biBitCount = bit_depth * comps;
		bmi->bmiHeader.biCompression = BI_RGB;
		bmi->bmiHeader.biSizeImage = 0;
		bmi->bmiHeader.biXPelsPerMeter = 0;
		bmi->bmiHeader.biYPelsPerMeter = 0;
		bmi->bmiHeader.biClrUsed = 0;
		bmi->bmiHeader.biClrImportant = 0;
		for(i = 0; i < height; i++)
			memcpy(&((unsigned char*)&bmi->bmiColors[0])[4 * i * width], ret[i], width * comps);

		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		if(infile)
			fclose(infile);
		else
			ZipFree(rpd.image_buffer);
		if(freeproc)
			*freeproc = free;
		return bmi;
	}
}

GLuint CallCacheBitmap5(const char *entry, const char *fname1, suftexparam_t *pstp1, const char *fname2, suftexparam_t *pstp2){
	const struct suftexcache *stc;
	suftexparam_t stp, stp2;
	BITMAPFILEHEADER *bfh = NULL, *bfhMask = NULL;
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
		/* If no luck yet, try jpeg decoding. */
		stp.bmi = ReadJpeg(fname1, NULL);
		jpeg = 1;
	}

	if(!stp.bmi){
		/* If still not, try PNG. */
		stp.bmi = ReadPNG(fname1, NULL);
		jpeg = 2;
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
			stp.bmiMask = ReadJpeg(dstr(&ds), NULL);
			dstrfree(&ds);
			mask = maskjpeg = !!stp.bmiMask;
		}
	}

	stp2 = pstp2 ? *pstp2 : defstp;
	if(fname2){
		/* Try loading from plain bitmap first */
		stp2.bmi = ReadBitmap(fname2);
		if(!stp2.bmi){
			/* If no luck yet, try jpeg decoding. */
			stp2.bmi = ReadJpeg(fname2, NULL);
			jpeg2 = 1;
		}
		if(!stp2.bmi)
			return 0;
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
		if(jpeg2)
			free(stp2.bmi);
		else if(stp2.bmi)
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
