// =============================================================================
// rasingFish_final.c
// SDL2 기반 물고기 키우기 게임 - 기말 과제 최종 버전
//
// [필수 기능 1] 물고기 종류 추가: Normal / Fast / Big
// [필수 기능 2] 물고기 종류별 물 소비량 및 성장 속도 차이
// [선택 기능 1] 물고기 상태(물 양)에 따른 이미지 색상 변경
// [선택 기능 2] 상황별 사운드 효과: 물 주기, 물고기 죽음, 레벨업
// =============================================================================

#include <SDL.h>
#include <SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

// M_PI 가 정의되지 않은 컴파일러(MSVC 등)를 위해 직접 정의
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─────────────────────────────────────────────────────────────────────────────
// 상수 정의
// ─────────────────────────────────────────────────────────────────────────────
#define SCREEN_WIDTH    800     // 화면 가로 크기 (픽셀)
#define SCREEN_HEIGHT   600     // 화면 세로 크기 (픽셀)
#define FISHTANK_WIDTH  100     // 어항 가로 크기
#define FISHTANK_HEIGHT 200     // 어항 세로 크기
#define NUM             6       // 어항(물고기) 수
#define SAMPLE_RATE     44100   // 오디오 샘플링 레이트 (Hz)

// ─────────────────────────────────────────────────────────────────────────────
// [필수 기능 1] 물고기 종류 상수
//   FISH_NORMAL: 기본 물고기 - 보통 성장, 보통 물 소비 (배율 x1)
//   FISH_FAST  : 빠른 물고기 - 2배 빠른 성장, 보통 물 소비
//   FISH_BIG   : 큰 물고기   - 보통 성장, 2배 물 소비
// ─────────────────────────────────────────────────────────────────────────────
#define FISH_NORMAL     0
#define FISH_FAST       1
#define FISH_BIG        2
#define FISH_TYPE_COUNT 3

// 화면에 표시할 물고기 종류 이름 (축약 / 전체)
static const char* FISH_TYPE_SHORT[FISH_TYPE_COUNT]    = { "Nml",    "Fst",   "Big"  };
static const char* FISH_TYPE_FULL[FISH_TYPE_COUNT]     = { "Normal", "Fast",  "Big"  };

// [필수 기능 2] 물고기 종류별 물 소비 배율 (클수록 물을 빨리 소비)
static const int WATER_CONSUME_RATE[FISH_TYPE_COUNT] = { 1, 1, 2 };

// [필수 기능 2] 물고기 종류별 성장 배율 (클수록 물고기가 빨리 자람)
static const int FISH_GROWTH_RATE[FISH_TYPE_COUNT]   = { 1, 2, 1 };

// ─────────────────────────────────────────────────────────────────────────────
// 구조체 정의
// ─────────────────────────────────────────────────────────────────────────────

// [필수 기능 1] fishType 필드 추가 - 어항마다 서로 다른 물고기 종류 저장
typedef struct {
    int fish;       // 물고기 크기 (0 ~ 100)
    int water;      // 물의 양   (0 ~ 100)
    int isAlive;    // 생존 여부 (1: 살아있음, 0: 죽음)
    int fishType;   // 물고기 종류 (FISH_NORMAL / FISH_FAST / FISH_BIG)
} FishTank;

// [선택 기능 2] 사운드 데이터를 한 쌍으로 관리하는 구조체
typedef struct {
    Uint8* buffer;  // 소리 PCM 데이터 포인터
    Uint32 length;  // 데이터 길이 (바이트)
} SoundBuffer;

// ─────────────────────────────────────────────────────────────────────────────
// 전역 변수
// ─────────────────────────────────────────────────────────────────────────────
FishTank fishTanks[NUM];        // 어항 배열
int  level         = 1;         // 현재 레벨 (1 ~ 5)
int  position      = 0;         // 현재 선택된 어항 인덱스 (커서)
bool running       = true;      // 게임 루프 실행 플래그
bool gameOver      = false;     // 게임 오버 플래그
bool gameWin       = false;     // 게임 승리 플래그
long startTime     = 0;         // 게임 시작 시각 (ms, SDL_GetTicks 기준)
long lastUpdateTime= 0;         // 마지막 updateGame() 호출 시각 (ms)

// SDL 관련
SDL_Window*    window      = NULL;
SDL_Renderer*  renderer    = NULL;
TTF_Font*      font        = NULL;
SDL_Texture*   fishTexture = NULL;  // fish.bmp 로 만든 텍스처

// 오디오 관련
SDL_AudioDeviceID audioDevice  = 0;
SoundBuffer       waterSound   = { NULL, 0 };   // 물 주기 효과음
SoundBuffer       deathSound   = { NULL, 0 };   // 물고기 죽음 효과음
SoundBuffer       levelUpSound = { NULL, 0 };   // 레벨업 효과음

// ─────────────────────────────────────────────────────────────────────────────
// 함수 프로토타입
// ─────────────────────────────────────────────────────────────────────────────
bool         engine_init();
void         initGame();
void         renderText(const char* text, int x, int y);
void         renderFishTanks();
void         updateGame();
void         renderGame();
void         cleanupGame();
void         handleInput(SDL_Event* e);
SDL_Texture* loadTexture(const char* path);
bool         initAudio();
SoundBuffer  generateTone(float frequency, float duration, float amplitude);
void         playSound(SoundBuffer* sb);
void         playWaterSound();
void         playDeathSound();
void         playLevelUpSound();

// =============================================================================
// main
// =============================================================================
int main(int argc, char* argv[]) {
    // 엔진(SDL + TTF + 오디오) 초기화
    if (!engine_init()) {
        printf("엔진 초기화 실패: %s\n", SDL_GetError());
        return 1;
    }

    initGame(); // 게임 상태 초기화

    // ── 메인 게임 루프 ──────────────────────────────────────────────────────
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                running = false;
            handleInput(&event);    // 키보드 입력 처리
        }
        updateGame();   // 게임 상태(물, 물고기, 레벨) 갱신
        renderGame();   // 화면 그리기
        SDL_Delay(100); // 100ms 대기 → 약 10 FPS
    }
    // ────────────────────────────────────────────────────────────────────────

    cleanupGame(); // 종료 화면 출력 + 리소스 해제
    return 0;
}

// =============================================================================
// engine_init : SDL / TTF / 오디오 초기화
// =============================================================================
bool engine_init() {
    // SDL 비디오 + 오디오 서브시스템 동시 초기화
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0)
        return false;

    // 창 생성 (제목, 위치: 화면 중앙, 크기, 플래그)
    window = SDL_CreateWindow(
        "Raising Fishes - Final",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH, SCREEN_HEIGHT, 0
    );
    if (!window) return false;

    // 하드웨어 가속 렌더러 생성
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;

    // SDL_ttf 초기화
    if (TTF_Init() != 0) return false;

    // 폰트 로드 (Windows 기본 Arial, 18pt)
    font = TTF_OpenFont("C:\\Windows\\Fonts\\arial.ttf", 18);
    if (!font) {
        printf("폰트 로드 실패: %s\n", TTF_GetError());
        return false;
    }

    // 물고기 BMP 이미지를 GPU 텍스처로 로드
    fishTexture = loadTexture("fish.bmp");
    if (!fishTexture) return false;

    // 오디오 초기화 (실패해도 게임은 소리 없이 계속 진행)
    if (!initAudio())
        printf("오디오 초기화 실패 - 소리 없이 진행합니다.\n");

    return true;
}

// =============================================================================
// initAudio : 오디오 장치 열기 + WAV 로드 + 효과음 생성
// =============================================================================
bool initAudio() {
    // 원하는 오디오 형식 지정: 44100Hz, 16비트 부호 정수, 모노
    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq     = SAMPLE_RATE;
    desired.format   = AUDIO_S16SYS; // 시스템 바이트 순서의 16비트 정수
    desired.channels = 1;            // 모노
    desired.samples  = 4096;         // 버퍼 크기

    // 기본 오디오 출력 장치 열기 (NULL = 시스템 기본 장치)
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, 0);
    if (audioDevice == 0) {
        printf("오디오 장치 열기 실패: %s\n", SDL_GetError());
        return false;
    }
    SDL_PauseAudioDevice(audioDevice, 0); // 0 = 재생 시작

    // ── water.wav 로드 및 형식 변환 ─────────────────────────────────────────
    SDL_AudioSpec wavSpec;
    Uint8*  rawBuf = NULL;
    Uint32  rawLen = 0;
    if (SDL_LoadWAV("water.wav", &wavSpec, &rawBuf, &rawLen) != NULL) {
        SDL_AudioCVT cvt;
        // 로드된 WAV 형식 → 장치 형식(S16SYS, 모노, 44100) 변환 준비
        int needConv = SDL_BuildAudioCVT(
            &cvt,
            wavSpec.format, wavSpec.channels, wavSpec.freq,  // 원본 형식
            AUDIO_S16SYS,   1,               SAMPLE_RATE     // 목표 형식
        );
        if (needConv > 0) {
            // 변환이 필요한 경우: 새 버퍼 할당 후 변환 수행
            cvt.buf = (Uint8*)SDL_malloc(rawLen * cvt.len_mult);
            SDL_memcpy(cvt.buf, rawBuf, rawLen);
            cvt.len = rawLen;
            SDL_ConvertAudio(&cvt);          // PCM 형식 변환
            waterSound.buffer = cvt.buf;
            waterSound.length = cvt.len_cvt;
            SDL_FreeWAV(rawBuf);             // 원본 버퍼 해제
        } else {
            // 변환 불필요: 그대로 사용
            waterSound.buffer = rawBuf;
            waterSound.length = rawLen;
        }
    } else {
        printf("water.wav 로드 실패: %s\n", SDL_GetError());
    }

    // ── 프로그래매틱 효과음 생성 ─────────────────────────────────────────────
    // 죽음 효과음: 220Hz 낮은 음, 1초 지속, 중간 볼륨
    deathSound   = generateTone(220.0f, 1.0f, 0.5f);
    // 레벨업 효과음: 880Hz 높은 음, 0.4초 지속, 높은 볼륨
    levelUpSound = generateTone(880.0f, 0.4f, 0.7f);

    return true;
}

// =============================================================================
// generateTone : 사인파 기반 PCM 효과음 생성 (지수 감쇠 엔벨로프)
//   frequency : 음 높이 (Hz)
//   duration  : 지속 시간 (초)
//   amplitude : 볼륨 (0.0 ~ 1.0)
// =============================================================================
SoundBuffer generateTone(float frequency, float duration, float amplitude) {
    SoundBuffer sb = { NULL, 0 };
    int numSamples = (int)(SAMPLE_RATE * duration);   // 총 샘플 수
    sb.length = (Uint32)(numSamples * sizeof(Sint16)); // 16비트 × 샘플 수
    sb.buffer = (Uint8*)SDL_malloc(sb.length);
    if (!sb.buffer) { sb.length = 0; return sb; }

    Sint16* samples = (Sint16*)sb.buffer;
    for (int i = 0; i < numSamples; i++) {
        float t = (float)i / SAMPLE_RATE;
        // 지수 감쇠: 소리가 시간이 지남에 따라 자연스럽게 줄어들도록
        float envelope = expf(-t * 5.0f);
        float value    = sinf(2.0f * (float)M_PI * frequency * t) * envelope * amplitude;
        samples[i] = (Sint16)(value * 32767.0f); // float → Sint16 변환
    }
    return sb;
}

// =============================================================================
// 사운드 재생 함수들 (SDL_QueueAudio 방식)
// =============================================================================
// 범용 재생: 큐에 남은 소리를 지우고 새 소리를 큐에 추가
void playSound(SoundBuffer* sb) {
    if (audioDevice == 0 || !sb || !sb->buffer) return;
    SDL_ClearQueuedAudio(audioDevice);              // 이전 소리 데이터 제거
    SDL_QueueAudio(audioDevice, sb->buffer, sb->length); // 새 소리 큐에 추가
}
void playWaterSound()  { playSound(&waterSound);   }
void playDeathSound()  { playSound(&deathSound);   }
void playLevelUpSound(){ playSound(&levelUpSound); }

// =============================================================================
// initGame : 게임 상태 초기화
// =============================================================================
void initGame() {
    for (int i = 0; i < NUM; i++) {
        fishTanks[i].fish    = 10;
        fishTanks[i].water   = 100;
        fishTanks[i].isAlive = 1;
        // [필수 기능 1] 어항마다 물고기 종류를 순환 배정
        //   i=0 → Normal, i=1 → Fast, i=2 → Big,
        //   i=3 → Normal, i=4 → Fast, i=5 → Big
        fishTanks[i].fishType = i % FISH_TYPE_COUNT;
    }
    startTime       = SDL_GetTicks(); // 게임 시작 시각 기록
    lastUpdateTime  = startTime;
}

// =============================================================================
// updateGame : 매 프레임 게임 상태 갱신
// =============================================================================
void updateGame() {
    long currentTime = SDL_GetTicks();
    long elapsed     = (currentTime - lastUpdateTime) / 1000; // 경과 시간 (초)

    if (elapsed <= 0) return; // 1초가 지나지 않으면 갱신 건너뜀

    int aliveCount = 0;
    for (int i = 0; i < NUM; i++) {
        if (fishTanks[i].isAlive != 1) continue;

        int type = fishTanks[i].fishType;

        // [필수 기능 2] 물 소비량 계산
        //   기본식: level × (물고기크기/20 + 1) × 경과초
        //   종류별 배율 적용: Normal×1, Fast×1, Big×2
        int consume = level
                    * (fishTanks[i].fish / 20 + 1)
                    * (int)elapsed
                    * WATER_CONSUME_RATE[type];
        fishTanks[i].water -= consume;

        if (fishTanks[i].water <= 0) {
            fishTanks[i].water   = 0;
            fishTanks[i].isAlive = 0;   // 물 소진 → 사망
            playDeathSound();            // [선택 기능 2] 죽음 효과음 재생
        }

        // [필수 기능 2] 물고기 성장 계산
        //   물이 있을 때만 성장, 종류별 성장 배율 적용: Normal×1, Fast×2, Big×1
        if (fishTanks[i].water > 0) {
            int grow = (fishTanks[i].water / 100 + 1)
                     * (int)elapsed
                     * FISH_GROWTH_RATE[type];
            fishTanks[i].fish += grow;
            if (fishTanks[i].fish > 100) fishTanks[i].fish = 100; // 최대 100
        }

        aliveCount++;
    }

    // 모든 물고기가 죽으면 게임 오버
    if (aliveCount == 0) {
        gameOver = true;
        running  = false;
    }

    // 레벨업 조건: 20초마다 레벨 +1 (최대 5)
    long totalElapsed  = (currentTime - startTime) / 1000;
    int  expectedLevel = (int)(totalElapsed / 20) + 1;
    if (expectedLevel > level) {
        int prevLevel = level;
        level = (expectedLevel > 5) ? 5 : expectedLevel;
        if (level > prevLevel) {
            playLevelUpSound(); // [선택 기능 2] 레벨업 효과음 재생
        }
        if (level >= 5) {
            level   = 5;
            gameWin = true;
            running = false;
        }
    }

    lastUpdateTime = currentTime; // 마지막 업데이트 시각 갱신
}

// =============================================================================
// renderGame : 전체 화면 렌더링
// =============================================================================
void renderGame() {
    SDL_SetRenderDrawColor(renderer, 20, 20, 40, 255); // 짙은 남색 배경
    SDL_RenderClear(renderer);

    renderFishTanks(); // 어항 6개 그리기

    // 현재 레벨 표시 (좌상단)
    char levelText[64];
    sprintf_s(levelText, sizeof(levelText), "Level %d / 5  (20sec per level)", level);
    renderText(levelText, 10, 10);

    // 조작 방법 안내
    renderText("J: Left   L: Right   K: Add Water   ESC: Quit", 10, 35);

    SDL_RenderPresent(renderer); // 렌더링 결과를 화면에 출력
}

// =============================================================================
// renderFishTanks : 어항 6개 렌더링
// =============================================================================
void renderFishTanks() {
    // 범례: 물고기 종류별 색상 안내
    renderText("Green = Normal (x1)   Blue = Fast (growth x2)   Red = Big (water x2)",
               10, 560);

    for (int i = 0; i < NUM; i++) {
        int x = 50 + i * (FISHTANK_WIDTH + 10); // 어항 X 시작 좌표

        // ── 어항 테두리 ───────────────────────────────────────────────────────
        SDL_Rect bowl = { x, 280, FISHTANK_WIDTH, FISHTANK_HEIGHT };
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255); // 파란색 테두리
        SDL_RenderDrawRect(renderer, &bowl);

        // ── 물 채우기 (수위에 비례) ───────────────────────────────────────────
        int waterHeight = fishTanks[i].water * FISHTANK_HEIGHT / 100;
        SDL_Rect waterRect = {
            x + 1,
            280 + FISHTANK_HEIGHT - waterHeight,
            FISHTANK_WIDTH - 2,
            waterHeight
        };
        SDL_SetRenderDrawColor(renderer, 0, 100, 220, 200);
        SDL_RenderFillRect(renderer, &waterRect);

        // ── 물고기 이미지 (살아있는 경우) ────────────────────────────────────
        if (fishTanks[i].isAlive && fishTexture != NULL) {
            // [선택 기능 1] 물 양에 따라 이미지 색상 틴트 변경
            //   물 > 60: 정상(흰색), 30~60: 경고(노란색), < 30: 위급(빨간색)
            if (fishTanks[i].water > 60)
                SDL_SetTextureColorMod(fishTexture, 255, 255, 255); // 정상: 흰색
            else if (fishTanks[i].water > 30)
                SDL_SetTextureColorMod(fishTexture, 255, 220, 50);  // 경고: 노란색
            else
                SDL_SetTextureColorMod(fishTexture, 255, 80,  80);  // 위급: 빨간색

            // 물고기를 수면 바로 위에 배치
            SDL_Rect fishRect = {
                x + 20,
                280 + FISHTANK_HEIGHT - waterHeight - 30,
                60, 30
            };
            SDL_RenderCopy(renderer, fishTexture, NULL, &fishRect);
            SDL_SetTextureColorMod(fishTexture, 255, 255, 255); // 틴트 초기화
        }

        // ── 상태 텍스트 (어항 하단) ──────────────────────────────────────────
        char line1[32], line2[32];
        if (fishTanks[i].isAlive) {
            // [필수 기능 1] 물고기 종류명 표시
            sprintf_s(line1, sizeof(line1), "%s", FISH_TYPE_FULL[fishTanks[i].fishType]);
            sprintf_s(line2, sizeof(line2), "F:%d W:%d", fishTanks[i].fish, fishTanks[i].water);
        } else {
            sprintf_s(line1, sizeof(line1), "DEAD");
            line2[0] = '\0';
        }
        renderText(line1, x + 5, 490);
        if (line2[0] != '\0')
            renderText(line2, x + 5, 510);

        // ── 물고기 종류 색상 인디케이터 (어항 상단 바) ──────────────────────
        SDL_Rect indicator = { x + 25, 268, 50, 8 };
        switch (fishTanks[i].fishType) {
        case FISH_NORMAL: SDL_SetRenderDrawColor(renderer,  80, 220,  80, 255); break; // 초록
        case FISH_FAST:   SDL_SetRenderDrawColor(renderer,  80,  80, 220, 255); break; // 파랑
        case FISH_BIG:    SDL_SetRenderDrawColor(renderer, 220,  80,  80, 255); break; // 빨강
        }
        SDL_RenderFillRect(renderer, &indicator);

        // ── 선택된 어항 커서 강조 (이중 노란 테두리) ─────────────────────────
        if (i == position) {
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDrawRect(renderer, &bowl);
            SDL_Rect outer = { x - 2, 278, FISHTANK_WIDTH + 4, FISHTANK_HEIGHT + 4 };
            SDL_RenderDrawRect(renderer, &outer);
        }
    }
}

// =============================================================================
// cleanupGame : 종료 메시지 표시 + 리소스 해제
// =============================================================================
void cleanupGame() {
    // 최종 결과 화면 표시 (3초간)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    if (gameWin)
        renderText("You Win! All 5 levels completed!", 220, 280);
    else if (gameOver)
        renderText("Game Over! All fish are dead!", 220, 280);
    else
        renderText("Game Over", 320, 280);
    SDL_RenderPresent(renderer);
    SDL_Delay(3000); // 3초 표시

    // 텍스처 해제
    if (fishTexture) SDL_DestroyTexture(fishTexture);

    // 오디오 리소스 해제
    if (audioDevice  != 0)   SDL_CloseAudioDevice(audioDevice);
    if (waterSound.buffer)   SDL_FreeWAV(waterSound.buffer);   // SDL_LoadWAV 할당
    if (deathSound.buffer)   SDL_free(deathSound.buffer);       // SDL_malloc 할당
    if (levelUpSound.buffer) SDL_free(levelUpSound.buffer);     // SDL_malloc 할당

    // SDL / TTF 종료
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

// =============================================================================
// renderText : 텍스트를 화면 좌표 (x, y)에 흰색으로 그리기
// =============================================================================
void renderText(const char* text, int x, int y) {
    if (!font || !text || text[0] == '\0') return;
    SDL_Color white = { 255, 255, 255, 255 };
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, white); // 텍스트 → Surface
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface); // Surface → Texture
    SDL_Rect dest = { x, y, surface->w, surface->h };
    SDL_RenderCopy(renderer, texture, NULL, &dest); // Texture → 화면
    SDL_FreeSurface(surface);    // Surface 메모리 해제
    SDL_DestroyTexture(texture); // Texture 메모리 해제
}

// =============================================================================
// handleInput : 키보드 입력 처리
// =============================================================================
void handleInput(SDL_Event* e) {
    if (e->type != SDL_KEYDOWN) return;
    switch (e->key.keysym.sym) {
    case SDLK_j:        // J: 왼쪽 어항으로 커서 이동
        if (position > 0) position--;
        break;
    case SDLK_l:        // L: 오른쪽 어항으로 커서 이동
        if (position < NUM - 1) position++;
        break;
    case SDLK_k:        // K: 선택된 어항에 물 주기 (+5)
        // 살아있고 물이 가득 차지 않은 경우에만 물 주기 가능
        if (fishTanks[position].isAlive && fishTanks[position].water < 100) {
            fishTanks[position].water += 5;
            if (fishTanks[position].water > 100) fishTanks[position].water = 100;
            playWaterSound(); // [선택 기능 2] 물 주기 효과음 재생
        }
        break;
    case SDLK_ESCAPE:   // ESC: 게임 종료
        running = false;
        break;
    }
}

// =============================================================================
// loadTexture : BMP 파일을 GPU 텍스처로 로드
// =============================================================================
SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* surface = SDL_LoadBMP(path); // BMP → CPU 메모리(Surface)
    if (!surface) {
        printf("이미지 로드 실패 (%s): %s\n", path, SDL_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface); // Surface → GPU Texture
    SDL_FreeSurface(surface); // Surface 메모리 해제 (Texture 이후 불필요)
    return texture;
}
