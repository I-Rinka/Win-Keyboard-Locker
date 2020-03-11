#include <windows.h>
#include <string.h>
#include <strsafe.h>
#include <tchar.h>
#include "Resource.h"
#include "Name.h"
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

//����ͼ��
NOTIFYICONDATA TrayIcon = {};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KbdHkProc(int, WPARAM, LPARAM);

void CreatTrayMenu(HWND);//��ʾ�˵�
void KeyboardLocker(HWND);//���ĳ��򣬶Թ��Ӿ�����������м��̵�����/����
void ExitWKBL(HWND);//�˳�����
void ShowMessage_Lock(HWND);
void ShowMessage_Unlock(HWND);



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
		MessageBox(NULL, WKBL_APP_FAIL, WKBL_APP_TITLE, NULL);
		return 1;
	}

	hInst = hInstance;

	//����һ���������Ĵ���
	HWND hwnd = CreateWindow(WKBL_APP_NAME, WKBL_APP_TITLE, WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, 0, 0, hInstance, NULL);
	if (!hwnd)
	{
		MessageBox(NULL, WKBL_APP_FAIL, WKBL_APP_TITLE, NULL);
		return 2;
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
				KeyboardLocker(hwnd);
			}
			if (LOWORD(wParam) == MENU_ACTION_EXIT)
			{
				ExitWKBL(hwnd);
			}
		}
		break;

	case WM_TRAYICON:
		if (lParam == WM_RBUTTONUP)
		{
			SetForegroundWindow(hwnd);//ʹ�ò˵����ֺ󣬵����˵����ʹ�ط�ѡ����ʧ
			CreatTrayMenu(hwnd);
		}
		if (lParam == NIN_BALLOONUSERCLICK)
		{
			MessageBox(hwnd, WKBL_APP_NAME, WKBL_APP_TITLE, 0);
		}
		//if (isLocked && lParam == NIN_BALLOONUSERCLICK)//�趨Ϊֻ����Ҫ����ʱ���֪ͨ������
		//{
		//	KeyboardLocker(hwnd);
		//}��֪ͨ���ĵ������Ȼ����Ӧ����Ҫ����
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
void ExitWKBL(HWND hwnd)
{
	Shell_NotifyIcon(NIM_DELETE, &TrayIcon);
	::DestroyWindow(hwnd);
}

//��ʾ��Ϣ
void ShowMessage_Unlock(HWND hwnd)
{
	TrayIcon.hWnd = hwnd;
	StringCchCopy(TrayIcon.szInfo, ARRAYSIZE(TrayIcon.szInfo), WKBL_NOTIFY_UNLOCK);
	Shell_NotifyIcon(NIM_MODIFY, &TrayIcon);
}
void ShowMessage_Lock(HWND hwnd)
{
	TrayIcon.hWnd = hwnd;
	StringCchCopy(TrayIcon.szInfo, ARRAYSIZE(TrayIcon.szInfo), WKBL_NOTIFY_LOCK);
	Shell_NotifyIcon(NIM_MODIFY, &TrayIcon);
}

//���ĳ��򣬿��ƽ������
void KeyboardLocker(HWND hwnd)
{
	if (isLocked)
	{
		UnhookWindowsHookEx(KBHook);//����
		ShowMessage_Unlock(hwnd);
		isLocked = false;
	}
	else
	{
		KBHook = SetWindowsHookEx(13, (HOOKPROC)KbdHkProc, (HINSTANCE)GetModuleHandle(NULL), 0);//��������
		ShowMessage_Lock(hwnd);
		isLocked = true;
	}
}
