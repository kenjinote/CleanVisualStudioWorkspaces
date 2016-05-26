#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"shlwapi")
#pragma comment(lib,"comctl32")
#include<windows.h>
#include<shlwapi.h>
#include<commctrl.h>

HWND hList;
TCHAR szClassName[] = TEXT("Window");

BOOL IsDeleteFile(LPCTSTR lpszFilePath, LPCTSTR lpszExtList)
{
	DWORD dwSize = lstrlen(lpszExtList);
	LPTSTR lpszExtList2 = (LPTSTR)GlobalAlloc(0, (dwSize + 1) * sizeof(TCHAR));
	lstrcpy(lpszExtList2, lpszExtList);
	LPCTSTR seps = TEXT(";");
	TCHAR *next;
	LPTSTR token = wcstok_s(lpszExtList2, seps, &next);
	while (token != NULL)
	{
		if (PathMatchSpec(lpszFilePath, token))
		{
			GlobalFree(lpszExtList2);
			return TRUE;
		}
		token = wcstok_s(0, seps, &next);
	}
	GlobalFree(lpszExtList2);
	return FALSE;
}

VOID DeleteOrCount(LPCTSTR lpInputPath, LPCTSTR lpszExtList)
{
	TCHAR szFullPattern[MAX_PATH];
	WIN32_FIND_DATA FindFileData;
	HANDLE hFindFile;
	PathCombine(szFullPattern, lpInputPath, TEXT("*"));
	hFindFile = FindFirstFile(szFullPattern, &FindFileData);
	if (hFindFile != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FindFileData.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if (lstrcmp(FindFileData.cFileName, TEXT("..")) != 0 &&
					lstrcmp(FindFileData.cFileName, TEXT(".")) != 0)
				{
					PathCombine(szFullPattern, lpInputPath, FindFileData.cFileName);
					DeleteOrCount(szFullPattern, lpszExtList);
				}
			}
			else
			{
				PathCombine(szFullPattern, lpInputPath, FindFileData.cFileName);
				if (IsDeleteFile(szFullPattern, lpszExtList))
				{
					SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szFullPattern);
				}
			}
		} while (FindNextFile(hFindFile, &FindFileData));
		FindClose(hFindFile);
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit, hProgress;
	switch (msg)
	{
	case WM_CREATE:
		InitCommonControls();
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"),
			TEXT("*.aps;*.bsc;*.dep;*.idb;*.ilk;*.ipch;*.lastbuildstate;")
			TEXT("*.log;*.ncb;*.map;*.obj;*.opensdf;*.opt;*.pch;*.pdb;*.plg;*.res;")
			TEXT("*.sbr;*.scc;*.sdf;*.suo;*.tlb;*.tlog;*.user;*.vspscc;")
			TEXT("*.vssscc;BuildLog.htm;*.VC.db;*.VC.VC.opendb;*.iobj;*.ipdb"),
			WS_VISIBLE | WS_CHILD | ES_AUTOHSCROLL,
			10, 10, 1024, 32, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit, EM_LIMITTEXT, 0, 0);
		hProgress = CreateWindow(TEXT("msctls_progress32"), 0, WS_VISIBLE | WS_CHILD,
			0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hList = CreateWindow(TEXT("LISTBOX"), 0, WS_CHILD | WS_VISIBLE | WS_VSCROLL |
			WS_HSCROLL | LBS_NOINTEGRALHEIGHT, 0, 0, 0, 0, hWnd, 0,
			((LPCREATESTRUCT)lParam)->hInstance, 0);
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_SIZE:
		MoveWindow(hEdit, 10, 10, LOWORD(lParam) - 20, 32, 1);
		MoveWindow(hProgress, 10, 50, LOWORD(lParam) - 20, 32, 1);
		MoveWindow(hList, 10, 90, LOWORD(lParam) - 20, HIWORD(lParam) - 100, 1);
		break;
	case WM_DROPFILES:
		{
			TCHAR szTmp[MAX_PATH];

			DWORD dwSize = GetWindowTextLength(hEdit);
			LPTSTR lpszExtList = (LPTSTR)GlobalAlloc(0, sizeof(TCHAR)*(dwSize + 1));
			GetWindowText(hEdit, lpszExtList, dwSize + 1);

			const UINT iFileNum = DragQueryFile((HDROP)wParam, -1, NULL, 0);
			for (UINT i = 0; i<iFileNum; ++i)
			{
				DragQueryFile((HDROP)wParam, i, szTmp, MAX_PATH);
				if (PathIsDirectory(szTmp))
				{
					DeleteOrCount(szTmp, lpszExtList);
				}
				else if (IsDeleteFile(szTmp, lpszExtList))
				{
					SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szTmp);
				}
			}
			DragFinish((HDROP)wParam);
			GlobalFree(lpszExtList);

			SendMessage(hProgress, PBM_SETRANGE32, 0, SendMessage(hList, LB_GETCOUNT, 0, 0));
			SendMessage(hProgress, PBM_SETSTEP, 1, 0);
			SendMessage(hProgress, PBM_SETPOS, 0, 0);

			while (SendMessage(hList, LB_GETCOUNT, 0, 0))
			{
				SendMessage(hList, LB_GETTEXT, 0, (LPARAM)szTmp);
				DeleteFile(szTmp);
				SendMessage(hList, LB_DELETESTRING, 0, 0);
				SendMessage(hProgress, PBM_STEPIT, 0, 0);
			}

			MessageBox(hWnd, TEXT("削除が完了しました。"), TEXT("確認"), 0);

			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		0,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		(HBRUSH)(COLOR_WINDOW + 1),
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("指定された拡張子のファイルを一括削除（サブフォルダも検索）。")
		TEXT("ワークスペースフォルダをクライアント領域にドラッグで開始。"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}
