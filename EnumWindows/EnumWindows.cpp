#include "pch.h"

std::wstring buf;
std::wstring ident;

bool convert_from(const wchar_t* str, long long int* value)
{
	wchar_t* endptr;
	*value = wcstoll(str, &endptr, 16);

	return *endptr == L'\0';
}

bool get_GetWindowTextW(HWND hwnd, std::wstring* buf)
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
		buf->clear();
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

bool get_WM_GETTEXT(HWND hwnd, std::wstring* buf, DWORD maxChars = 255)
{
	buf->resize(maxChars);
	LRESULT copiedChars = SendMessageW(hwnd, WM_GETTEXT, buf->size(), LPARAM(buf->data()));
	buf->resize(copiedChars);

	return copiedChars == 0 ? false : true;
}

// typedef BOOL (CALLBACK* WNDENUMPROC)(HWND, LPARAM);
BOOL CALLBACK proc_enumWindows(HWND hwnd, LPARAM lparam)
{
	const int depth = (int)lparam;
	
	if (get_WM_GETTEXT(hwnd, &buf))
	{
		for (auto c = buf.begin(); c != buf.end(); ++c)
		{
			if (*c == L'\r' || *c == L'\n')
			{
				*c = L' ';
			}
		}
		ident.assign(2 * depth, L' ');
		wprintf(L"%s0x%p\t[%s]\n", ident.c_str(), hwnd, buf.c_str());
	}

	EnumChildWindows(hwnd, proc_enumWindows, depth + 1);

	return TRUE;
}

int wmain(int argc, wchar_t* argv[])
{
	int rc;

	if (argc == 1)
	{
		rc = EnumWindows(proc_enumWindows, 0) == TRUE ? 0 : 1;
	}
	else if (argc == 2)
	{
		long long int hwnd; ;
		if ( !convert_from(argv[1], &hwnd) )
		{
			fprintf(stderr, "E: could not convert %S to a hex value\n", argv[1]);
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
		printf("usage: %S [HWND handle (hex)]\n", argv[0]);
	}
	return rc;
}

