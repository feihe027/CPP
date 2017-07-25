// ExceptionReport.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "ExceptionReport.h"
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

#include"resource.h"



HWND g_childWnd = NULL;


LRESULT CALLBACK WndProc     (HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK DlgProc (HWND, UINT, WPARAM, LPARAM);
int WINAPI CreateDlg(HINSTANCE hInstance);

LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* _pExcp) 
{   
    HANDLE hFile = CreateFile(  
        "d:\\Exception.dmp",   
        GENERIC_WRITE,   
        0,   
        NULL,   
        CREATE_ALWAYS,   
        FILE_ATTRIBUTE_NORMAL,   
        NULL);  
    if (INVALID_HANDLE_VALUE == hFile)  
    {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    MINIDUMP_EXCEPTION_INFORMATION einfo = {0};  
    einfo.ThreadId = ::GetCurrentThreadId();  
    einfo.ExceptionPointers = _pExcp;  
    einfo.ClientPointers = FALSE;  

    MiniDumpWriteDump(GetCurrentProcess(),   
        GetCurrentProcessId(),   
        hFile,   
        MiniDumpNormal,   
        &einfo,   
        NULL,   
        NULL);  
    CloseHandle(hFile);   

    HINSTANCE hModule = GetModuleHandle("ExceptionReport.dll");
    CreateDlg(hModule);

    return   EXCEPTION_EXECUTE_HANDLER;   
}

LPTOP_LEVEL_EXCEPTION_FILTER SetCustomUnhandledExceptionFilter()
{
    return SetUnhandledExceptionFilter(ExceptionFilter);
}

BOOL CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_INITDIALOG :
        return true;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        PostQuitMessage (0) ;
        return TRUE;
    case WM_COMMAND:
        {
            switch(LOWORD(wParam))  
            {  
            case IDC_SEND:
                MessageBox(hDlg, "报告发送成功!", "提示", MB_OK);
                SendMessage(GetParent(hDlg), WM_CLOSE, NULL, NULL);
                return TRUE;
            case IDC_CANCEL:  
                SendMessage(GetParent(hDlg), WM_CLOSE, NULL, NULL);
                return TRUE;
            default:  
                break;  
            }  
        }
    }

    return FALSE;
}
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{   
    switch (message)
    { 
    case WM_CREATE:  
        {  
            if (NULL == g_childWnd)
            {
                //基于对话框模板方式创建一个非模态对话框窗口  
                HINSTANCE hModule = GetModuleHandle("ExceptionReport.dll");
                g_childWnd = CreateDialog(hModule,MAKEINTRESOURCE(IDD_EXCP_DLG),hwnd,DlgProc);  
                /*初始化窗口标题栏，并显示窗口*/  
                ShowWindow(g_childWnd,1);  
            }
        }
        break;
    case WM_CLOSE:
        SendMessage(g_childWnd, message, NULL, NULL);
        DestroyWindow(hwnd);
        PostQuitMessage (0) ;
        return 0 ;
    case WM_DESTROY:
        PostQuitMessage (0) ;
        return 0 ;
    }
    return DefWindowProc (hwnd, message, wParam, lParam) ;
}

int WINAPI CreateDlg(HINSTANCE hInstance)  
{  
    WNDCLASSEX wndclass ;                                       //窗口类结构  
    ZeroMemory(&wndclass,sizeof(wndclass));                     //初始化结构  
    wndclass.cbSize = sizeof (WNDCLASSEX);                      //cbSize大小，window检测窗口类型的参数。  
    wndclass.style= CS_HREDRAW | CS_VREDRAW;                    //样式  
    wndclass.lpfnWndProc= WndProc;                              //指定窗口消息处理函数  
    wndclass.cbClsExtra= 0 ;                                    //窗口类预留字节空间，这里为0字节  
    wndclass.cbWndExtra = DLGWINDOWEXTRA ;                      //窗口结构预留字节  
    wndclass.hInstance= hInstance ;                             //窗口类模块句柄   
    wndclass.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON)) ;
    wndclass.hbrBackground= (HBRUSH) (COLOR_BTNFACE + 1) ;      //设置背景色  
    wndclass.lpszMenuName= NULL;                                //窗口应用程序菜单 NULL  
    wndclass.lpszClassName= "ExceptionWnd" ;                         //窗口类别名称，对话框窗口和模板类名称相同  
    //注册窗口  
    if(!RegisterClassEx(&wndclass))  
    {   
        MessageBox(NULL,TEXT("窗口注册失败!"),TEXT("Notepad"),MB_ICONERROR);  
        return 0;  
    }  
    /*--end 注册窗口*/  

    HWND hWnd = CreateWindow ("ExceptionWnd",  ("错误报告"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 224, NULL, NULL, hInstance, NULL) ;
    ShowWindow(hWnd, SW_SHOW);  

    //队列消息变量  
    MSG msg;                           
    //*主消息循环*//  
    while (GetMessage(&msg, NULL, 0, 0))  
    {  
        if (hWnd==NULL || !IsDialogMessage (hWnd, &msg))  
        {  
            TranslateMessage(&msg);  
            DispatchMessage(&msg);  
        }  
    }  

    g_childWnd = NULL;
    UnregisterClass("ExceptionWnd", hInstance);
    FreeLibrary(hInstance);     //释放模块句柄  

    return int(msg.wParam);  
}  