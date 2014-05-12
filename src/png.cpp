/** \file png.cpp
 * \brief Adaptation source for libpng
 */
#include "bitmap.h"
extern "C"{
#include <clib/zip/UnZip.h>
}
#include <png.h>
#include <stdlib.h>

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

