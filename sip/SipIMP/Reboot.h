#pragma once


// CReboot dialog

class CReboot : public CDialog
{
	DECLARE_DYNAMIC(CReboot)

public:
	CReboot(CWnd* pParent = NULL);   // standard constructor
	virtual ~CReboot();

// Dialog Data
	enum { IDD = IDD_DIALOG_REBOOT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
