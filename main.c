#include "engine.h"

// 전역 변수 정의
Game game;
FishTank fishTanks[NUM];
int level = 1;
int position = 0;
bool running = true;
bool gameOver = false;
bool gameWin = false;
long startTime = 0;
long lastUpdateTime = 0;

// SDL 관련 변수
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
SDL_Texture* fishTextures[FISH_TYPE_COUNT] = { NULL };
SDL_Texture* deadFishTexture = NULL; // 추가

// 오디오 관련 변수
SDL_AudioDeviceID audioDevice = 0;
SDL_AudioSpec wavSpec;
Uint8* wavBuffer = NULL;
Uint32 wavLength = 0;

int main(int argc, char* argv[]) {
    if (!engine_init()) {
        printf("Error initializing engine\n");
        return 1;
    }

    game.state = STATE_MENU;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;

            handleInput(&event);
        }

        if (game.state == STATE_MENU) {
            menuRender();
        }
        else if (game.state == STATE_PLAYING) {
            updateGame();
            renderGame();
        }
        else if (game.state == STATE_GAME_OVER) {
            running = false;
        }

        SDL_Delay(16);
    }

    cleanupGame();

    return 0;
}