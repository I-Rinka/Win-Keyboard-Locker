#include <windows.h>
#include <string.h>
#include <strsafe.h>
#include <tchar.h>
#include "Resource.h"
#include "Name.h"
#include"wintoastlib.h"
//方便统一调用名称,前缀为WKBDL_

//定义WinProc处理的信息，对托盘图标操作后会发送WM_TRAYICON的message
#define WM_TRAYICON WM_USER+1

enum
{
	MENU_ACTION_LOCK,
	MENU_ACTION_UNLOCK,
	MENU_ACTION_EXIT
};

//使用全局句柄
HINSTANCE hInst;
HHOOK KBHook;//钩子句柄

//通过全局变量isLocked来判断当前键盘上锁状态
bool isLocked = false;
//判断当前操作系统是否支持Toast
bool SystemIsCopatiple = false;

//托盘图标
NOTIFYICONDATA TrayIcon = {};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KbdHkProc(int, WPARAM, LPARAM);

void CreatTrayMenu(HWND);//显示菜单
void KeyboardLocker();//核心程序，对钩子句柄操作，进行键盘的锁定/解锁
void ShowMessage_Lock();
void ShowMessage_Unlock();

void APP_EXIT(HWND);//退出程序
void APP_ERROR(int);

using namespace WinToastLib;

//在此实现对Toast选框的操作
class WinToastHandler : public IWinToastHandler {
public:
	void toastActivated() const {
		//click toast
	}

	void toastActivated(int actionIndex) const {
		//click button
		KeyboardLocker();
	}

	void toastDismissed(WinToastDismissalReason state) const {
		//switch state
	}

	void toastFailed() const {
		//error
	}
};

WinToastTemplate InitializeWinToastTemlate();
WinToastTemplate Toast32;


int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
	//windows窗口默认设置
	WNDCLASS wcex;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = WKBL_APP_NAME;

	if (!RegisterClass(&wcex))
	{
		APP_ERROR(3);
	}

	hInst = hInstance;

	//创建一个看不见的窗口
	HWND hwnd = CreateWindow(WKBL_APP_NAME, WKBL_APP_TITLE, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, NULL);
	if (!hwnd)
	{
		APP_ERROR(2);
	}

	//定义托盘图标
	TrayIcon.cbSize = sizeof(NOTIFYICONDATA);
	TrayIcon.hWnd = hwnd;
	TrayIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_KEYBOARDLOCKER));
	TrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
	TrayIcon.uCallbackMessage = WM_TRAYICON;
	StringCchCopy(TrayIcon.szTip, ARRAYSIZE(TrayIcon.szTip), WKBL_TRAYICON_HOVER);
	StringCchCopy(TrayIcon.szInfoTitle, ARRAYSIZE(TrayIcon.szInfoTitle), WKBL_APP_TITLE);
	StringCchCopy(TrayIcon.szInfo, ARRAYSIZE(TrayIcon.szInfo), WKBL_NOTIFY_START);
	Shell_NotifyIcon(NIM_ADD, &TrayIcon);

	//初始化Toast
	if (WinToast::isCompatible())
	{
		SystemIsCopatiple = true;
		WinToast::instance()->setAppName(WKBL_APP_NAME);
		WinToast::instance()->setAppUserModelId(WKBL_APP_TITLE);
		WinToast::instance()->initialize();
		Toast32 = InitializeWinToastTemlate();
	}


	// Main message loop:
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		if (HIWORD(wParam) == 0)
		{
			if (LOWORD(wParam) == MENU_ACTION_LOCK)
			{
				KeyboardLocker();
			}
			if (LOWORD(wParam) == MENU_ACTION_EXIT)
			{
				APP_EXIT(hwnd);
			}
		}
		break;

	case WM_TRAYICON:
		if (lParam == WM_RBUTTONUP)
		{
			SetForegroundWindow(hwnd);//使得菜单出现后，单击菜单外的使地方选框消失
			CreatTrayMenu(hwnd);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

LRESULT CALLBACK KbdHkProc(int code, WPARAM wParam, LPARAM lParam)
{
	//忽略键盘的动作,以后或许可以在这里来扩充忽略的输入类型
	if (code == HC_ACTION)
	{
		return 1;
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}

//创建菜单
void CreatTrayMenu(HWND hwnd)
{
	HMENU hMenu = CreatePopupMenu();

	//规定菜单选项
	if (isLocked)
	{
		AppendMenu(hMenu, MF_STRING, MENU_ACTION_LOCK, WKBL_MENU_UNLOCK);//因为此时键盘锁定了，所以要显示“解锁”
	}
	else
	{
		AppendMenu(hMenu, MF_STRING, MENU_ACTION_LOCK, WKBL_MENU_LOCK);
	}
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, MENU_ACTION_EXIT, WKBL_MENU_EXIT);

	//鼠标点击菜单传出消息
	POINT pt;
	GetCursorPos(&pt);
	TrackPopupMenu(hMenu, TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, NULL);
}

//退出程序
void APP_EXIT(HWND hwnd)
{
	Shell_NotifyIcon(NIM_DELETE, &TrayIcon);
	::DestroyWindow(hwnd);
}

void APP_ERROR(int ErrorNumber)
{
	MessageBox(NULL, WKBL_APP_ERROR, WKBL_APP_TITLE, NULL);
	exit(ErrorNumber);
}

WinToastTemplate InitializeWinToastTemlate()
{
	WinToastTemplate temp(WinToastTemplate::Text03);
	temp.setFirstLine(WKBL_APP_TITLE);
	temp.setSecondLine(WKBL_NOTIFY_LOCK);
	temp.setAttributionText(WKBL_NOTIFY_LOCK2);
	temp.addAction(WKBL_MENU_UNLOCK);
	temp.setDuration(WinToastTemplate::Duration::Short);
	return temp;
}

//显示消息
void ShowMessage_Unlock()
{
	StringCchCopy(TrayIcon.szInfo, ARRAYSIZE(TrayIcon.szInfo), WKBL_NOTIFY_UNLOCK);
	Shell_NotifyIcon(NIM_MODIFY, &TrayIcon);
}
void ShowMessage_Lock()
{
	StringCchCopy(TrayIcon.szInfo, ARRAYSIZE(TrayIcon.szInfo), WKBL_NOTIFY_LOCK);
	Shell_NotifyIcon(NIM_MODIFY, &TrayIcon);
}

//核心程序，控制解锁与否
void KeyboardLocker()
{
	if (isLocked)
	{
		UnhookWindowsHookEx(KBHook);//解锁
		ShowMessage_Unlock();
		isLocked = false;
	}
	else
	{
		KBHook = SetWindowsHookEx(13, (HOOKPROC)KbdHkProc, (HINSTANCE)GetModuleHandle(NULL), 0);//锁定键盘
		if (SystemIsCopatiple)
		{
			WinToast::instance()->showToast(Toast32, new WinToastHandler());
		}
		else
		{
			ShowMessage_Lock();
		}
		isLocked = true;
	}
}
