/*
 * sun-2 emulator
 * 10/2014  Brad Parker <brad@heeltoe.com>
 *
 * Sun-2 video and keyboard emulation via SDL
 *
 * Copyright (C) 2017-2018 Brad Parker <brad@heeltoe.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>

#ifdef __APPLE__
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "sim.h"

static unsigned char *fbmem;
static SDL_Surface *screen;
static int rows, cols;

static int toggle_trace;
static unsigned int fbctrl;

void sdl_clear(void)
{
  unsigned char *p = screen->pixels;
  int i, j;

  for (i = 0; i < cols; i++)
    for (j = 0; j < rows; j++) {
      *p = ~*p;
      p++;
    }

  SDL_UpdateRect(screen, 0, 0, cols, rows);
}

#define COLOR_WHITE	0xff
#define COLOR_BLACK	0

void sdl_write(unsigned int offset, int size, unsigned value)
{
  unsigned char *ps = screen->pixels;
  int i, h, v;

  if (0) printf("sdl_write offset %x size %d <- %x\n", offset, size, value);

  offset *= 8;
  v = offset / cols;
  h = offset % cols;

  switch (size) {
  case 1:
    for (i = 0; i < 8; i++) {
      ps[offset + i] = (value & 0x80) ? COLOR_BLACK : COLOR_WHITE;
      value <<= 1;
    }
    break;
  case 2:
    if (value == 0x0) { for (i = 0; i < 16; i++) ps[offset + i] = COLOR_WHITE; } else
    if (value == 0xffff) { for (i = 0; i < 16; i++) ps[offset + i] = COLOR_BLACK; } else
    for (i = 0; i < 16; i++) {
      ps[offset + i] = (value & 0x8000) ? COLOR_BLACK : COLOR_WHITE;
      value <<= 1;
    }
    break;
  case 4:
    if (value == 0x0) { for (i = 0; i < 32; i++) ps[offset + i] = COLOR_WHITE; } else
    if (value == 0xffffffff) { for (i = 0; i < 32; i++) ps[offset + i] = COLOR_BLACK; } else
    for (i = 0; i < 32; i++) {
      ps[offset + i] = (value & 0x80000000) ? COLOR_BLACK : COLOR_WHITE;
      value <<= 1;
    }
    break;

  }

#ifdef __linux__
//  accumulate_update(h, v, 32, 1);
  // for some reason this slows down mac os with SDL 1.2
  SDL_UpdateRect(screen, h, v, size*8, 1);
#endif
}

void sdl_init(void)
{
    int flags;

#if 0
    cols = 1152;
    rows = 900;
#else
    cols = 1024;
    rows = 1024;
#endif

    if (0) printf("Initialize display %dx%d\n", cols, rows);

    flags = SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE;

    if (SDL_Init(flags)) {
        printf("SDL initialization failed\n");
        return;
    }

    flags = SDL_HWSURFACE|SDL_ASYNCBLIT|SDL_HWACCEL;

    screen = SDL_SetVideoMode(cols, rows, 8, flags);

    if (!screen) {
        printf("Could not open SDL display\n");
        return;
    }

    SDL_WM_SetCaption("Sun2", "Sun2");
}

void sun2_sdl_key(int sdl_code, int modifiers, unsigned int unicode, int down);

void sdl_poll(void)
{
  SDL_Event ev1, *ev = &ev1;

//  send_accumulated_updates();
  SDL_UpdateRect(screen, 0, 0, cols-1, rows-1);

  while (SDL_PollEvent(ev)) {
    switch (ev->type) {
    case SDL_VIDEOEXPOSE:
//      sdl_update(ds, 0, 0, screen->w, screen->h);
      break;

    case SDL_KEYDOWN:
      sun2_sdl_key(ev->key.keysym.sym, ev->key.keysym.mod, ev->key.keysym.unicode, 1);
      break;

    case SDL_KEYUP:
      sun2_sdl_key(ev->key.keysym.sym, ev->key.keysym.mod, ev->key.keysym.unicode, 0);
      break;
    case SDL_QUIT:
//      sdl_system_shutdown_request();
      break;
    case SDL_MOUSEMOTION:
//      sdl_send_mouse_event();
      break;
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    {
      /*SDL_MouseButtonEvent *bev = &ev->button;*/
//      sdl_send_mouse_event();
    }
    break;
    }
  }
}

void sun2_fb_alloc(void)
{
  if (fbmem == NULL) {
    fbmem = malloc(128*1024);
    memset(fbmem, 0, 128*1024);
    sdl_init();
  }
}

unsigned int sun2_video_read(unsigned int address, int size)
{
  unsigned int *p32;
  unsigned short *p16;
  unsigned char *p08;
  unsigned offset;

  sun2_fb_alloc();

  offset = address & 0xfffff;
  if (offset >= 128*1024)
    return 0xffffffff;

  switch (size) {
  case 1:
    p08 = (unsigned char *)(fbmem + offset);
    return *p08;
  case 2:
    p08 = (unsigned char *)(fbmem + offset);
    return (p08[0] << 8) | p08[1];
  case 4:
    p08 = (unsigned char *)(fbmem + offset);
    return (p08[0] << 24) | (p08[1] << 16) | (p08[2] << 8) | p08[3];
  default:
    return 0x0;
  }
}

unsigned int sun2_video_write(unsigned int address, int size, unsigned int value)
{
  unsigned int *p32;
  unsigned short *p16;
  unsigned char *p08;
  unsigned offset;

  sun2_fb_alloc();

  offset = address & 0xfffff;
  if (offset >= 128*1024)
    return -1;

  switch (size) {
  case 1:
    p08 = (unsigned char *)(fbmem + (address & 0xfffff));
    *p08 = value;
    break;
  case 2:
    p08 = (unsigned char *)(fbmem + (address & 0xfffff));
    p08[0] = value >> 8;
    p08[1] = value;
    break;
  case 4:
    p08 = (unsigned char *)(fbmem + (address & 0xfffff));
    p08[0] = value >> 24;
    p08[1] = value >> 16;
    p08[2] = value >> 8;
    p08[3] = value;
    break;
  }
  sdl_write(address & 0xfffff, size, value);
  return 0;
}

unsigned int sun2_kbm_read(unsigned int address, int size)
{
  unsigned int value;
  if (0) printf("sun2: scc read %x (%d)\n", address, size);
  value = scc_read(address, size);
  if (0) printf("sun2: scc read %x -> %x (%d)\n", address, value, size);
  return value;
}

unsigned int sun2_kbm_write(unsigned int address, int size, unsigned int value)
{
  if (0) printf("sun2: scc write %x <- %x(%d)\n", address, value, size);
  scc_write(address, value, size);
  return 0;
}

unsigned int sun2_video_ctl_read(unsigned int address, int size)
{
  printf("sun2: fb ctrl @ %x -> %x (%d)\n", address, fbctrl, size);
//fbctrl = 0;
  return fbctrl;
}

unsigned int sun2_video_ctl_write(unsigned int address, int size, unsigned int value)
{
  printf("sun2: fb ctrl @ %x <- %x (%d)\n", address, value, size);
  fbctrl = value & 0xe07e;
  return 0;
}

/* ----- */

void sun2_kb_write(int value, int size)
{
    switch (value) {
    case 0x01: /* reset */
      scc_in_push(3, 0xff);
      scc_in_push(3, 0x02);
      scc_in_push(3, 0x7f);
      break;
    case 0x02: /* bell on */
      break;
    case 0x03: /* bell off */
      /* send abort */
      scc_in_push(3, 0x00+1);
      scc_in_push(3, 0x00+77);
      scc_in_push(3, 0x80+77);
      scc_in_push(3, 0x80+1);

      scc_in_push(3, 0x7f);
      break;
    }
}

unsigned int map_sdl_to_sun2kb[512];

#define SHIFTED 0x100

void sun2_sdl_key(int sdl_code, int modifiers, unsigned int unicode, int down)
{
  unsigned int mapped, shifted;

  if (0) printf("sdl: %d %d %d %d\n", sdl_code, modifiers, unicode, down);

  mapped = map_sdl_to_sun2kb[sdl_code];
//  shifted = mapped & SHIFTED;

  mapped &= 0xff;
  if (down == 0)
    mapped |= 0x80;

//  if (shifted) {
//    scc_in_push(3, 0x84);
//    scc_in_push(3, mapped);
//    scc_in_push(3, 0x84);
//    return;
//  }

#if 0
  if (sdl_code == SDLK_SLASH && down) {
    extern unsigned char g_ram[];
    unsigned char *p = g_ram;
    printf("0: %02x%02x%02x%02x\n", p[0], p[1], p[2], p[3]); p += 4;
    printf("4: %02x%02x%02x%02x\n", p[0], p[1], p[2], p[3]); p += 4;
    printf("8: %02x%02x%02x%02x\n", p[0], p[1], p[2], p[3]); p += 4;
    printf("c: %02x%02x%02x%02x\n", p[0], p[1], p[2], p[3]);
  }
#endif

#if 1
  if (sdl_code == SDLK_QUOTE && down) {
    extern int trace_armed;
    if (trace_armed == 0) {
      trace_armed = 1;
      printf("TRACE ARMED!\n");
    } else {
      trace_armed = 0;
      printf("TRACE unarmed!\n");
    }
  }
#endif

#if 0
  if (sdl_code == SDLK_SEMICOLON && down) {
    toggle_trace = !toggle_trace;
    if (toggle_trace) {
      printf("TRACE ENABLED!\n");
      enable_trace(1);
    } else {
      printf("TRACE DISABLED!\n");
      enable_trace(0);
    }
  }
#endif

  scc_in_push(3, mapped);
}

#define m(f,t) map_sdl_to_sun2kb[(f)] = (t);
#define m_sh(f,t) map_sdl_to_sun2kb[(f)] = (t) | SHIFTED;

void sun2_init(void)
{
  m(SDLK_ESCAPE, 29);

  m(SDLK_1, 30);
  m(SDLK_2, 31);
  m(SDLK_3, 32);
  m(SDLK_4, 33);
  m(SDLK_5, 34);
  m(SDLK_6, 35);
  m(SDLK_7, 36);
  m(SDLK_8, 37);
  m(SDLK_9, 38);
  m(SDLK_0, 39);
  m(SDLK_MINUS, 40);
  m(SDLK_EQUALS, 41);
  m(SDLK_BACKQUOTE, 42);
  m(SDLK_BACKSPACE, 43);

  m(SDLK_TAB, 53);
  m(SDLK_q, 54);
  m(SDLK_w, 55);

  m(SDLK_e, 56);
  m(SDLK_r, 57);
  m(SDLK_t, 58);
  m(SDLK_y, 59);
  m(SDLK_u, 60);
  m(SDLK_i, 61);
  m(SDLK_o, 62);
  m(SDLK_p, 63);

  m(SDLK_LEFTBRACKET, 64);
  m(SDLK_RIGHTBRACKET, 65);

  m(SDLK_a, 77);
  m(SDLK_s, 78);
  m(SDLK_d, 79);

  m(SDLK_f, 80);
  m(SDLK_g, 81);
  m(SDLK_h, 82);
  m(SDLK_j, 83);
  m(SDLK_k, 84);
  m(SDLK_l, 85);
  m(SDLK_SEMICOLON, 86);
  m(SDLK_QUOTE, 87);

  m(SDLK_BACKSLASH, 88);
  m(SDLK_RETURN, 89);

  m(SDLK_z, 100);
  m(SDLK_x, 101);
  m(SDLK_c, 102);
  m(SDLK_v, 103);

  m(SDLK_b, 104);
  m(SDLK_n, 105);
  m(SDLK_m, 106);
  m(SDLK_COMMA, 107);
  m(SDLK_PERIOD, 108);
  m(SDLK_SLASH, 109);

  m(SDLK_SPACE, 121);

  m(SDLK_LCTRL, 76);
  m(SDLK_RCTRL, 76);
  m(SDLK_LSHIFT, 99);
  m(SDLK_RSHIFT, 111);

#if 0
  /* shifted */
  m_sh(SDLK_EXCLAIM, 30);
  m_sh(SDLK_AT, 31);

  m_sh(SDLK_HASH, 32);
  m_sh(SDLK_DOLLAR, 33);
  m_sh('%', 34);
  m_sh(SDLK_CARET, 35);
  m_sh(SDLK_AMPERSAND, 36);
  m_sh(SDLK_ASTERISK, 37);
  m_sh(SDLK_LEFTPAREN, 38);
  m_sh(SDLK_RIGHTPAREN, 39);

  m_sh(SDLK_UNDERSCORE, 40);
  m_sh(SDLK_PLUS, 41);
  m_sh('~', 42);

  m_sh('{', 64);
  m_sh('}', 65);

  m_sh(SDLK_COLON, 86);
  m_sh(SDLK_QUOTEDBL, 87);
  m_sh('|', 88);

  m_sh(SDLK_LESS, 107);
  m_sh(SDLK_GREATER, 108);
  m_sh(SDLK_QUESTION, 109);
#endif

//-----
//	SDLK_CLEAR		
//	SDLK_PAUSE		

//
//	SDLK_DELETE		
//
//	SDLK_UP
//	SDLK_DOWN		
//	SDLK_RIGHT		
//	SDLK_LEFT		
//	SDLK_INSERT		
//	SDLK_HOME		
//	SDLK_END		
//	SDLK_PAGEUP		
//	SDLK_PAGEDOWN		
//
//	SDLK_F1
//	SDLK_F2
//	SDLK_F3
//	SDLK_F4
//	SDLK_F5
//	SDLK_F6
//	SDLK_F7
//	SDLK_F8
//	SDLK_F9
//	SDLK_F10		
//	SDLK_F11		
//	SDLK_F12		
//	SDLK_F13		
//	SDLK_F14		
//	SDLK_F15		
//
//	SDLK_NUMLOCK		
//	SDLK_CAPSLOCK		
//	SDLK_SCROLLOCK		
//	SDLK_RSHIFT		
//	SDLK_LSHIFT		
//	SDLK_RCTRL		
//	SDLK_LCTRL		
//	SDLK_RALT		
//	SDLK_LALT		
//	SDLK_RMETA		
//	SDLK_LMETA		
//	SDLK_LSUPER		
//	SDLK_RSUPER		
//	SDLK_MODE		
//	SDLK_COMPOSE		
//
//	SDLK_HELP		
//	SDLK_PRINT		
//	SDLK_SYSREQ		
//	SDLK_BREAK		
//	SDLK_MENU		
//	SDLK_POWER		
//	SDLK_EURO		
//	SDLK_UNDO		

}

/* Local Variables:  */
/* mode: c           */
/* c-basic-offset: 2 */
/* End:              */
