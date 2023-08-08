// SPDX-License-Identifier: Unlicense

#include <windows.h>
#include <winioctl.h>
#include <inttypes.h>

#include "nkctx.h"

typedef struct _UNICODE_STRING
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, * PUNICODE_STRING;

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

#define MB_TO_BYTES(mb) (1024ULL * 1024ULL * mb)

#define REG_PageFile TEXT("PagingFiles")
#define REG_MemMgr TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management")

static DWORD
get_privilege(LPWSTR privilege)
{
	HANDLE token;
	TOKEN_PRIVILEGES tkp = { 0 };
	BOOL res;
	// Obtain required privileges
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
		return GetLastError();
	res = LookupPrivilegeValueW(NULL, privilege, &tkp.Privileges[0].Luid);
	if (!res)
		return GetLastError();
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	AdjustTokenPrivileges(token, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
	return GetLastError();
}

static VOID*
get_reg_value(HKEY root, LPCWSTR subkey, LPCWSTR value, LPDWORD data_size, LPDWORD type)
{
	HKEY key = NULL;
	DWORD size = 0;
	LSTATUS rc;
	VOID* data = NULL;

	rc = RegOpenKeyExW(root, subkey, 0, KEY_QUERY_VALUE, &key);
	if (rc != ERROR_SUCCESS)
		goto fail;
	rc = RegQueryValueExW(key, value, NULL, type, NULL, &size);
	if (rc != ERROR_SUCCESS || size == 0)
		goto fail;
	data = malloc(size);
	if (!data)
		goto fail;
	rc = RegQueryValueExW(key, value, NULL, type, data, &size);
	if (rc != ERROR_SUCCESS)
		goto fail;
	RegCloseKey(key);
	*data_size = size;
	return data;
fail:
	if (key)
		RegCloseKey(key);
	if (data)
		free(data);
	*data_size = 0;
	return NULL;
}

static BOOL
set_reg_value(HKEY root, LPCWSTR subkey, LPCWSTR value, LPCVOID data, DWORD data_size, DWORD type)
{
	HKEY key = NULL;
	LSTATUS rc;
	rc = RegOpenKeyExW(root, subkey, 0, KEY_SET_VALUE, &key);
	if (rc != ERROR_SUCCESS)
		return FALSE;
	rc = RegSetValueExW(key, value, 0, type, data, data_size);
	RegCloseKey(key);
	return rc == ERROR_SUCCESS;
}

static void
set_page_file_reg(WCHAR drive, LPCWSTR path, nk_size min_mb, nk_size max_mb)
{
	WCHAR buf[MAX_PATH];
	WCHAR* old_reg = NULL;
	WCHAR* new_reg = NULL;
	DWORD old_size = 0;
	DWORD new_size = 0;
	DWORD type = 0;
	DWORD pos = 0;

	swprintf(buf, MAX_PATH, L"%C:\\%s %" PRIuPTR "u %" PRIuPTR "u", drive, path, min_mb, max_mb);

	old_reg = get_reg_value(HKEY_LOCAL_MACHINE, REG_MemMgr, REG_PageFile, &old_size, &type);
	if (type != REG_MULTI_SZ)
		goto fail;
	old_size /= sizeof(WCHAR);
	if (old_reg != NULL && old_size > 2)
		pos = old_size - 1;
	else
		old_size = 0;
	new_size = old_size + (DWORD)wcslen(buf) + 1;
	new_reg = calloc(new_size, sizeof(WCHAR));
	if (!new_reg)
		goto fail;
	if (old_reg)
		memcpy(new_reg, old_reg, old_size * sizeof(WCHAR));
	memcpy(&new_reg[pos], buf, wcslen(buf) * sizeof(WCHAR));
	new_reg[new_size - 1] = L'\0';
	set_reg_value(HKEY_LOCAL_MACHINE, REG_MemMgr, REG_PageFile, new_reg, new_size * sizeof(WCHAR), REG_MULTI_SZ);
fail:
	if (old_reg)
		free(old_reg);
	if (new_reg)
		free(new_reg);
}


BOOL nkctx_page(WCHAR drive, LPCWSTR path, nk_size min_mb, nk_size max_mb)
{
	NTSTATUS rc;
	UNICODE_STRING str;
	WCHAR device[] = L"C:";
	WCHAR buf[MAX_PATH];
	ULARGE_INTEGER min_size, max_size;
	VOID(NTAPI * NtInitUnicodeString)(PUNICODE_STRING dst, PCWSTR src) = NULL;
	NTSTATUS(NTAPI * NtCreatePagingFile)(PUNICODE_STRING path, PULARGE_INTEGER min_size, PULARGE_INTEGER max_size, ULONG priority) = NULL;
	HMODULE ntdll = GetModuleHandleW(L"ntdll");
	if (!ntdll)
		return FALSE;
	*(FARPROC*)&NtCreatePagingFile = GetProcAddress(ntdll, "NtCreatePagingFile");
	if (!NtCreatePagingFile)
		return FALSE;
	*(FARPROC*)&NtInitUnicodeString = GetProcAddress(ntdll, "RtlInitUnicodeString");
	if (!NtInitUnicodeString)
		return FALSE;
	if (get_privilege(SE_CREATE_PAGEFILE_NAME) != ERROR_SUCCESS)
		return FALSE;
	if (max_mb < min_mb)
		max_mb = min_mb;
	if (path == NULL)
		path = L"\\pagefile.sys";
	device[0] = towupper(drive);
	if (QueryDosDeviceW(device, buf, MAX_PATH) > 0)
		wcscat_s(buf, MAX_PATH, path);
	else
		return FALSE;
	NtInitUnicodeString(&str, buf);
	min_size.QuadPart = MB_TO_BYTES(min_mb);
	max_size.QuadPart = MB_TO_BYTES(max_mb);
	rc = NtCreatePagingFile(&str, &min_size, &max_size, 0);
	if (!NT_SUCCESS(rc))
		return FALSE;
	set_page_file_reg(drive, path, min_mb, max_mb);
	return TRUE;
}

