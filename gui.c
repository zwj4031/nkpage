// SPDX-License-Identifier: Unlicense

#include "nkctx.h"
#include <inttypes.h>

static WCHAR
draw_drive_list(struct nk_context* ctx)
{
	static const char* items[] =
	{ "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z" };
	static int count = 0;
	static int selected = 2;
	int i = 0;
	if (nk_combo_begin_label(ctx, items[selected], nk_vec2(nk_widget_width(ctx), 96)))
	{
		DWORD mask = GetLogicalDrives();
		if (mask == 0)
			mask = 1 << 2;
		nk.drive_change = TRUE;
		nk_layout_row_dynamic(ctx, 32, 1);
		for (i = 0; i < 26; ++i)
		{
			if (mask & (1 << i))
			{
				if (nk_combo_item_label(ctx, items[i], NK_TEXT_LEFT))
					selected = i;
			}
		}
		nk_combo_end(ctx);
	}
	return items[selected][0];
}

static nk_size
get_drive_mb(WCHAR drive)
{
	WCHAR path[] = L"?:\\";
	ULARGE_INTEGER space = { 0 };
	path[0] = drive;
	if (GetDiskFreeSpaceExW(path, NULL, NULL, &space))
		return (nk_size)(space.QuadPart / (1024 * 1024));
	return 0;
}

static void
draw_size_editor(struct nk_context* ctx, WCHAR drive)
{
#define EDIT_SIZE 32
	static char buf[EDIT_SIZE] = "8192";
	nk_size size;

	if (nk.drive_change)
	{
		nk.drive_change = FALSE;
		nk.drive_mb = get_drive_mb(drive);
		if (nk.page_size > nk.drive_mb)
			nk.page_size = nk.drive_mb / 2;
		snprintf(buf, EDIT_SIZE, "%" PRIuPTR, nk.page_size);
	}
	if (nk.page_size > nk.drive_mb)
	{
		nk.page_size = nk.drive_mb;
		snprintf(buf, EDIT_SIZE, "%" PRIuPTR, nk.page_size);
	}
	if (nk_progress(ctx, &nk.page_size, nk.drive_mb, TRUE))
	{
		snprintf(buf, EDIT_SIZE, "%" PRIuPTR, nk.page_size);
	}
	size = (nk_size)strtoul(buf, NULL, 10);
	if (size != nk.page_size)
	{
		nk.page_size = size;
	}
	nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buf, EDIT_SIZE, nk_filter_decimal);
}

void
nkctx_main_window(struct nk_context* ctx, float width, float height)
{
	UINT size = 8192;
	WCHAR drive = L'C';
	WCHAR* path = L"\\pagefile.sys";
	if (!nk_begin(ctx, "Main", nk_rect(0.0f, 0.0f, width, height), NK_WINDOW_BACKGROUND))
		goto out;

	nk_layout_row_dynamic(ctx, 20, 1);
	nk_spacer(ctx);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 2, (float[2]) { 0.25f, 0.75f });
	drive = draw_drive_list(ctx);
	nk_labelf(ctx, NK_TEXT_LEFT, "%lC:%S", drive, path);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.25f, 0.3f, 0.45f });
	draw_size_editor(ctx, drive);
	nk_labelf(ctx, NK_TEXT_LEFT, "MB / %" PRIuPTR " MB", nk.drive_mb);

	nk_layout_row(ctx, NK_DYNAMIC, 0, 3, (float[3]) { 0.25f, 0.4f, 0.35f });
	nk_spacer(ctx);
	if (nk_button_label(ctx, u8"设置"))
	{
		if (nkctx_page(drive, path, nk.page_size, nk.page_size))
			MessageBoxW(nk.wnd, L"设置成功", L"页面文件", MB_OK);
		else
			MessageBoxW(nk.wnd, L"设置失败", L"页面文件", MB_OK);
	}
	nk_spacer(ctx);

out:
	nk_end(ctx);
}
