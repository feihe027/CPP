// TestException.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <windows.h>
#include "..\ExceptionReport\ExceptionReport.h"

#ifdef _DEBUG
#pragma comment(lib, "..\\debug\\ExceptionReport.lib")
#else
#pragma comment(lib, "..\\release\\ExceptionReport.lib")
#endif


int _tmain(int argc, _TCHAR* argv[])
{
    SetCustomUnhandledExceptionFilter();
    int* p = NULL;
    *p = 1;
	return 0;
}

