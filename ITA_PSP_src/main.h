#ifndef _MAIN_H_
#define _MAIN_H_

// Game states
#define GAME_INIT		0
#define GAME_MAINMENU	1
#define GAME_STARTING	2
#define GAME_RUN		3
#define GAME_GAMEMENU	4
#define GAME_RESTART	5
#define GAME_EXIT		6
#define	GAME_CREDIT		7
#define GAME_SCORE		8

// Stage states
#define GAME_PAUSE		0
#define GAME_TITLE		1
#define GAME_OVER		2
#define SCORE_REG		3
#define GAME_END		4
#define GAMESTAGE1		5
#define GAMESTAGE1TITLE	6
#define GAMESTAGE1BOSS	7
#define GAMESTAGE1END	8

// Difficulty
#define EASYGAME	1
#define NORMALGAME	2
#define HARDGAME	3

// States of my plane, enemies and bullets
#define JUSTOUT		-1
#define NODAMAGE	-2
#define WAITING		-3
#define DEAD		-4
#define NORMAL		0
#define DESTROY		1

// Event definitions
#define CREATEENEMY		0
#define SHOT			1
#define PATHCHANGE		2
#define TITLE			3
#define CONTINUE		4
#define AUTOTRACKSHOT	5
#define BOSSOUT			6
#define CIRCLESHOT		7
#define CTRCLEAUTOSHOT	8

// Some other definitions
#define STEP			7	// pixels / one movement
#define SBSPEED			10	// Speed of my bullets
#define TBK				8	// Speed of drawing titles
#define NODAMAGETIME	3000	//自机处于无敌状态的时间长度

// Image size and delta x in resources
// My plane
#define MPWidth		25
#define MPHeight	25
// Enemy
#define EYWidth		25
#define EYHeight	25
#define EYDELTA		25
// Life
#define LifeWidth	20
#define LifeHeight	20
#define LifeDELTA	75
// Bullets of enemies
#define EBWidth		9
#define EBHeight	9
#define EBDELTA		95
// Bullets of myself
#define SBWidth		3
#define SBHeight	19
#define SBDELTA		104
// Game background
#define BGWidth		960
#define BGHeight	272
// Game menu
#define MENUWidth	80
#define MENUHeight	100

// Structs
struct MyPlane
{
	int cx;
	int cy;
	int life;
	long score;
	int status;
	int NoDamageTime;
	bool visable;
};

struct sbullet
{
	int cx;
	int cy;
	int status;
	struct sbullet *pre;
	struct sbullet *next;
};

struct enemy
{
	int UnitNumber;
	float cx;
	float cy;
	float dx;
	float dy;
	int status;
	int MaxHP;
	int HP;
	struct enemy* pre;
	struct enemy* next;
};

struct eybullet
{
	int FromUnitNumber;
	float cx;
	float cy;
	float dx;
	float dy;
	int status;
	struct eybullet* pre;
	struct eybullet* next;
};

struct circleshot
{
	int EventCount;
	int UnitNumber;
	float speed;
	int angle;
	int mode;
	int CountNext;
	int EndCount;
	struct circleshot* pre;
	struct circleshot* next;
};

struct event
{
	int EventCount;
	int EventNumber;
	int UnitNumber;
	int startx;
	int starty;
	float speed;
	int angle;
	int HP;
	struct event* pre;
	struct event* next;
};

struct score
{
	char name[8];
	char s[8];
};

#endif
