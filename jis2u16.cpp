#include <windows.h>

// 开cl用的
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")

#pragma comment(lib, "copyHook.lib")

#define WM_TRAY   (WM_USER + 100)
#define WM_COPIED (WM_USER + 128)

#define ID_ENABLE  40001
#define ID_DISABLE 40002
#define ID_EXIT    40003

NOTIFYICONDATA nid;
HMENU hMenu;

ATOM enableKey, disableKey, exitKey;
TCHAR szAppName[] = TEXT("JIS2U16");

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
VOID WINAPI ConvertClipboard(HWND);

// 懒得弄头文件
_declspec(dllexport) BOOL WINAPI LoadCopyHook(HWND hwnd);
_declspec(dllexport) BOOL WINAPI UnloadCopyHook();


int APIENTRY WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR szCmdLine, int iCmdShow)
{	
	HWND     hWnd;
	MSG      msg;
	WNDCLASS wc;

	wc.style         = NULL;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInst;
	wc.hIcon         = NULL;
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = szAppName;	
	
	if (!RegisterClass(&wc))
		return 1;
	
	hWnd = CreateWindowEx(WS_EX_TOOLWINDOW,
						  szAppName,
						  szAppName,
						  WS_POPUP,
						  CW_USEDEFAULT,
						  CW_USEDEFAULT,
						  CW_USEDEFAULT,
						  CW_USEDEFAULT,
						  NULL,
						  NULL,
						  hInst,
						  NULL
	);
	
	ShowWindow(hWnd, iCmdShow);
	UpdateWindow(hWnd);
	
	// 创建托盘图标
	
	nid.cbSize           = sizeof(NOTIFYICONDATA);
	nid.hWnd             = hWnd;
	nid.uID              = 101; // 没有资源文件，随便填
	nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	nid.uCallbackMessage = WM_TRAY;
	nid.hIcon            = LoadIcon(NULL, IDI_APPLICATION);
	lstrcpy(nid.szTip, szAppName);
	
	hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, ID_ENABLE,  TEXT("开启 (Ctrl+E)"));
	AppendMenu(hMenu, MF_STRING, ID_DISABLE, TEXT("关闭 (Ctrl+D)"));
	AppendMenu(hMenu, MF_STRING, ID_EXIT,    TEXT("退出 (Alt+Q)"));
	// 开启和关闭是互斥的
	EnableMenuItem(hMenu, ID_ENABLE, MF_DISABLED | MF_GRAYED);
	
	Shell_NotifyIcon(NIM_ADD, &nid);

	// 注册全局热键
	
	enableKey  = GlobalAddAtom(TEXT("enableKey"))  - 0xc000;
	disableKey = GlobalAddAtom(TEXT("disableKey")) - 0xc000;
	exitKey    = GlobalAddAtom(TEXT("exitKey"))    - 0xc000;
	
	RegisterHotKey(hWnd, enableKey,  MOD_CONTROL, 'E');
	RegisterHotKey(hWnd, disableKey, MOD_CONTROL, 'D');
	RegisterHotKey(hWnd, exitKey, MOD_ALT, 'Q');

	// 挂上钩子监测Ctrl+C事件
	if (!LoadCopyHook(hWnd))
	{
		MessageBox(hWnd, TEXT("装载键盘钩子失败，退出程序"), TEXT("错误"), MB_OK);
		return 1;
	}
	
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bConv = TRUE;
	static BOOL bMsgbox = FALSE;
	
	switch (uMsg)
	{
	case WM_TRAY:
		switch (lParam)
		{
		case WM_RBUTTONDOWN:
			{
				POINT pt;
				GetCursorPos(&pt);
				
				SetForegroundWindow(hWnd);
				
				int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hWnd, NULL);
				switch (cmd)
				{
enable:			case ID_ENABLE:
					EnableMenuItem(hMenu, ID_DISABLE, MF_ENABLED);
					EnableMenuItem(hMenu, ID_ENABLE, MF_DISABLED | MF_GRAYED);

					bConv = TRUE;
					break;
disable:		case ID_DISABLE:
					EnableMenuItem(hMenu, ID_ENABLE, MF_ENABLED);
					EnableMenuItem(hMenu, ID_DISABLE, MF_DISABLED | MF_GRAYED);

					bConv = FALSE;
					break;
exit:			case ID_EXIT:
					PostMessage(hWnd, WM_DESTROY, NULL, NULL);
				}
			}
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			// 不懂左键弹窗怎么弄，只能用这种粗鄙的方式（绝望）
			if (!bMsgbox)
			{
				bMsgbox = TRUE;
				MessageBox(hWnd,
						   TEXT("这个东西会自动把你剪切板里的东西当成Shift-JIS文本直接强转成UTF-16，右键菜单开启或关闭。\n"
							    "作者Voyage65535，如有问题请联系voyage65535@163.com"),
						   szAppName,
					       MB_OK);
				bMsgbox = FALSE;
			}
		}
		break;
	case WM_HOTKEY:
		if (wParam == enableKey)
			goto enable;
		else if (wParam == disableKey)
			goto disable;
		else if (wParam == exitKey)
			goto exit;			
		break;
	case WM_COPIED:
		if (bConv)
			ConvertClipboard(hWnd);
		break;
	case WM_DESTROY:

		// 销毁托盘图标
		Shell_NotifyIcon(NIM_DELETE, &nid);
		
		// 注销全局热键
		UnregisterHotKey(hWnd, enableKey);
		UnregisterHotKey(hWnd, disableKey);
		UnregisterHotKey(hWnd, exitKey);
		GlobalDeleteAtom(enableKey);
		GlobalDeleteAtom(disableKey);
		GlobalDeleteAtom(exitKey);
		
		// 卸载钩子
		UnloadCopyHook();
		
		PostQuitMessage (0);
		return 0;
	}
	
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}

VOID WINAPI ConvertClipboard(HWND hWnd)
{
	if (IsClipboardFormatAvailable(CF_TEXT))
		if (OpenClipboard(hWnd))
		{
			GLOBALHANDLE hGlobal = GetClipboardData(CF_TEXT);
			if (hGlobal)
			{
				LPSTR data = (LPSTR) malloc (GlobalSize (hGlobal));
				strcpy(data, (LPCSTR)GlobalLock(hGlobal));
				GlobalUnlock(hGlobal);

				// 932是JIS的代码页
				// 这个函数可以算出所需的缓冲区大小
				SIZE_T n = MultiByteToWideChar(932, 0, data, -1, NULL, 0);
				
				hGlobal = GlobalAlloc(GHND | GMEM_SHARE, n * sizeof(WCHAR));
				LPWSTR buf = (LPWSTR)GlobalLock(hGlobal);
				MultiByteToWideChar(932, 0, data, -1, buf, n);
				GlobalUnlock(hGlobal);

				EmptyClipboard();
				SetClipboardData(CF_UNICODETEXT, hGlobal);
	
				free(data);
			}	
			CloseClipboard();
		}
}
