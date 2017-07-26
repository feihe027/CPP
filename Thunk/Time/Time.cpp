
// Time.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Time.h"
#include "TimeDlg.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTimeApp

BEGIN_MESSAGE_MAP(CTimeApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

std::string ExtractFileDir(const std::string strFileName)
{
	int pos = (int)strFileName.find_last_of('\\');
	if(pos == -1)
		return "";
	return strFileName.substr(0, pos);
}

Bool DirectoryExists(const std::string strDir)
{
	// 如果是空字符串，则认为已经是最深层目录
	if (strDir == "")
	{
		return True;
	}

	// 判断是否为根目录
	if(strDir.length() == 2 && strDir.at(1) == ':')
	{
		return True;
	}

	Bool bRet = False;
	if(strDir.empty())
		return bRet;

	int Code = GetFileAttributes(strDir.c_str());

	if ((Code != -1) && ((FILE_ATTRIBUTE_DIRECTORY & Code) != 0))
	{
		bRet = True;
	}

	return bRet;
}

Bool ForceDirectories(const std::string strDir)
{
	std::string path = ExtractFileDir(strDir);
	if(!DirectoryExists(path))
		ForceDirectories(path);
	if(False == DirectoryExists(strDir))
	{
		if(::CreateDirectory(strDir.c_str(),NULL) == 0)
		{
			return False;
		}
	}
	return True;
}

// CTimeApp construction

CTimeApp::CTimeApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


template< typename TDst, typename TSrc >
TDst  UnionCastType( TSrc src )
{
	union
	{
		TDst  uDst;
		TSrc  uSrc;
	}uMedia;
	uMedia.uSrc  =  src;
	return uMedia.uDst;
}
#pragma pack( push, 1 )
struct  MemFunToStdCallThunk
{
	 BYTE          m_mov;
	 DWORD      m_this;   //mov ecx pThis
	BYTE          m_jmp;
	DWORD      m_relproc;  //jmp  偏移地址



	void* GetCodeAddress()
	{
		return this;
	}
};
#pragma  pack( pop )

class  CTestClass
{
public:
	typedef  void  (__thiscall *StdCallFun )( int, int );
	int  m_nBase;
	MemFunToStdCallThunk*  m_thunk;

public:
	void Init(DWORD_PTR proc, void* pThis)
	{
		m_thunk = (MemFunToStdCallThunk*)VirtualAlloc(NULL, sizeof(MemFunToStdCallThunk), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		m_thunk->m_mov = 0xB9;   // mov ecx
		m_thunk->m_this = PtrToUlong(pThis);

		m_thunk->m_jmp = 0xE9;   // jmp
		//jmp跳转的地址为相对地址,相对地址 = 目标地址 - 当前指令下一条指令的地址
		m_thunk->m_relproc = DWORD((INT_PTR)proc - ((INT_PTR)m_thunk+sizeof(MemFunToStdCallThunk)));
		//::FlushInstructionCache( ::GetCurrentProcess(), m_thunk, sizeof(MemFunToStdCallThunk) );
	}
	void  memFun( int m, int n )
	{
		int  nSun = m_nBase + m + n;
		MessageBox( NULL, _T("abc"), "dd", MB_OK);
	}

public:
	CTestClass()
	{
		m_nBase  = 10;
	}

	void  Test()
	{
		//UnionCastType:利用联合将函数指针转换成DWORD_PTR
		//m_thunk.Init( UnionCastType<DWORD_PTR>(&CTestClass::memFun), this);
		//Init(UnionCastType<DWORD_PTR>(&CTestClass::memFun), this);
		StdCallFun fun = (StdCallFun)m_thunk->GetCodeAddress();
		fun(1, 3);
	}
};

// The one and only CTimeApp object

CTimeApp theApp;


// CTimeApp initialization

BOOL CTimeApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();
	std::string str = "f:\\aba1c\\dd\\ee\\edde";
	ForceDirectories(str);

	CTestClass test;
	test.Init(UnionCastType<DWORD_PTR>(&CTestClass::memFun), this);
	CTestClass::StdCallFun fun = (CTestClass::StdCallFun)(test.m_thunk->GetCodeAddress());
	//fun(1, 3);
	AfxEnableControlContainer();

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CTimeDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Delete the shell manager created above.
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

