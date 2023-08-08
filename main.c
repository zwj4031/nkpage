// SPDX-License-Identifier: Unlicense

#include "nkctx.h"

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	nkctx_init(hInstance, CW_USEDEFAULT, CW_USEDEFAULT, 400, 200, L"NkPageWindowClass", L"NkPage", L"Courier New", 16);
	nkctx_loop();
	nkctx_fini(0);
	return 0;
}
