//
//////////////////////////////////////////////////////////////////
//
//  Windows Mbox Viewer is a free tool to view, search and print mbox mail archives.
//
// Source code and executable can be downloaded from
//  https://sourceforge.net/projects/mbox-viewer/  and
//  https://github.com/eneam/mboxviewer
//
//  Copyright(C) 2019  Enea Mansutti, Zbigniew Minciel
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the version 3 of GNU Affero General Public License
//  as published by the Free Software Foundation; 
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this program; if not, write to the
//  Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110 - 1301, USA.
//
//////////////////////////////////////////////////////////////////
//

// mboxview.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ctype.h"
#include "locale.h"
#include "FileUtils.h"
#include "TextUtilsEx.h"
#include "mboxview.h"
#include "profile.h"
#include "LinkCursor.h"
#include "MainFrm.h"
#include "MboxMail.h"

#include <afxadv.h> //Has CRecentFileList class definition.
#include "afxlinkctrl.h"
#include "afxwin.h"

#include "ExceptionUtil.h"

//#include "afxglobals.h"

#ifdef _DEBUG
#undef THIS_FILE
#define THIS_FILE __FILE__
#define new DEBUG_NEW
//#define _CRTDBG_MAP_ALLOC  
//#include <stdlib.h>  
//#include <crtdbg.h> 
#endif

// UpdateData(FALSE) from Dialog members to UI controls 
// UpdateData(TRUE) from UI controls to Dialog members -  be carefull if you call TRUE in dialog followed by CANCEL

LONG WINAPI MyUnhandledExceptionFilter(PEXCEPTION_POINTERS pExceptionPtrs)
{
	// Do something, for example generate error report
	//..

	UINT seNumb = 0;
	if (pExceptionPtrs->ExceptionRecord)
		seNumb = pExceptionPtrs->ExceptionRecord->ExceptionCode;

	char const*  szCause = seDescription(seNumb);

	char *stackDumpFileName = "UnhandledException_StackDump.txt";
	BOOL ret = DumpStack(stackDumpFileName, (TCHAR*)szCause, seNumb, pExceptionPtrs->ContextRecord);

	CString progDir;
	BOOL retDir = GetProgramDir(progDir);

	char *exceptionName = "UnhandledException";

	CString errorTxt;
#ifdef USE_STACK_WALKER
	errorTxt.Format(_T("%s: Code=%8.8x Description=%s\n\n"
		"To help to diagnose the problem, created file\n\n%s\n\nin\n\n%s directory.\n\n"
		"Please provide the files to the development team.\n\n"),
		exceptionName, seNumb, szCause, stackDumpFileName, progDir);
#else
	errorTxt.Format(_T("%s: Code=%8.8x Description=%s\n\n"), exceptionName, seNumb, szCause);
#endif

	AfxMessageBox((LPCTSTR)errorTxt, MB_OK | MB_ICONHAND);

	// Execute default exception handler next
	//return EXCEPTION_EXECUTE_HANDLER;
	//return EXCEPTION_CONTINUE_EXECUTION;
	return EXCEPTION_CONTINUE_SEARCH;
}

const char *sz_Software_mboxview = "SOFTWARE\\mboxview";

/////////////////////////////////////////////////////////////////////////////
// CmboxviewApp

BEGIN_MESSAGE_MAP(CmboxviewApp, CWinApp)
	//{{AFX_MSG_MAP(CmboxviewApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE(ID_FILE_MRU_FIRST, ID_FILE_MRU_LAST, MyMRUFileHandler) 
//	ON_UPDATE_COMMAND_UI(ID_FILE_MRU_FILE1, OnUpdateRecentFileMenu)
END_MESSAGE_MAP()


void Com_Initialize()
{
	//DWORD dwCoInit = COINIT_MULTITHREADED;
	DWORD dwCoInit = COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE;

	// call to CoInitializeEx() seem to be required but 
	// Fails anyway with "HRESULT - 0x80010106 - Cannot change thread mode after it is set. "
	HRESULT result = CoInitializeEx(0, dwCoInit);
	int deb = 1;
}


void CmboxviewApp::MyMRUFileHandler(UINT i)
{
	ASSERT_VALID(this);
	ASSERT(m_pRecentFileList != NULL);

	ASSERT(i >= ID_FILE_MRU_FILE1);
	ASSERT(i < ID_FILE_MRU_FILE1 + (UINT)m_pRecentFileList->GetSize());

	CString strName, strCurDir, strMessage;
	int nIndex = i - ID_FILE_MRU_FILE1;
	ASSERT((*m_pRecentFileList)[nIndex].GetLength() != 0);

	strName.Format("MRU: open file (%d) '%s'.\n", (nIndex) + 1,(LPCTSTR)(*m_pRecentFileList)[nIndex]);

	if( ! FileUtils::PathDirExists((*m_pRecentFileList)[nIndex]) ) {
		m_pRecentFileList->Remove(nIndex);
		return;
	}
	((CMainFrame*)AfxGetMainWnd())->DoOpen((*m_pRecentFileList)[nIndex]);
}

/////////////////////////////////////////////////////////////////////////////
// CmboxviewApp construction

CmboxviewApp::CmboxviewApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance

	//bool ret = TextUtilities::TestAll();

	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//SetNoOaCache();

	//int test_enc();
	//test_enc();


	//FileUtils FU;  FU.UnitTest();
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CmboxviewApp object

CmboxviewApp theApp;

void SetBrowserEmulation()
{
	CString ieVer = CProfile::_GetProfileString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Internet Explorer", "Version");
	int p = ieVer.Find('.');
	int maj = atoi(ieVer);
	int min = atoi(ieVer.Mid(p + 1));
	DWORD dwEmulation = CProfile::_GetProfileInt(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", "mboxview.exe");
	if (maj == 9 && min >= 10) {
		DWORD dwem = min * 1000 + 1;
		if (dwEmulation != dwem) {
			CProfile::_WriteProfileInt(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", "mboxview.exe", dwem);
			CProfile::_WriteProfileInt(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", "mboxview.exe", dwem);
		}
	}
	else
		if (maj == 9) {
			if (dwEmulation != 9999) {
				CProfile::_WriteProfileInt(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", "mboxview.exe", 9999);
				CProfile::_WriteProfileInt(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", "mboxview.exe", 9999);
			}
		}
		else
			if (maj == 8) {
				if (dwEmulation != 8888) {
					CProfile::_WriteProfileInt(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", "mboxview.exe", 8888);
					CProfile::_WriteProfileInt(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION", "mboxview.exe", 8888);
				}
			}
}

#pragma comment(linker, "/defaultlib:version.lib")

BOOL GetFileVersionInfo(HMODULE hModule, DWORD &ms, DWORD &ls)
{
	// get module handle
	TCHAR filename[_MAX_PATH];

	// get module file name
	DWORD len = GetModuleFileName(hModule, filename,
		sizeof(filename) / sizeof(filename[0]));
	if (len <= 0)
		return FALSE;

	// read file version info
	DWORD dwDummyHandle; // will always be set to zero
	len = GetFileVersionInfoSize(filename, &dwDummyHandle);
	if (len <= 0)
		return FALSE;

	BYTE* pVersionInfo = new BYTE[len]; // allocate version info
	if (pVersionInfo == NULL)
		return FALSE;
	if (!::GetFileVersionInfo(filename, 0, len, pVersionInfo)) {
		delete[] pVersionInfo;
		return FALSE;
	}

	LPVOID lpvi;
	UINT iLen;
	if (!VerQueryValue(pVersionInfo, _T("\\"), &lpvi, &iLen)) {
		delete[] pVersionInfo;
		return FALSE;
	}

	ms = ((VS_FIXEDFILEINFO*)lpvi)->dwFileVersionMS;
	ls = ((VS_FIXEDFILEINFO*)lpvi)->dwFileVersionLS;
	BOOL res = (((VS_FIXEDFILEINFO*)lpvi)->dwSignature == VS_FFI_SIGNATURE);

	delete[] pVersionInfo;

	return res;
}

/////////////////////////////////////////////////////////////////////////////
// CmboxviewApp message handlers

int CmboxviewApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();
	CFont m_linkFont;
// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnStnClickedDonation();
	CLinkCursor m_link;
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnClose();
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_DONATION, m_link);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
	ON_STN_CLICKED(IDC_DONATION, &CAboutDlg::OnStnClickedDonation)
	ON_WM_CTLCOLOR()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

// App command to run the dialog
void CmboxviewApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CmboxviewApp message handlers

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	DWORD ms, ls;
    if (GetFileVersionInfo((HMODULE)AfxGetInstanceHandle(), ms, ls)) {
		CString txt;
        // display file version from VS_FIXEDFILEINFO struct
        txt.Format("Version %d.%d.%d.%d", 
                 HIWORD(ms), LOWORD(ms),
                 HIWORD(ls), LOWORD(ls));
		GetDlgItem(IDC_STATIC1)->SetWindowText(txt);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

class CCmdLine : public CCommandLineInfo
{
public:
	BOOL m_bError;
	BOOL m_bLastPathSet;
	CCmdLine::CCmdLine() {
		m_bError = FALSE; m_bLastPathSet = FALSE;
	}
	void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);
};

void CCmdLine::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL) // bLast )
{
	if (m_bError)
		return;
	if (bFlag) 
	{
		if (strncmp(lpszParam, _T("FOLDER="), 7) == 0) 
		{
			CString openFolder = lpszParam + 7;
			CMainFrame::m_commandLineParms.m_mboxFolderPath = openFolder;
		}
		else if (strncmp(lpszParam, _T("MAIL_FILE="), 10) == 0) 
		{
			CString mailFile = lpszParam + 10;

			CMainFrame::m_commandLineParms.m_mboxFileNameOrPath = mailFile;
		}
		else if (strncmp(lpszParam, _T("EXPORT_EML="), 11) == 0) 
		{
			CString exportEML = lpszParam + 11;
			exportEML.MakeLower();
			if (!((exportEML.Compare("y") == 0) || (exportEML.Compare("n") == 0))) 
			{
				CString txt = _T("Invalid Command Line Option Value \"");
				CString opt = lpszParam;
				txt += opt + _T("\". Valid are \"y|n\". Note that once defined valid EXPORT_EML persists in the registry.\nDo you want to continue?");
				HWND h = NULL; // we don't have any window yet  
				int answer = ::MessageBox(h, txt, _T("Error"), MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO);
				if (answer == IDNO)
					m_bError = TRUE;
			}
			else {
				CProfile::_WriteProfileString(HKEY_CURRENT_USER, sz_Software_mboxview, _T("exportEML"), exportEML);

				CMainFrame::m_commandLineParms.m_exportEml = FALSE;
				if (exportEML.Compare("y") == 0)
					CMainFrame::m_commandLineParms.m_exportEml = TRUE;
			}
		}
		else if (strncmp(lpszParam, _T("PROGRESS_BAR_DELAY="), 19) == 0) 
		{
			CString barDelay = lpszParam + 19;
			// Validate
			if (!TextUtilsEx::isNumeric(barDelay))
			{
				CString txt = _T("Invalid Command Line Option Value \"");
				CString opt = lpszParam;
				txt += opt + _T("\".\nDo you want to continue?");
				HWND h = NULL; // we don't have any window yet  
				int answer = ::MessageBox(h, txt, _T("Error"), MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO);
				if (answer == IDNO)
					m_bError = TRUE;
			}
			else 
			{
				DWORD progressBarDelay = _tstoi(barDelay);
				CProfile::_WriteProfileInt(HKEY_CURRENT_USER, sz_Software_mboxview, _T("progressBarDelay"), progressBarDelay);
				CMainFrame::m_commandLineParms.m_progressBarDelay = progressBarDelay;
			}
		}
		else if (strncmp(lpszParam, _T("EML_PREVIEW_MODE"), 19) == 0)
		{
			CMainFrame::m_commandLineParms.m_bEmlPreviewMode = TRUE;
		}
		else if (strncmp(lpszParam, _T("MBOX_MERGE_LIST_FILE"), 20) == 0)
		{
			CString emlListFile = lpszParam + 21;
			CMainFrame::m_commandLineParms.m_mboxListFilePath = emlListFile;
		}
		else if (strncmp(lpszParam, _T("MBOX_MERGE_TO_FILE"), 18) == 0)
		{
			CString mergeToFilePath = lpszParam + 19;
			CMainFrame::m_commandLineParms.m_mergeToFilePath = mergeToFilePath;
		}
		else if (strncmp(lpszParam, _T("TRACE_CASE"), 10) == 0)
		{
			CString traceCase = lpszParam + 11;
			DWORD nTraceCase = _tstoi(traceCase);
			CMainFrame::m_commandLineParms.m_traceCase = nTraceCase;
		}
		else 
		{
			// Unknown argument
			CString txt = _T("Invalid Command Line Option \"");
			CString opt = lpszParam;
			txt += opt 
				+ _T("\"")
				+ _T("\n\nValid options:\n")
				+ _T("  -FOLDER=Folder Path\n")
				+ _T("  -MAIL_FILE=Mbox File Path or Name to open\n")
				+ _T("  -EXPORT_EML=y or n\n")
				+ _T("  -PROGRESS_BAR_DELAY=Seconds\n")
				+ _T("  -EML_PREVIEW_MODE\n")
				+ _T("  -MBOX_MERGE_LIST_FILE=Path to File containing list of mbox files to merge\n")
				+ _T("  -MBOX_MERGE_TO_FILE=Path to File to save merge results\n")
				+ _T("\nDo you want to continue?")
				;

			CMainFrame::m_commandLineParms.Clear();
			HWND h = NULL; // we don't have any window yet
			int answer = ::MessageBox(h, txt, _T("Error"), MB_APPLMODAL | MB_ICONERROR | MB_YESNO);
			if (answer == IDNO)
				m_bError = TRUE;
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// CmboxviewApp initialization

#if 0
static int tot = 0;
void* __cdecl malloc(size_t size)
{
	int deb = 1;
	char mem[1];
	void *p = new char[size];
	tot += size;
	return p;
}
#endif

BOOL CmboxviewApp::InitInstance()
{
	HANDLE procHandle = GetCurrentProcess();
	BOOL priClass = SetPriorityClass(procHandle, ABOVE_NORMAL_PRIORITY_CLASS);
	DWORD err = GetLastError();

#ifdef USE_STACK_WALKER
	SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
#endif

	//AfxEnableMemoryTracking(TRUE);
	//afxMemDF = allocMemDF | delayFreeMemDF | checkAlwaysMemDF;

	CCmdLine cmdInfo;
	CString mailFile;
	CProfile::_WriteProfileString(HKEY_CURRENT_USER, sz_Software_mboxview, "mailFile", mailFile);

	CMainFrame::m_commandLineParms.m_allCommanLineOptions = CWinApp::m_lpCmdLine;
	ParseCommandLine(cmdInfo);
	if (cmdInfo.m_bError)
	{
		MboxMail::ReleaseResources();
		return FALSE;
	}
	else
	{
		// Do additional Check of command line options
		int retCode = CMainFrame::m_commandLineParms.VerifyParameters();
		if (retCode == -1)  // error and don't continue
		{
			MboxMail::ReleaseResources();
			return FALSE;
		}
		else if (retCode == -2)  // error but continue; probably overkill
		{
			CMainFrame::m_commandLineParms.Clear();
		}
	}

	CString processPath = CProfile::_GetProfileString(HKEY_CURRENT_USER, sz_Software_mboxview, "processPath");
#if 0
	// Commented out for now since it seem to confuse users. Need to find better solution.
	if (!processPath.IsEmpty())
	{
		CString txt = _T("Mbox viewer instance might be running already:\n\n") + processPath;
		txt += _T("\n\n");
		txt += _T("Only single instance should be running to avoid potential\n");
		txt += _T("issues since all instances will share the same data in the registry.\n\n");
		txt += _T("In most cases this warning can be ignored.\n"
			"It will be generated if program crashes or it is killed from Task Manager.\n\n");
		txt += _T("Do you want to continue?");
		HWND h = NULL; // we don't have any window yet  
		int answer = MessageBox(h, txt, _T("Error"), MB_APPLMODAL | MB_ICONQUESTION | MB_YESNO);
		if (answer == IDNO)
		{
			MboxMail::ReleaseResources();
			return FALSE;
		}
	}
#endif

	char *pValue;
	errno_t  er = _get_pgmptr(&pValue);
	CString procFullPath;
	if ((er == 0) && pValue)
		procFullPath.Append(pValue);

	CProfile::_WriteProfileString(HKEY_CURRENT_USER, sz_Software_mboxview, _T("processPath"), procFullPath);


	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	SetBrowserEmulation();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	//Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("mboxview"));

	m_pRecentFileList = new
		CRecentFileList(0, "MRUs",
		"Path %d", 16);
	m_pRecentFileList->ReadList();
	// Initialize all Managers for usage. They are automatically constructed
	// if not yet present
	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object.

	g_tu.Init();

	DWORD msgViewPosition = 1;
	BOOL retval;
	retval = CProfile::_GetProfileInt(HKEY_CURRENT_USER, sz_Software_mboxview, _T("messageWindowPosition"), msgViewPosition);
	if (retval == TRUE) {
		if ((msgViewPosition < 1) && (msgViewPosition > 3))
			msgViewPosition = 1;  // bottom=1
	}

	CMainFrame* pFrame = new CMainFrame(msgViewPosition);
	if (pFrame->GetSafeHwnd() == 0)
		int deb = 1;
	m_pMainWnd = pFrame;

	// create and load the frame with its resources

	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// The one and only window has been initialized, so show and update it.
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	
	DWORD ms, ls;
	if (GetFileVersionInfo((HMODULE)AfxGetInstanceHandle(), ms, ls)) {
		CString txt;
		// display file version from VS_FIXEDFILEINFO struct
		txt.Format("%d.%d.%d.%d",
			HIWORD(ms), LOWORD(ms),
			HIWORD(ls), LOWORD(ls));
		CString savedVer = CProfile::_GetProfileString(HKEY_CURRENT_USER, sz_Software_mboxview, "version");
		if (savedVer.Compare(txt) != 0) {
			CProfile::_WriteProfileString(HKEY_CURRENT_USER, sz_Software_mboxview, "version", txt);
			pFrame->PostMessage(WM_COMMAND, ID_APP_ABOUT, 0);
		}
	}

	MboxMail::LoadHintBitmap();

#if 0
	// TEST if file name can be longet than 260 characters

	CFile testFile;
	CString fileName = "\\\\?";

	fileName.AppendChar('\\');

	fileName.Empty();
	fileName.Append("C:\\");
	for (int i = 0; i < 512; i++)
		fileName.Append("x");

	fileName.Append("Y.txt");

	UINT nFlags = CFile::modeWrite | CFile::modeCreate;
	BOOL ret = testFile.Open(fileName, nFlags);
	if (ret)
		testFile.Close();

	DWORD dwAccess = GENERIC_WRITE;
	DWORD dwCreationDisposition = CREATE_ALWAYS;

	CStringW fileNameW;
	DWORD error;

	BOOL r = TextUtilsEx::Ansi2Wide(fileName, fileNameW, error);

	HANDLE hFile = CreateFileW(fileNameW, dwAccess, FILE_SHARE_READ, NULL, dwCreationDisposition, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		BOOL retClose = CloseHandle(hFile);
	}
#endif

	return TRUE;
}


void CAboutDlg::OnStnClickedDonation()
{
	HINSTANCE result = ShellExecute(NULL, _T("open"), "https://sourceforge.net/p/mbox-viewer/donate/", NULL, NULL, SW_SHOWNORMAL);
}


HBRUSH CAboutDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if (nCtlColor == CTLCOLOR_STATIC &&
		pWnd->GetSafeHwnd() == m_link.GetSafeHwnd()
		) {
		pDC->SetTextColor(RGB(0, 0, 200));
		if (m_linkFont.m_hObject == NULL) {
			LOGFONT lf;
			GetFont()->GetObject(sizeof(lf), &lf);
			lf.lfUnderline = TRUE;
			m_linkFont.CreateFontIndirect(&lf);
		}
		pDC->SelectObject(&m_linkFont);
	}
	return hbr;
}


void CAboutDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnClose();
}

