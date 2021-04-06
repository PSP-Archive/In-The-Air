#ifndef _GFX_H_
#define _GFX_H_

void Plot(int x, int y, u32 color);
void Plot2(int x, int y, u32 color);
void FillRect(int x, int y, int width, int height, u32 color);
void FillRect2(int x, int y, int width, int height, u32 color);
void DrawImage(Image* image, int x, int y, int xImage, int yImage, int width, int height, u32 abgr);
void DrawImage2(Image* image, int x, int y, int xImage, int yImage, int width, int height, u32 abgr);
void DrawTextEn(int x, int y, char* str);
void DrawTextEn2(int x, int y, char* str);

void ScreenShot(const char* filename);

Image* LoadImage(const char* filename);
void FreeImage(Image* image);

int Sys_Milliseconds(void);
void itoa(int value, char* str, int radix);
void ltoa(long value, char* str, int radix);

#endif
