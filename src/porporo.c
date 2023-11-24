#include <SDL2/SDL.h>
#include <stdio.h>

#include "uxn.h"
#include "devices/system.h"

/*
Copyright (c) 2023 Devine Lu Linvega

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define HOR 100
#define VER 70
#define PAD 2
#define SZ (HOR * VER * 16)

typedef struct Connection {
	Uint8 ap, bp;
	struct Program *a, *b;
} Connection;

typedef struct Program {
	char *rom;
	int x, y, w, h, clen;
	Connection out[0x100];
	Uxn u;
} Program;

typedef enum { INPUT,
	OUTPUT,
	POOL,
	BASIC } GateType;

typedef struct {
	int x, y;
} Point2d;

typedef struct Noton {
	int alive, frame;
	unsigned int speed;
} Noton;

typedef struct Brush {
	int down;
	Point2d pos;
} Brush;

static int WIDTH = 8 * HOR + 8 * PAD * 2;
static int HEIGHT = 8 * (VER + 2) + 8 * PAD * 2;
static int ZOOM = 1, GUIDES = 1;

static Program programs[0x10];
static int plen;

static Noton noton;
static Brush brush;
static Uint8 *ram;

/* clang-format off */

static Uint32 theme[] = {0xffffff, 0xffb545, 0x72DEC2, 0x000000, 0xeeeeee};

Uint8 icons[][8] = {
    {0x38, 0x7c, 0xee, 0xd6, 0xee, 0x7c, 0x38, 0x00}, /* gate:input */
    {0x38, 0x44, 0x92, 0xaa, 0x92, 0x44, 0x38, 0x00}, /* gate:output */
    {0x38, 0x44, 0x82, 0x92, 0x82, 0x44, 0x38, 0x00}, /* gate:output-sharp */
    {0x38, 0x7c, 0xfe, 0xfe, 0xfe, 0x7c, 0x38, 0x00}, /* gate:binary */
    {0x10, 0x00, 0x10, 0xaa, 0x10, 0x00, 0x10, 0x00}, /* guide */
    {0x00, 0x00, 0x00, 0x82, 0x44, 0x38, 0x00, 0x00}, /* eye open */
    {0x00, 0x38, 0x44, 0x92, 0x28, 0x10, 0x00, 0x00}, /* eye closed */
    {0x10, 0x54, 0x28, 0xc6, 0x28, 0x54, 0x10, 0x00}  /* unsaved */
};

static Uint8 font[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 
	0x00, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x7f, 0x22, 0x22, 0x22, 0x7f, 0x22, 
	0x00, 0x08, 0x7f, 0x40, 0x3e, 0x01, 0x7f, 0x08, 0x00, 0x21, 0x52, 0x24, 0x08, 0x12, 0x25, 0x42, 
	0x00, 0x3e, 0x41, 0x42, 0x38, 0x05, 0x42, 0x3d, 0x00, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x08, 0x00, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 
	0x00, 0x00, 0x2a, 0x1c, 0x3e, 0x1c, 0x2a, 0x00, 0x00, 0x00, 0x08, 0x08, 0x3e, 0x08, 0x08, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 
	0x00, 0x3e, 0x41, 0x41, 0x41, 0x41, 0x41, 0x3e, 0x00, 0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x1c, 
	0x00, 0x7e, 0x01, 0x01, 0x3e, 0x40, 0x40, 0x7f, 0x00, 0x7e, 0x01, 0x01, 0x3e, 0x01, 0x01, 0x7e, 
	0x00, 0x11, 0x21, 0x41, 0x7f, 0x01, 0x01, 0x01, 0x00, 0x7f, 0x40, 0x40, 0x3e, 0x01, 0x01, 0x7e, 
	0x00, 0x3e, 0x41, 0x40, 0x7e, 0x41, 0x41, 0x3e, 0x00, 0x7f, 0x01, 0x01, 0x02, 0x04, 0x08, 0x08, 
	0x00, 0x3e, 0x41, 0x41, 0x3e, 0x41, 0x41, 0x3e, 0x00, 0x3e, 0x41, 0x41, 0x3f, 0x01, 0x02, 0x04, 
	0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x10, 
	0x00, 0x00, 0x08, 0x10, 0x20, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x00, 
	0x00, 0x00, 0x10, 0x08, 0x04, 0x08, 0x10, 0x00, 0x00, 0x3e, 0x41, 0x01, 0x06, 0x08, 0x00, 0x08, 
	0x00, 0x3e, 0x41, 0x5d, 0x55, 0x45, 0x59, 0x26, 0x00, 0x3e, 0x41, 0x41, 0x7f, 0x41, 0x41, 0x41, 
	0x00, 0x7e, 0x41, 0x41, 0x7e, 0x41, 0x41, 0x7e, 0x00, 0x3e, 0x41, 0x40, 0x40, 0x40, 0x41, 0x3e, 
	0x00, 0x7c, 0x42, 0x41, 0x41, 0x41, 0x42, 0x7c, 0x00, 0x3f, 0x40, 0x40, 0x7f, 0x40, 0x40, 0x3f, 
	0x00, 0x7f, 0x40, 0x40, 0x7e, 0x40, 0x40, 0x40, 0x00, 0x3e, 0x41, 0x50, 0x4e, 0x41, 0x41, 0x3e, 
	0x00, 0x41, 0x41, 0x41, 0x7f, 0x41, 0x41, 0x41, 0x00, 0x1c, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1c, 
	0x00, 0x7f, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7e, 0x00, 0x41, 0x42, 0x44, 0x78, 0x44, 0x42, 0x41, 
	0x00, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x7f, 0x00, 0x63, 0x55, 0x49, 0x41, 0x41, 0x41, 0x41, 
	0x00, 0x61, 0x51, 0x51, 0x49, 0x49, 0x45, 0x43, 0x00, 0x1c, 0x22, 0x41, 0x41, 0x41, 0x22, 0x1c, 
	0x00, 0x7e, 0x41, 0x41, 0x7e, 0x40, 0x40, 0x40, 0x00, 0x3e, 0x41, 0x41, 0x41, 0x45, 0x42, 0x3d, 
	0x00, 0x7e, 0x41, 0x41, 0x7e, 0x44, 0x42, 0x41, 0x00, 0x3f, 0x40, 0x40, 0x3e, 0x01, 0x01, 0x7e, 
	0x00, 0x7f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x41, 0x41, 0x41, 0x41, 0x41, 0x42, 0x3d, 
	0x00, 0x41, 0x41, 0x41, 0x41, 0x22, 0x14, 0x08, 0x00, 0x41, 0x41, 0x41, 0x41, 0x49, 0x55, 0x63, 
	0x00, 0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41, 0x00, 0x41, 0x22, 0x14, 0x08, 0x08, 0x08, 0x08, 
	0x00, 0x7f, 0x01, 0x02, 0x1c, 0x20, 0x40, 0x7f, 0x00, 0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x18, 
	0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x18, 
	0x00, 0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 
	0x00, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x01, 0x3e, 0x41, 0x7d, 
	0x00, 0x00, 0x00, 0x40, 0x7e, 0x41, 0x41, 0x7e, 0x00, 0x00, 0x00, 0x3e, 0x41, 0x40, 0x41, 0x3e, 
	0x00, 0x00, 0x00, 0x01, 0x3f, 0x41, 0x41, 0x3f, 0x00, 0x00, 0x00, 0x3e, 0x41, 0x7e, 0x40, 0x3f, 
	0x00, 0x00, 0x00, 0x3f, 0x40, 0x7e, 0x40, 0x40, 0x00, 0x00, 0x00, 0x3f, 0x41, 0x3f, 0x01, 0x7e, 
	0x00, 0x00, 0x00, 0x40, 0x7e, 0x41, 0x41, 0x41, 0x00, 0x00, 0x00, 0x08, 0x00, 0x08, 0x08, 0x08, 
	0x00, 0x00, 0x00, 0x7f, 0x01, 0x01, 0x02, 0x7c, 0x00, 0x00, 0x00, 0x41, 0x46, 0x78, 0x46, 0x41, 
	0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40, 0x3f, 0x00, 0x00, 0x00, 0x76, 0x49, 0x41, 0x41, 0x41, 
	0x00, 0x00, 0x00, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00, 0x00, 0x3e, 0x41, 0x41, 0x41, 0x3e, 
	0x00, 0x00, 0x00, 0x7e, 0x41, 0x7e, 0x40, 0x40, 0x00, 0x00, 0x00, 0x3f, 0x41, 0x3f, 0x01, 0x01, 
	0x00, 0x00, 0x00, 0x5e, 0x61, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, 0x3f, 0x40, 0x3e, 0x01, 0x7e, 
	0x00, 0x00, 0x00, 0x7f, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x42, 0x3d, 
	0x00, 0x00, 0x00, 0x41, 0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00, 0x41, 0x41, 0x41, 0x49, 0x37, 
	0x00, 0x00, 0x00, 0x41, 0x22, 0x1c, 0x22, 0x41, 0x00, 0x00, 0x00, 0x41, 0x22, 0x1c, 0x08, 0x08, 
	0x00, 0x00, 0x00, 0x7f, 0x01, 0x3e, 0x40, 0x7f, 0x00, 0x08, 0x10, 0x10, 0x20, 0x10, 0x10, 0x08, 
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x04, 0x04, 0x02, 0x04, 0x04, 0x08, 
	0x00, 0x00, 0x00, 0x30, 0x49, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 	
};

/* clang-format on */

static SDL_Window *gWindow = NULL;
static SDL_Renderer *gRenderer = NULL;
static SDL_Texture *gTexture = NULL;
static Uint32 *pixels;

#pragma mark - Helpers

int
distance(Point2d a, Point2d b)
{
	return (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
}

Point2d *
setpt2d(Point2d *p, int x, int y)
{
	p->x = x;
	p->y = y;
	return p;
}

Point2d *
clamp2d(Point2d *p, int step)
{
	return setpt2d(p, p->x / step * step, p->y / step * step);
}

Point2d
Pt2d(int x, int y)
{
	Point2d p;
	setpt2d(&p, x, y);
	return p;
}

#pragma mark - Options

static void
pause(Noton *n)
{
	n->alive = !n->alive;
	printf("%s\n", n->alive ? "Playing.." : "Paused.");
}

/* Add/Remove */

#pragma mark - Draw

static void
putpixel(Uint32 *dst, int x, int y, int color)
{
	if(x >= 0 && x < WIDTH - 8 && y >= 0 && y < HEIGHT - 8)
		dst[(y + PAD * 8) * WIDTH + (x + PAD * 8)] = theme[color];
}

static void
line(Uint32 *dst, int ax, int ay, int bx, int by, int color)
{
	int dx = abs(bx - ax), sx = ax < bx ? 1 : -1;
	int dy = -abs(by - ay), sy = ay < by ? 1 : -1;
	int err = dx + dy, e2;
	for(;;) {
		putpixel(dst, ax, ay, color);
		if(ax == bx && ay == by)
			break;
		e2 = 2 * err;
		if(e2 >= dy)
			err += dy, ax += sx;
		if(e2 <= dx)
			err += dx, ay += sy;
	}
}

static void
drawicn(Uint32 *dst, int x, int y, Uint8 *sprite, int fg)
{
	int v, h;
	for(v = 0; v < 8; v++)
		for(h = 0; h < 8; h++) {
			int ch1 = (sprite[v] >> (7 - h)) & 0x1;
			if(ch1)
				putpixel(dst, x + h, y + v, fg);
		}
}

static void
drawconnection(Uint32 *dst, Program *p, int color)
{
	int i;
	for(i = 0; i < p->clen; i++) {
		Connection *c = &p->out[i];
		int x1 = p->x + p->w + 3, y1 = p->y + 3;
		int x2 = c->b->x - 5, y2 = c->b->y + 3;
		line(dst, x1, y1, x2, y2, color);
	}
}

static void
drawguides(Uint32 *dst)
{
	int x, y;
	for(x = 0; x < HOR; x++)
		for(y = 0; y < VER; y++)
			drawicn(dst, x * 8, y * 8, icons[4], 4);
}

static void
drawui(Uint32 *dst)
{
	int bottom = VER * 8 + 8;
	drawicn(dst, 0 * 8, bottom, icons[GUIDES ? 6 : 5], GUIDES ? 1 : 2);
}

static void
linerect(Uint32 *dst, int x1, int y1, int x2, int y2, int color)
{
	int x, y;
	for(y = y1 - 1; y < y2 + 1; y++) {
		putpixel(dst, x1, y, color), putpixel(dst, x2, y, color);
		putpixel(dst, x1 - 1, y, color), putpixel(dst, x2 + 1, y, color);
	}
	for(x = x1 - 1; x < x2 + 1; x++) {
		putpixel(dst, x, y1, color), putpixel(dst, x, y2, color);
		putpixel(dst, x, y1 - 1, color), putpixel(dst, x, y2 + 1, color);
	}
	putpixel(dst, x2 + 1, y2 + 1, color);
}

static void
drawrect(Uint32 *dst, int x1, int y1, int x2, int y2, int color)
{
	int x, y;
	for(y = y1; y < y2; y++)
		for(x = x1; x < x2; x++)
			putpixel(dst, x, y, color);
}

static void
drawtext(Uint32 *dst, int x, int y, char *t, int color)
{
	char c;
	x -= 8;
	while((c = *t++))
		drawicn(dst, x += 8, y, &font[(c - 0x20) << 3], color);
}

static char
nibble(Uint8 v)
{
	return v > 0x9 ? 'a' + v - 10 : '0' + v;
}

static void
drawbyte(Uint32 *dst, int x, int y, Uint8 byte, int color)
{
	drawicn(dst, x += 8, y, &font[(nibble(byte >> 4) - 0x20) << 3], color);
	drawicn(dst, x += 8, y, &font[(nibble(byte & 0xf) - 0x20) << 3], color);
}

static void
drawpage(Uint32 *dst, int x, int y, Uint8 *r)
{
	int i;
	for(i = 0; i < 0x8; i++)
		drawbyte(dst, x + (i * 0x18), y, r[i], 3);
}

static void
drawprogram(Uint32 *dst, Program *p)
{
	drawrect(dst, p->x, p->y, p->x + p->w, p->y + p->h, 4);
	linerect(dst, p->x, p->y, p->x + p->w, p->y + p->h, 3);
	drawtext(dst, p->x + 2, p->y + 2, p->rom, 3);
	/* ports */
	drawicn(dst, p->x - 8, p->y, icons[0], 1);
	drawicn(dst, p->x + p->w, p->y, icons[0], 2);
	drawbyte(dst, p->x - 8, p->y - 9, 0x12, 3);
	drawbyte(dst, p->x + p->w - 8, p->y - 9, 0x18, 3);
	drawpage(dst, p->x, p->y + 0x10, &p->u.ram[0x100]);
}

static void
clear(Uint32 *dst)
{
	int i, j;
	for(i = 0; i < HEIGHT; i++)
		for(j = 0; j < WIDTH; j++)
			dst[i * WIDTH + j] = theme[0];
}

static void
redraw(Uint32 *dst)
{
	int i;
	clear(dst);

	if(GUIDES)
		drawguides(dst);
	for(i = 0; i < plen; i++)
		drawprogram(dst, &programs[i]);
	for(i = 0; i < plen; i++)
		drawconnection(dst, &programs[i], 3);

	drawui(dst);
	SDL_UpdateTexture(gTexture, NULL, dst, WIDTH * sizeof(Uint32));
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
	SDL_RenderPresent(gRenderer);
}

/* operation */

static void
savemode(int *i, int v)
{
	*i = v;
	redraw(pixels);
}

static void
selectoption(int option)
{
	switch(option) {
	case 0:
		savemode(&GUIDES, !GUIDES);
		break;
	}
}

static void
run(Noton *n)
{
	n->frame++;
}

/* options */

static int
error(char *msg, const char *err)
{
	printf("Error %s: %s\n", msg, err);
	return 0;
}

static void
modzoom(int mod)
{
	if((mod > 0 && ZOOM < 4) || (mod < 0 && ZOOM > 1)) {
		ZOOM += mod;
		SDL_SetWindowSize(gWindow, WIDTH * ZOOM, HEIGHT * ZOOM);
	}
}

static void
quit(void)
{
	free(pixels);
	SDL_DestroyTexture(gTexture);
	gTexture = NULL;
	SDL_DestroyRenderer(gRenderer);
	gRenderer = NULL;
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;
	SDL_Quit();
	exit(0);
}

#pragma mark - Triggers

static void
domouse(SDL_Event *event, Brush *b)
{
	setpt2d(&b->pos, (event->motion.x - (PAD * 8 * ZOOM)) / ZOOM, (event->motion.y - (PAD * 8 * ZOOM)) / ZOOM);
	switch(event->type) {
	case SDL_MOUSEBUTTONDOWN:
		if(event->motion.y / ZOOM / 8 - PAD == VER + 1) {
			selectoption(event->motion.x / ZOOM / 8 - PAD);
			break;
		}
		if(event->button.button == SDL_BUTTON_RIGHT)
			break;
		b->down = 1;
		break;
	case SDL_MOUSEMOTION:
		if(event->button.button == SDL_BUTTON_RIGHT)
			break;
		break;
	case SDL_MOUSEBUTTONUP:
		if(event->button.button == SDL_BUTTON_RIGHT) {
			if(b->pos.x < 24 || b->pos.x > HOR * 8 - 72)
				return;
			if(b->pos.x > HOR * 8 || b->pos.y > VER * 8)
				return;
			redraw(pixels);
			break;
		}
		b->down = 0;
		break;
	}
}

static void
dokey(Noton *n, SDL_Event *event)
{
	switch(event->key.keysym.sym) {
	case SDLK_EQUALS:
	case SDLK_PLUS:
		modzoom(1);
		break;
	case SDLK_UNDERSCORE:
	case SDLK_MINUS:
		modzoom(-1);
		break;
	case SDLK_BACKSPACE:
		break;
	case SDLK_SPACE:
		pause(n);
		break;
	}
}

static int
init(void)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return error("Init", SDL_GetError());
	gWindow = SDL_CreateWindow("Noton", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * ZOOM, HEIGHT * ZOOM, SDL_WINDOW_SHOWN);
	if(gWindow == NULL)
		return error("Window", SDL_GetError());
	gRenderer = SDL_CreateRenderer(gWindow, -1, 0);
	if(gRenderer == NULL)
		return error("Renderer", SDL_GetError());
	gTexture = SDL_CreateTexture(gRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, WIDTH, HEIGHT);
	if(gTexture == NULL)
		return error("Texture", SDL_GetError());
	pixels = (Uint32 *)malloc(WIDTH * HEIGHT * sizeof(Uint32));
	if(pixels == NULL)
		return error("Pixels", "Failed to allocate memory");
	clear(pixels);
	return 1;
}

static Program *
addprogram(int x, int y, int w, int h, char *rom)
{
	Program *p = &programs[plen++];
	p->x = x, p->y = y, p->w = w, p->h = h, p->rom = rom;
	p->u.ram = ram + (plen - 1) * 0x10000;
	p->u.id = plen - 1;
	system_init(&p->u, ram, rom);
	uxn_eval(&p->u, 0x100);
	return p;
}

static void
connectports(Program *a, Program *b, Uint8 ap, Uint8 bp)
{
	Connection *c = &a->out[a->clen++];
	c->ap = ap, c->bp = bp;
	c->a = a, c->b = b;
}

Uint8
emu_dei(Uxn *u, Uint8 addr)
{
	Program *prg = &programs[u->id];
	printf("?????\n");
	switch(addr & 0xf0) {
	case 0x10: printf("<< %s\n", prg->rom); break;
	}
	return u->dev[addr];
}

void
emu_deo(Uxn *u, Uint8 addr, Uint8 value)
{
	Uint8 p = addr & 0x0f, d = addr & 0xf0;
	Program *prg = &programs[u->id];
	u->dev[addr] = value;

	if(!u->id) {
		printf("PORPORO\n");
	}

	switch(d) {
	case 0x10: {
		Program *tprg = prg->out[0].b;
		if(tprg) {
			Uint16 vector = (tprg->u.dev[0x10] << 8) | tprg->u.dev[0x11];
			tprg->u.dev[0x12] = value;
			if(vector) {
				printf(">> %s to %s [%04x]\n", prg->rom, tprg->rom, vector);
				uxn_eval(&tprg->u, vector);
			}
		}
	} break;
	}
}

int
main(int argc, char **argv)
{
	Uint32 begintime = 0;
	Uint32 endtime = 0;
	Uint32 delta = 0;
	Program *a, *listen, *c, *d, *hello, *f, *porporo;
	(void)argc;
	(void)argv;

	ram = (Uint8 *)calloc(0x10000 * RAM_PAGES, sizeof(Uint8));

	noton.alive = 1;
	noton.speed = 40;

	if(!init())
		return error("Init", "Failure");

	porporo = addprogram(550, 350, 140, 20, "bin/porporo.rom");

	a = addprogram(150, 30, 120, 120, "console.rom");
	listen = addprogram(520, 120, 140, 140, "bin/listen.rom");
	c = addprogram(10, 130, 100, 70, "keyb.rom");
	d = addprogram(190, 170, 100, 80, "debug.rom");
	hello = addprogram(300, 300, 140, 140, "bin/hello.rom");

	connectports(hello, listen, 0x12, 0x18);
	connectports(c, a, 0x12, 0x18);

	connectports(listen, porporo, 0x12, 0x18);

	uxn_eval(&hello->u, 0x100);

	printf("%c\n", nibble(0xe));

	while(1) {
		SDL_Event event;
		if(!begintime)
			begintime = SDL_GetTicks();
		else
			delta = endtime - begintime;
		if(delta < noton.speed)
			SDL_Delay(noton.speed - delta);
		if(noton.alive) {
			run(&noton);
			/* redraw(pixels); */
		}
		while(SDL_PollEvent(&event) != 0) {
			if(event.type == SDL_QUIT)
				quit();
			else if(event.type == SDL_MOUSEBUTTONUP ||
				event.type == SDL_MOUSEBUTTONDOWN ||
				event.type == SDL_MOUSEMOTION) {
				domouse(&event, &brush);
			} else if(event.type == SDL_KEYDOWN)
				dokey(&noton, &event);
			else if(event.type == SDL_WINDOWEVENT)
				if(event.window.event == SDL_WINDOWEVENT_EXPOSED)
					redraw(pixels);
		}
		begintime = endtime;
		endtime = SDL_GetTicks();
	}
	quit();
	return 0;
}