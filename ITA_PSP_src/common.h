#ifndef _COMMON_H_
#define _COMMON_H_

#define SCREEN_WIDTH 			480
#define SCREEN_HEIGHT 			272
#define PIXEL_SIZE				4

#define	FRAME_BUFFER_WIDTH		512
#define FRAME_BUFFER_SIZE		FRAME_BUFFER_WIDTH*SCREEN_HEIGHT*PIXEL_SIZE

#define ARGB(a, r, g, b)		(a<<24|b<<16|g<<8|r)

#define PAINTSRC 0xFF000000

#define printf pspDebugScreenPrintf

#define bool	int
#define true	1
#define false	0
#define PI		3.1415926f

typedef struct
{
	int mWidth;
	int mHeight;
	u32* mBits;
} Image;

#endif

