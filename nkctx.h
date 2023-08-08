// SPDX-License-Identifier: Unlicense
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR

#include <assert.h>
#define NK_ASSERT(expr) assert(expr)

#include <nuklear.h>
#include <nuklear_gdip.h>

GdipFont*
nk_gdip_load_font(LPCWSTR name, int size, WORD fallback);

#define NK_COLOR_YELLOW     {0xFF, 0xEA, 0x00, 0xff}
#define NK_COLOR_RED        {0xFF, 0x17, 0x44, 0xff}
#define NK_COLOR_GREEN      {0x00, 0xE6, 0x76, 0xff}
#define NK_COLOR_CYAN       {0x03, 0xDA, 0xC6, 0xff}
#define NK_COLOR_BLUE       {0x29, 0x79, 0xFF, 0xff}
#define NK_COLOR_WHITE      {0xFF, 0xFF, 0xFF, 0xff}
#define NK_COLOR_BLACK      {0x00, 0x00, 0x00, 0xff}
#define NK_COLOR_GRAY       {0x1E, 0x1E, 0x1E, 0xff}
#define NK_COLOR_LIGHT      {0xBF, 0xBF, 0xBF, 0xff}
#define NK_COLOR_DARK       {0x2D, 0x2D, 0x2D, 0xFF}

typedef struct _NK_GUI_CTX
{
	HINSTANCE inst;
	HWND wnd;
	WNDCLASSW wc;
	RECT rect;
	GdipFont* font;
	struct nk_context* ctx;
	float width;
	float height;
	BOOL drive_change;
	nk_size drive_mb;
	nk_size page_size;
} NK_GUI_CTX;
extern NK_GUI_CTX nk;

void
nkctx_init(HINSTANCE inst,
	int x, int y, int width, int height,
	LPCWSTR class_name, LPCWSTR title,
	LPCWSTR font_name, int font_size);

void
nkctx_loop(void);

_Noreturn void
nkctx_fini(int code);

BOOL
nkctx_page(WCHAR drive, LPCWSTR path, nk_size min_mb, nk_size max_mb);

void
nkctx_main_window(struct nk_context* ctx, float width, float height);
