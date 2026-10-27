/*
 * Fish Frenzy: PS2 homebrew adaptation
 * A lightweight, controller-driven feeding-frenzy arcade game.
 * Graphics are drawn from GS primitives; no external art/audio files needed.
 */
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <libpad.h>
#include <gsKit.h>
#include <dmaKit.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SW 640
#define MAX_ENEMIES 9
#define MAX_BUBBLES 12
#define PLAYER_SPEED 3.1f
#define SCORE_STEP 350

typedef enum { TITLE, PLAYING, PAUSED, GAME_OVER } Screen;
typedef struct { float x, y, r, vx; int kind, alive; } Fish;
typedef struct { float x, y, speed; } Bubble;

static GSGLOBAL *gs;
extern unsigned char asset_menu_bg[], asset_reef_bg[], asset_city_bg[];
extern unsigned char asset_logo[], asset_player[], asset_prey[], asset_predator[];
static GSTEXTURE texMenu, texReef, texCity, texLogo, texPlayer, texPrey, texPredator;
static char padBuf[256] __attribute__((aligned(64)));
static struct padButtonStatus padStatus;
static u64 cDeep, cWater, cStripe, cWhite, cInk, cCyan, cGold, cCoral, cGreen, cDim;
static Screen screen = TITLE;
static Fish player;
static Fish enemies[MAX_ENEMIES];
static Bubble bubbles[MAX_BUBBLES];
static int points = 0, lives = 3, level = 1, lastButtons = 0xFFFF;
static int frameCount = 0, dashCooldown = 0, dashFrames = 0;

static u64 rgba(int r, int g, int b) { return GS_SETREG_RGBAQ(r, g, b, 0x00, 0x00); }
static void upload_tex(GSTEXTURE *t, unsigned char *pixels, int w, int h) {
    memset(t, 0, sizeof(*t));
    t->Width=(u32)w; t->Height=(u32)h; t->PSM=GS_PSM_CT32;
    t->Mem=(u32 *)pixels; t->Filter=GS_FILTER_LINEAR;
    t->Vram=gsKit_vram_alloc(gs,gsKit_texture_size(w,h,GS_PSM_CT32),GSKIT_ALLOC_USERBUFFER);
    gsKit_texture_upload(gs,t);
}
static void draw_texture(GSTEXTURE *t,float x,float y,float w,float h,int flip) {
    u64 white=GS_SETREG_RGBAQ(0xFF,0xFF,0xFF,0xFF,0x00);
    gsKit_set_primalpha(gs,GS_SETREG_ALPHA(0,1,0,1,0),0);
    gsKit_set_test(gs,GS_ATEST_OFF);
    if (flip)
        gsKit_prim_sprite_texture(gs,t,x,y,0,(float)t->Width,x+w,y+h,0,(float)t->Height,2,white);
    else
        gsKit_prim_sprite_texture(gs,t,x,y,0,0,x+w,y+h,(float)t->Width,(float)t->Height,2,white);
    gsKit_set_test(gs,GS_ATEST_ON);
    gsKit_set_primalpha(gs,GS_BLEND_BACK2FRONT,0);
}
static void rect(float x, float y, float w, float h, u64 color) {
    gsKit_prim_sprite(gs, x, y, x + w, y + h, 2, color);
}
static void tri(float x1,float y1,float x2,float y2,float x3,float y3,u64 color) {
    gsKit_prim_triangle(gs,x1,y1,x2,y2,x3,y3,2,color);
}

/* Compact 5x7 font. Each row is a five-bit mask, MSB is the left pixel. */
typedef struct { char ch; unsigned char row[7]; } Glyph;
static const Glyph font[] = {
 {'A',{14,17,17,31,17,17,17}}, {'B',{30,17,17,30,17,17,30}},
 {'C',{14,17,16,16,16,17,14}}, {'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}}, {'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,15}}, {'H',{17,17,17,31,17,17,17}},
 {'I',{14,4,4,4,4,4,14}}, {'J',{7,2,2,2,18,18,12}},
 {'K',{17,18,20,24,20,18,17}}, {'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}}, {'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}}, {'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}}, {'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}}, {'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}}, {'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}}, {'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}}, {'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}}, {'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}}, {'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}}, {'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}}, {'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}}, {'9',{14,17,17,15,1,1,14}},
 {':',{0,4,4,0,4,4,0}}, {'-',{0,0,0,31,0,0,0}},
 {'!',{4,4,4,4,4,0,4}}, {'/',{1,2,2,4,8,8,16}},
 {'.',{0,0,0,0,0,12,12}}, {' ',{0,0,0,0,0,0,0}}
};
static void text(const char *s, int x, int y, int scale, u64 color) {
    int ox = x;
    while (*s) {
        char ch = *s++;
        int i, r, c;
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        for (i = 0; i < (int)(sizeof(font)/sizeof(font[0])); ++i) if (font[i].ch == ch) break;
        if (i == (int)(sizeof(font)/sizeof(font[0]))) { x += 6 * scale; continue; }
        for (r = 0; r < 7; ++r) {
            unsigned char bits = font[i].row[r];
            for (c = 0; c < 5; ++c) if (bits & (1 << (4-c)))
                rect((float)(x+c*scale), (float)(y+r*scale), (float)scale, (float)scale, color);
        }
        x += 6 * scale;
        if (x > SW - 10) { x = ox; y += 9 * scale; }
    }
}

static int just_pressed(int buttons, int mask) {
    return (buttons & mask) == 0 && (lastButtons & mask) != 0;
}
static int rand_range(int lo, int hi) { return lo + rand() % (hi - lo + 1); }
static void spawn_enemy(int i, int fromLeft) {
    Fish *f = &enemies[i];
    f->kind = (rand() % 4 == 0) ? 1 : 0; /* one larger predator among prey */
    f->r = f->kind ? rand_range(21, 28) : rand_range(10, 18);
    f->y = (float)rand_range(80, (int)gs->Height - 70);
    f->x = fromLeft ? -f->r - rand_range(0, 170) : SW + f->r + rand_range(0, 170);
    f->vx = (fromLeft ? 1.0f : -1.0f) * (f->kind ? 1.35f : 1.0f) * (1.25f + (float)level * 0.12f + (float)(rand()%100)/100.0f);
    f->alive = 1;
}
static void reset_game(void) {
    int i;
    points = 0; lives = 3; level = 1; dashCooldown = 0; dashFrames = 0;
    player.x = 315; player.y = gs->Height * 0.52f; player.r = 17; player.alive = 1;
    for (i=0;i<MAX_ENEMIES;i++) spawn_enemy(i, rand()%2);
    screen = PLAYING;
}
static void draw_fish(float x, float y, float r, int facingRight, u64 body, int predator) {
    GSTEXTURE *t;
    float w,h;
    if (predator) { t=&texPredator; w=r*3.6f; h=r*3.15f; }
    else if (body==cCyan) { t=&texPlayer; w=r*3.0f; h=r*2.55f; }
    else { t=&texPrey; w=r*3.25f; h=r*2.55f; }
    draw_texture(t,x-w*0.5f,y-h*0.5f,w,h,facingRight);
}
static void draw_bubble(float x,float y,float s) {
    rect(x-s*0.40f,y-s,s*0.8f,2,cCyan); rect(x-s*0.40f,y+s-2,s*0.8f,2,cCyan);
    rect(x-s,y-s*0.40f,2,s*0.8f,cCyan); rect(x+s-2,y-s*0.40f,2,s*0.8f,cCyan);
    rect(x-s*0.35f,y-s*0.35f,2,2,cWhite);
}
static void background(void) {
    int i, h=(int)gs->Height;
    rect(0,0,SW,(float)h,cDeep);
    if (screen==TITLE) draw_texture(&texMenu,0,0,SW,(float)h,0);
    else if (level>=3) draw_texture(&texCity,0,0,SW,(float)h,0);
    else draw_texture(&texReef,0,0,SW,(float)h,0);
    for (i=0;i<8;i++) rect(0,30+i*58,SW,2,cStripe);
    for (i=0;i<MAX_BUBBLES;i++) {
        int yy=(i*71 + frameCount*(1+i%3))%(h+24);
        int xx=(i*97+31)%SW;
        draw_bubble((float)xx,(float)(h-yy),5+(i%3)*2);
    }
    /* distant kelp */
    for(i=0;i<10;i++) {
        int x=i*72+12;
        rect((float)x,(float)h-28,5,35,cGreen);
        rect((float)x-8,(float)h-48,8,4,cGreen);
        rect((float)x+3,(float)h-68,8,4,cGreen);
    }
}
static void draw_scene(void) {
    int i;
    char buf[40];
    gsKit_queue_reset(gs->Os_Queue);
    background();
    if (screen == TITLE) {
        draw_texture(&texLogo,192,62,256,106,0);
        text("EAT SMALLER FISH", 198, 190, 2, cWhite);
        text("AVOID THE BIG ONES", 180, 218, 2, cWhite);
        draw_fish(270,310,22,1,cCyan,0); draw_fish(380,328,31,0,cCoral,1);
        text("PRESS X TO START", 210, 380, 2, cGold);
        text("D PAD MOVE   START PAUSE", 160, 405, 1, cWhite);
    } else {
        for(i=0;i<MAX_ENEMIES;i++) if(enemies[i].alive)
            draw_fish(enemies[i].x,enemies[i].y,enemies[i].r,enemies[i].vx>0,enemies[i].kind?cCoral:cGold,enemies[i].kind);
        draw_fish(player.x,player.y,player.r,1,cCyan,0);
        snprintf(buf,sizeof(buf),"SCORE %d",points); text(buf,18,16,2,cWhite);
        snprintf(buf,sizeof(buf),"LIVES %d",lives); text(buf,440,16,2,cGold);
        snprintf(buf,sizeof(buf),"LEVEL %d",level); text(buf,270,16,2,cCyan);
        if (screen == PAUSED) {
            rect(140,175,360,115,cDim);
            text("PAUSED", 240, 198, 4, cGold);
            text("START RESUME", 222, 258, 2, cWhite);
        } else if (screen == GAME_OVER) {
            rect(100,165,440,140,cDim);
            text("GAME OVER", 200, 190, 4, cCoral);
            text("FINAL SCORE", 228, 240, 2, cWhite);
            snprintf(buf,sizeof(buf),"%d",points); text(buf,300,270,2,cGold);
            text("X RETRY   START MENU", 165, 335, 2, cWhite);
        }
    }
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
}

static int setup_pad(void) {
    int retries=0;
    SifInitRpc(0);
    SifLoadModule("rom0:SIO2MAN",0,NULL);
    SifLoadModule("rom0:PADMAN",0,NULL);
    padInit(0);
    if (!padPortOpen(0,0,padBuf)) return 0;
    while (retries++ < 80) {
        int state=padGetState(0,0);
        if (state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) return 1;
        DelayThread(10000);
    }
    return 1; /* allow booting; input can become available after startup */
}
static int read_buttons(void) {
    int state=padGetState(0,0);
    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1) return 0xFFFF;
    if (padRead(0,0,&padStatus) == 0) return 0xFFFF;
    return padStatus.btns;
}

int main(int argc, char *argv[]) {
    int i;
    gs=gsKit_init_global();
    if (!gs) return 1;
    gs->Width=SW;
    gs->ZBuffering=GS_SETTING_OFF;
    dmaKit_init(D_CTRL_RELE_OFF,D_CTRL_MFD_OFF,D_CTRL_STS_UNSPEC,D_CTRL_STD_OFF,D_CTRL_RCYC_8,1<<DMA_CHANNEL_GIF);
    dmaKit_chan_init(DMA_CHANNEL_GIF);
    gsKit_init_screen(gs);
    gsKit_mode_switch(gs,GS_PERSISTENT);
    upload_tex(&texMenu,asset_menu_bg,256,256);
    upload_tex(&texReef,asset_reef_bg,256,256);
    upload_tex(&texCity,asset_city_bg,256,256);
    upload_tex(&texLogo,asset_logo,256,128);
    upload_tex(&texPlayer,asset_player,128,128);
    upload_tex(&texPrey,asset_prey,64,64);
    upload_tex(&texPredator,asset_predator,128,128);
    gsKit_set_test(gs,GS_ZTEST_OFF);
    cDeep=rgba(2,28,65); cWater=rgba(5,53,91); cStripe=rgba(8,59,92);
    cWhite=rgba(240,250,255); cInk=rgba(4,28,46); cCyan=rgba(35,211,224);
    cGold=rgba(255,191,64); cCoral=rgba(255,104,104); cGreen=rgba(20,121,104);
    cDim=rgba(7,32,59);
    srand(0xF15F);
    setup_pad();
    for(i=0;i<MAX_BUBBLES;i++) { bubbles[i].x=(float)rand_range(8,SW-8); bubbles[i].y=(float)rand_range(0,(int)gs->Height); }
    while (1) {
        int buttons=read_buttons();
        int pressedX=just_pressed(buttons,PAD_CROSS);
        int pressedStart=just_pressed(buttons,PAD_START);
        if (screen==TITLE) {
            if (pressedX || pressedStart) reset_game();
        } else if (screen==GAME_OVER) {
            if (pressedX) reset_game();
            else if (pressedStart) screen=TITLE;
        } else if (screen==PAUSED) {
            if (pressedStart || pressedX) screen=PLAYING;
        } else {
            if (pressedStart) screen=PAUSED;
            else {
                float speed=PLAYER_SPEED;
                if ((buttons & PAD_R1)==0 && dashCooldown==0) { dashFrames=10; dashCooldown=90; }
                if (dashFrames>0) { speed=5.5f; dashFrames--; }
                if (dashCooldown>0) dashCooldown--;
                if ((buttons & PAD_LEFT)==0) player.x-=speed;
                if ((buttons & PAD_RIGHT)==0) player.x+=speed;
                if ((buttons & PAD_UP)==0) player.y-=speed;
                if ((buttons & PAD_DOWN)==0) player.y+=speed;
                if (player.x<player.r+4) player.x=player.r+4;
                if (player.x>SW-player.r-4) player.x=SW-player.r-4;
                if (player.y<60) player.y=60;
                if (player.y>gs->Height-56) player.y=gs->Height-56;
                for(i=0;i<MAX_ENEMIES;i++) if(enemies[i].alive) {
                    Fish *f=&enemies[i];
                    f->x+=f->vx;
                    if (f->x < -f->r-4 || f->x > SW+f->r+4) spawn_enemy(i,rand()%2);
                    {
                        float dx=f->x-player.x, dy=f->y-player.y;
                        float d2=dx*dx+dy*dy, reach=f->r+player.r*0.82f;
                        if (d2 < reach*reach) {
                            if (!f->kind && player.r >= f->r*0.72f) {
                                points += 100;
                                if (points/SCORE_STEP+1 > level) level=points/SCORE_STEP+1;
                                if (player.r < 34) player.r += 1.6f;
                                spawn_enemy(i,rand()%2);
                            } else if (f->kind || player.r < f->r*0.72f) {
                                lives--;
                                player.x=315; player.y=gs->Height*0.52f;
                                spawn_enemy(i,rand()%2);
                                if (lives<=0) screen=GAME_OVER;
                                break;
                            }
                        }
                    }
                }
            }
        }
        lastButtons=buttons;
        frameCount++;
        draw_scene();
    }
    return 0;
}
