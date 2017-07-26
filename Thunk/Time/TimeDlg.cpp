#include "stdafx.h"
// TimeDlg.cpp : implementation file//
#include "Time.h"
#include "TimeDlg.h"
#include "afxdialogex.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTimeDlg dialog
class ZThunk  
{ 
private: 
	unsigned char* m_ThiscallCode/*[10]*/;	
	unsigned char m_StdcallCode[16];
	ZThunk* m_pThunk; 
	BOOL m_bDEP;
public: 
	ZThunk()		
	{ 		
		//������ݶ�ִ�д��뱣���Ƿ���� Data Execution Prevention (DEP)		
		m_bDEP = IsProcessorFeaturePresent(PF_NX_ENABLED); 			//�ڵ�ǰ���̵�4GB�����ַ�ռ�������һ���������ڸ���ָ���ڴ�ο�ִ�е����ԣ��Ӷ������ݶ�ִ�л�����	
		if (m_bDEP)	
		{ 			
			m_ThiscallCode = (unsigned char*)VirtualAlloc(NULL, sizeof(ZThunk), MEM_COMMIT, PAGE_EXECUTE_READWRITE);		
			//m_ThiscallCode = new unsigned char[10];
		}			
		else	
		{ 		
			m_pThunk = this; 	
		}
	} 	
	~ZThunk()	
	{  		
		if (m_bDEP && m_pThunk)	
		{     		
			BOOL bSuccess = VirtualFree(m_pThunk,     // Base address of block      
				sizeof(ZThunk),     // Bytes of committed pages        		
				MEM_RELEASE);  // Decommit the pages      			
			if (bSuccess) 				
				m_pThunk = NULL; 
		}	
	} 	
	enum CALLINGCONVENTIONS {  STDCALL = 1,  THISCALL= 2 }; 
public:  	
	template <class T> 
	void* Callback(void* pThis,T MemberOffset,CALLINGCONVENTIONS CallingConvention = STDCALL)	
	{ 		
		// these codes only use in stdcall 	
		if(CallingConvention == THISCALL)	
		{  // Encoded machine instruction   Equivalent assembly languate notation 		
			// ---------------------------   -------------------------------------  // B9                    mov    ecx, pThis  ; Load ecx with this pointer  // E9                    jmp    target addr  Jump to target message handler   			
			char Buf[33]={0}; 		
			sprintf(Buf,"%d",MemberOffset); 	
			unsigned long JmpAddr = (unsigned long) atol(Buf) - (unsigned long) &(m_ThiscallCode[0]) - 10; 	
			m_ThiscallCode[0] = 0xB9;//mov  ecx, pThis  ָ������� 	
			//m_ThiscallCode[0] = 0x8B;//mov  ecx, pThis  ָ������� 			
			m_ThiscallCode[5] = 0xE9;//jmp   xxxxxx    ָ������� 		
			*((unsigned long *) &(m_ThiscallCode[1])) = (unsigned long) pThis;//thisָ��		
			*((unsigned long *) &(m_ThiscallCode[6])) = JmpAddr;//ƫ�Ƶ�ַ  		
			::FlushInstructionCache( ::GetCurrentProcess(), pThis, 10);			
			return (void*)(m_ThiscallCode); 	
		}   // these codes only use in thiscall 		
		else if(CallingConvention == STDCALL)	
		{  // Encoded machine instruction   Equivalent assembly languate notation  		
			// ---------------------------   -------------------------------------  			
			// FF 34 24 push  dword ptr [esp] Save (or duplicate)                                                                    ; the Return Addr into stack    
			// C7 44 24 04           mov   dword ptr [esp+4], pThis  Overwite the old                                                                    ; Return Addr with 'this pointer'  
			// E9                    jmp   target addr              ; Jump to target message handler    		
			char Buf[33]={0};   			
			sprintf(Buf,"%d",MemberOffset);   	
			unsigned long JmpAddr = (unsigned long) atol(Buf) - (unsigned long) &m_StdcallCode[0] - 16;   		
			m_StdcallCode[11] = 0xE9;    		
			*((unsigned long *)  &m_StdcallCode[ 0]) = 0x002434FF;  	
			*((unsigned long *)  &m_StdcallCode[ 3]) = 0x042444C7;  		
			*((unsigned long *)  &m_StdcallCode[ 7]) = (unsigned long) pThis; 		
			*((unsigned long *)  &m_StdcallCode[12]) = JmpAddr;    			
			return (void*)m_StdcallCode;  	
		}   	
		return 0; 
	}
};

class CTimer 
{ 
private: 	// �˶��������ʱΪ������ݳ�Ա����ȫ�ֶ���,�Ա�֤������������	
	ZThunk m_thunk; 
	int m_value;public: 	//��װ��ʱ��	
	void Set() 
	{ 		//����ص�������ַ		
		void* pAddr=m_thunk.Callback(this,&CTimer::TimerProc,ZThunk::THISCALL); 	
		//��װ��ʱ��	
		SetTimer(NULL,1,1000,(TIMERPROC)pAddr); 	
	} 	//��ʱ���ص�����,��ȫ����װ�����Ա����! 
	void TimerProc(HWND hWnd, DWORD dwMsg , WPARAM wPa, LPARAM lPa) 
	{ 		// to do something 		
		TRACE("%d", lPa-m_value);		
		m_value = lPa;		
		int n = 0;	
	}
};

typedef struct AA
{
	int a[2];
};

CTimeDlg::CTimeDlg(CWnd* pParent /*=NULL*/)	: CDialogEx(CTimeDlg::IDD, pParent)
{	
	AA m = {0};
	int* p = m.a + 1;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}
void CTimeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CTimeDlg, CDialogEx)
ON_WM_PAINT()	
ON_WM_QUERYDRAGICON()END_MESSAGE_MAP()// CTimeDlg message handlers

BOOL CTimeDlg::OnInitDialog()
{	
	CDialogEx::OnInitDialog();	// Set the icon for this dialog.  The framework does this automatically	//  when the application's main window is not a dialog	
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon	

	{	
		ZThunk m_thunk;		
		CTimer timer;		
		BYTE b = 4;
		timer.TimerProc(GetSafeHwnd(), 10, 100, 1000);
		__asm	
		{	
			push        3E8h  		
			push        64h  		
			push        0Ah
			mov         ecx,dword ptr[ebp-18h]
			call        CWnd::GetSafeHwnd		
			push        eax			
			lea         ecx,[timer]
			call        CTimer::TimerProc
		}		
		void* pAddr = m_thunk.Callback(&timer, &CTimer::TimerProc, ZThunk::THISCALL); 		
		TIMERPROC fun = TIMERPROC(pAddr);	
		fun(GetSafeHwnd(), 10, 100, 1000);		
		//void TimerProc(HWND hWnd, DWORD dwMsg , WPARAM wPa, LPARAM lPa){...}	
		// AfxBeginThread(&CTimer::TimerProc, &timer);		// ��װ��ʱ��		
		//SetTimer(1111, 1000, (TIMERPROC)pAddr);	
	}	

	// TODO: Add extra initialization here	
	return TRUE; 
	// return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below//  to draw the icon.  For MFC applications using the document/view model,//  this is automatically done for you by the framework.
void CTimeDlg::OnPaint()
{	
	if (IsIconic())
	{	
		CPaintDC dc(this); // device context for painting		
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);		
		// Center icon in client rectangle		
		int cxIcon = GetSystemMetrics(SM_CXICON);	
		int cyIcon = GetSystemMetrics(SM_CYICON);	
		CRect rect;		
		GetClientRect(&rect);		
		int x = (rect.Width() - cxIcon + 1) / 2;		
		int y = (rect.Height() - cyIcon + 1) / 2;	
		// Draw the icon	
		dc.DrawIcon(x, y, m_hIcon);
	}	
	else
	{	
		CDialogEx::OnPaint();	
	}
}

// The system calls this function to obtain the cursor to display while the user drags//  the minimized window.
HCURSOR CTimeDlg::OnQueryDragIcon()
{	
	return static_cast<HCURSOR>(m_hIcon);
}