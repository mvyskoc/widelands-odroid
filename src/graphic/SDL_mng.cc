/*
    SDL_mng:  A library to load MNG files
    Copyright (C) 2003, Thomas Kircher

    PNG code based on SDL_png.c, Copyright (C) 1998 Philippe Lavoie

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include "SDL_mng.h"

#include <SDL.h>
#include <SDL_endian.h>

#include <png.h>

/* Chunk structure */
struct chunk_t {
	unsigned int  chunk_ID;
	unsigned int  chunk_size;
	char         * chunk_data;
	unsigned int  chunk_CRC;
};

struct MNG_Frame {
	SDL_Surface * frame;
	MNG_Frame   * next;
};

/* Some select chunk IDs, from libmng.h */
#define MNG_UINT_MHDR 0x4d484452L
#define MNG_UINT_BACK 0x4241434bL
#define MNG_UINT_PLTE 0x504c5445L
#define MNG_UINT_tRNS 0x74524e53L
#define MNG_UINT_IHDR 0x49484452L
#define MNG_UINT_IDAT 0x49444154L
#define MNG_UINT_IEND 0x49454e44L
#define MNG_UINT_MEND 0x4d454e44L
#define MNG_UINT_FRAM 0x4652414dL
#define MNG_UINT_LOOP 0x4c4f4f50L
#define MNG_UINT_ENDL 0x454e444cL
#define MNG_UINT_TERM 0x5445524dL

/* Internal MNG functions */
unsigned char MNG_read_byte     (SDL_RWops *);
unsigned int  MNG_read_uint32   (SDL_RWops *);
chunk_t       MNG_read_chunk    (SDL_RWops *);
MNG_Image   * MNG_iterate_chunks(SDL_RWops *);

/* Read one MNG frame and return it as an SDL_Surface */
SDL_Surface * MNG_read_frame(SDL_RWops * src);

/* Return whether or not the file claims to be an MNG */
int IMG_isMNG(SDL_RWops * const src)
{
	unsigned char buf[8];

	if (SDL_RWread(src, buf, 1, 8) != 8)
		return 0;

	return !memcmp(buf, "\212MNG\r\n\032\n", 8);
}

MNG_Image * IMG_loadMNG(char * const file)
{
	SDL_RWops * const src = SDL_RWFromFile(file, "rb");

	if (not src)
		return 0;

	/* See whether or not this data source can handle seeking */
	if (SDL_RWseek(src, 0, SEEK_CUR) < 0) {
		SDL_SetError("Can not seek in this data source");
		SDL_RWclose(src);
		return 0;
	}

	/* Verify MNG signature */
	if (!IMG_isMNG(src)) {
		SDL_SetError("File is not an MNG file");
		SDL_RWclose(src);
		return 0;
	}

	return (MNG_iterate_chunks(src));
}

/* Read a byte from the src stream */
unsigned char MNG_read_byte(SDL_RWops * const src)
{
	unsigned char ch;

	SDL_RWread(src, &ch, 1, 1);

	return ch;
}

/* Read a 4-byte uint32 from the src stream */
unsigned int MNG_read_uint32(SDL_RWops * const src)
{
	unsigned char buffer[4];
	unsigned int value;

	buffer[0] = MNG_read_byte(src); buffer[1] = MNG_read_byte(src);
	buffer[2] = MNG_read_byte(src); buffer[3] = MNG_read_byte(src);

	value  = buffer[0] << 24; value |= buffer[1] << 16;
	value |= buffer[2] << 8;  value |= buffer[3];

	return value;
}

/* Read an MNG chunk */
chunk_t MNG_read_chunk(SDL_RWops * const src)
{
	chunk_t this_chunk;

	this_chunk.chunk_size = MNG_read_uint32(src);
	this_chunk.chunk_ID   = MNG_read_uint32(src);

	this_chunk.chunk_data =
		static_cast<char *>(calloc(sizeof(char), this_chunk.chunk_size));

	SDL_RWread(src, this_chunk.chunk_data, 1, this_chunk.chunk_size);

	this_chunk.chunk_CRC = MNG_read_uint32(src);

	return this_chunk;
}

/* Read MHDR chunk data */
MHDR_chunk read_MHDR(SDL_RWops * const src)
{
	MHDR_chunk mng_header;

	mng_header.Frame_width         = MNG_read_uint32(src);
	mng_header.Frame_height        = MNG_read_uint32(src);
	mng_header.Ticks_per_second    = MNG_read_uint32(src);
	mng_header.Nominal_layer_count = MNG_read_uint32(src);
	mng_header.Nominal_frame_count = MNG_read_uint32(src);
	mng_header.Nominal_play_time   = MNG_read_uint32(src);
	mng_header.Simplicity_profile  = MNG_read_uint32(src);

	/* skip CRC bits */
	MNG_read_uint32(src);

	return mng_header;
}

/* Iterate through the MNG chunks */
MNG_Image * MNG_iterate_chunks(SDL_RWops * const src)
{
	chunk_t current_chunk;

	uint32_t byte_count  = 0;
	uint32_t frame_count = 0;

	MNG_Frame * start_frame   = 0;
	MNG_Frame * current_frame = 0;

	MNG_Image * const image =
		static_cast<MNG_Image *>(malloc(sizeof(MNG_Image)));

	do {
		current_chunk = MNG_read_chunk(src);
		byte_count   += current_chunk.chunk_size + 12;

		switch (current_chunk.chunk_ID) {
			/* Read MHDR chunk, and store in image struct */
			case MNG_UINT_MHDR:
				SDL_RWseek(src, -(current_chunk.chunk_size + 4), SEEK_CUR);
				image->mhdr = read_MHDR(src);
				break;

			/* Reset our byte count */
			case MNG_UINT_IHDR:
				byte_count = current_chunk.chunk_size + 12;
				break;

			/* We've reached the end of a PNG - seek to IHDR and read */
			case MNG_UINT_IEND:
				SDL_RWseek(src, -byte_count, SEEK_CUR);

				/* We don't know how many frames there will be, really. */
				if (not start_frame) {
					current_frame =
						static_cast<MNG_Frame *>(malloc(sizeof(MNG_Frame)));
					start_frame   = current_frame;
				} else {
					current_frame->next =
						static_cast<MNG_Frame *>(malloc(sizeof(MNG_Frame)));
					current_frame = current_frame->next;
				}

				current_frame->frame = MNG_read_frame(src);
				++frame_count;

				break;

			default:
				break;
		}
	} while (current_chunk.chunk_ID != MNG_UINT_MEND);

	/* Now that we're done, move the frames into our image struct */
	image->frame_count = frame_count;
	image->frame =
		static_cast<SDL_Surface * *>(calloc(sizeof(SDL_Surface *), frame_count));

	current_frame = start_frame;

	for (uint32_t i = 0; i < frame_count; ++i) {
		image->frame[i] = current_frame->frame;
		current_frame   = current_frame->next;
	}
	return image;
}

/* png_read_data callback; return <size> bytes from wherever */
static void png_read_data(png_structp ctx, png_bytep area, png_size_t size)
{
	SDL_RWread(static_cast<SDL_RWops *>(png_get_io_ptr(ctx)), area, size, 1);
}

/* Read a PNG frame from the MNG file */
SDL_Surface * MNG_read_frame(SDL_RWops * const src)
{
	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	Uint32 Rmask;
	Uint32 Gmask;
	Uint32 Bmask;
	Uint32 Amask;
	SDL_Palette * palette;
	volatile int ckey = -1;
	png_color_16 * transv;
	png_colorp png_palette = NULL;
    int num_palette = 0;

	/* Initialize the data we will clean up when we're done */
	png_structp png_ptr = 0;
	png_infop info_ptr = 0;
	png_bytep * volatile row_pointers = 0;
	SDL_Surface * volatile surface = 0;

	/* Check to make sure we have something to do */
	if (! src)
		goto done;

	/* Create the PNG loading context structure */
	png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (not png_ptr) {
		SDL_SetError("Could not allocate memory for PNG file");
		goto done;
	}

	//  Allocate/initialize the memory for image information.  REQUIRED.
	info_ptr = png_create_info_struct(png_ptr);
	if (not info_ptr) {
		SDL_SetError("Could not create image information for PNG file");
		goto done;
	}

	/* Set error handling if you are using setjmp/longjmp method (this is
	 * the normal method of doing things with libpng).  REQUIRED unless you
	 * set up your own error handlers in png_create_read_struct() earlier.
	 */
	if (setjmp(png_jmpbuf(png_ptr))) {
		SDL_SetError("Error reading the PNG file.");
		goto done;
	}

	/* Set up the input control */
	png_set_read_fn(png_ptr, src, png_read_data);

	/* tell PNG not to read the signature */
	png_set_sig_bytes(png_ptr, 8);

	/* Read PNG header info */
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR
		(png_ptr, info_ptr,
		 &width, &height,
		 &bit_depth,
		 &color_type, &interlace_type,
		 0, 0);

	/* tell libpng to strip 16 bit/color files down to 8 bits/color */
	png_set_strip_16(png_ptr);

	/* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
	 * byte into separate bytes (useful for paletted and grayscale images).
	 */
	png_set_packing(png_ptr);

	/* scale greyscale values to the range 0..255 */
	if (color_type == PNG_COLOR_TYPE_GRAY)
		png_set_expand(png_ptr);

	/* For images with a single "transparent colour", set colour key;
	   if more than one index has transparency, or if partially transparent
	   entries exist, use full alpha channel */
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
		int num_trans;
		Uint8 * trans;
		png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &transv);
		if (color_type == PNG_COLOR_TYPE_PALETTE) {
			/* Check if all tRNS entries are opaque except one */
			uint32_t i = 0;
			int32_t  t = -1;
			for (; i < static_cast<uint32_t>(num_trans); ++i)
				if (trans[i] == 0) {
					if (t >= 0)
						break;
					t = i;
				} else if (trans[i] != 255)
					break;
			if (i == static_cast<uint32_t>(num_trans)) {
				/* exactly one transparent index */
				ckey = t;
			} else {
				/* more than one transparent index, or translucency */
				png_set_expand(png_ptr);
			}
		} else
			ckey = 0; //  actual value will be set later
	}

	if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	png_get_IHDR
		(png_ptr, info_ptr,
		 &width, &height,
		 &bit_depth,
		 &color_type, &interlace_type,
		 0, 0);

	/* Allocate the SDL surface to hold the image */
	Rmask = Gmask = Bmask = Amask = 0;
	if (color_type != PNG_COLOR_TYPE_PALETTE) {
		if (SDL_BYTEORDER == SDL_LIL_ENDIAN) {
			Rmask = 0x000000FF;
			Gmask = 0x0000FF00;
			Bmask = 0x00FF0000;
			Amask = (png_get_channels(png_ptr, info_ptr) == 4) ? 0xFF000000 : 0;
		} else {
			int const s = (png_get_channels(png_ptr, info_ptr) == 4) ? 0 : 8;
			Rmask = 0xFF000000 >> s;
			Gmask = 0x00FF0000 >> s;
			Bmask = 0x0000FF00 >> s;
			Amask = 0x000000FF >> s;
		}
	}
	surface =
		SDL_AllocSurface
			(SDL_SWSURFACE,
			 width, height,
			 bit_depth * png_get_channels(png_ptr, info_ptr),
			 Rmask, Gmask, Bmask, Amask);
	if (not surface) {
		SDL_SetError("Out of memory");
		goto done;
	}

	if (ckey != -1) {
		if (color_type != PNG_COLOR_TYPE_PALETTE)
			/* FIXME: Should these be truncated or shifted down? */
			ckey =
				SDL_MapRGB
					(surface->format,
					 static_cast<Uint8>(transv->red),
					 static_cast<Uint8>(transv->green),
					 static_cast<Uint8>(transv->blue));
		SDL_SetColorKey(surface, SDL_SRCCOLORKEY, ckey);
	}

	/* Create the array of pointers to image data */
	row_pointers = static_cast<png_bytep *>(malloc(sizeof(png_bytep) * height));
	if (not row_pointers) {
		SDL_SetError("Out of memory");
		SDL_FreeSurface(surface);
		surface = 0;
		goto done;
	}
	for (uint32_t row = 0; row < height; ++row)
		row_pointers[row] =
			static_cast<png_bytep>(static_cast<Uint8 *>(surface->pixels)) +
			row * surface->pitch;

	/* Read the entire image in one go */
	png_read_image(png_ptr, row_pointers);

	/* read rest of file, get additional chunks in info_ptr - REQUIRED */
	png_read_end(png_ptr, info_ptr);

	/* Load the palette, if any */
	if ((palette = surface->format->palette)) {
		if (color_type == PNG_COLOR_TYPE_GRAY) {
			palette->ncolors = 256;
			for (uint16_t i = 0; i < 256; ++i) {
				palette->colors[i].r = i;
				palette->colors[i].g = i;
				palette->colors[i].b = i;
			}
		} else if ((png_get_PLTE(png_ptr, info_ptr, &png_palette, &num_palette) &
                  PNG_INFO_PLTE) && num_palette > 0 && png_palette != NULL) {
			palette->ncolors = num_palette;
			for (uint32_t i = 0; i < num_palette; ++i) {
				palette->colors[i].b = png_palette[i].blue;
				palette->colors[i].g = png_palette[i].green;
				palette->colors[i].r = png_palette[i].red;
			}
		}
	}

done:    /* Clean up and return */
	png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : 0, 0);
	free(row_pointers);
	return (surface);
}
