#include <windows.h>

#pragma comment(lib, "user32.lib")

#define WM_COPIED (WM_USER + 128)

HWND   hWnd;
HHOOK  hHook;
HINSTANCE hModule;

LRESULT CALLBACK KbdProc(int nCode, WPARAM wParam, LPARAM lParam);

BOOL APIENTRY DllMain(HINSTANCE hMod, DWORD dwReason, LPVOID lpReserved)
{
	hModule = hMod;

	switch (dwReason)
	{
	case DLL_PROCESS_DETACH:
	case DLL_THREAD_DETACH:
		if (hHook)
			UnhookWindowsHookEx(hHook);
	}
	
	return TRUE;
}

// 指定发送消息到哪个窗口类（嗯，实际只有一个）
_declspec(dllexport) BOOL WINAPI LoadCopyHook(HWND hwnd)
{
	hWnd  = hwnd;
	hHook = SetWindowsHookEx(WH_KEYBOARD_LL, KbdProc, hModule, NULL);
	if (!hHook)
		return FALSE;
	return true;
}

_declspec(dllexport) BOOL WINAPI UnloadCopyHook()
{
	if (!UnhookWindowsHookEx(hHook))
		return FALSE;
	hHook = NULL;
	return TRUE;
}

// nCode不重要，每次都调用CallNextHookEx就行了
LRESULT CALLBACK KbdProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (wParam == WM_KEYUP)
		if (((KBDLLHOOKSTRUCT*)lParam)->vkCode == 'C')
			// 如果这个键被按下了，返回值(SHORT)高位是1
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
				PostMessage(hWnd, WM_COPIED, 0, 0);
	
	return CallNextHookEx(hHook, nCode, wParam, lParam);
}
