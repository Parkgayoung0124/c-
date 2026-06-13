#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM 6
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define FISHTANK_WIDTH 100
#define FISHTANK_HEIGHT 200
#define FISH_TYPE_COUNT 5  

// 게임 상태를 나타내는 열거형 정의
typedef enum {
    STATE_MENU,
    STATE_PLAYING,
    STATE_GAME_OVER,
    STATE_WIN
} GameState;

// 물고기 종류 열거형 정의
typedef enum {
    NORMAL,
    FAST,
    BIG,
    SPECIAL,
    SLOW
} FishType;

// 게임 상태 구조체 정의
typedef struct {
    FishType type;          // 물고기 종류
    int fish;               // 물고기 레벨/크기 (0~100)
    int water;              // 물 높이 (0~100)
    int isAlive;            // 1: alive, 0: dead
    double growth;          // 성장 단계 (0.0 ~ 1.0)
    int waterUse;           // 물 소비량
    int hp;                 // 체력 (0~100) - 추가
    char name[32];          // 이름 - 추가
    SDL_Texture* texture;   // 물고기 텍스처
} FishTank;

// 전체 구조체 정의
typedef struct {
    GameState state;
    int score;
    int level;
    int lives;
} Game;

// 전역 변수 정의
extern Game game;
extern FishTank fishTanks[NUM];
extern int level;
extern int position;
extern bool running;
extern bool gameOver;
extern bool gameWin;
extern long startTime;
extern long lastUpdateTime;

// SDL 관련 변수
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern TTF_Font* font;
extern SDL_Texture* fishTextures[FISH_TYPE_COUNT];
extern SDL_Texture* deadFishTexture; // 죽은 물고기 텍스처 - 추가

// 오디오 관련 변수
extern SDL_AudioDeviceID audioDevice;
extern SDL_AudioSpec wavSpec;
extern Uint8* wavBuffer;
extern Uint32 wavLength;

// 함수 프로토타입 선언
void menuRender();
bool engine_init();
void initGame();
void renderText(const char* text, int x, int y);
void renderTextColor(const char* text, int x, int y, SDL_Color color); // 색상 지원 텍스트 함수 - 추가
void renderFishTanks();
void updateGame();
void renderGame();
void cleanupGame();
void handleInput(SDL_Event* e);
SDL_Texture* loadTexture(const char* path);
bool initAudio();
void playWaterSound();