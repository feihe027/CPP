// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once


#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <stdio.h>
#include <tchar.h>
#include <Windows.h>
#include <memory>
#include <string>


// DeviceIO
#include <Ntddscsi.h>
#include <winioctl.h>
#include <cfgmgr32.h>
#include <winnls.h>
#include <Setupapi.h>
#pragma comment(lib, "setupapi.lib")


// TODO: reference additional headers your program requires here
#ifndef SAFE_DELETE 
#define SAFE_DELETE(p) { if(p) { delete (p); (p)=NULL; } } 
#endif 
#ifndef SAFE_DELETE_ARRAY 
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p)=NULL; } } 
#endif 
#ifndef SAFE_RELEASE 
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } } 
#endif 