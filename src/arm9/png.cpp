#include "png.h"

#include <stdlib.h>

typedef struct {
	const unsigned char *data;
	png_size_t size;
	png_size_t seek;
} PngData;

static void ReadPngData(png_structp png_ptr, png_bytep data, png_size_t length) {
	PngData* pngData = (PngData*) png_get_io_ptr(png_ptr);

	if (pngData) {
		for (png_size_t i = 0; i < length; i++) {
			if (pngData->seek >= pngData->size) break;
			data[i] = pngData->data[pngData->seek++];
		}
	}
}

bool PNGGetBounds(char* data, int dataL, int* w, int* h) {
    NDSRECT destrect;
    if (PNGGetBounds(data, dataL, &destrect)) {
        *w = destrect.w;
        *h = destrect.h;
        return true;
    }
    return false;
}
bool PNGGetBounds(char* data, int dataL, NDSRECT* destrect) {
	png_structp png_ptr;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
        return false;
    }

	PngData pngData;
	pngData.data = (const unsigned char*)data;
	pngData.seek = 0;
	pngData.size = dataL;
	png_set_read_fn(png_ptr, (void *) &pngData, ReadPngData);

	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;

	png_infop info_ptr;
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return false;
	}

	png_set_sig_bytes(png_ptr, sig_read);

    png_read_info(png_ptr, info_ptr); //<--- THIS LINE IS THE PROBLEM!!!

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

    destrect->w = width;
    destrect->h = height;

    png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
    return true;
}

bool PNGLoadImage(char* data, int dataL, u16* out, u8* alphaOut, int w, int h) {
    NDSRECT destrect;
    destrect.x = destrect.y = 0;
    destrect.w = w;
    destrect.h = h;

    return PNGLoadImage(data, dataL, out, alphaOut, destrect);
}

bool PNGLoadImage(char* data, int dataL, u16* out, u8* alphaOut, NDSRECT destrect) {
	png_structp png_ptr;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (png_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
        return false;
    }

	PngData pngData;
	pngData.data = (const unsigned char*)data;
	pngData.seek = 0;
	pngData.size = dataL;
	png_set_read_fn(png_ptr, (void *) &pngData, ReadPngData);

	unsigned int sig_read = 0;
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_infop info_ptr;
	info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
		return false;
	}

    if (setjmp(png_jmpbuf(png_ptr))) { // IO init error
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        return false;
    }

	png_set_sig_bytes(png_ptr, sig_read);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);

	png_set_strip_16(png_ptr);
	png_set_packing(png_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
	png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);

//------------------------------------------------------------------------------
    // allocate row buffer
    png_byte* row = new png_byte[4 * width];
	if (!row) {
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		return false;
	}

    int s = 0;
    for(int y = 0; y < destrect.y + destrect.h; y++) {
		png_read_row(png_ptr, row, png_bytep_NULL);
        if (y < destrect.y) {
            continue;
        }

        int t = destrect.x * 4;
        for (int x = destrect.x; x < destrect.w; x++) {
            int r = row[t++] >> 3;
            int g = row[t++] >> 3;
            int b = row[t++] >> 3;

            if (alphaOut != NULL) {
                alphaOut[s] = row[t++];
            } else {
                t++;
            }

            out[s] = RGB15(r, g, b) | BIT(15);
            s++;
        }
    }

	delete[] row;
//------------------------------------------------------------------------------

	png_read_end(png_ptr, info_ptr);
	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
	return true;
}

bool PNGSaveImage(u16* data, int width, int height) {
    char filename[32];
    time_t ti = time(NULL);
    tm* tm = localtime(&ti);
    unsigned int timestamp = tm->tm_sec + 60*(tm->tm_min + 60*(tm->tm_hour + 24*(tm->tm_yday + 366 * tm->tm_year)));
    sprintf(filename, "%u.png", timestamp);
    FILE *file = fopen(filename, "wb");
    if (!file) {
       return false;
    }    
    
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
        (png_voidp)NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(file);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(file);
        return false;
    }
    
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(file);
        return false;
    }
    
    png_init_io(png_ptr, file);
    
    png_set_IHDR(png_ptr, info_ptr, width, height,
       8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
       PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
       
	png_write_info(png_ptr, info_ptr);
	
	int t = 0;
	png_byte* line = new png_byte[width * 3];
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width * 3; ) {
			line[x++] = ((data[t]    ) & 31) << 3;
			line[x++] = ((data[t]>> 5) & 31) << 3;
			line[x++] = ((data[t]>>10) & 31) << 3;
			t++;
		}
		png_write_row(png_ptr, line);
	}
	delete[] line;
	
	png_write_end(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
	return true;
}
