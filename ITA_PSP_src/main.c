#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspdebug.h> 
#include <pspctrl.h>
#include <pspaudiolib.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>
#include <mikmod.h>

#include "common.h"
#include "gfx.h"
#include "mm.h"
#include "main.h"

// app name and version number
//PSP_MODULE_INFO("ITA PSP", 0, 1, 1);
PSP_MODULE_INFO("ITA PSP", 0x1000, 1, 1);

// optional
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
//PSP_MAIN_THREAD_ATTR(0);

int done = 0;
int currentPage;
SceCtrlData pad;
struct timeval tick;
unsigned int oldButtons = 0;

// VRAM actually starts from 0x04000000 but need to OR with 0x40000000 to prevent 
// unpredictable behaviours due to caching
u32* pVRAM = (u32*)(0x04000000+0x40000000);	
u32* pBufferPointer[2];

// image resources
Image* text;
Image* itares;
Image* itamenu;
Image* bg1;
Image* bg2;

// sounds
UNIMOD *music = NULL;	// for Mod
SAMPLE *shoot = NULL;	// for WAV
SAMPLE *explo = NULL;	// for WAV
SAMPLE *menuitem = NULL;// for WAV
int voice;				// handle for WAV

// pointers
struct MyPlane* mp = NULL;
struct sbullet* sbHead = NULL;
struct sbullet* sbEnd = NULL;
struct enemy* eyHead = NULL;
struct enemy* eyEnd = NULL;
struct eybullet* ebHead = NULL;
struct eybullet* ebEnd = NULL;
struct circleshot* csHead = NULL;
struct circleshot* csEnd = NULL;
struct event* eventp = NULL;
struct event* eventEnd = NULL;
struct enemy* lastHitted = NULL;
struct score* top10[10];

float betax = 1.1f;			// suit scene(Windows) to PSP screen size
float betay = 1.35f;		// suit scene(Windows) to PSP screen size

int debugMode = false;		// Debug mode flag
int game_state = GAME_INIT;	// Global game state
int gameMode;				// Difficulty: 1-easy, 2-normal, 3-hard
int stageState;				// Control stage state
long timeCount = 0;			// Global time counter
int BossRemainTime;			// Time counter for fighting boss
bool enableInput = true;	// Controller enable flag
int enshot = false;			// Enable shooting flag
int enshotT = false;		// Shooting triggle
int bgx = 0;				// x position of the background to draw
int cursor_y = 180;			// for the cursor in the menu
int offset = 0;				// for the menu background
int lPressed = false;		// for the menu
int OKPressed = false;		// for the menu
int musicCount = 0;			// for playing music
int cheatFlag = false;		// maybe it is useful

int lastTime;
int timeNow;
int delta;

float SinIs[360];
float CosIs[360];

int EndStatus;
int EndTextCount;
long TitleStartTime = 0;
int TitleTextCount = 255;
int TitleBkCount = 255;
int TitleBkNowPos = 0;
int TitleStatus = 0;
int GameOverCount=255;

//struct score* top10[10];


int exit_callback(int arg1, int arg2, void *common);
int CallbackThread(SceSize args, void *argp);
int SetupCallbacks(void);
void MyExceptionHandler(PspDebugRegBlock *regs);
__attribute__((constructor)) void handlerInit();

void DrawFps(int delta);
void DrawCount(void);
int arcsin(int x,int y);

void DrawBackGround(void);
void DrawOthers(void);
void DrawMyPlane(void);
void DrawSB(void);
void DrawEnemy(void);
void DrawEB(void);
void DrawTitle(void);
void BossTime(void);

void CreateSB(int, int);
void DeleteSB(struct sbullet*);
void CreateEnemy(struct enemy*);
void DeleteEnemy(struct enemy*);
void CreateEB(struct eybullet*);
void DeleteEB(struct eybullet*);
void CreateCS(struct circleshot*);
void DeleteCS(struct circleshot*);

void CreateEvent();
void EventRead();

void Init(void);
int MainMenu(void);
int InGameMenu(void);
int Get_Menu_Input(void);
void Setup_For_Run(void);
void Get_Input(void);
void DealWithImpact(void);
void Render_Frame(void);
void Wait(void);
void Fixup(void);
void Release_And_Cleanup(void);
int GameCredit(void);
//int ScoreReg(void);
//void ScoreDisp(void);


int main()
{
	lastTime = Sys_Milliseconds();

	while (!done)
	{
		timeNow = Sys_Milliseconds();
		delta = timeNow-lastTime;
		lastTime = timeNow;

		gettimeofday(&tick, 0);

    	sceCtrlPeekBufferPositive(&pad, 1);		// using sceCtrlPeekBufferPositive is faster than sceCtrlReadBufferPositive
												// because sceCtrlReadBufferPositive waits for vsync

		// flip and bring the background page to the front
		sceDisplayWaitVblankStart();
		sceDisplaySetFrameBuf(pVRAM, FRAME_BUFFER_WIDTH, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);

		// make the previous page as the background page for rendering
		currentPage = 1 - currentPage;
		pVRAM = pBufferPointer[currentPage];

		if (musicCount < 14100)
		{
			if (game_state == GAME_RUN)
				musicCount += 2;
			else
				musicCount++;
		}
		else
		{
			Player_Stop();
			MikMod_FreeSong(music);
			music = MikMod_LoadSong("12warrior.mod", MAX_CHANNEL);
			Player_Start(music);
			musicCount = 0;
		}

		switch(game_state)
		{
		case GAME_INIT:
			{
				Init();
				game_state = GAME_MAINMENU;
			} break;

		case GAME_MAINMENU:
			{
				game_state = MainMenu();
				if (game_state != GAME_MAINMENU)
				{
					OKPressed = false;
					lPressed = false;
					cursor_y = 180;
				}
			} break;

		case GAME_STARTING:
			{
				Setup_For_Run();
				game_state = GAME_RUN;
			} break;

		case GAME_RUN:
			{
				Get_Input();
				DealWithImpact();
				Render_Frame();
				if (game_state == GAME_MAINMENU)
				{
					OKPressed = false;
					lPressed = false;
					cursor_y = 180;
				}
				Wait();
			} break;

		case GAME_GAMEMENU:
			{
				game_state = InGameMenu();
				if (game_state != GAME_GAMEMENU)
				{
					OKPressed = false;
					lPressed = false;
					cursor_y = 180;
				}
			} break;

		case GAME_RESTART:
			{
				game_state = GAME_STARTING;
				oldButtons = 0;
			} break;

		case GAME_CREDIT:
			{
				game_state = GameCredit();
				if (game_state != GAME_CREDIT)
				{
					OKPressed = false;
					lPressed = false;
					cursor_y = 180;
				}
			} break;

/*		case GAME_SCORE:
			{
				game_state = ScoreReg();
			}
*/
		case GAME_EXIT:
			{
				done = 1;
			} break;
		}

		if (debugMode)
		{
			DrawFps(delta);
			DrawCount();
		}
	}

	Release_And_Cleanup();

	return 0;
}







//------------------------------------------------------------------------------------------------
// Exit callback
int exit_callback(int arg1, int arg2, void *common)
{
	done = 1;
	return 0;
}


//------------------------------------------------------------------------------------------------
// Callback thread
int CallbackThread(SceSize args, void *argp)
{
	int cbid;
	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}


//------------------------------------------------------------------------------------------------
// Sets up the callback thread and returns its thread id
int SetupCallbacks(void)
{
	int thid = 0;
    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if(thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}
    return thid;
}


//------------------------------------------------------------------------------------------------
// Custom exception handler
void MyExceptionHandler(PspDebugRegBlock *regs)
{
	pspDebugScreenInit();

	pspDebugScreenSetBackColor(0x00FF0000);
	pspDebugScreenSetTextColor(0xFFFFFFFF);
	pspDebugScreenClear();

	pspDebugScreenPrintf("I regret to inform you your psp has just crashed\n");
	pspDebugScreenPrintf("Please contact Sony technical support for further information\n\n");
	pspDebugScreenPrintf("Exception Details:\n");
	pspDebugDumpException(regs);
	pspDebugScreenPrintf("\nBlame the 3rd party software, it cannot possibly be our fault!\n");

	sceKernelExitGame();
}


//------------------------------------------------------------------------------------------------
// Sort of hack to install exception handler under USER THREAD
__attribute__((constructor)) void handlerInit()
{
	pspKernelSetKernelPC();
	pspDebugInstallErrorHandler(MyExceptionHandler);
}


//------------------------------------------------------------------------------------------------
// Draw FPS
void DrawFps(int delta)
{
	static int fpscount = 0;	// for fps counting
	static int mseccount = 0;	// for sum of milliseconds
	static char fpsshow[4];		// for displaying fps
	int i;
	mseccount += delta;
	if (mseccount >= 1000)
	{
		for (i=0; i<4; i++)
		{
			fpsshow[i] = '\0';
		}
		itoa(fpscount, fpsshow, 10);
		fpsshow[3] = '\0';
		DrawTextEn2(0, 0, fpsshow);
		fpscount = 0;
		mseccount = 0;
	}
	else
	{
		DrawTextEn2(0, 0, fpsshow);
	}
	fpscount++;
}


//------------------------------------------------------------------------------------------------
// Global time count
void DrawCount()
{
	int i;
	char str[6];
	for (i=0; i<6; i++)
		str[i] = '\0';
	ltoa(timeCount, str, 10);
	DrawTextEn2(0, 460, str);

	for (i=0; i<6; i++)
		str[i] = '\0';
	ltoa(musicCount, str, 10);
	DrawTextEn2(0, 440, str);

	if (enshotT)
		DrawTextEn2(0, 420, "F");
	if (cheatFlag)
		DrawTextEn2(0, 400, "C");
}


//------------------------------------------------------------------------------------------------
// Enable input
void EnableInput(bool value)
{
	enableInput = value;
}

//------------------------------------------------------------------------------------------------
// Init
void Init()
{
	int i;

	pBufferPointer[0] = (u32*)(0x04000000+0x40000000);						// pointer to 1st page in VRAM
	pBufferPointer[1] = (u32*)(0x04000000+0x40000000+FRAME_BUFFER_SIZE);	// pointer to 2nd page in VRAM

	pspDebugScreenInit();	// do this so that we can use pspDebugScreenPrintf
	SetupCallbacks();

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	// mode 0, pixel format: ABGR
	sceDisplaySetMode(0, SCREEN_WIDTH, SCREEN_HEIGHT);

	currentPage = 1;	// prepare a page to be displayed
	pVRAM = pBufferPointer[currentPage];
	
	text = LoadImage("text.png");
	itares = LoadImage("itares.png");
	bg1 = LoadImage("bg1.png");
	bg2 = LoadImage("bg2.png");

	for(i=0; i<360; i++)
	{
		SinIs[i] = sinf((float)i/360.0f*2*PI);
		CosIs[i] = cosf((float)i/360.0f*2*PI);
	}

	InitMikMod();
	music = MikMod_LoadSong("12warrior.mod", MAX_CHANNEL);
	explo = WAV_LoadFN("explo.wav");
	shoot = WAV_LoadFN("shoot.wav");
	menuitem = WAV_LoadFN("menuitem.wav");
	Player_Start(music);
}


//------------------------------------------------------------------------------------------------
// Draw main menu and return a game state
int MainMenu()
{
	// Draw moving background
	int i, j, xx, yy;
	offset--;				// change the offset position to draw the background image
	if (offset <= -128)		// this will give us an illusion of a moving background
		offset = 0;
	yy = offset / 2;		// our background image is 64x64 but the offset is 128, so divide it by 2
	for (i=0; i<6; i++)		// to get the proper offset
	{
		xx = offset / 2;
		for (j=0; j<9; j++)
		{
			DrawImage(bg2, xx, yy, 0, 0, bg2->mWidth, bg2->mHeight, PAINTSRC);
			xx += 64;
		}
		yy += 64;
	}

	// draw menu
	FillRect2(92, 177, MENUWidth+3, MENUHeight, 0xff800000);
	Get_Menu_Input();
	FillRect2(92, cursor_y-2, MENUWidth+3, 17, 0xff008000);
	DrawTextEn2(96, 180, "EASY");
	DrawTextEn2(96, 200, "NORMAL");
	DrawTextEn2(96, 220, "HARD");
	//DrawTextEn2(96, 240, "TOP 10");
	DrawTextEn2(96, 240, "CREDIT");
	DrawTextEn2(96, 260, "EXIT");

	if (lPressed)
	{
		OKPressed = true;
		lPressed = false;
		if (cursor_y>=180 && cursor_y<=220)
			DrawTextEn2(20, 350, "Loading...Please wait");

		switch (cursor_y)
		{
		case 180:	// (SCREEN_WIDTH-MENUHeight)/2
			{
				gameMode = EASYGAME;
				return GAME_STARTING;
			} break;
		case 200:	// (SCREEN_WIDTH-MENUHeight)/2+20
			{
				gameMode = NORMALGAME;
				return GAME_STARTING;
			} break;
		case 220:	// (SCREEN_WIDTH-MENUHeight)/2+40
			{
				gameMode = HARDGAME;
				return GAME_STARTING;
			} break;
		case 240:	// (SCREEN_WIDTH-MENUHeight)/2+60
			{
				return GAME_CREDIT;
			} break;
		case 260:	// (SCREEN_WIDTH-MENUHeight)/2+80
			{
				return GAME_EXIT;
			} break;
		default: break;
		}
	}

	return game_state;
}


//------------------------------------------------------------------------------------------------
// Draw game menu and return a game state
int InGameMenu()
{
	FillRect2(92, 177, MENUWidth+3, MENUHeight, 0xff800000);
	Get_Menu_Input();
	if (cursor_y == 240)
	{
		if (oldButtons & PSP_CTRL_LEFT)
			cursor_y = 260;
		else if (oldButtons & PSP_CTRL_RIGHT)
			cursor_y = 220;
	}
	FillRect2(92, cursor_y-2, MENUWidth+3, 17, 0xff008000);
	DrawTextEn2(96, 180, "Resume");
	DrawTextEn2(96, 200, "Restart");
	DrawTextEn2(96, 220, "Select");
	DrawTextEn2(96, 260, "EXIT");

	if (lPressed)
	{
		OKPressed = true;
		lPressed = false;

		switch (cursor_y)
		{
		case 180:	// (SCREEN_WIDTH-MENUHeight)/2
			{
				return GAME_RUN;
			} break;
		case 200:	// (SCREEN_WIDTH-MENUHeight)/2+20
			{
				return GAME_RESTART;
			} break;
		case 220:	// (SCREEN_WIDTH-MENUHeight)/2+40
			{
				return GAME_MAINMENU;
			} break;
		case 260:	// (SCREEN_WIDTH-MENUHeight)/2+80
			{
				return GAME_EXIT;
			} break;
		default: break;
		}
	}

	return game_state;
}


//------------------------------------------------------------------------------------------------
// Get input in menu and return the position y of the cursor
int Get_Menu_Input()
{
	if (pad.Buttons != 0)
	{
		if ((pad.Buttons & PSP_CTRL_RIGHT) && !(oldButtons & PSP_CTRL_RIGHT))
		{
			if (cursor_y > 180)
				cursor_y -= 20;
			else
				cursor_y = 260;
			voice = MikMod_PlaySample(menuitem, 0, 0);
		}
		else if ((pad.Buttons & PSP_CTRL_LEFT) && !(oldButtons & PSP_CTRL_LEFT))
		{
			if (cursor_y < 260)
				cursor_y += 20;
			else
				cursor_y = 180;
			voice = MikMod_PlaySample(menuitem, 0, 0);
		}
		else if ( ((pad.Buttons & PSP_CTRL_LTRIGGER) && !(oldButtons & PSP_CTRL_LTRIGGER))
			|| ((pad.Buttons & PSP_CTRL_CROSS) && !(oldButtons & PSP_CTRL_CROSS)) )
		{
			lPressed = true;
			voice = MikMod_PlaySample(menuitem, 0, 0);
		}

		if ((pad.Buttons & PSP_CTRL_CIRCLE) && !(oldButtons & PSP_CTRL_CIRCLE))	
		{
			ScreenShot("ms0:/PSP/PHOTO/screenshot.png");
		}

		if ((pad.Buttons & PSP_CTRL_RTRIGGER) && !(oldButtons & PSP_CTRL_RTRIGGER))
		{
			if (debugMode == false)
				debugMode = true;
			else
				debugMode = false;
		}
	}

	oldButtons = pad.Buttons;

	return cursor_y;
}


//------------------------------------------------------------------------------------------------
// Preparation for running
void Setup_For_Run()
{
	// Links operations

	// create my plane
	if (mp != NULL)
	{
		free(mp);
		mp = NULL;
	}
	mp = (struct MyPlane*)malloc(sizeof(struct MyPlane));
	mp->life = 3;
	mp->status = JUSTOUT;
	mp->cx = SCREEN_HEIGHT / 3;
	mp->cy = SCREEN_WIDTH - (MPHeight>>1);
	mp->NoDamageTime = 0;
	mp->score = 0;
	mp->visable = false;

	// initialize the bullets link of my plane
	sbEnd = sbHead;
	while (sbEnd != NULL)
	{
		sbHead = sbEnd;
		sbEnd = sbEnd->next;
		free(sbHead);
	}
	sbHead = NULL;
	sbEnd = NULL;

	// initialize the enemies link
	eyEnd = eyHead;
	while (eyEnd != NULL)
	{
		eyHead = eyEnd;
		eyEnd = eyEnd->next;
		free(eyHead);
	}
	eyHead = NULL;
	eyEnd = NULL;

	// initialize the bullets link of enemies
	ebEnd = ebHead;
	while (ebEnd != NULL)
	{
		ebHead = ebEnd;
		ebEnd = ebEnd->next;
		free(ebHead);
	}
	ebHead = NULL;
	ebEnd = NULL;

	// initialize the circle bullets link of enemies
	csEnd = csHead;
	while (csEnd != NULL)
	{
		csHead = csEnd;
		csEnd = csEnd->next;
		free(csHead);
	}
	csHead = NULL;
	csEnd = NULL;

	// the enemy hitted
	lastHitted = NULL;

	// initialize the events link
	eventEnd = eventp;
	while (eventEnd != NULL)
	{
		eventp = eventEnd;
		eventEnd = eventEnd->next;
		free(eventp);
	}
	eventp = NULL;
	eventEnd = NULL;

	timeCount = 0;
	BossRemainTime = 5000;
	bgx = 0;

	EndStatus = 0;
	EndTextCount = 255;
	GameOverCount = 255;
	TitleStartTime = 0;
	TitleTextCount = 255;
	TitleBkCount = 255;
	TitleBkNowPos = 0;
	TitleStatus = 0;

	cursor_y = 180;				// reset cursor_y
	stageState = GAME_TITLE;	// for render

	// Create events from file
	CreateEvent();
}


//------------------------------------------------------------------------------------------------
// Get button and pad input
void Get_Input()
{
	if (pad.Buttons != 0)		// one or more buttons have been pressed
	{
		if ((pad.Buttons & PSP_CTRL_CIRCLE) && !(oldButtons & PSP_CTRL_CIRCLE))
		{
			ScreenShot("ms0:/PSP/PHOTO/screenshot.png");
		}
		if ((pad.Buttons & PSP_CTRL_START) && !(oldButtons & PSP_CTRL_START))
		{
			game_state = GAME_GAMEMENU;
		}
		if ((pad.Buttons & PSP_CTRL_RTRIGGER) && !(oldButtons & PSP_CTRL_RTRIGGER))
		{
			if (debugMode == false)
				debugMode = true;
			else
				debugMode = false;
		}

		if (!enableInput)
			return;

		if ((pad.Buttons & PSP_CTRL_TRIANGLE) && !(oldButtons & PSP_CTRL_TRIANGLE))
		{
			if (enshotT == false)
				enshotT = true;
			else
				enshotT = false;
		}

		if ((pad.Buttons & PSP_CTRL_SQUARE) && !(oldButtons & PSP_CTRL_SQUARE))
		{
			if (cheatFlag == false)
				cheatFlag = true;
			else
				cheatFlag = false;
		}

		if ( ((pad.Buttons & PSP_CTRL_LTRIGGER) || (pad.Buttons & PSP_CTRL_CROSS)) 
			&& !enshotT)
		{
			if (enshot == false)
				enshot = true;
			else
				enshot = false;
			if (mp->status!=WAITING && mp->status!=DESTROY && mp->status!=DEAD)
			{
				if (cheatFlag)
					enshot = true;
				if (enshot == true)
				{
					CreateSB(mp->cx-6, mp->cy-9);
					CreateSB(mp->cx+6, mp->cy-9);
					voice = MikMod_PlaySample(shoot, 0, 0);
				}
			}
		}
		// DIRECTION buttons: move, 90 degree rolled
		if (pad.Buttons & PSP_CTRL_UP)
		{
			if (mp->cx > 15)
				mp->cx -= STEP;
		}
		else if (pad.Buttons & PSP_CTRL_DOWN)
		{
			if (mp->cx < SCREEN_HEIGHT-15)
				mp->cx += STEP;
		}
		else if (pad.Buttons & PSP_CTRL_RIGHT)
		{
			if (mp->cy > 15)
				mp->cy -= STEP;
		}
		else if (pad.Buttons & PSP_CTRL_LEFT)
		{
			if (mp->cy < SCREEN_WIDTH-15)
				mp->cy += STEP;
		}
	}

	if (enshotT)
	{
		if (enshot == false)
			enshot = true;
		else
			enshot = false;
		if (mp->status!=WAITING && mp->status!=DESTROY && mp->status!=DEAD)
		{
			if (cheatFlag)
				enshot = true;
			if (enshot == true)
			{
				CreateSB(mp->cx-6, mp->cy-9);
				CreateSB(mp->cx+6, mp->cy-9);
				voice = MikMod_PlaySample(shoot, 0, 0);
			}
		}
	}

	oldButtons = pad.Buttons;

	// range of pad.Lx and pad.Ly is 0-255
	if (pad.Ly < 64)
	{
		if (mp->cx > 15)
			mp->cx -= STEP;
	}
	else if (pad.Ly > 196)
	{
		if (mp->cx < SCREEN_HEIGHT-15)
			mp->cx += STEP;
	}
	if (pad.Lx < 64)
	{
		if (mp->cy < SCREEN_WIDTH-15)
			mp->cy += STEP;
	}
	else if (pad.Lx > 196)
	{
		if (mp->cy > 15)
			mp->cy -= STEP;
	}
}


//------------------------------------------------------------------------------------------------
// Render a frame
void Render_Frame()
{
	int i;
	switch(stageState)
	{
	case GAME_TITLE:
		{
			stageState = GAMESTAGE1;
		} break;

	case GAMESTAGE1:
		{
			EnableInput(true);
			EventRead();
			DrawBackGround();
			DrawSB();
			DrawEnemy();
			DrawMyPlane();
			DrawEB();
			DrawOthers();
		} break;

	case GAMESTAGE1TITLE:
		{
			if (mp->cy < 400)
			{
				mp->cy += 3;
			}
			EventRead();
			DrawBackGround();
			DrawSB();
			DrawEnemy();
			DrawMyPlane();
			DrawEB();
			DrawOthers();
			DrawTitle();
		} break;
		
	case GAMESTAGE1BOSS:
		{
			EventRead();
			DrawBackGround();
			DrawSB();
			DrawEnemy();
			DrawMyPlane();
			DrawEB();
			DrawOthers();
			if (timeCount < 2000)
			{
				EnableInput(false);
				if (enshotT)
					enshotT = false;
				else if (timeCount == 1999)
					enshotT = true;
				if (timeCount%30 < 27)
				{
					DrawTextEn2(81, 200, "D A N G E R");	// (SCREEN_HEIGHT-110)/2, SCREEN_WIDTH*2/5
				}
				if(mp->cy < 360)	// SCREEN_WIDTH*3/4=360
				{
					mp->status = NODAMAGE;
					mp->cy += STEP;
				}
			}
			else
			{
				if (eyHead == NULL)
				{
					stageState = GAMESTAGE1END;
				}
				EnableInput(true);
				if (BossRemainTime > 0)
					BossRemainTime -= 2;
			}
			BossTime();
		} break;

	case GAMESTAGE1END:
		{
			cheatFlag = false;
			enshotT = false;
			EndTextCount -= 5;
			EnableInput(false);
			DrawBackGround();
			DrawMyPlane();
			DrawOthers();
			BossTime();

			if (EndStatus == 0)
			{
				DrawTextEn2(76, 80, "S T A G E  I");	// (SCREEN_HEIGHT-120)/2, SCREEN_WIDTH/6
				if (EndTextCount <= 0)
				{
					EndStatus = 1;
					EndTextCount = 255;
				}
			}
			else if (EndStatus == 1)
			{
				DrawTextEn2(76, 80, "S T A G E  I");
				DrawTextEn2(41, 160, "B O S S   S C O R E");
				if (EndTextCount <= 0)
				{
					EndStatus = 2;
					EndTextCount = 255;
				}
			}
			else if (EndStatus == 2)
			{
				DrawTextEn2(76, 80, "S T A G E  I");
				DrawTextEn2(41, 160, "B O S S   S C O R E");
				char bstime[5];
				for(i=0; i<5; i++)
				{
					bstime[i] = '\0';
				}
				itoa(BossRemainTime, bstime, 10);
				DrawTextEn2(116, 240, bstime);
				if (EndTextCount <= 0)
				{
					EndStatus = 3;
					EndTextCount = 255;
					mp->score += BossRemainTime;
					mp->score += mp->life*1000;
					if (gameMode == 1)
					{
						mp->score = mp->score>>1;
					}
					else if (gameMode == 3)
					{
						mp->score = mp->score+(mp->score>>1);
					}
				}
			}
			else if (EndStatus == 3)
			{
				DrawTextEn2(76, 80, "S T A G E  I");
				DrawTextEn2(41, 160, "B O S S   S C O R E");
				char bstime[5];
				for(i=0; i<5; i++)
				{
					bstime[i] = '\0';
				}
				itoa(BossRemainTime, bstime, 10);
				DrawTextEn2(116, 240, bstime);
				if (EndTextCount <= 0)
				{
					EndStatus = 4;
				}
			}
			else if (EndStatus == 4)
			{
				DrawTextEn2(90, 120, "Thank you");
				DrawTextEn2(32, 240, "for playing this game");
				DrawTextEn2(46, 360, "Meet you next time");
				DrawTextEn2(25, 460, "Press L to continue");

				Get_Menu_Input();
				
				if (lPressed)
				{
					OKPressed = true;
					lPressed = false;
					game_state = GAME_MAINMENU;
				}
			}
		} break;

	case GAME_OVER:
		{
			cheatFlag = false;
			enshotT = false;
			GameOverCount -= 5;
			EnableInput(false);
			EventRead();
			DrawBackGround();
			DrawSB();
			DrawEnemy();
			DrawMyPlane();
			DrawEB();
			DrawOthers();

			if (GameOverCount >= 0)
			{
				DrawTextEn2(51, 199, "G A M E   O V E R");
			}
			else
			{
				DrawTextEn2(90, 120, "Thank you");
				DrawTextEn2(32, 240, "for playing this game");
				DrawTextEn2(46, 360, "Meet you next time");
				DrawTextEn2(25, 460, "Press L to continue");

				Get_Menu_Input();
				
				if (lPressed)
				{
					OKPressed = true;
					lPressed = false;
					game_state = GAME_MAINMENU;
				}
			}
		} break;
		
	default: break;
	}

	timeCount++;
}

void Wait()
{
	while((Sys_Milliseconds()-lastTime) <= 33);	// 30fps
}


void Release_And_Cleanup()
{
	// clear my plane
	if (mp != NULL)
	{
		free(mp);
		mp = NULL;
	}

	// clear the bullets link of my plane
	sbEnd = sbHead;
	while (sbEnd != NULL)
	{
		sbHead = sbEnd;
		sbEnd = sbEnd->next;
		free(sbHead);
	}
	sbHead = NULL;
	sbEnd = NULL;

	// clear the enemies link
	eyEnd = eyHead;
	while (eyEnd != NULL)
	{
		eyHead = eyEnd;
		eyEnd = eyEnd->next;
		free(eyHead);
	}
	eyHead = NULL;
	eyEnd = NULL;

	// clear the bullets link of enemies
	ebEnd = ebHead;
	while (ebEnd != NULL)
	{
		ebHead = ebEnd;
		ebEnd = ebEnd->next;
		free(ebHead);
	}
	ebHead = NULL;
	ebEnd = NULL;

	// clear the circle bullets link of enemies
	csEnd = csHead;
	while (csEnd != NULL)
	{
		csHead = csEnd;
		csEnd = csEnd->next;
		free(csHead);
	}
	csHead = NULL;
	csEnd = NULL;

	// clear the enemy hitted
	lastHitted = NULL;

	// clear the events link
	eventEnd = eventp;
	while (eventEnd != NULL)
	{
		eventp = eventEnd;
		eventEnd = eventEnd->next;
		free(eventp);
	}
	eventp = NULL;
	eventEnd = NULL;

	// free sounds
	Player_Stop();
	MikMod_FreeSong(music);
	WAV_Free(shoot);
	WAV_Free(explo);
	WAV_Free(menuitem);
	MikMod_Exit();
	
	// free the images
	FreeImage(text);
	FreeImage(itares);
	FreeImage(bg1);
	FreeImage(bg2);

	sceKernelExitGame();
}


//------------------------------------------------------------------------------------------------
// Draw my plane
void DrawMyPlane(void)
{
	if (mp == NULL)
		return;
	if (mp->status == NORMAL)
	{
		DrawImage2(itares, mp->cx-MPWidth/2, mp->cy-MPHeight/2, 0, 0, MPWidth, MPHeight, PAINTSRC);
	}
	else if (mp->status == JUSTOUT)
	{
		EnableInput(false);
		mp->cy -= 3;
		if (timeCount%30 < 24)
		{
			DrawImage2(itares, mp->cx-MPWidth/2, mp->cy-MPHeight/2, 0, 0, MPWidth, MPHeight, PAINTSRC);
		}
		if (mp->cy < 385)
		{
			mp->status = NODAMAGE;
		}
	}
	else if (mp->status == NODAMAGE)
	{
		EnableInput(true);
		if (timeCount%20 < 17)
		{
			DrawImage2(itares, mp->cx-MPWidth/2, mp->cy-MPHeight/2, 0, 0, MPWidth, MPHeight, PAINTSRC);
		}
		mp->NoDamageTime++;
		if (mp->NoDamageTime >= 100)
		{
			mp->NoDamageTime = 0;
			mp->status = NORMAL;
		}
	}
	else if (mp->status == WAITING)
	{
		mp->NoDamageTime++;
		mp->cx = 136;	// SCREEN_HEIGHT/2;
		mp->cy = 576;	// SCREEN_WIDTH*12/10;
		if (mp->NoDamageTime >= 30)
		{
			mp->status = JUSTOUT;
			mp->NoDamageTime = 0;
			mp->cx = 90;	// SCREEN_HEIGHT/3;
			mp->cy = 480;	// SCREEN_WIDTH;
		}
	}
	else if (mp->status == DEAD)
	{
		mp->cx = 136;	// SCREEN_HEIGHT/2;
		mp->cy = 576;	// SCREEN_WIDTH*12/10;
	}
}


//------------------------------------------------------------------------------------------------
// Draw Background in the game
void DrawBackGround(void)
{
	if (bgx != SCREEN_WIDTH)
	{
		if (timeCount%8 == 0)
			bgx++;
		DrawImage(bg1, 0, 0, bgx, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PAINTSRC);
	}
	else
	{
		DrawImage(bg1, 0, 0, SCREEN_WIDTH, 0, SCREEN_WIDTH, SCREEN_HEIGHT, PAINTSRC);
	}
}


//------------------------------------------------------------------------------------------------
// Draw others
void DrawOthers(void)
{
	int i;

	// draw max HP & HP
	if (lastHitted!=NULL && lastHitted->status!=DESTROY)
	{
		FillRect2(26, 25, 216*lastHitted->MaxHP/800, 3, 0xff0000FF);
		FillRect2(26, 25, 216*lastHitted->HP/800, 3, 0xff179AE8);
	}

	// draw score
	char str[8];
	for (i=0; i<8; i++)
		str[i]='\0';
	ltoa(mp->score, str, 10);
	DrawTextEn2(180, 0, str);

	// draw life
	if (mp->life > 0)
	{
		for(i=0; i<mp->life; i++)
			DrawImage2(itares, 251, SCREEN_WIDTH-i*26-LifeHeight, 75, 0, LifeWidth, LifeHeight, PAINTSRC);
	}
}


//------------------------------------------------------------------------------------------------
// Create a bullet into the link of my bullets
void CreateSB(int cx, int cy)
{
	struct sbullet* p = (struct sbullet *)malloc(sizeof(struct sbullet));
	p->cx = cx;
	p->cy = cy;
	p->status = NORMAL;
	p->pre = NULL;
	p->next = NULL;
	if (sbHead == NULL)
	{
		sbHead = p;
		sbEnd = p;
	}
	else
	{
		sbEnd->next = p;
		p->pre = sbEnd;
		sbEnd = p;
	}
}


//------------------------------------------------------------------------------------------------
// Delete a bullet from the link of my bullets
void DeleteSB(struct sbullet* p)
{
	if (p == sbHead)
	{
		if (sbHead == sbEnd)
		{
			sbHead = NULL;
			sbEnd = NULL;
			free(p);
		}
		else
		{
			sbHead = p->next;
			p->next->pre = NULL;
			free(p);
		}
		return;
	}
	else if (p == sbEnd)
	{
		sbEnd = p->pre;
		p->pre->next = NULL;
		free(p);
		return;
	}
	else
	{
		p->pre->next = p->next;
		p->next->pre = p->pre;
		free(p);
		return;
	}
}


//------------------------------------------------------------------------------------------------
// Draw bullets of my plane and move them
void DrawSB()
{
	struct sbullet* p = sbHead;
	struct sbullet* ptemp = NULL;
	while (p != NULL)
	{
		// Delete this bullet from the link of my bullets
		// if the bullet is in a state of destroied
		if (p->status == DESTROY)
		{
			ptemp = p;
			p = p->next;
			DeleteSB(ptemp);
			continue;
		}

		// Draw the bullet
		if (p->cy >= SBHeight/2)
		{
			DrawImage2(itares, p->cx-SBWidth/2, p->cy-SBHeight/2, 104, 0, SBWidth, SBHeight, PAINTSRC);
		}
		else
		{
			DrawImage2(itares, p->cx-SBWidth/2, 0, 104, 0, SBWidth, SBHeight, PAINTSRC);
		}

		// Make a movement
		p->cy -= SBSPEED;

		// Check boundary
		if (p->cy < 0)
			p->status = DESTROY;

		p = p->next;
	}
}


//------------------------------------------------------------------------------------------------
// Create events
void CreateEvent()
{
	int fd = -1;
	char buffer;
	int unitnumber=0;
	int value;
	float valuef;
	struct event* p;
	struct event* pc;

	// open Seen file
	if (gameMode == EASYGAME)
	{
		fd = sceIoOpen("easy1.txt", PSP_O_RDONLY, 0777);
	}
	else if (gameMode == NORMALGAME)
	{
		fd = sceIoOpen("normal1.txt", PSP_O_RDONLY, 0777);
	}
	else if (gameMode == HARDGAME)
	{
		fd = sceIoOpen("hard1.txt", PSP_O_RDONLY, 0777);
	}

	if (fd < 0)
	{
		printf("ERROR: Fail to load Seen files.\n\nPress <HOME> key to exit.");
		game_state = GAME_EXIT;
		sceIoClose(fd);
		return;
	}

	// 开始读文件并处理
	sceIoRead(fd, &buffer, 1);
	buffer = buffer^'I';
	while (buffer != '!')
	{
		switch (buffer)
		{
		case '#':
			{
				unitnumber++;
			} break;
		case '<':
			{
				p = (struct event*)malloc(sizeof(struct event));
				p->pre = NULL;
				p->next = NULL;
				p->UnitNumber = unitnumber;
				sceIoRead(fd, &buffer, 1);
				buffer = buffer^'I';
				while (buffer != '>')
				{
					switch (buffer)
					{
					case 'E':
						{
							sceIoRead(fd, &buffer, 1);
							buffer = buffer^'I';
							if (buffer == 'C')   //EC
							{
								value = 0;
								sceIoRead(fd, &buffer, 1);   //读入等号
								buffer = buffer^'I';
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
								while (buffer!=',' && buffer!='>')
								{
									value *= 10;
									value += atoi(&buffer);
									sceIoRead(fd, &buffer, 1);
									buffer = buffer^'I';
								}
								p->EventCount = value;
							}
							else if (buffer == 'N')   //EN
							{
								sceIoRead(fd, &buffer, 1);   //读入等号
								buffer = buffer^'I';
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
								if (buffer == 'C')
								{
									p->EventNumber = CREATEENEMY;
								}
								else if (buffer == 'S')
								{
									p->EventNumber = SHOT;
								}
								else if (buffer == 'P')
								{
									p->EventNumber = PATHCHANGE;
								}
								else if (buffer == 'T')
								{
									p->EventNumber = TITLE;
								}
								else if (buffer == 'O')
								{
									p->EventNumber = CONTINUE;
								}
								else if (buffer == 'A')
								{
									p->EventNumber = AUTOTRACKSHOT;
								}
								else if (buffer == 'B')
								{
									p->EventNumber = BOSSOUT;
								}
								else if (buffer == 'X')
								{
									p->EventNumber = CIRCLESHOT;
								}
								else if (buffer == 'Y')
								{
									p->EventNumber = CTRCLEAUTOSHOT;
								}
								else
								{
									printf("ERROR: Fail to load Seen files.\n\nPress <HOME> key to exit.");
									game_state = GAME_EXIT;
									sceIoClose(fd);
									return;
								}
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
							}
							else
							{
								printf("ERROR: Fail to load Seen files.\n\nPress <HOME> key to exit.");
								game_state = GAME_EXIT;
								sceIoClose(fd);
								return;
							}
						} break;
					case 'S':
						{
							int a = 1;
							sceIoRead(fd, &buffer, 1);
							buffer = buffer^'I';
							if (buffer == 'X')
							{
								value = 0;
								sceIoRead(fd, &buffer, 1);   //读入等号
								buffer = buffer^'I';
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
								if (buffer == '-')
								{
									a = -1;
									sceIoRead(fd, &buffer, 1);
									buffer = buffer^'I';
								}
								while (buffer!=',' && buffer!='>')
								{
									value *= 10;
									value += atoi(&buffer);
									sceIoRead(fd, &buffer, 1);
									buffer = buffer^'I';
								}
								p->startx = value*a;
							}
							else if (buffer == 'Y')
							{
								value = 0;
								sceIoRead(fd, &buffer, 1);   //读入等号
								buffer = buffer^'I';
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
								if (buffer == '-')
								{
									a = -1;
									sceIoRead(fd, &buffer, 1);
									buffer = buffer^'I';
								}
								while (buffer!=',' && buffer!='>')
								{
									value *= 10;
									value += atoi(&buffer);
									sceIoRead(fd, &buffer, 1);
									buffer = buffer^'I';
								}
								p->starty = value*a;
							}
							else if (buffer == 'D')
							{
								float a = 0.1f;
								value = 0;
								valuef = 0;
								sceIoRead(fd, &buffer, 1);   //读入等号
								buffer = buffer^'I';
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
								while (buffer!='.' && buffer!=',' && buffer!='>')
								{
									value *= 10;
									value += atoi(&buffer);
									sceIoRead(fd, &buffer, 1);
									buffer = buffer^'I';
								}
								if (buffer == '.')
								{
									sceIoRead(fd, &buffer, 1);
									buffer = buffer^'I';
									while (buffer!=',' && buffer!='>')
									{
										valuef += (float)atoi(&buffer)*a;
										a *= 0.1f;
										sceIoRead(fd, &buffer, 1);
										buffer = buffer^'I';
									}
								}
								p->speed = (float)value+valuef;
							}
							else
							{
								printf("ERROR: Fail to load Seen files.\n\nPress HOME key to exit.");
								game_state = GAME_EXIT;
								sceIoClose(fd);
								return;
							}
						} break;
					case 'A':
						{
							value = 0;
							sceIoRead(fd, &buffer, 1);   //读入E
							buffer = buffer^'I';
							sceIoRead(fd, &buffer, 1);   //读入=
							buffer = buffer^'I';
							sceIoRead(fd, &buffer, 1);
							buffer = buffer^'I';
							while (buffer!=',' && buffer!='>')
							{
								value *= 10;
								value += atoi(&buffer);
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
							}
							p->angle = value;
						} break;
					case 'H':
						{
							value = 0;
							sceIoRead(fd, &buffer, 1);   //读入P
							buffer = buffer^'I';
							sceIoRead(fd, &buffer, 1);   //读入=
							buffer = buffer^'I';
							sceIoRead(fd, &buffer, 1);
							buffer = buffer^'I';
							while (buffer!=',' && buffer!='>')
							{
								value *= 10;
								value += atoi(&buffer);
								sceIoRead(fd, &buffer, 1);
								buffer = buffer^'I';
							}
							p->HP = value;
						} break;

					default:
						{
							printf("ERROR: Fail to load Seen files.\n\nPress <HOME> key to exit.");
							game_state = GAME_EXIT;
							sceIoClose(fd);
							return;
						} break;
					}
					if (buffer != '>')
					{
						sceIoRead(fd, &buffer, 1);
						buffer = buffer^'I';
					}
				}

				if (eventp == NULL) 
				{
					eventp = p;
					eventEnd = p;
				}
				else
				{
					pc = eventp;
					while(1)
					{
						if (p->EventCount < pc->EventCount)
						{
							if (pc == eventp)
							{
								p->next = pc;
								pc->pre = p;
								eventp = p;
							}
							else
							{
								pc->pre->next = p;
								p->pre = pc->pre;
								p->next = pc;
								pc->pre = p;
							}
							break;
						}
						if (pc == eventEnd)
						{
							pc->next = p;
							p->pre  =pc;
							eventEnd = p;
							break;
						}
						pc = pc->next;
					}
				}
			}
		}
		sceIoRead(fd, &buffer, 1);
		buffer = buffer^'I';
	}

	sceIoClose(fd);
}


//------------------------------------------------------------------------------------------------
// Read event
void EventRead()
{
	if (eventp == NULL)
		return;
	while (eventp->EventCount < timeCount)
	{
		eventp = eventp->next;
	}
	while (eventp->EventCount == timeCount)
	{
		switch (eventp->EventNumber)
		{
		case CREATEENEMY:
			{
				struct enemy* p;
				p = (struct enemy*)malloc(sizeof(struct enemy));
				p->UnitNumber = eventp->UnitNumber;
				p->cx = (float)eventp->startx/100.0f*(float)SCREEN_HEIGHT;
				p->cy = (float)eventp->starty/100.0f*(float)SCREEN_WIDTH;
				p->HP = eventp->HP;
				p->MaxHP = eventp->HP;
				p->status = JUSTOUT;
				p->dx = eventp->speed*CosIs[eventp->angle]*betax;
				p->dy = eventp->speed*SinIs[eventp->angle]*betay;
				p->pre = NULL;
				p->next = NULL;
				CreateEnemy(p);
			}
			break;
		case SHOT:
			{
				struct enemy* ep;
				ep = eyHead;
				while (ep != NULL)
				{
					if (ep->UnitNumber == eventp->UnitNumber)
					{
						struct eybullet* p;
						p = (struct eybullet*)malloc(sizeof(struct eybullet));
						p->FromUnitNumber = eventp->UnitNumber;
						p->status = NORMAL;
						p->cx = ep->cx;
						p->cy = ep->cy;
						p->dx = eventp->speed*CosIs[eventp->angle];
						p->dy = eventp->speed*SinIs[eventp->angle];
						p->pre = NULL;
						p->next = NULL;
						CreateEB(p);
						break;
					}
					ep = ep->next;
				}
			}
			break;
		case PATHCHANGE:
			{
				struct enemy* p;
				p = eyHead;
				while (p != NULL)
				{
					if (p->UnitNumber == eventp->UnitNumber)
					{
						p->dx = eventp->speed*CosIs[eventp->angle]*betax;
						p->dy = eventp->speed*SinIs[eventp->angle]*betay;
						break;
					}
					p = p->next;
				}
			}
			break;
		case TITLE:
			{
				if (eventp->HP == 1)
					stageState = GAMESTAGE1TITLE;
			}
			break;
		case CONTINUE:
			{
				if (eventp->HP == 1)
					stageState = GAMESTAGE1;
			}
			break;
		case AUTOTRACKSHOT:
			{
				struct enemy* ep;
				ep = eyHead;
				while (ep != NULL)
				{
					if (ep->UnitNumber == eventp->UnitNumber)
					{
						struct eybullet* p;
						p = (struct eybullet*)malloc(sizeof(struct eybullet));
						p->FromUnitNumber = eventp->UnitNumber;
						p->status = NORMAL;
						p->cx = ep->cx;
						p->cy = ep->cy;
						p->dx = eventp->speed*CosIs[(eventp->angle+arcsin(mp->cx-(int)ep->cx,mp->cy-(int)ep->cy))%360];
						p->dy = eventp->speed*SinIs[(eventp->angle+arcsin(mp->cx-(int)ep->cx,mp->cy-(int)ep->cy))%360];
						p->pre = NULL;
						p->next = NULL;
						CreateEB(p);
						break;
					}
					ep = ep->next;
				}
			}
			break;
		case BOSSOUT:
			{
				stageState = GAMESTAGE1BOSS;
			}
			break;
		case CIRCLESHOT:
			{
				struct enemy* ep;
				ep = eyHead;
				while (ep != NULL)
				{
					if (ep->UnitNumber == eventp->UnitNumber)
					{
						struct circleshot* p;
						p = (struct circleshot*)malloc(sizeof(struct circleshot));
						p->UnitNumber = eventp->UnitNumber;
						p->EventCount = eventp->EventCount;
						p->angle = eventp->angle;
						p->speed = eventp->speed;
						p->CountNext = eventp->startx;
						p->EndCount = eventp->starty;
						p->mode = 0;
						p->pre = NULL;
						p->next = NULL;
						CreateCS(p);
						break;
					}
					ep = ep->next;
				}
			}
			break;
		case CTRCLEAUTOSHOT:
			{
				struct enemy* ep;
				ep = eyHead;
				while (ep != NULL)
				{
					if (ep->UnitNumber == eventp->UnitNumber)
					{
						struct circleshot* p;
						p = (struct circleshot*)malloc(sizeof(struct circleshot));
						p->UnitNumber = eventp->UnitNumber;
						p->EventCount = eventp->EventCount;
						p->angle = eventp->angle;
						p->speed = eventp->speed;
						p->CountNext = eventp->startx;
						p->EndCount = eventp->starty;
						p->mode = 1;
						p->pre = NULL;
						p->next = NULL;
						CreateCS(p);
						break;
					}
					ep = ep->next;
				}
			}
			break;
		}
		eventp = eventp->next;
		if (eventp == NULL)
			return;
	}
}


//------------------------------------------------------------------------------------------------
// Create an enemy into the link of enemies
void CreateEnemy(struct enemy* p)
{
	if (eyHead == NULL)
	{
		eyHead = p;
		eyEnd = p;
	}
	else
	{
		eyEnd->next = p;
		p->pre = eyEnd;
		eyEnd = p;
	}
}


//------------------------------------------------------------------------------------------------
// Delete an enemy from the link of enemies
void DeleteEnemy(struct enemy* p)
{
	if (p == eyHead)
	{
		if (eyHead == eyEnd)
		{
			eyHead = NULL;
			eyEnd = NULL;
			free(p);
		}
		else
		{
			eyHead = p->next;
			p->next->pre = NULL;
			free(p);
		}
		return;
	}
	else if (p == eyEnd)
	{
		eyEnd = p->pre;
		p->pre->next = NULL;
		free(p);
		return;
	}
	else
	{
		p->pre->next = p->next;
		p->next->pre = p->pre;
		free(p);
		return;
	}
}


//------------------------------------------------------------------------------------------------
// Draw the enemies and move them
void DrawEnemy()
{
	struct enemy* p = eyHead;
	struct enemy* ptemp = NULL;
	int bossdelta = 0;

	while (p != NULL)
	{
		// Delete this enemy from the link of enemies 
		// if the enemy is in a state of destroied
		if (p->status == DESTROY)
		{
			ptemp = p;
			p = p->next;
			DeleteEnemy(ptemp);
			continue;
		}

		// The boss is different from others
		if (p->MaxHP >= 800)
			bossdelta = EYWidth;
		else
			bossdelta = 0;

		// Draw an enemy
		if ( (int)(p->cx) < 0
			|| (int)(p->cy) < 0
			|| (int)(p->cx) > SCREEN_HEIGHT
			|| (int)(p->cy) >SCREEN_WIDTH )
		{
			DrawImage2(itares, (int)(p->cx)-EYWidth/2, (int)(p->cy)-EYHeight/2, EYDELTA+bossdelta, 0, EYWidth, EYHeight, PAINTSRC);
		}
		else
		{
			DrawImage2(itares, (int)(p->cx)-EYWidth/2, (int)(p->cy)-EYHeight/2, EYDELTA+bossdelta, 0, EYWidth, EYHeight, PAINTSRC);
			p->status = NORMAL;
		}

		// Make a movement
		p->cx += p->dx;
		p->cy += p->dy;

		// Check boundary
		if ((p->cx < 0
			|| p->cy < 0
			|| p->cx > SCREEN_HEIGHT
			|| p->cy > SCREEN_WIDTH)
			&& p->status != JUSTOUT)
			p->status = DESTROY;

		p = p->next;
	}
}


//------------------------------------------------------------------------------------------------
// Create a bullet into the link of enemies' bullets
void CreateEB(struct eybullet* p)
{
	if (ebHead == NULL)
	{
		ebHead = p;
		ebEnd = p;
	}
	else
	{
		ebEnd->next = p;
		p->pre = ebEnd;
		ebEnd = p;
	}
}


//------------------------------------------------------------------------------------------------
// Delete a bullet from the link of enemies' bullets
void DeleteEB(struct eybullet *p)
{
	if (p == ebHead)
	{
		if (ebHead == ebEnd)
		{
			ebHead = NULL;
			ebEnd = NULL;
			free(p);
		}
		else
		{
			ebHead = p->next;
			p->next->pre = NULL;
			free(p);
		}
		return;
	}
	else if(p == ebEnd)
	{
		ebEnd = p->pre;
		p->pre->next = NULL;
		free(p);
		return;
	}
	else
	{
		p->pre->next = p->next;
		p->next->pre = p->pre;
		free(p);
		return;
	}
}


//------------------------------------------------------------------------------------------------
// Draw enemies' bullets and move them
void DrawEB()
{
	// circleshot
	struct circleshot* pc = csHead;
	struct circleshot* pctemp = NULL;
	while (pc != NULL)
	{
		if (pc->EventCount >= pc->EndCount)
		{
			pctemp = pc;
			pc = pc->next;
			DeleteCS(pctemp);
			continue;
		}
		if (pc->EventCount == timeCount)
		{
			if (pc->mode == 0)
			{
				struct enemy* ep;
				ep = eyHead;
				while (ep != NULL)
				{
					if (ep->UnitNumber == pc->UnitNumber)
					{
						struct eybullet* p;
						p = (struct eybullet*)malloc(sizeof(struct eybullet));
						p->FromUnitNumber = pc->UnitNumber;
						p->status = NORMAL;
						p->cx = ep->cx;
						p->cy = ep->cy;
						p->dx = pc->speed*CosIs[pc->angle]*betax;
						p->dy = pc->speed*SinIs[pc->angle]*betay;
						p->pre = NULL;
						p->next = NULL;
						CreateEB(p);
						break;
					}
					ep = ep->next;
				}
			}
			else if (pc->mode == 1)
			{
				struct enemy* ep;
				ep = eyHead;
				while (ep != NULL)
				{
					if (ep->UnitNumber == pc->UnitNumber)
					{
						struct eybullet* p;
						p = (struct eybullet*)malloc(sizeof(struct eybullet));
						p->FromUnitNumber = pc->UnitNumber;
						p->status = NORMAL;
						p->cx = ep->cx;
						p->cy = ep->cy;
						p->dx = pc->speed*CosIs[(pc->angle+arcsin(mp->cx-(int)ep->cx,mp->cy-(int)ep->cy))%360];
						p->dy = pc->speed*SinIs[(pc->angle+arcsin(mp->cx-(int)ep->cx,mp->cy-(int)ep->cy))%360];
						p->pre = NULL;
						p->next = NULL;
						CreateEB(p);
						break;
					}
					ep = ep->next;
				}
			}
			pc->EventCount += pc->CountNext;
		}
		pc = pc->next;
	}

	// eybullet
	struct eybullet* p = ebHead;
	struct eybullet* ptemp = NULL;
	while (p != NULL)
	{
		// Delete the bullet from the link of bullets 
		// if the bullet is in a state of destroied
		if (p->status == DESTROY)
		{
			ptemp = p;
			p = p->next;
			DeleteEB(ptemp);
			continue;
		}

		// Draw the bullet
		DrawImage2(itares, (int)(p->cx)-EBWidth/2, (int)(p->cy)-EBHeight/2, 95, 0, EBWidth, EBHeight, PAINTSRC);
		p->status=NORMAL;

		// Make a movement
		p->cx += p->dx;
		p->cy += p->dy;

		// Check boundary
		if ((p->cx < 0
			|| p->cy < 0
			|| p->cx > SCREEN_HEIGHT
			|| p->cy > SCREEN_WIDTH) 
			&& p->status != JUSTOUT)
			p->status = DESTROY;

		p = p->next;
	}
}


//------------------------------------------------------------------------------------------------
// Create a "circle shot" into the link of circleshot
void CreateCS(struct circleshot *p)
{
	if (csHead == NULL)
	{
		csHead = p;
		csEnd = p;
	}
	else
	{
		csEnd->next = p;
		p->pre = csEnd;
		csEnd = p;
	}
}


//------------------------------------------------------------------------------------------------
// Delete a "circle shot" from the link of circleshot
void DeleteCS(struct circleshot *p)
{
	if (p == csHead)
	{
		if (csHead == csEnd)
		{
			csHead = NULL;
			csEnd = NULL;
			free(p);
		}
		else
		{
			csHead = p->next;
			p->next->pre = NULL;
			free(p);
		}
		return;
	}
	else if (p == csEnd)
	{
		csEnd = p->pre;
		p->pre->next = NULL;
		free(p);
		return;
	}
	else
	{
		p->pre->next = p->next;
		p->next->pre = p->pre;
		free(p);
		return;
	}
}


//------------------------------------------------------------------------------------------------
// Game main logic
void DealWithImpact()
{
	if (mp->life <= 0)
	{
		mp->status = DEAD;
		stageState = GAME_OVER;
	}

	// My plane VS the bullets of the enemies -_-b
	struct eybullet *eyp = ebHead;
	while (eyp != NULL)
	{
		if (abs(eyp->cy - mp->cy) < 12 
			&& mp->status!=JUSTOUT 
			&& mp->status!=NODAMAGE 
			&& mp->status!=WAITING)
		{
			if ( ((eyp->cx-mp->cx)*(eyp->cx-mp->cx)+(eyp->cy-mp->cy)*(eyp->cy-mp->cy)) < 144 )
			{
				mp->life--;
				mp->status = WAITING;
				eyp->status = DESTROY;
				voice = MikMod_PlaySample(explo, 0, 0);
			}
		}
		eyp = eyp->next;
	}

	// My plane VS enemies -_-b, enemies VS my bullets ^o^
	bool IsActive = false;			// Flag for displaying HP
	struct enemy *ep = eyHead;
	struct sbullet *sp = NULL;
	if (ep == NULL)
	{
		lastHitted = NULL;
	}
	while (ep != NULL) 
	{
		if (ep->HP <= 0)
		{
			ep->status = DESTROY;
			mp->score += ep->MaxHP;		// Get score
			voice = MikMod_PlaySample(explo, 0, 0);
		}

		if (abs(ep->cy - mp->cy) < 20 
			&& mp->status!=JUSTOUT 
			&& mp->status!=NODAMAGE 
			&& mp->status!=WAITING)
		{
			if ( ((ep->cx-mp->cx)*(ep->cx-mp->cx)+(ep->cy-mp->cy)*(ep->cy-mp->cy)) < 400 )
			{
				mp->life--;
				mp->status = WAITING;
				ep->HP -= 50;
				voice = MikMod_PlaySample(explo, 0, 0);
			}
		}
		
		sp = sbHead;
		while (sp != NULL)
		{
			if (abs(ep->cx - sp->cx) < 12)
			{
				if (abs(ep->cy-sp->cy) < 19)
				{
					ep->HP -= 20;
					sp->status = DESTROY;
					voice = MikMod_PlaySample(explo, 0, 0);
					if (ep->HP > 0)
						lastHitted = ep;	// for displaying HP
					else
						lastHitted = NULL;
				}
			}
			sp = sp->next;
		}
		if (ep == lastHitted)
		{
			IsActive = true;
		}
		ep = ep->next;
	}
	
	if (IsActive == false)
	{
		lastHitted = NULL;
	}
}


//------------------------------------------------------------------------------------------------
// return arcSin
int arcsin(int x,int y)
{
	// 特殊情况
	if (x>0 && y==0)
		return 0;
	else if (x<0 && y==0)
		return 180;
	else if (x==0 && y>0)
		return 90;
	else if (x==0 && y<0)
		return 270;
	// 一般情况
	int i;
	float r = sqrtf((float)(x*x+y*y));
	float l = (float)y / (float)r;
	if (x>0 && y>0)
	{
		for (i=0; i<90; i++)
		{
			if (SinIs[i]<l && SinIs[i+1]>l)
				return i;
		}
	}
	else if (x<0 && y>0)
	{
		for (i=90; i<180; i++)
		{
			if (SinIs[i]>l && SinIs[i+1]<l)
				return i;
		}
	}
	else if (x<0 && y<0)
	{
		for (i=180; i<270; i++)
		{
			if (SinIs[i]>l && SinIs[i+1]<l)
				return i;
		}
	}
	else if (x>0 && y<0)
	{
		for (i=270; i<360; i++)
		{
			if (i == 359) return i;
			if (SinIs[i]<l && SinIs[i+1]>l)
				return i;
		}
	}
	return 0;
}


//------------------------------------------------------------------------------------------------
// Draw stage title
void DrawTitle()
{
	if (TitleStartTime == 0)
		TitleStartTime = timeCount;

	switch (TitleStatus)
	{
	case 0:
		{
			if (TitleBkNowPos < SCREEN_HEIGHT*2/3)
			{
				TitleBkNowPos += TBK;
				TitleBkCount -= 4;
				FillRect2(0, SCREEN_WIDTH/4, TitleBkNowPos, 32, 0xff520000);
			}
			else
			{
				FillRect2(0, SCREEN_WIDTH/4, TitleBkNowPos, 32, 0xff520000);
				TitleStartTime = timeCount;
				TitleStatus = 1;
			}
		} break;
	case 1:
		{
			if (TitleTextCount > 0)
			{
				FillRect2(0, SCREEN_WIDTH/4, TitleBkNowPos, 32, 0xff520000);
				DrawTextEn2((TitleBkNowPos-120)/2, SCREEN_WIDTH/4+9, "S T A G E  I");
			}
			else
			{
				FillRect2(0, SCREEN_WIDTH/4, TitleBkNowPos, 32, 0xff520000);
				DrawTextEn2((TitleBkNowPos-120)/2, SCREEN_WIDTH/4+9, "S T A G E  I");
				TitleTextCount = 255;
				TitleStartTime = timeCount;
				TitleStatus = 2;
			}
		} break;
	case 2:
		{
			if (timeCount-TitleStartTime < 45)
			{
				FillRect2(0, SCREEN_WIDTH/4, TitleBkNowPos, 32, 0xff260000);
				DrawTextEn2((TitleBkNowPos-120)/2, SCREEN_WIDTH/4+9, "S T A G E  I");
			}
			else
			{
				FillRect2(0, SCREEN_WIDTH/4, TitleBkNowPos, 32, 0xff260000);
				DrawTextEn2((TitleBkNowPos-120)/2, SCREEN_WIDTH/4+9, "S T A G E  I");
				TitleStartTime = timeCount;
				TitleStatus = 3;
			}
		} break;
	case 3:
		{
			EnableInput(true);
			if (TitleBkCount < 255)
			{
				TitleBkCount += 3;
				FillRect2(0, SCREEN_WIDTH/4, TitleBkNowPos, 32, 0x07260000);
				DrawTextEn2((TitleBkNowPos-120)/2, SCREEN_WIDTH/4+9, "S T A G E  I");
			}
			else
			{
				TitleStartTime = timeCount;
				TitleStatus = 3;
			}
		} break;
	}
}


//------------------------------------------------------------------------------------------------
// time counting for fighting BOSS
void BossTime(void)
{
	int i;
	char BRT[10];
	BRT[0]='T';
	BRT[1]='I';
	BRT[2]='M';
	BRT[3]='E';
	for(i=4; i<10; i++)
		BRT[i] = '\0';
	itoa(BossRemainTime, &BRT[4], 10);
	DrawTextEn2(149, 55, BRT);
}


//------------------------------------------------------------------------------------------------
// Credit
int GameCredit(void)
{
	// Draw moving background
	int i, j, xx, yy;
	offset--;				// change the offset position to draw the background image
	if (offset <= -128)		// this will give us an illusion of a moving background
		offset = 0;
	yy = offset / 2;		// our background image is 64x64 but the offset is 128, so divide it by 2
	for (i=0; i<6; i++)		// to get the proper offset
	{
		xx = offset / 2;
		for (j=0; j<9; j++)
		{
			DrawImage(bg2, xx, yy, 0, 0, bg2->mWidth, bg2->mHeight, PAINTSRC);
			xx += 64;
		}
		yy += 64;
	}

	DrawTextEn2(25, 30, "In The Air v0.1");
	DrawTextEn2(25, 50, "  PSP version");
	DrawTextEn2(15, 100, "Original Game Designer");
	DrawTextEn2(15, 120, "  wzysj@citiz.net");
	DrawTextEn2(15, 150, "PSP version Designer");
	DrawTextEn2(15, 170, "  lylatitude@126.com");
	DrawTextEn2(15, 220, "Special thanks to");
	DrawTextEn2(15, 250, "  dr.watson @ PSPChina");
	DrawTextEn2(10, 280, "  ps2Dev.org Team");
	DrawTextEn2(10, 320, "and");
	DrawTextEn2(10, 350, "  Everyone @ PSPChina");
	DrawTextEn2(20, 450, "Press L to Main Menu");

	Get_Menu_Input();

	if (lPressed)
	{
		OKPressed = true;
		lPressed = false;
		return GAME_MAINMENU;
	}

	return game_state;
}


/*
//------------------------------------------------------------------------------------------------
// Store score and enter name
int ScoreReg(void)
{
	return game_state;
}


//------------------------------------------------------------------------------------------------
// Display top 10
void ScoreDisp(void)
{
	;
}
*/


