#pragma once

// CAddDlg dialog

class CAddDlg : public CDialog
{
	DECLARE_DYNAMIC(CAddDlg)

public:
	CAddDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAddDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_ADD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString newBuddyName;
	afx_msg void OnBnClickedOk();
};
