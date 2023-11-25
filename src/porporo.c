#include <SDL2/SDL.h>
#include <stdio.h>

#include "uxn.h"
#include "devices/system.h"
#include "devices/screen.h"
#include "devices/controller.h"
#include "devices/mouse.h"
#include "devices/file.h"
#include "devices/datetime.h"

/*
Copyright (c) 2023 Devine Lu Linvega

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

#define HOR 100
#define VER 60
#define SZ (HOR * VER * 16)

static int WIDTH = 8 * HOR, HEIGHT = 8 * VER;
static int ZOOM = 1, GUIDES = 1;

static Program programs[0x10], *porporo, *focused;
static Uint8 *ram;
static int plen;

static int reqdraw;
static int dragx, dragy, movemode;
static int camerax, cameray;

static SDL_Window *gWindow = NULL;
static SDL_Renderer *gRenderer = NULL;
static SDL_Texture *gTexture = NULL;
static Uint32 *pixels;

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

static char
nibble(Uint8 v)
{
	return v > 0x9 ? 'a' + v - 10 : '0' + v;
}

/* Add/Remove */

#pragma mark - Draw

static void
putpixel(Uint32 *dst, int x, int y, int color)
{
	if(x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT)
		dst[y * WIDTH + x] = theme[color];
}

static void
drawscreen(Uint32 *dst, Screen *scr, int x1, int y1)
{
	int x, y, w = scr->w, x2 = x1 + scr->w, y2 = y1 + scr->h;
	for(y = y1; y < y2; y++) {
		for(x = x1; x < x2; x++) {
			if(x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
				int index = (x - x1) + (y - y1) * w;
				dst[y * WIDTH + x] = scr->pixels[index];
			}
		}
	}
}

static void
line(Uint32 *dst, int ax, int ay, int bx, int by, int color)
{
	int dx = abs(bx - ax), sx = ax < bx ? 1 : -1;
	int dy = -abs(by - ay), sy = ay < by ? 1 : -1;
	int err = dx + dy, e2;
	for(;;) {
		putpixel(dst, ax, ay, color);
		if(ax == bx && ay == by) break;
		e2 = 2 * err;
		if(e2 >= dy) err += dy, ax += sx;
		if(e2 <= dx) err += dx, ay += sy;
	}
}

static void
drawicn(Uint32 *dst, int x, int y, Uint8 *sprite, int fg)
{
	int v, h;
	for(v = 0; v < 8; v++)
		for(h = 0; h < 8; h++)
			if((sprite[v] >> (7 - h)) & 0x1) putpixel(dst, x + h, y + v, fg);
}

static void
drawconnection(Uint32 *dst, Program *p, int color)
{
	int i;
	for(i = 0; i < p->clen; i++) {
		Connection *c = &p->out[i];
		int x1 = p->x + 3 + camerax, y1 = p->y + 3 + cameray;
		int x2 = c->b->x - 5 + camerax, y2 = c->b->y + 3 + cameray;
		line(dst, x1, y1, x2, y2, color);
	}
}

static void
drawguides(Uint32 *dst)
{
	int x, y;
	for(x = 2; x < HOR - 2; x++)
		for(y = 2; y < VER - 2; y++)
			drawicn(dst, x * 8, y * 8, icons[4], 4);
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

static void
drawbyte(Uint32 *dst, int x, int y, Uint8 byte, int color)
{
	drawicn(dst, x += 8, y, &font[(nibble(byte >> 4) - 0x20) << 3], color);
	drawicn(dst, x += 8, y, &font[(nibble(byte & 0xf) - 0x20) << 3], color);
}

static void
linerect(Uint32 *dst, int x1, int y1, int x2, int y2, int color)
{
	int x, y;
	for(y = y1 - 1; y < y2 + 1; y++) {
		putpixel(dst, x1 - 2, y, color), putpixel(dst, x2, y, color);
		putpixel(dst, x1 - 1, y, color), putpixel(dst, x2 + 1, y, color);
	}
	for(x = x1 - 2; x < x2 + 2; x++) {
		putpixel(dst, x, y1 - 2, color), putpixel(dst, x, y2, color);
		putpixel(dst, x, y1 - 1, color), putpixel(dst, x, y2 + 1, color);
	}
}

static void
drawprogram(Uint32 *dst, Program *p)
{
	int w = p->screen.w, h = p->screen.h, x = p->x + camerax, y = p->y + cameray;
	/* drawrect(dst, p->x, p->y, p->x + w, p->y + h, 4); */
	linerect(dst, x, y, x + w, y + h, 2 + !movemode);
	drawtext(dst, x + 2, y - 12, p->rom, 3);
	/* ports */
	drawicn(dst, x - 8, y, icons[0], 1);
	drawicn(dst, x + w, y, icons[0], 2);
	drawbyte(dst, x - 32, y - 12, 0x12, 3);
	drawbyte(dst, x + w - 8, y - 9, 0x18, 3);
	/* display */
	if(p->screen.x2)
		screen_redraw(&p->screen);
	drawscreen(pixels, &p->screen, x, y);
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
	if(GUIDES)
		drawguides(dst);
	for(i = 0; i < plen; i++)
		drawprogram(dst, &programs[i]);
	for(i = 0; i < plen; i++)
		drawconnection(dst, &programs[i], 3);
	SDL_UpdateTexture(gTexture, NULL, dst, WIDTH * sizeof(Uint32));
	SDL_RenderClear(gRenderer);
	SDL_RenderCopy(gRenderer, gTexture, NULL, NULL);
	SDL_RenderPresent(gRenderer);
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
drag_start(int x, int y)
{
	dragx = x, dragy = y;
}

static void
drag_move(int x, int y)
{
	camerax += x - dragx, cameray += y - dragy;
	dragx = x, dragy = y;
	clear(pixels);
	reqdraw = 1;
}

static void
drag_end(void)
{
	dragx = dragy = 0;
}

int dragprgx, dragprgy;

static void
drag_prg_start(int x, int y)
{
	dragprgx = x, dragprgy = y;
}

static void
drag_prg_move(int x, int y)
{
	focused->x += x - dragprgx, focused->y += y - dragprgy;
	dragprgx = x, dragprgy = y;
	clear(pixels);
	reqdraw = 1;
}

static void
drag_prg_end(void)
{
	dragprgx = dragprgy = 0;
}

static int
withinprogram(Program *p, int x, int y)
{
	return x > p->x && x < p->x + p->screen.w && y > p->y && y < p->y + p->screen.h;
}

static void
handle_mouse(SDL_Event *event)
{
	int i, desk = 1, x = event->motion.x - camerax, y = event->motion.y - cameray;
	for(i = plen - 1; i; i--) {
		Program *p = &programs[i];
		if(withinprogram(p, x, y)) {
			focused = p;
			if(movemode) {
				if(event->type == SDL_MOUSEBUTTONDOWN)
					drag_prg_start(event->motion.x, event->motion.y);
				if(event->type == SDL_MOUSEMOTION && event->button.button)
					drag_prg_move(event->motion.x, event->motion.y);
				if(event->type == SDL_MOUSEBUTTONUP)
					drag_prg_end();
				return;
			} else {
				int xx = x - p->x, yy = y - p->y, desk = 0;
				if(event->type == SDL_MOUSEMOTION)
					mouse_pos(&p->u, &p->u.dev[0x90], xx, yy);
				else if(event->type == SDL_MOUSEBUTTONDOWN)
					mouse_down(&p->u, &p->u.dev[0x90], SDL_BUTTON(event->button.button));
				else if(event->type == SDL_MOUSEBUTTONUP)
					mouse_up(&p->u, &p->u.dev[0x90], SDL_BUTTON(event->button.button));
				return;
			}
		}
	}
	if(desk) {
		/* on desktop */
		if(event->type == SDL_MOUSEBUTTONDOWN)
			drag_start(event->motion.x, event->motion.y);
		if(event->type == SDL_MOUSEMOTION && event->button.button)
			drag_move(event->motion.x, event->motion.y);
		if(event->type == SDL_MOUSEBUTTONUP)
			drag_end();
		focused = porporo;
	}
	reqdraw = 1;
}

static void
domouse(SDL_Event *event)
{

	switch(event->type) {
	case SDL_MOUSEBUTTONDOWN: handle_mouse(event); break;
	case SDL_MOUSEMOTION: handle_mouse(event); break;
	case SDL_MOUSEBUTTONUP: handle_mouse(event); break;
	case SDL_MOUSEWHEEL: handle_mouse(event); break;
	}
}

static Uint8
get_button(SDL_Event *event)
{
	switch(event->key.keysym.sym) {
	case SDLK_LCTRL: return 0x01;
	case SDLK_LALT: return 0x02;
	case SDLK_LSHIFT: return 0x04;
	case SDLK_HOME: return 0x08;
	case SDLK_UP: return 0x10;
	case SDLK_DOWN: return 0x20;
	case SDLK_LEFT: return 0x40;
	case SDLK_RIGHT: return 0x80;
	}
	return 0x00;
}

static Uint8
get_key(SDL_Event *event)
{
	int sym = event->key.keysym.sym;
	SDL_Keymod mods = SDL_GetModState();
	if(sym < 0x20 || sym == SDLK_DELETE)
		return sym;
	if(mods & KMOD_CTRL) {
		if(sym < SDLK_a)
			return sym;
		else if(sym <= SDLK_z)
			return sym - (mods & KMOD_SHIFT) * 0x20;
	}
	return 0x00;
}

static void
dokey(SDL_Event *event)
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
	}
}

static void
on_mouse_wheel(int x, int y)
{
	Uxn *u = &focused->u;
	mouse_scroll(u, &u->dev[0x90], x, y);
}

static int
init(void)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
		return error("Init", SDL_GetError());
	gWindow = SDL_CreateWindow("Porporo", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * ZOOM, HEIGHT * ZOOM, SDL_WINDOW_SHOWN);
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
addprogram(int x, int y, char *rom)
{
	Program *p = &programs[plen++];
	p->x = x, p->y = y, p->rom = rom;
	p->u.ram = ram + (plen - 1) * 0x10000;
	p->u.id = plen - 1;
	system_init(&p->u, p->u.ram, rom);
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
	switch(addr & 0xf0) {
	case 0x00: break;
	case 0x10: break;
	case 0xc0: return datetime_dei(u, addr); break;
	}
	return u->dev[addr];
}

void
emu_deo(Uxn *u, Uint8 addr, Uint8 value)
{
	Uint8 p = addr & 0x0f, d = addr & 0xf0;
	Program *prg = &programs[u->id];
	u->dev[addr] = value;
	switch(d) {
	case 0x00:
		if(p > 0x7 && p < 0xe) screen_palette(&prg->screen, &u->dev[0x8]);
		break;
	case 0x10: {
		Program *tprg = prg->out[0].b;
		if(tprg) {
			Uint16 vector = (tprg->u.dev[0x10] << 8) | tprg->u.dev[0x11];
			tprg->u.dev[0x12] = value;
			if(vector) {
				uxn_eval(&tprg->u, vector);
			} else if(tprg == porporo)
				printf("%c", value);
		}
	} break;
	case 0x20:
		screen_deo(prg, u->ram, &u->dev[d], p);
		break;
	case 0xa0: file_deo(prg, 0, u->ram, &u->dev[d], p); break;
	case 0xb0: file_deo(prg, 1, u->ram, &u->dev[d], p); break;
	}
}

void
screenvector(void)
{
	int i;
	if(!focused || focused == porporo)
		return;
	Uint16 vector = (focused->u.dev[0x20] << 8) | focused->u.dev[0x21];
	if(vector)
		uxn_eval(&focused->u, vector);
	reqdraw = 1;
}

static void
porporo_key(char c)
{
	switch(c) {
	case 'd':
		movemode = !movemode;
		break;
	}
}

int
main(int argc, char **argv)
{
	Uint32 begintime = 0;
	Uint32 endtime = 0;
	Uint32 delta = 0;
	Program *prg_listen, *prg_hello, *prg_left, *prg_bicycle;
	(void)argc;
	(void)argv;

	ram = (Uint8 *)calloc(0x10000 * RAM_PAGES, sizeof(Uint8));

	if(!init())
		return error("Init", "Failure");

	porporo = addprogram(150, 150, "bin/porporo.rom");
	prg_listen = addprogram(920, 140, "bin/listen.rom");
	prg_hello = addprogram(700, 300, "bin/hello.rom");

	addprogram(20, 30, "bin/screen.pixel.rom");
	addprogram(150, 90, "bin/oekaki.rom");
	addprogram(650, 160, "bin/catclock.rom");
	addprogram(750, 300, "bin/left.rom");

	connectports(prg_hello, prg_listen, 0x12, 0x18);
	connectports(prg_listen, porporo, 0x12, 0x18);

	uxn_eval(&prg_hello->u, 0x100);

	fflush(stdout);

	while(1) {
		SDL_Event event;
		if(!begintime)
			begintime = SDL_GetTicks();
		else
			delta = endtime - begintime;
		if(delta < 40)
			SDL_Delay(40 - delta);
		screenvector();
		if(reqdraw) {
			redraw(pixels);
			reqdraw = 0;
		}
		while(SDL_PollEvent(&event) != 0) {
			switch(event.type) {
			case SDL_QUIT: quit(); break;
			case SDL_MOUSEWHEEL: on_mouse_wheel(event.wheel.x, event.wheel.y); break;
			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEMOTION:
				domouse(&event);
				break;
			case SDL_TEXTINPUT:
				if(focused == porporo) {
					porporo_key(event.text.text[0]);
				} else {
					controller_key(&focused->u, &focused->u.dev[0x80], event.text.text[0]);
				}
				reqdraw = 1;
				break;
			case SDL_KEYDOWN:
				if(get_key(&event))
					controller_key(&focused->u, &focused->u.dev[0x80], get_key(&event));
				else if(get_button(&event))
					controller_down(&focused->u, &focused->u.dev[0x80], get_button(&event));
				reqdraw = 1;
				break;
			case SDL_KEYUP:
				controller_up(&focused->u, &focused->u.dev[0x80], get_button(&event));
				reqdraw = 1;
				break;
			case SDL_WINDOWEVENT:
				if(event.window.event == SDL_WINDOWEVENT_EXPOSED)
					reqdraw = 1;
				break;
			}
		}
		begintime = endtime;
		endtime = SDL_GetTicks();
	}
	quit();
	return 0;
}