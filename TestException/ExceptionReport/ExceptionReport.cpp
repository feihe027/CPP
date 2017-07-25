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
                MessageBox(hDlg, "���淢�ͳɹ�!", "��ʾ", MB_OK);
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
                //���ڶԻ���ģ�巽ʽ����һ����ģ̬�Ի��򴰿�  
                HINSTANCE hModule = GetModuleHandle("ExceptionReport.dll");
                g_childWnd = CreateDialog(hModule,MAKEINTRESOURCE(IDD_EXCP_DLG),hwnd,DlgProc);  
                /*��ʼ�����ڱ�����������ʾ����*/  
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
    WNDCLASSEX wndclass ;                                       //������ṹ  
    ZeroMemory(&wndclass,sizeof(wndclass));                     //��ʼ���ṹ  
    wndclass.cbSize = sizeof (WNDCLASSEX);                      //cbSize��С��window��ⴰ�����͵Ĳ�����  
    wndclass.style= CS_HREDRAW | CS_VREDRAW;                    //��ʽ  
    wndclass.lpfnWndProc= WndProc;                              //ָ��������Ϣ������  
    wndclass.cbClsExtra= 0 ;                                    //������Ԥ���ֽڿռ䣬����Ϊ0�ֽ�  
    wndclass.cbWndExtra = DLGWINDOWEXTRA ;                      //���ڽṹԤ���ֽ�  
    wndclass.hInstance= hInstance ;                             //������ģ����   
    wndclass.hIcon = LoadIcon (hInstance, MAKEINTRESOURCE(IDI_ICON)) ;
    wndclass.hbrBackground= (HBRUSH) (COLOR_BTNFACE + 1) ;      //���ñ���ɫ  
    wndclass.lpszMenuName= NULL;                                //����Ӧ�ó���˵� NULL  
    wndclass.lpszClassName= "ExceptionWnd" ;                         //����������ƣ��Ի��򴰿ں�ģ����������ͬ  
    //ע�ᴰ��  
    if(!RegisterClassEx(&wndclass))  
    {   
        MessageBox(NULL,TEXT("����ע��ʧ��!"),TEXT("Notepad"),MB_ICONERROR);  
        return 0;  
    }  
    /*--end ע�ᴰ��*/  

    HWND hWnd = CreateWindow ("ExceptionWnd",  ("���󱨸�"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 224, NULL, NULL, hInstance, NULL) ;
    ShowWindow(hWnd, SW_SHOW);  

    //������Ϣ����  
    MSG msg;                           
    //*����Ϣѭ��*//  
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
    FreeLibrary(hInstance);     //�ͷ�ģ����  

    return int(msg.wParam);  
}  