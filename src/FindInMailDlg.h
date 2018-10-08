#pragma once


// CFindInMailDlg dialog

class CFindInMailDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFindInMailDlg)

public:
	CFindInMailDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CFindInMailDlg();

// Dialog Data
	//{{AFX_DATA(CFindInMailDlg)
	enum { IDD = IDD_FIND_IN_MAIL };

	CString	m_string;
	BOOL	m_bWholeWord;
	BOOL	m_bCaseSensitive;
	//}}AFX_DATA

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
