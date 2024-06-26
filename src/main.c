#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include "stack.h"

#define USE_SUPER_CHIP_QUIRKS
#define RAM_SIZE 4096
#define TIMERS_RATE 60.0
#define FRAME_RATE 380.0
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 320
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define V_REG_COUNT 16
#define STACK_SIZE 16
#define KEYPAD_SIZE 16
#define VF v[0xF]


void init();
void draw();
void fetchExecute();
void handleInput();
void CleanUp_SDL();
uint32_t loadRom(char* file);

typedef struct int24{
    unsigned int data : 24;
} uint24 ;

char quit = 0;
uint8_t memory[RAM_SIZE];
uint16_t opcode; 
uint8_t v[V_REG_COUNT];
uint16_t I; 
uint16_t PC;
	
uint8_t gfx[DISPLAY_WIDTH * DISPLAY_HEIGHT];
uint8_t chip8_fontset[80] =
{
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
	
uint8_t delay_timer;
uint8_t sound_timer;

Stack stack;
uint16_t sp;

uint8_t keypad[KEYPAD_SIZE];	
	
uint_fast8_t drawflag;

SDL_Event windowEvent;
SDL_Window *win;
SDL_Renderer *renderer;

int main(int argc, char **argv)
{
    
    char * title = "CHIP-8";
    init();
    SDL_SetWindowTitle(win, (char(*)) & title);

    SDL_PollEvent(&windowEvent);
    // float dtime = DELTA_TIME_MILIS;
    SDL_SetWindowTitle(win, title);
    float frameTimeMilis = 1000.0 / FRAME_RATE;
    float timersCycleTimeMilis =  1000 / TIMERS_RATE;

    if (argc < 2) {
        CleanUp_SDL();
        return 1;
    }

    if (!loadRom(argv[1])){
        CleanUp_SDL();
        return 1;
    }
    unsigned int startTimeTimers = SDL_GetTicks();
    unsigned int startTimeFrames = SDL_GetTicks();
    while (!quit){
        unsigned int deltaTimeTimers = (SDL_GetTicks() - startTimeTimers);
        unsigned int deltaTimeFrames = SDL_GetTicks() - startTimeFrames;
        if ((float)deltaTimeFrames >= frameTimeMilis) {
            handleInput();
            fetchExecute();
            draw();
            startTimeFrames = SDL_GetTicks();
        }

        if ((float)deltaTimeTimers >= timersCycleTimeMilis){
            if (delay_timer > 0) {
                delay_timer--;
            }
            if (sound_timer > 0) {
                sound_timer--;
            }
            startTimeTimers = SDL_GetTicks();
        }
		
    }

    CleanUp_SDL();
    return 0;
}

//Initialize everything
void init()
{
    srand(SDL_GetTicks());
    delay_timer = 0;
	sound_timer = 0;
	opcode = 0;
	PC = 0x200;
	I = 0;
	sp = 0;
    initStack(&stack, STACK_SIZE, sizeof(uint16_t));
	memset(memory,0,RAM_SIZE);
	memset(v,0,V_REG_COUNT);
	memset(gfx,0,DISPLAY_HEIGHT*DISPLAY_WIDTH);
	memset(keypad,0,KEYPAD_SIZE);
	// Load fonts
	memcpy(memory,chip8_fontset,80*sizeof(int8_t));

    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_CreateWindowAndRenderer(WINDOW_WIDTH, WINDOW_HEIGHT, 0, &win, &renderer); 
    SDL_RenderSetScale(renderer, 1, 1);
    // start = SDL_GetTicks();
}

void fetchExecute(){
    uint8_t x, y, n, nn;
	uint16_t nnn;
    // fetch
    opcode = memory[PC] << 8 | memory[PC + 1];
	PC +=2;

    // decode
    x = (opcode & 0x0F00) >> 8;
    y = (opcode & 0X00F0) >> 4;
    n = (opcode & 0x000F);
    nn = (opcode & 0x00FF);
    nnn = (opcode & 0x0FFF);

    // execute
    switch ((opcode & 0xF000) >> 12){

        
        case 0x0000:
            switch (nn)
            {
            // 00E0 clear screen
            case  0xE0:
                memset(gfx,0,DISPLAY_HEIGHT*DISPLAY_WIDTH);
                break;
            
            // 00EE return from call
            case 0xEE:
                PC = *((uint16_t *)pop(&stack));
                break;
            }
            break;

        // 1NNN jump
        case 0x1:
            PC = nnn;
            break;
        
        // 2NNN call
        case 0x2:
            if (push(&stack, &PC)){
                PC = nnn;
            } else {
                printf("stack overflow");
                quit = true;
                return;
            }
            break;

        // 3XNN skip if vx == nn
        case 0x3:
            if (v[x] == nn) {
                PC += 2;
            }
            break;
        
        // 4XNN skip if vx != nn
        case 0x4:
            if (v[x] != nn) {
                PC += 2;
            }
            break;
        
        // 5XY0 skip if vx == vy
        case 0x5:
            if (v[x] == v[y]) {
                PC += 2;
            }
            break;

        // 6XNN set vx register
        case 0x6:
            v[x] = nn;
            break;

        // 7XNN add to vx register no carry
        case 0x7:
            v[x] += nn;
            break;

        case 0x8:
            switch (n) {
                // 8XY0 set vx to vy
                case 0x0:
                    v[x] = v[y];
                    break;

                // 8XY1 set vx to binary vx or vy
                case 0x1:
                    v[x] = (v[x] | v[y]);
                    break;
                
                // 8XY2 set vx to binary vx or vy
                case 0x2:
                    v[x] = (v[x] & v[y]);
                    break;

                // 8XY3 set vx to binary vx xor vy
                case 0x3:
                    v[x] = (v[x] ^ v[y]);
                    break;

                // 8XY4 add vy to vx with carry
                case 0x4:
                    if (v[x] > 0 && v[y] > 0xFF - v[x]) {
                        VF = 1;
                    } else {
                        VF = 0;
                    }
                    v[x] += v[y];
                    break;

                // 8XY5 set vx = vx - vy with carry
                case 0x5:
                    if (v[x] < v[y]) {
                        VF = 0;
                    } else {
                        VF = 1;
                    }
                    v[x] -= v[y];
                    break;
                
                // 8XY6 right shift vx 
                case 0x6:
                    #ifndef USE_SUPER_CHIP_QUIRKS
                    v[x] = v[y];
                    #endif
                    VF = (0x01 & v[x]);
                    v[x] >>= 1;
                    break;
                
                // 8XY7 set vx = vy - vx with carry
                case 0x7:
                    if (v[y] < v[x]) {
                        VF = 0;
                    } else {
                        VF = 1;
                    }
                    v[x] = v[y] - v[x];
                    break;
                
                // 8XYE right shift vx 
                case 0xE:
                    #ifndef USE_SUPER_CHIP_QUIRKS
                    v[x] = v[y];
                    #endif
                    VF = (0x80 & v[x]);
                    v[x] <<= 1;
                    break;
            }
            break;

        // 9XY0 skip if vx != vy
        case 0x9:
            if (v[x] != v[y]) {
                PC += 2;
            }
            break;

        // ANNN set index reg I
        case 0xA:
            I = nnn;
            break;

        // BNNN jump with offset
        case 0xB:
            #ifndef USE_SUPER_CHIP_QUIRKS
            PC = v[0] + nnn;
            #else
            PC = v[x] + nnn;
            #endif
            break;
        
        // CXNN set vx to random & nn
        case 0xC:
            uint8_t r = (uint8_t)rand();
            v[x] = r & nn;
            break;

        // DXYN draw to screen
        case 0xD:
            uint8_t cx = v[x] % DISPLAY_WIDTH;
            uint8_t cy = v[y] % DISPLAY_HEIGHT;
            VF = 0;
            
            for (int i = 0; i < n; i++){
                uint8_t spriteData = memory[I + i];
                if ((cy + i) > DISPLAY_HEIGHT) {
                    break;
                }
                for (int j = 0; j < 8; j++) {
                    if ((cx + j) > DISPLAY_WIDTH) {
                        break;
                    }
                    if (((spriteData << j) & 0x80)) { // if sprite pixel on
                        if (gfx[cx + j + ((cy + i)*DISPLAY_WIDTH)]) { // if screen pixel on
                            gfx[cx + j + ((cy + i)*DISPLAY_WIDTH)] = 0;
                            VF = 1;
                        } else { // if screen pixel is off
                            gfx[cx + j + ((cy + i)*DISPLAY_WIDTH)] = 1;
                        }

                    }
                }
            }
            break;
        
        case 0xE:
            if (v[x] > 0x0F){
                printf("key checked does not exists");
                quit = true;
                return;
            }
            switch (nn)
            {
                // EX9E skip if key corresponding to vx is pressed
                case 0x9E:
                    if (keypad[v[x]]){
                        PC += 2;
                    }
                break;

                // EXA1 skip if key corresponding to vx is not pressed
                case 0xA1:
                 if (!(keypad[v[x]])){
                        PC += 2;
                    }
                break;

            }
            break;
        
        case 0xF:
            switch (nn)
            {
                // FX07 set vx to time in delay timer
                case 0x07:
                v[x] = delay_timer;
                break;

                // FX15 set delay timer to vx
                case 0x15:
                delay_timer = v[x];
                break;

                // FX18 set sound timer to vx
                case 0x18:
                sound_timer = v[x];
                break;

                // FX1E add vx to I
                case 0x1E:
                #ifdef USE_SUPER_CHIP_QUIRKS
                VF = ((I+x) >= 0x1000);
                #endif
                I += x;
                break;

                // FX0A wait for vx keypress and release
                case 0x0A:
                static bool lastKeyState = false;
                if (v[x] > 0x0F){
                    printf("key does not exist");
                    quit = true;
                    return;
                }
                if (!(lastKeyState && !((bool)(keypad[x])))) {
                    lastKeyState = keypad[x];
                    PC -= 2;
                } else {
                    lastKeyState = false;
                }
                break;

                // FX29 set I to font character 
                case 0x29:
                I = (0x0F & v[x]);
                break;

                // FX33 convert vx to decimal and save digits at I
                case 0x33:
                uint8_t vx = v[x];
                for (int i = 2; i >= 0; i--){
                    memory[I+i] = memory[vx % 10];
                    vx /= 10;
                }
                break;

                // FX55 save v0 to vx (including vx) to memory
                case 0x55:
                memcpy((memory + I), v, x + 1);
                #ifndef USE_SUPER_CHIP_QUIRKS
                I += x + 1;
                #endif
                break;

                // FX65 read from memory to v0 to vx (including vx) 
                case 0x56:
                memcpy(v, (memory + I), x + 1);
                #ifndef USE_SUPER_CHIP_QUIRKS
                I += x + 1;
                #endif
                break;

            }
            break;
        
            default:
            break;

    }


}

void draw(){
    SDL_RenderClear(renderer);
    SDL_Surface * canvas = SDL_CreateRGBSurfaceWithFormat(0, DISPLAY_WIDTH, DISPLAY_HEIGHT, 1, SDL_PIXELFORMAT_BGR24);
    unsigned char * pixelsBuffer = (unsigned char*)canvas -> pixels;
    for (unsigned int i = 0; i < DISPLAY_HEIGHT*DISPLAY_WIDTH; i++ ){
        if (((uint8_t *)gfx)[i]){
            (*(uint24 *)(pixelsBuffer + (i*3))).data = 0xFFFFFF;
        } else {
            (*(uint24 *)(pixelsBuffer + (i*3))).data = 0;
        }
        
    }
    SDL_Texture * texture = SDL_CreateTextureFromSurface(renderer, canvas);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(canvas);
}


void CleanUp_SDL(){
    SDL_DestroyWindow(win);
    SDL_Quit();
}

// Load rom file into memory
uint32_t loadRom(char* file)
{
	FILE * fp = fopen(file,"rb");
	
	if(fp == NULL)
	{
		fprintf(stderr,"Can't open the file rom \n");
		return 0;
	}	
	
	fseek(fp, 0, SEEK_END);
	int size = ftell(fp); 
	fseek(fp, 0 ,SEEK_SET);

	fread(memory+0x200,sizeof(uint16_t),size,fp);
	
	return 1;
}


void handleInput(){
    if (SDL_PollEvent(&windowEvent)) {
        switch(windowEvent.type) {
            case SDL_QUIT:
            quit = true;
            break;

            case SDL_KEYDOWN:

            switch (windowEvent.key.keysym.sym)
            {
                case SDLK_ESCAPE:quit = true;break;
                // case SDLK_F2:speed -= 1;break;
                // case SDLK_F3:speed += 1;break;
                case SDLK_x:keypad[0] = 1;break;
                case SDLK_1:keypad[1] = 1;break;
                case SDLK_2:keypad[2] = 1;break;
                case SDLK_3:keypad[3] = 1;break;
                case SDLK_q:keypad[4] = 1;break;
                case SDLK_w:keypad[5] = 1;break;
                case SDLK_e:keypad[6] = 1;break;
                case SDLK_a:keypad[7] = 1;break;
                case SDLK_s:keypad[8] = 1;break;
                case SDLK_d:keypad[9] = 1;break;
                case SDLK_z:keypad[0xA] = 1;break;
                case SDLK_c:keypad[0xB] = 1;break;
                case SDLK_4:keypad[0xC] = 1;break;
                case SDLK_r:keypad[0xD] = 1;break;
                case SDLK_f:keypad[0xE] = 1;break;
                case SDLK_v:keypad[0xF] = 1;break;
            }
            break;
            
            case SDL_KEYUP:
            
            switch (windowEvent.key.keysym.sym)
            {
                case SDLK_x:keypad[0] = 0;break;
                case SDLK_1:keypad[1] = 0;break;
                case SDLK_2:keypad[2] = 0;break;
                case SDLK_3:keypad[3] = 0;break;
                case SDLK_q:keypad[4] = 0;break;
                case SDLK_w:keypad[5] = 0;break;
                case SDLK_e:keypad[6] = 0;break;
                case SDLK_a:keypad[7] = 0;break;
                case SDLK_s:keypad[8] = 0;break;
                case SDLK_d:keypad[9] = 0;break;
                case SDLK_z:keypad[0xA] = 0;break;
                case SDLK_c:keypad[0xB] = 0;break;
                case SDLK_4:keypad[0xC] = 0;break;
                case SDLK_r:keypad[0xD] = 0;break;
                case SDLK_f:keypad[0xE] = 0;break;
                case SDLK_v:keypad[0xF] = 0;break;
            }
            break;
        }
    }
}

