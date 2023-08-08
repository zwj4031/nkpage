// SPDX-License-Identifier: Unlicense

#include "nkctx.h"
#include "resource.h"

NK_GUI_CTX nk;

static LRESULT CALLBACK
nkctx_window_proc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_DPICHANGED:
		break;
	case WM_SIZE:
		nk.height = (float)HIWORD(lparam);
		nk.width = (float)LOWORD(lparam);
		break;
	}
	if (nk_gdip_handle_event(wnd, msg, wparam, lparam))
		return 0;
	return DefWindowProcW(wnd, msg, wparam, lparam);
}

static void
set_style(struct nk_context* ctx)
{
	ctx->style.button.rounding = 0;
	ctx->style.button.border = 1.0f;
	ctx->style.button.padding = nk_vec2(0.0f, 0.0f);
}

void
nkctx_init(HINSTANCE inst,
	int x, int y, int width, int height,
	LPCWSTR class_name, LPCWSTR title,
	LPCWSTR font_name, int font_size)
{
	DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	DWORD exstyle = WS_EX_APPWINDOW;

	ZeroMemory(&nk, sizeof(nk));

	nk.inst = inst;
	nk.drive_change = TRUE;
	nk.page_size = 8192;

	nk.wc.style = CS_DBLCLKS;
	nk.wc.lpfnWndProc = nkctx_window_proc;
	nk.wc.hInstance = inst;
	nk.wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(IDI_MAIN_ICON));
	nk.wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	nk.wc.lpszClassName = class_name;
	RegisterClassW(&nk.wc);

	nk.wnd = CreateWindowExW(exstyle, class_name, title, style,
		x, y, width, height,
		NULL, NULL, inst, NULL);
	GetWindowRect(nk.wnd, &nk.rect);
	AdjustWindowRectExForDpi(&nk.rect, style, FALSE, exstyle, USER_DEFAULT_SCREEN_DPI);

	nk.width = (float)(nk.rect.right - nk.rect.left);
	nk.height = (float)(nk.rect.bottom - nk.rect.top);
	nk.ctx = nk_gdip_init(nk.wnd, (unsigned)nk.width, (unsigned)nk.height);
	set_style(nk.ctx);

	nk.font = nk_gdip_load_font(font_name, font_size, IDR_DEFAULT_FONT);
	nk_gdip_set_font(nk.font);
	//MessageBoxW(nk.wnd, L"INIT OK", title, MB_OK);
}

void
nkctx_loop(void)
{
	int running = 1;
	int needs_refresh = 1;

	while (running)
	{
		/* Input */
		MSG msg;
		nk_input_begin(nk.ctx);
		if (needs_refresh == 0)
		{
			if (GetMessageW(&msg, NULL, 0, 0) <= 0)
				running = 0;
			else
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
			needs_refresh = 1;
		}
		else
			needs_refresh = 0;
		while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				running = 0;
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
			needs_refresh = 1;
		}
		nk_input_end(nk.ctx);

		/* GUI */
		nkctx_main_window(nk.ctx, nk.width, nk.height);

		/* Draw */
		nk_gdip_render(NK_ANTI_ALIASING_ON, (struct nk_color)NK_COLOR_BLACK);
	}
}

_Noreturn void
nkctx_fini(int code)
{
	nk_gdipfont_del(nk.font);
	nk_gdip_shutdown();
	UnregisterClassW(nk.wc.lpszClassName, nk.wc.hInstance);
	exit(code);
}
