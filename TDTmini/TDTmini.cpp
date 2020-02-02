// TDTmini.cpp : 定义应用程序的入口点。
//

#include "framework.h"
#include "TDTmini.h"
#include <TlHelp32.h>
#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING] = L"被捕捉的极域电子教室";                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING] = L"TDK";            // 主窗口类名
wchar_t szVDesk[] = L"TDTdesk", ScreenWindow[] = L"tdhwscr";
// 此代码模块中包含的函数的前向声明:
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
ATOM MyRegisterClass(HINSTANCE hInstance, WNDPROC proc, LPCWSTR cs);
bool NewDesktop = false;
HWND tdhw, tdhw2, tdhwscr, CatchWnd;
void s(LPCWSTR a)//调试用MessageBox
{
	MessageBox(NULL, a, L"", NULL);
}
void s(int a)
{
	wchar_t tmp[34];
	_itow_s(a, tmp, 10);
	MessageBox(NULL, tmp, L"", NULL);
}
HHOOK KeyboardHook, MouseHook;
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) { return CallNextHookEx(MouseHook, nCode, wParam, lParam); }//空的全局钩子函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) { return CallNextHookEx(KeyboardHook, nCode, wParam, lParam); }//防止极域钩住这些

DWORD WINAPI SDThread(LPVOID pM)
{//useless?
	(pM);
	HDESK vdback;
	if (!NewDesktop)//从原始桌面切换到新桌面
		vdback = OpenDesktopW(szVDesk, DF_ALLOWOTHERACCOUNTHOOK, false, GENERIC_ALL);
	else//切换回来
		vdback = OpenDesktopW(L"Default", DF_ALLOWOTHERACCOUNTHOOK, false, GENERIC_ALL);
	NewDesktop = !NewDesktop;
	SetThreadDesktop(vdback);
	SwitchDesktop(vdback);//while (NewDesktop != ndback)Sleep(100);
	return 0;
}
LRESULT CALLBACK ScreenProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) { return 0; }
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	MyRegisterClass(hInstance, WndProc, szWindowClass);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nCmdShow))return FALSE;

	MSG msg;

	// 主消息循环:
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}
void KillProcess(LPCWSTR ProcessName)//根据进程名结束进程.
{

	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;//创建进程快照
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapShot, &pe))return;
	while (Process32Next(hSnapShot, &pe))
	{
		pe.szExeFile[3] = 0;//根据进程名前三个字符标识
		_wcslwr_s(pe.szExeFile);
		if (wcsstr(ProcessName, pe.szExeFile) != 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
			TerminateProcess(hProcess, 1);
			CloseHandle(hProcess);
		}
	}
}

ATOM MyRegisterClass(HINSTANCE h, WNDPROC proc, LPCWSTR ClassName)
{//封装过的注册Class函数.
	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style =0;
	wcex.lpfnWndProc = proc;
	wcex.hInstance = h;
	wcex.hIcon = LoadIcon(h, MAKEINTRESOURCE(101));//这个函数不支持自定义窗体图标
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_GUI);
	wcex.lpszClassName = ClassName;//自定义ClassName和WndProc
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(101));//小图标
	return RegisterClassExW(&wcex);
}

BOOL CALLBACK EnumChildwnd(HWND hwnd, LPARAM lParam)//查找极域广播子窗口的枚举函数.
{
	if (lParam == 1)
	{
		EnableWindow(hwnd, true);
		return true;
	}
	LONG A;//遍历子窗口
	A = GetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE;
	if (A == 0)return 1;//极域2015和2016版本的广播窗口不是单一的，
	wchar_t title[301];//正常看到的全屏窗口里面还有TDDesk Render Window(广播内容窗口)和工具栏窗口。
	GetWindowText(hwnd, title, 300);//这里枚举极域广播窗口的子窗口，找到TDDesk Render Window，记录下来，
	if (wcsstr(title, L"TDDesk Render Window") != 0)//然后捕捉到CatchWnd里面去
	{
		SetParent(hwnd, CatchWnd);
		tdhw = hwnd;
		return 0;//return 0 代表中断查找
	}
	return 1;
}
bool FS = true;
BOOL CALLBACK EnumBroadcastwnd(HWND hwnd, LPARAM lParam)//查找"屏幕广播"窗口的枚举函数.
{
	LONG A;//遍历所有窗口
	A = GetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE;
	if (A == 0)return 1;
	wchar_t title[301];
	GetWindowText(hwnd, title, 300);
	if (wcscmp(title, L"屏幕广播") == 0)
	{
		EnumChildWindows(hwnd, EnumChildwnd, NULL);
		if (tdhw == 0)return FALSE;
		ShowWindow(CatchWnd, SW_SHOW);
		tdhwscr = CreateWindowW(szWindowClass, L"1", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInst, nullptr);
		SetParent(hwnd, tdhwscr);
		tdhw2 = hwnd;
		return FALSE;
	}
	return 1;
}
BOOL CALLBACK EnumBroadcastwndOld(HWND hwnd, LPARAM lParam)//查找被极域广播窗口的枚举函数.
{
	LONG A;//遍历所有窗口
	A = GetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE;//这个函数用于2007、2010、2012版本的极域
	if (A == 0)return 1;
	wchar_t title[301];//较老版本的极域里面没有TDDesk Render Window，极域广播窗口也不叫"屏幕广播"，所以需要专门处理
	GetWindowText(hwnd, title, 300);
	if (wcsstr(title, L"屏幕演播室窗口") != 0 || wcsstr(title, L"屏幕广播窗口") != 0)//屏幕演播室窗口->2010 or 2012版，屏幕广播窗口->2007版
	{
		ShowWindow(CatchWnd, SW_SHOW);
		tdhw = hwnd; tdhw2 = (HWND)(-2);
		SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_WINDOWEDGE | WS_EX_TOPMOST | WS_EX_LEFT | WS_EX_CLIENTEDGE);
		SetParent(hwnd, CatchWnd);//2010、2012版极域使用这个功能效果较好，
		return 0;
	}
	return 1;
}

void CALLBACK TimerProc(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime)//主计时器
{
	(hWnd); (nMsg); (dwTime); (nTimerid);
	if (nTimerid == 1)
	{
		if (tdhw != 0 || tdhw2 != 0)return;
		tdhw = tdhw2 = 0;
		EnumWindows(EnumBroadcastwnd, NULL);
		if (tdhw == 0)EnumWindows(EnumBroadcastwndOld, NULL);
		if (tdhw != 0)
		{
			if (tdhw2 == (HWND)-2)
			{
				RECT rc;
				GetClientRect(CatchWnd, &rc);
				SetWindowPos(tdhw, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOREDRAW);
			}
			else
				SetTimer(hWnd, 2, 100, (TIMERPROC)TimerProc);
		}
		return;
	}
	if (nTimerid == 2)
	{
		RECT rc;
		GetClientRect(CatchWnd, &rc);
		SetWindowPos(tdhw, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOREDRAW);
		if (!IsWindow(tdhw))
		{
			ShowWindow(CatchWnd, SW_HIDE);
			KillTimer(hWnd, 2);
			if (tdhwscr != NULL)tdhwscr = 0;
			tdhw = tdhw2 = 0;
		}
		return;
	}
	if (nTimerid == 3)//ANTI-HOOK
	{
		MouseHook = (HHOOK)SetWindowsHookEx(WH_MOUSE_LL, (HOOKPROC)MouseProc, hInst, 0);
		KeyboardHook = (HHOOK)SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardProc, hInst, 0);
		return;
	}
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	CreateDesktop(szVDesk, NULL, NULL, DF_ALLOWOTHERACCOUNTHOOK, GENERIC_ALL, NULL);//创建虚拟桌面

	CatchWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
	
	if (!CatchWnd)return FALSE;

	if (!RegisterHotKey(CatchWnd, 1, NULL, VK_SCROLL) ||
		!RegisterHotKey(CatchWnd, 2, MOD_CONTROL, 'B'))
	{
		if (MessageBox(CatchWnd, L"程序可能已经启动，多个程序一起运行可能导致错误，是否要继续启动?", L"TDTmini", MB_ICONINFORMATION | MB_OKCANCEL) != IDOK)return false;
	}
	else
		MessageBox(0, L"TopDomainTools 启动成功!\n按Ctrl + B 切换桌面\n按Scroll Lock结束极域(位于键盘最上一排中间偏右)\n被老师控制时会自动极域窗口化", L"TDTmini", MB_ICONINFORMATION);
	SetTimer(CatchWnd, 1, 1000, (TIMERPROC)TimerProc);
	SetTimer(CatchWnd, 3, 80, (TIMERPROC)TimerProc);
	//ShowWindow(CatchWnd, nCmdShow);
	//UpdateWindow(CatchWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_HOTKEY:
	{
		if (wParam == 2)CreateThread(0, 0, SDThread, 0, 0, 0); else KillProcess(L"stu");
		break;
	}
	case WM_DESTROY:
	{
		if (tdhw != 0 && tdhw2 != 0 && tdhw2 != (HWND)-1)
		{
			SetParent(tdhw, NULL);
			SetWindowPos(tdhw, NULL, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_NOZORDER);
			tdhw2 = tdhw = 0;
		}
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}