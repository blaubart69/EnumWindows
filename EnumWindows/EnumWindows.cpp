#include "pch.h"

#include "../../dirNT/dirNT/beewstring.h"
#include "../../dirNT/dirNT/Write.h"
#include "../../dirNT/dirNT/beeLib.h"
#include "../../dirNT/dirNT/nt.h"

bee::wstring buf;
bee::wstring ident;

void replaceCRLFwithSpace(bee::wstring* str)
{
	for (int i = 0; i < str->length(); ++i)
	{
		WCHAR& c = (*str)[i];
		if (c == L'\r' || c == L'\n')
		{
			c = L' ';
		}
	}
}

bool convert_from(const wchar_t* str, unsigned long * value)
{
	int cbLen = lstrlenW(str) * sizeof(WCHAR);
	nt::UNICODE_STRING ucs;
	ucs.Length = cbLen;
	ucs.MaximumLength = cbLen;
	ucs.Buffer = (PWSTR)str;

	return NT_SUCCESS(nt::RtlUnicodeStringToInteger(&ucs, 0, value));
}

bool get_GetWindowTextW(HWND hwnd, bee::wstring* buf)
{
	const int textlen = GetWindowTextLengthW(hwnd);
	if (textlen == 0)
	{
		if (GetLastError() != 0)
		{
			//fprintf(stderr, "E: rc: %d GetWindowTextLengthW (%p)", rc, hwnd);
			return false;
		}
	}

	if (textlen == 0)
	{
		buf->resize(0);
	}
	else
	{
		buf->resize(textlen + 1);
		int charsWritten = GetWindowTextW(hwnd, buf->data(), (int)buf->length());
		if (charsWritten == 0)
		{
			if (GetLastError() != 0)
			{
				//fprintf(stderr, "E: rc: %d GetWindowTextW (%p)", rc, hwnd);
				return false;
			}
		}
		buf->resize(charsWritten);
	}
	return true;
}

bool get_WM_GETTEXT(HWND hwnd, bee::wstring* buf, DWORD maxChars = 255)
{
	buf->resize(maxChars);
	LRESULT copiedChars = SendMessageW(hwnd, WM_GETTEXT, buf->length(), LPARAM(buf->data()));
	buf->resize(copiedChars);

	return copiedChars == 0 ? false : true;
}

// typedef BOOL (CALLBACK* WNDENUMPROC)(HWND, LPARAM);
BOOL CALLBACK proc_enumWindows(HWND hwnd, LPARAM lparam)
{
	const int depth = (int)lparam;
	static bee::wstring outBuf;

	if (get_WM_GETTEXT(hwnd, &buf))
	{
		replaceCRLFwithSpace(&buf);
		ident.assign(2 * depth, L' ');
		
		const BOOL visible = IsWindowVisible(hwnd);

		outBuf.sprintf(L"%s0x%x\t%s\t%s\n"
			, ident.c_str()
			, hwnd
			, visible ? L"VISIBLE" : L"HIDDEN"
			, buf.c_str());
		bee::Writer::Out().Write(outBuf);
	}

	EnumChildWindows(hwnd, proc_enumWindows, depth + 1);

	return TRUE;
}

int beeMain(int argc, wchar_t* argv[])
{
	int rc;
	bee::wstring errBuf;
	if (argc == 1)
	{
		rc = EnumWindows(proc_enumWindows, 0) == TRUE ? 0 : 1;
	}
	else if (argc == 2)
	{
		unsigned long hwnd; ;
		if ( !convert_from(argv[1], &hwnd) )
		{
			errBuf.sprintf(L"E: could not convert %s to a hex value\n", argv[1]);
			bee::Writer::Err().Write(errBuf);
			rc = 8;
		}
		else
		{
			rc = EnumChildWindows((HWND)hwnd, proc_enumWindows, 0) == TRUE ? 0 : 1;
		}
	}
	else
	{
		rc = 4;
		errBuf.sprintf(L"usage: %s [HWND (prepend 0x for hex values)]\n", argv[0]);
		bee::Writer::Err().Write(errBuf);
	}
	return rc;
}

