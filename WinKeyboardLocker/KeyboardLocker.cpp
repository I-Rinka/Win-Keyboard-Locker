#include <windows.h>
#include <string.h>
#include <strsafe.h>
#include <tchar.h>
#include "Resource.h"
#include "Name.h"
#include"wintoastlib.h"
//����ͳһ��������,ǰ׺ΪWKBDL_

//����WinProc�������Ϣ��������ͼ�������ᷢ��WM_TRAYICON��message
#define WM_TRAYICON WM_USER+1

enum
{
	MENU_ACTION_LOCK,
	MENU_ACTION_UNLOCK,
	MENU_ACTION_EXIT
};

//ʹ��ȫ�־��
HINSTANCE hInst;
HHOOK KBHook;//���Ӿ��

//ͨ��ȫ�ֱ���isLocked���жϵ�ǰ��������״̬
bool isLocked = false;
//�жϵ�ǰ����ϵͳ�Ƿ�֧��Toast
bool SystemIsCopatiple = false;

//����ͼ��
NOTIFYICONDATA TrayIcon = {};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KbdHkProc(int, WPARAM, LPARAM);

void CreatTrayMenu(HWND);//��ʾ�˵�
void KeyboardLocker();//���ĳ��򣬶Թ��Ӿ�����������м��̵�����/����
void ShowMessage_Lock();
void ShowMessage_Unlock();

void APP_EXIT(HWND);//�˳�����
void APP_ERROR(int);

using namespace WinToastLib;

//�ڴ�ʵ�ֶ�Toastѡ��Ĳ���
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
	//windows����Ĭ������
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

	//����һ���������Ĵ���
	HWND hwnd = CreateWindow(WKBL_APP_NAME, WKBL_APP_TITLE, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, NULL);
	if (!hwnd)
	{
		APP_ERROR(2);
	}

	//��������ͼ��
	TrayIcon.cbSize = sizeof(NOTIFYICONDATA);
	TrayIcon.hWnd = hwnd;
	TrayIcon.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_KEYBOARDLOCKER));
	TrayIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_INFO;
	TrayIcon.uCallbackMessage = WM_TRAYICON;
	StringCchCopy(TrayIcon.szTip, ARRAYSIZE(TrayIcon.szTip), WKBL_TRAYICON_HOVER);
	StringCchCopy(TrayIcon.szInfoTitle, ARRAYSIZE(TrayIcon.szInfoTitle), WKBL_APP_TITLE);
	StringCchCopy(TrayIcon.szInfo, ARRAYSIZE(TrayIcon.szInfo), WKBL_NOTIFY_START);
	Shell_NotifyIcon(NIM_ADD, &TrayIcon);

	//��ʼ��Toast
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
			SetForegroundWindow(hwnd);//ʹ�ò˵����ֺ󣬵����˵����ʹ�ط�ѡ����ʧ
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
	//���Լ��̵Ķ���,�Ժ���������������������Ե���������
	if (code == HC_ACTION)
	{
		return 1;
	}
	return CallNextHookEx(NULL, code, wParam, lParam);
}

//�����˵�
void CreatTrayMenu(HWND hwnd)
{
	HMENU hMenu = CreatePopupMenu();

	//�涨�˵�ѡ��
	if (isLocked)
	{
		AppendMenu(hMenu, MF_STRING, MENU_ACTION_LOCK, WKBL_MENU_UNLOCK);//��Ϊ��ʱ���������ˣ�����Ҫ��ʾ��������
	}
	else
	{
		AppendMenu(hMenu, MF_STRING, MENU_ACTION_LOCK, WKBL_MENU_LOCK);
	}
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, MENU_ACTION_EXIT, WKBL_MENU_EXIT);

	//������˵�������Ϣ
	POINT pt;
	GetCursorPos(&pt);
	TrackPopupMenu(hMenu, TPM_RIGHTALIGN, pt.x, pt.y, 0, hwnd, NULL);
}

//�˳�����
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

//��ʾ��Ϣ
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

//���ĳ��򣬿��ƽ������
void KeyboardLocker()
{
	if (isLocked)
	{
		UnhookWindowsHookEx(KBHook);//����
		ShowMessage_Unlock();
		isLocked = false;
	}
	else
	{
		KBHook = SetWindowsHookEx(13, (HOOKPROC)KbdHkProc, (HINSTANCE)GetModuleHandle(NULL), 0);//��������
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
