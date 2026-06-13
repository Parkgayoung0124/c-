#pragma execution_character_set("utf-8") 
#include <stdio.h>
#include <SDL.h>
#include <SDL_ttf.h>    
#include <stdbool.h>            
#include "engine.h"

// [추가된 전역 변수] 메뉴 선택 위치 (0: 게임 시작, 1: 게임 나가기)
int menuSelection = 0;

bool engine_init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        return false;

    window = SDL_CreateWindow("Raising Fishes", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window)
        return false;

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        return false;
    }

    if (TTF_Init() != 0)
        return false;

    // 윈도우 기본 한글 폰트 로드 경로
    font = TTF_OpenFont("C:\\Windows\\Fonts\\malgun.ttf", 15);
    if (!font) {
        font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 15);
        if (!font) {
            printf("폰트 로드 실패\n");
            SDL_Quit();
            return false;
        }
    }

    // 물고기 텍스처 로드 (파일이 없어도 NULL 처리 후 진행)
    fishTextures[NORMAL] = loadTexture("normalfish.bmp");
    fishTextures[FAST] = loadTexture("fastfish.bmp");
    fishTextures[BIG] = loadTexture("bigfish.bmp");
    fishTextures[SPECIAL] = loadTexture("specialfish.bmp");
    fishTextures[SLOW] = loadTexture("slowfish.bmp");
    deadFishTexture = loadTexture("deadfish.bmp");

    initAudio();

    return true;
}

bool initAudio()
{
    if (SDL_LoadWAV("water.wav", &wavSpec, &wavBuffer, &wavLength) == NULL) {
        return false;
    }

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, 0);
    if (audioDevice == 0) {
        SDL_FreeWAV(wavBuffer);
        wavBuffer = NULL;
        return false;
    }

    SDL_PauseAudioDevice(audioDevice, 0);
    return true;
}

void playWaterSound()
{
    if (audioDevice != 0 && wavBuffer != NULL) {
        SDL_ClearQueuedAudio(audioDevice);
        SDL_QueueAudio(audioDevice, wavBuffer, wavLength);
    }
}

void initGame() {
    game.state = STATE_PLAYING;
    const char* defaultNames[] = { "니모", "골디", "버블", "핑키", "치코", "나비" };

    for (int i = 0; i < NUM; i++) {
        FishType type = (FishType)(rand() % FISH_TYPE_COUNT);
        fishTanks[i].type = type;
        fishTanks[i].fish = 1;
        fishTanks[i].water = 50;
        fishTanks[i].isAlive = 1;
        fishTanks[i].growth = 0.0;
        fishTanks[i].hp = 100;
        fishTanks[i].texture = fishTextures[type];
        sprintf_s(fishTanks[i].name, sizeof(fishTanks[i].name), "%s", defaultNames[i % 6]);

        switch (type) {
        case SLOW:     fishTanks[i].waterUse = 2;   break;
        case NORMAL:   fishTanks[i].waterUse = 5;   break;
        case SPECIAL:  fishTanks[i].waterUse = 5;   break;
        case FAST:     fishTanks[i].waterUse = 8;   break;
        case BIG:      fishTanks[i].waterUse = 14;  break;
        }
    }
    startTime = SDL_GetTicks();
    lastUpdateTime = startTime;
}

void renderGame() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    renderFishTanks();

    char levelText[64];
    sprintf_s(levelText, sizeof(levelText), "전체 레벨: %d", level);
    renderText(levelText, 10, 10);

    SDL_RenderPresent(renderer);
}

void cleanupGame() {
    for (int i = 0; i < FISH_TYPE_COUNT; i++) {
        if (fishTextures[i] != NULL) {
            SDL_DestroyTexture(fishTextures[i]);
            fishTextures[i] = NULL;
        }
    }
    if (deadFishTexture != NULL) {
        SDL_DestroyTexture(deadFishTexture);
        deadFishTexture = NULL;
    }

    if (audioDevice != 0) {
        SDL_CloseAudioDevice(audioDevice);
        audioDevice = 0;
    }

    if (wavBuffer != NULL) {
        SDL_FreeWAV(wavBuffer);
        wavBuffer = NULL;
    }

    if (font) {
        TTF_CloseFont(font);
        font = NULL;
    }

    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = NULL;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    TTF_Quit();
    SDL_Quit();
}

void renderTextColor(const char* text, int x, int y, SDL_Color color) {
    if (!font || !renderer || !text || text[0] == '\0') return;

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text, color);
    if (!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect dest = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dest);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}

void renderText(const char* text, int x, int y) {
    SDL_Color white = { 255, 255, 255, 255 };
    renderTextColor(text, x, y, white);
}

void renderFishTanks() {
    for (int i = 0; i < NUM; i++) {
        int x = 40 + i * (FISHTANK_WIDTH + 20);
        SDL_Rect bowl = { x, 280, FISHTANK_WIDTH, FISHTANK_HEIGHT };
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
        SDL_RenderDrawRect(renderer, &bowl);

        int waterHeight = fishTanks[i].water * FISHTANK_HEIGHT / 100;
        SDL_Rect water = { x, 280 + FISHTANK_HEIGHT - waterHeight, FISHTANK_WIDTH, waterHeight };
        SDL_SetRenderDrawColor(renderer, 0, 128, 255, 255);
        SDL_RenderFillRect(renderer, &water);

        // 상단 정보 패널
        char buf[64];
        SDL_Color textColor = { 255, 255, 255, 255 };
        if (!fishTanks[i].isAlive) textColor = (SDL_Color){ 120, 120, 120, 255 };

        sprintf_s(buf, sizeof(buf), "%s", fishTanks[i].name);
        renderTextColor(buf, x + 5, 150, textColor);

        const char* typeStr = "일반";
        switch (fishTanks[i].type) {
        case FAST:    typeStr = "신속"; break;
        case BIG:     typeStr = "대형"; break;
        case SPECIAL: typeStr = "특별"; break;
        case SLOW:    typeStr = "느림"; break;
        }
        sprintf_s(buf, sizeof(buf), "[%s]", typeStr);
        renderTextColor(buf, x + 5, 175, textColor);

        // 어항 상태 경고
        if (fishTanks[i].isAlive) {
            if (fishTanks[i].hp <= 30) {
                renderTextColor("위험!", x + 5, 205, (SDL_Color) { 255, 0, 0, 255 });
            }
            else if (fishTanks[i].water <= 30) {
                renderTextColor("물부족", x + 5, 205, (SDL_Color) { 255, 140, 0, 255 });
            }
            else if (fishTanks[i].water >= 90) {
                renderTextColor("급성장", x + 5, 205, (SDL_Color) { 0, 255, 0, 255 });
            }

            // 살아있는 물고기 출력
            SDL_Rect fishRect = { x + 10, 280 + FISHTANK_HEIGHT - waterHeight - 20, 80, 80 };
            if (fishTanks[i].texture) {
                SDL_RenderCopy(renderer, fishTanks[i].texture, NULL, &fishRect);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                SDL_RenderFillRect(renderer, &fishRect);
            }
        }
        else {
            renderTextColor("사망", x + 5, 205, (SDL_Color) { 128, 128, 128, 255 });

            // 죽은 물고기 출력
            SDL_Rect fishRect = { x + 10, 280 + FISHTANK_HEIGHT - 65, 80, 80 };
            if (deadFishTexture) {
                SDL_RenderCopy(renderer, deadFishTexture, NULL, &fishRect);
            }
            else {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &fishRect);
            }
        }

        // 하단 정보 패널 
        sprintf_s(buf, sizeof(buf), "HP: %d", fishTanks[i].hp);
        renderTextColor(buf, x + 5, 490, fishTanks[i].isAlive ? (SDL_Color) { 255, 100, 100, 255 } : textColor);

        sprintf_s(buf, sizeof(buf), "물: %d%%", fishTanks[i].water);
        renderTextColor(buf, x + 5, 510, fishTanks[i].isAlive ? (SDL_Color) { 100, 180, 255, 255 } : textColor);

        sprintf_s(buf, sizeof(buf), "레벨: %d", fishTanks[i].fish);
        renderTextColor(buf, x + 5, 530, textColor);

        sprintf_s(buf, sizeof(buf), "성장: %.0f%%", fishTanks[i].growth * 100.0);
        renderTextColor(buf, x + 5, 550, textColor);

        if (i == position) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &bowl);
        }
    }
}

// [수정된 함수] 시작 메뉴 화면 디자인 및 버튼 렌더링
void menuRender() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // 타이틀 제목
    SDL_Color titleColor = { 100, 180, 255, 255 };
    renderTextColor("★ 어항 키우기 게임 ★", 300, 180, titleColor);

    // 색상 정의 (선택된 메뉴는 노란색, 선택 안 된 메뉴는 흰색)
    SDL_Color selectedColor = { 255, 255, 0, 255 };
    SDL_Color normalColor = { 255, 255, 255, 255 };

    // 버튼 텍스트 색상 분기
    SDL_Color startBtnColor = (menuSelection == 0) ? selectedColor : normalColor;
    SDL_Color exitBtnColor = (menuSelection == 1) ? selectedColor : normalColor;

    // 메뉴 버튼 텍스트 그리기 (선택된 메뉴 앞에 '>' 표시 추가)
    if (menuSelection == 0) {
        renderTextColor("> 게임 시작", 350, 260, startBtnColor);
        renderTextColor("  게임 나가기", 350, 300, exitBtnColor);
    }
    else {
        renderTextColor("  게임 시작", 350, 260, startBtnColor);
        renderTextColor("> 게임 나가기", 350, 300, exitBtnColor);
    }

    // 하단 조작 가이드 안내
    SDL_Color guideColor = { 150, 150, 150, 255 };
    renderTextColor("(방향키 ↕ 로 이동 / Enter 키로 선택 / ESC 키로 종료)", 230, 400, guideColor);

    SDL_RenderPresent(renderer);
}

void updateGame() {
    long currentTime = SDL_GetTicks();
    long elapsed = (currentTime - lastUpdateTime) / 1000;
    if (elapsed > 0) {
        int aliveCount = 0;
        for (int i = 0; i < NUM; i++) {
            if (fishTanks[i].isAlive == 1) {
                fishTanks[i].water -= (fishTanks[i].waterUse * elapsed);
                fishTanks[i].water -= (level * elapsed);
                if (fishTanks[i].water < 0) fishTanks[i].water = 0;

                double waterModifier = 1.0;
                if (fishTanks[i].water >= 90) {
                    waterModifier = 2.0;
                }
                else if (fishTanks[i].water <= 30) {
                    waterModifier = 0.3;
                    fishTanks[i].hp -= (10 * elapsed);
                }
                else {
                    fishTanks[i].hp += (3 * elapsed);
                    if (fishTanks[i].hp > 100) fishTanks[i].hp = 100;
                }

                if (fishTanks[i].water > 0) {
                    double baseGrowthSpeed = 1.0;
                    if (fishTanks[i].type == FAST) {
                        baseGrowthSpeed = 2.0;
                    }

                    fishTanks[i].growth += (fishTanks[i].water / 100.0) * elapsed * baseGrowthSpeed * waterModifier;
                    if (fishTanks[i].growth >= 1.0) {
                        fishTanks[i].fish += 1;
                        fishTanks[i].growth = 0.0;
                    }
                }

                if (fishTanks[i].hp <= 0) {
                    fishTanks[i].hp = 0;
                    fishTanks[i].isAlive = 0;
                }
            }

            if (fishTanks[i].isAlive == 1) {
                aliveCount++;
            }
        }

        if (aliveCount == 0) {
            gameOver = true;
            game.state = STATE_GAME_OVER;
        }

        long totalElapsed = (currentTime - startTime) / 1000;
        if (totalElapsed / 20 > level - 1) {
            level++;
            if (level > 5) {
                level = 5;
                gameWin = true;
                running = false;
            }
        }

        lastUpdateTime = currentTime;
    }
}

// [수정된 함수] 메뉴 상태에서의 방향키 및 엔터/ESC 조작 매핑
void handleInput(SDL_Event* e) {
    if (e->type == SDL_KEYDOWN) {
        // [공통 조작] 어떤 화면에서든 ESC 키를 누르면 프로그램 즉시 종료
        if (e->key.keysym.sym == SDLK_ESCAPE) {
            running = false;
            return;
        }

        // 1. 시작 메뉴 화면일 때의 조작
        if (game.state == STATE_MENU) {
            switch (e->key.keysym.sym) {
            case SDLK_UP: // 위 방향키
            case SDLK_w:
                menuSelection = 0; // '게임 시작' 선택
                break;
            case SDLK_DOWN: // 아래 방향키
            case SDLK_s:
                menuSelection = 1; // '게임 나가기' 선택
                break;
            case SDLK_RETURN: // 엔터 키 입력시 실행
                if (menuSelection == 0) {
                    initGame(); // 게임 시작
                }
                else if (menuSelection == 1) {
                    running = false; // 게임 나가기 종료
                }
                break;
            }
        }
        // 2. 실제 게임 플레이 중일 때의 조작
        else if (game.state == STATE_PLAYING) {
            switch (e->key.keysym.sym) {
            case SDLK_j: // 왼쪽 어항 이동
            case SDLK_LEFT: // 방향키 왼쪽도 지원
                if (position > 0) position--;
                break;
            case SDLK_l: // 오른쪽 어항 이동
            case SDLK_RIGHT: // 방향키 오른쪽도 지원
                if (position < NUM - 1) position++;
                break;
            case SDLK_k: // 물 주기
            case SDLK_UP: // 플레이 중엔 위 방향키나 스페이스바로도 물 주기 가능하도록 배려
            case SDLK_SPACE:
                if (fishTanks[position].isAlive && fishTanks[position].water < 100) {
                    if (fishTanks[position].type == SPECIAL) {
                        fishTanks[position].water += 50;
                    }
                    else {
                        fishTanks[position].water += 5;
                    }
                    playWaterSound();
                    if (fishTanks[position].water > 100) fishTanks[position].water = 100;
                }
                break;
            }
        }
    }
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = SDL_LoadBMP(path);
    if (!surface) {
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}