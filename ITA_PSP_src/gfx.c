#include <stdio.h>
#include <stdlib.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>

#include <png.h>

#include "common.h"

extern u32* pVRAM;
extern Image* text;


//------------------------------------------------------------------------------------------------
// taken from:
// .../psp/trunk/pspgl/test-q3/generic/main.c
// returns number of milliseconds since game started
int Sys_Milliseconds(void)
{
	static long sys_timeBase = 0;
	struct timeval tp;
	gettimeofday(&tp, NULL);
	if (!sys_timeBase) {
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec / 1000;
	}
	return (tp.tv_sec - sys_timeBase) * 1000 + tp.tv_usec / 1000;
}


//------------------------------------------------------------------------------------------------
// integer to ASCII, simple convertion
void itoa(int value, char* str, int radix)
{
	char *psLetter="0123456789";
	int i = 0, j;
	char* temp = (char*)malloc(sizeof(str));
	while (value%radix != 0 || value>=0)
	{
		if (value == 0)
            break;
		i++;
		temp[i-1] = psLetter[value%radix];
		value = value / radix;
	}
	for (j = 0; j < i; j++)
	{
		str[j] = temp[i-j-1];
	}
}


//------------------------------------------------------------------------------------------------
// long to ASCII, simple convertion
void ltoa(long value, char* str, int radix)
{
	char *psLetter="0123456789";
	int i = 0, j;
	char* temp = (char*)malloc(sizeof(str));
	while (value%radix != 0 || value >= 0)
	{
        if (value == 0)
            break;
		i++;
		temp[i-1] = psLetter[value%radix];
		value = value / radix;
	}
	for (j = 0; j < i; j++)
	{
		str[j] = temp[i-j-1];
	}
}


//------------------------------------------------------------------------------------------------
// Plot a single pixel on screen
//------------------------------------------------------------------------------------------------
void Plot(int x, int y, u32 color)
{
	// get starting addr of the pixel
	u32* p = pVRAM + y * FRAME_BUFFER_WIDTH + x;
	*p = color;
}


//------------------------------------------------------------------------------------------------
// Plot a single pixel on screen rotated 90 degree
//------------------------------------------------------------------------------------------------
void Plot2(int x, int y, u32 color)
{
	// get starting addr of the pixel
	u32* p = pVRAM + x * FRAME_BUFFER_WIDTH + SCREEN_WIDTH - y;
	*p = color;
}


//------------------------------------------------------------------------------------------------
// Fill a rectangular area with a specific color on screen without boundary check
//------------------------------------------------------------------------------------------------
void FillRect(int x, int y, int width, int height, u32 color)
{
	// get starting addr of the 1st pixel
	u32* p = pVRAM + y * FRAME_BUFFER_WIDTH + x;
	int i, j;
	for (j=0; j<height; j++)
	{
		for (i=0; i<width; i++)		// plot one row
		{
			*(p+i) = color;
		}
		p += FRAME_BUFFER_WIDTH;	// move pointer to the next row
	}
}


//------------------------------------------------------------------------------------------------
// Fill a rectangular area with a specific color on screen rotated 90 degree without boundary check
//------------------------------------------------------------------------------------------------
void FillRect2(int x, int y, int width, int height, u32 color)
{
	// get starting addr of the 1st pixel
	u32* p = pVRAM + x * FRAME_BUFFER_WIDTH + SCREEN_WIDTH - y - height;
	int i, j;
	for (j=0; j<width; j++)
	{
		for (i=0; i<height; i++)	// plot one row
		{
			*(p+i) = color;
		}
		p += FRAME_BUFFER_WIDTH;	// move pointer to the next row
	}
}


//------------------------------------------------------------------------------------------------
// Draw part of an image to the screen with simple boundary check
//------------------------------------------------------------------------------------------------
void DrawImage(Image* image, int x, int y, int xImage, int yImage, int width, int height, u32 abgr)
{
	// do some simple boundary check
	if (x>SCREEN_WIDTH || y>SCREEN_HEIGHT)
		return;
	if (x < 0)
	{
		xImage -= x;
		width += x;
		x = 0;
	}
	if (y < 0)
	{
		yImage -= y;
		height += y;
		y = 0;
	}
	if (width<=0 || height<=0)
		return;
	if (x+width > SCREEN_WIDTH)
		width = SCREEN_WIDTH - x;
	if (y+height > SCREEN_HEIGHT)
		height = SCREEN_HEIGHT - y;

	// get starting addr of the 1st pixel
	u32* pFrameBuffer = pVRAM + y * FRAME_BUFFER_WIDTH + x;
	u32* pImage = image->mBits + yImage * image->mWidth + xImage;
	
	int i, j;
	u32* pSrc;
	u32* pDest;
	for (i=0; i<height; i++)
	{
		pSrc = pImage;
		pDest = pFrameBuffer;
		for (j=0; j<width; j++)
		{
			if (*pSrc & abgr)
				*pDest = *pSrc;
			pDest++;
			pSrc++;
		}
		pImage += image->mWidth;
		pFrameBuffer += FRAME_BUFFER_WIDTH;
	}
}


//------------------------------------------------------------------------------------------------
// Draw part of an image to the screen rotated 90 degree with simple boundary check
//------------------------------------------------------------------------------------------------
void DrawImage2(Image* image, int x, int y, int xImage, int yImage, int width, int height, u32 abgr)
{
	// do some simple boundary check
	if (x>SCREEN_HEIGHT || y>SCREEN_WIDTH)
		return;
	if (x < 0)
	{
		xImage -= x;
		width += x;
		x = 0;
	}
	if (y < 0)
	{
		yImage -= y;
		height += y;
		y = 0;
	}
	if (width<=0 || height<=0)
		return;
	if (x+width > SCREEN_HEIGHT)
		width = SCREEN_HEIGHT - x;
	if (y+height > SCREEN_WIDTH)
		height = SCREEN_WIDTH - y;

	// get starting addr of the 1st pixel
	u32* pFrameBuffer = pVRAM + x * FRAME_BUFFER_WIDTH + SCREEN_WIDTH - y - height;
	u32* pImage = image->mBits + (yImage + height - 1) * image->mWidth + xImage;

	int i, j;
	u32* pSrc;
	u32* pDest;
	for (i=0; i<width; i++)
	{
		pSrc = pImage;
		pDest = pFrameBuffer;
		for (j=0; j<height; j++)
		{
			if (*pSrc & abgr)
				*pDest = *pSrc;
			pDest++;
			pSrc -= image->mWidth;
		}
		pImage++;
		pFrameBuffer += FRAME_BUFFER_WIDTH;
	}
}


//----------------------------------------------------------------------------------------------------
// Draw some basic characters to the screen without boundary check
//----------------------------------------------------------------------------------------------------
void DrawTextEn(int x, int y, char* str)
{
	int row, col, i = 0;
	int orix = x;
	while (str[i] != '\0')
	{
		col = str[i] - '0';
		if (str[i] == ' ')
			x += 7;
		else if (str[i] == ',')
		{
			row = 0;
			DrawImage(text, x, y, 90, row, 9, 12, PAINTSRC);
			x += 7;
		}
		else if (str[i] == '.')
		{
			row = 0;
			DrawImage(text, x, y, 99, row, 9, 12, PAINTSRC);
			x += 7;
		}
		else if (str[i] == '@')
		{
			row = 0;
			DrawImage(text, x, y, 108, row, 11, 12, PAINTSRC);
			x += 13;
		}
		else if (col >= 0 && col<= 9)
		{
			row = 0;
			DrawImage(text, x, y, col*9, row, 9, 12, PAINTSRC);
			x += 11;
		}
		else
		{
			if (str[i] >= 'A' && str[i]<= 'Z')
			{
				col = str[i] - 'A';
				row = 27;
				if (str[i] == 'W')
				{
					DrawImage(text, x, y, col*11, row, 15, 12, PAINTSRC);
					x += 17;
				}
				else if (str[i] > 'W')
				{
					DrawImage(text, x, y, col*11+4, row, 11, 12, PAINTSRC);
					x += 13;
				}
				else
				{
					DrawImage(text, x, y, col*11, row, 11, 12, PAINTSRC);
					x += 13;
				}
			}
			else
			{
				col = str[i] - 'a';
				row = 12;
				if (str[i]=='g' || str[i]=='p' || str[i]=='q' || str[i]=='y')
				{
					DrawImage(text, x, y, col*11, row, 11, 15, PAINTSRC);
					x += 11;
				}
				else
				{
					DrawImage(text, x, y, col*11, row, 11, 12, PAINTSRC);
					x += 11;
				}
			}
		}
		if (x+17 > 469)	//SCREEN_WIDTH-11=469
		{
			y += 16;
			x = orix;
		}
		i++;
	}
}


//----------------------------------------------------------------------------------------------------
// Draw some basic characters to the screen rotated 90 degree without boundary check
//----------------------------------------------------------------------------------------------------
void DrawTextEn2(int x, int y, char* str)
{
	int row, col, i = 0;
	int orix = x;
	while (str[i] != '\0')
	{
		col = str[i] - '0';
		if (str[i] == ' ')
			x += 7;
		else if (str[i] == ',')
		{
			row = 0;
			DrawImage2(text, x, y, 90, row, 9, 12, PAINTSRC);
			x += 7;
		}
		else if (str[i] == '.')
		{
			row = 0;
			DrawImage2(text, x, y, 99, row, 9, 12, PAINTSRC);
			x += 7;
		}
		else if (str[i] == '@')
		{
			row = 0;
			DrawImage2(text, x, y, 108, row, 11, 12, PAINTSRC);
			x += 13;
		}
		else if (col >= 0 && col<= 9)
		{
			row = 0;
			DrawImage2(text, x, y, col*9, row, 9, 12, PAINTSRC);
			x += 11;
		}
		else 
		{
			if (str[i] >= 'A' && str[i]<= 'Z')
			{
				col = str[i] - 'A';
				row = 27;
				if (str[i] == 'W')
				{
					DrawImage2(text, x, y, col*11, row, 15, 12, PAINTSRC);
					x += 17;
				}
				else if (str[i] > 'W')
				{
					DrawImage2(text, x, y, col*11+4, row, 11, 12, PAINTSRC);
					x += 13;
				}
				else
				{
					DrawImage2(text, x, y, col*11, row, 11, 12, PAINTSRC);
					x += 13;
				}
			}
			else
			{
				col = str[i] - 'a';
				row = 12;
				if (str[i]=='g' || str[i]=='p' || str[i]=='q' || str[i]=='y')
				{
					DrawImage2(text, x, y, col*11, row, 11, 15, PAINTSRC);
					x += 11;
				}
				else
				{
					DrawImage2(text, x, y, col*11, row, 11, 12, PAINTSRC);
					x += 11;
				}
			}
		}
		if (x+17 > 261)	//SCREEN_HEIGHT-11=261
		{
			y += 16;
			x = orix;
		}
		i++;
	}
}


//------------------------------------------------------------------------------------------------
// Taken from:
// http://svn.ps2dev.org/filedetails.php?repname=psp&path=/trunk/libpng/screenshot/main.c&rev=0&sc=0
// Save current visible screen as PNG
//------------------------------------------------------------------------------------------------
void ScreenShot(const char* filename)
{
        u32* vram32;
        u16* vram16;
        void* temp;
        int bufferwidth;
        int pixelformat;
        int unknown;
        int i, x, y;
        png_structp png_ptr;
        png_infop info_ptr;
        FILE* fp;
        u8* line;
        
        fp = fopen(filename, "wb");
        if (!fp) return;
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr) return;
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
                png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
                fclose(fp);
                return;
        }
        png_init_io(png_ptr, fp);
        png_set_IHDR(png_ptr, info_ptr, SCREEN_WIDTH, SCREEN_HEIGHT,
                8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png_ptr, info_ptr);
        line = (u8*) malloc(SCREEN_WIDTH * 3);
        sceDisplayWaitVblankStart();  // if framebuf was set with PSP_DISPLAY_SETBUF_NEXTFRAME, wait until it is changed
        sceDisplayGetFrameBuf(&temp, &bufferwidth, &pixelformat, &unknown);
        vram32 = (u32*) temp;
        vram16 = (u16*) vram32;
        for (y = 0; y < SCREEN_HEIGHT; y++) {
                for (i = 0, x = 0; x < SCREEN_WIDTH; x++) {
                        u32 color = 0;
                        u8 r = 0, g = 0, b = 0;
                        switch (pixelformat) {
                                case PSP_DISPLAY_PIXEL_FORMAT_565:
                                        color = vram16[x + y * bufferwidth];
                                        r = (color & 0x1f) << 3; 
                                        g = ((color >> 5) & 0x3f) << 2 ;
                                        b = ((color >> 11) & 0x1f) << 3 ;
                                        break;
                                case PSP_DISPLAY_PIXEL_FORMAT_5551:
                                        color = vram16[x + y * bufferwidth];
                                        r = (color & 0x1f) << 3; 
                                        g = ((color >> 5) & 0x1f) << 3 ;
                                        b = ((color >> 10) & 0x1f) << 3 ;
                                        break;
                                case PSP_DISPLAY_PIXEL_FORMAT_4444:
                                        color = vram16[x + y * bufferwidth];
                                        r = (color & 0xf) << 4; 
                                        g = ((color >> 4) & 0xf) << 4 ;
                                        b = ((color >> 8) & 0xf) << 4 ;
                                        break;
                                case PSP_DISPLAY_PIXEL_FORMAT_8888:
                                        color = vram32[x + y * bufferwidth];
                                        r = color & 0xff; 
                                        g = (color >> 8) & 0xff;
                                        b = (color >> 16) & 0xff;
                                        break;
                        }
                        line[i++] = r;
                        line[i++] = g;
                        line[i++] = b;
                }
                png_write_row(png_ptr, line);
        }
        free(line);
        png_write_end(png_ptr, info_ptr);
        png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
        fclose(fp);
}


//------------------------------------------------------------------------------------------------
// Based on:
// http://svn.ps2dev.org/filedetails.php?repname=psp&path=/trunk/libpng/screenshot/main.c&rev=0&sc=0
// Load PNG into memory
//------------------------------------------------------------------------------------------------
void user_warning_fn(png_structp png_ptr, png_const_charp warning_msg)
{
        // ignore PNG warnings
}


Image* LoadImage(const char* filename)
{
		u32* p32;
        u16* p16;
        int pixelformat = PSP_DISPLAY_PIXEL_FORMAT_8888;	// we're interested in 32bit image only
        png_structp png_ptr;
        png_infop info_ptr;
        unsigned int sig_read = 0;
        png_uint_32 width, height;
        int bit_depth, color_type, interlace_type, x, y;
        u32* line;
        FILE *fp;

        if ((fp = fopen(filename, "rb")) == NULL) return NULL;
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (png_ptr == NULL) {
                fclose(fp);
                return NULL;
        }
        png_set_error_fn(png_ptr, (png_voidp) NULL, (png_error_ptr) NULL, user_warning_fn);
        info_ptr = png_create_info_struct(png_ptr);
        if (info_ptr == NULL) {
                fclose(fp);
                png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
                return NULL;
        }
        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, sig_read);
        png_read_info(png_ptr, info_ptr);
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, int_p_NULL, int_p_NULL);
        png_set_strip_16(png_ptr);
        png_set_packing(png_ptr);
        if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png_ptr);
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) png_set_gray_1_2_4_to_8(png_ptr);
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png_ptr);
        png_set_filler(png_ptr, 0xff, PNG_FILLER_AFTER);
        line = (u32*) malloc(width * 4);
        if (!line) {
                fclose(fp);
                png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
                return NULL;
        }
        
        Image* image = (Image*) malloc(sizeof(Image));
        if (image)
        {
	        image->mBits = (u32*) malloc(width * height * sizeof(u32));
	        if (image->mBits)
	        {
				image->mWidth = width;
				image->mHeight = height;
				
		        p32 = image->mBits;
		        p16 = (u16*) p32;

		        for (y = 0; y < height; y++) 
		        {
	                png_read_row(png_ptr, (u8*) line, png_bytep_NULL);
	                for (x = 0; x < width; x++) 
	                {
                        u32 color32 = line[x];
                        u16 color16;
						int a = (color32 >> 24) & 0xff;
                        int r = color32 & 0xff;
                        int g = (color32 >> 8) & 0xff;
                        int b = (color32 >> 16) & 0xff;
                        switch (pixelformat) {
                                case PSP_DISPLAY_PIXEL_FORMAT_565:
                                        color16 = (r >> 3) | ((g >> 2) << 5) | ((b >> 3) << 11);
                                        *(p16+x) = color16;
                                        break;
                                case PSP_DISPLAY_PIXEL_FORMAT_5551:
                                        color16 = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
                                        *(p16+x) = color16;
                                        break;
                                case PSP_DISPLAY_PIXEL_FORMAT_4444:
                                        color16 = (r >> 4) | ((g >> 4) << 4) | ((b >> 4) << 8);
                                        *(p16+x) = color16;
                                        break;
                                case PSP_DISPLAY_PIXEL_FORMAT_8888:
                                        color32 = r | (g << 8) | (b << 16) | (a<<24);
                                        *(p32+x) = color32;
                                        break;
                        }
	                }
		            p32 += width;
		            p16 += width;
		        }

		    }
		    else
		    {
		    	free(image);
		    	image = NULL;
		    }
	    }
        free(line);
        png_read_end(png_ptr, info_ptr);
        png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
        fclose(fp);
        
        return image;
}


//------------------------------------------------------------------------------------------------
void FreeImage(Image* image)
{
	if (image->mBits)
		free(image->mBits);

	if (image)
		free(image);
}

