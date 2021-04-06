#include <mikmod.h>

// Mikmod specific variables
extern int _mm_errno;
extern BOOL _mm_critical;
extern char *_mm_errmsg[];
extern UWORD md_mode;
extern UBYTE md_reverb;
extern UBYTE md_pansep;


void my_error_handler(void)
{
	printf("_mm_critical %d\n", _mm_critical);
	printf("_mm_errno %d\n", _mm_errno);
	printf("%s\n", _mm_errmsg[_mm_errno]);
	return;
}


void InitMikMod()
{
	_mm_RegisterErrorHandler(my_error_handler);
	MikMod_RegisterAllLoaders();
	MikMod_RegisterAllDrivers();
	md_mode = DMODE_16BITS|DMODE_STEREO|DMODE_SOFT_SNDFX|DMODE_SOFT_MUSIC; 
	md_reverb = 0;
	md_pansep = 128;
	MikMod_Init();
	MikMod_SetNumVoices(-1, 8);
}

