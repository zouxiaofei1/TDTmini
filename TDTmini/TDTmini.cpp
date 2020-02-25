#include <windows.h>
#include <TlHelp32.h>
#define MAX_LOADSTRING 100

#define IDI_ICON1                       101
// 全局变量:
HINSTANCE hInst;                                // 当前实例
const wchar_t szTitle[] = L"被捕捉的极域电子教室",szWindowClass[] = L"TDK";
wchar_t szVDesk[] = L"TDTdesk", ScreenWindow[] = L"tdhwscr";
BOOL                InitInstance();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
bool NewDesktop = false;
HWND tdhw, tdhw2, tdhwscr, CatchWnd;
//void s(LPCSTR a)//调试用MessageBox
//{
//	MessageBox(NULL, a, L"", NULL);
//}
//void s(int a)
//{
//	wchar_t tmp[34];
//	_itow_s(a, tmp, 10);
//	MessageBox(NULL,tmp, L"", NULL);
//}
HHOOK KeyboardHook, MouseHook;


wchar_t*  _wcsstr(
	const wchar_t* wcs1,
	const wchar_t* wcs2
)
{
	wchar_t* cp = (wchar_t*)wcs1;
	wchar_t* s1, * s2;
	if (!*wcs2)
		return (wchar_t*)wcs1;
	while (*cp)
	{
		s1 = cp;
		s2 = (wchar_t*)wcs2;
		while (*s1 && *s2 && !(*s1 - *s2))
			s1++, s2++;
		if (!*s2)
			return(cp);
		cp++;
	}
	return(NULL);
}
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(MouseHook, nCode, wParam, lParam);
}//空的全局钩子函数
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) { 
	return CallNextHookEx(KeyboardHook, nCode, wParam, lParam); 
}
DWORD WINAPI SDThread(LPVOID pM)
{
	(pM);
	HDESK vdback;
	if (!NewDesktop)//从原始桌面切换到新桌面
		vdback = OpenDesktopW(szVDesk, DF_ALLOWOTHERACCOUNTHOOK, false, GENERIC_ALL);
	else//切换回来
		vdback = OpenDesktopW(L"Default", DF_ALLOWOTHERACCOUNTHOOK, false, GENERIC_ALL);
	NewDesktop = !NewDesktop;
	SetThreadDesktop(vdback);
	SwitchDesktop(vdback);
	return 0;
}

BOOL CALLBACK EnumChildwnd(HWND hwnd, LPARAM lParam)//查找极域广播子窗口的枚举函数.
{//这一段代码和TopDomainTools的完全相同，不做过多介绍
	if (!IsWindowVisible(hwnd))return 1;
	wchar_t title[301];
	GetWindowText(hwnd, title, 300);
	if (_wcsstr(title, L"TDDesk Render Window") != 0)
	{
		SetParent(hwnd, CatchWnd);
		tdhw = hwnd;
		return 0;//return 0 代表中断查找
	}
	return 1;
}

BOOL CALLBACK EnumBroadcastwnd(HWND hwnd, LPARAM lParam)//查找"屏幕广播"窗口的枚举函数.
{
	if (!IsWindowVisible(hwnd))return 1;
	wchar_t title[301];
	GetWindowText(hwnd, title, 300);
	if (_wcsstr(title, L"屏幕广播") != 0)
	{
		EnumChildWindows(hwnd, EnumChildwnd, NULL);//由于TDTmini是定时器查找，
		if (tdhw == 0)return FALSE;//有时候极域广播的大窗口加载出来了，但是里面的广播内容窗口还没加载好，
		ShowWindow(CatchWnd, SW_SHOW);//就会出现捕捉不到窗口的情况
		tdhwscr = CreateWindowW(szWindowClass, L"1", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInst, nullptr);
		SetParent(hwnd, tdhwscr);//因此当tdhw = 0(找不到内容窗口)时一定要退出
		tdhw2 = hwnd;
		return FALSE;
	}
	return 1;
}
BOOL CALLBACK EnumBroadcastwndOld(HWND hwnd, LPARAM lParam)
{
	if (!IsWindowVisible(hwnd))return 1;
	wchar_t title[301];
	GetWindowText(hwnd, title, 300);
	if (_wcsstr(title, L"屏幕演播室窗口") != 0 || _wcsstr(title, L"屏幕广播窗口") != 0)
	{
		ShowWindow(CatchWnd, SW_SHOW);
		tdhw = hwnd; tdhw2 = (HWND)(-4);
		SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
		SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_WINDOWEDGE | WS_EX_TOPMOST | WS_EX_LEFT | WS_EX_CLIENTEDGE);
		SetParent(hwnd, CatchWnd);
		return 0;
	}
	return 1;
}

int main(){

	// 执行应用程序初始化:
	if (!InitInstance())return FALSE;

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0))
	{// 主消息循环:
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
void KillProcess()//根据进程名结束进程
{
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(hSnapShot, &pe))return;
	while (Process32Next(hSnapShot, &pe))
	{
		pe.szExeFile[3] = 0;
		//_wcslwr_s(pe.szExeFile);
		if (_wcsstr(L"Stu", pe.szExeFile) != 0|| _wcsstr(L"stu", pe.szExeFile) != 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
			TerminateProcess(hProcess, 1);
			CloseHandle(hProcess);
		}
	}
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
			if (tdhw2 == (HWND)-4)
			{
				RECT rc;
				GetClientRect(CatchWnd, &rc);
				SetWindowPos(tdhw, 0, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOREDRAW);
			}
			else
				SetTimer(hWnd, 2, 33, (TIMERPROC)TimerProc);
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
BOOL InitInstance()
{
	hInst = GetModuleHandleW(0); // 将实例句柄存储在全局变量中

	CreateDesktop(szVDesk, NULL, NULL, DF_ALLOWOTHERACCOUNTHOOK, GENERIC_ALL, NULL);//创建虚拟桌面

	WNDCLASSEXW wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInst;
	wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(101));//这个函数不支持自定义窗体图标
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszClassName = szWindowClass;//自定义ClassName和WndProc
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(101));//小图标
	RegisterClassExW(&wcex);

	CatchWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInst, nullptr);
	
	if (!CatchWnd)return FALSE;

	if (!RegisterHotKey(CatchWnd, 1, NULL, VK_SCROLL) ||
		!RegisterHotKey(CatchWnd, 2, MOD_CONTROL, 'B'))
	{
		if (MessageBox(CatchWnd, L"程序可能已经启动，多个程序一起运行可能导致错误，是否要继续启动?", L"TDTmini", MB_ICONINFORMATION | MB_OKCANCEL) != IDOK)return false;
	}
	else
		MessageBox(0, L"TopDomainTools 启动成功!\n按Ctrl + B 切换桌面\n按Scroll Lock结束极域(位于键盘最上一排中间偏右)\n被老师控制时会自动极域窗口化", L"TDTmini", MB_ICONINFORMATION);
	SetTimer(CatchWnd, 1, 250, (TIMERPROC)TimerProc);
	SetTimer(CatchWnd, 3, 100, (TIMERPROC)TimerProc);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_HOTKEY:
	{
		if (wParam == 2)CreateThread(0, 0, SDThread, 0, 0, 0); else KillProcess();
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