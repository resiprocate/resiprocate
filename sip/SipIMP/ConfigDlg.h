#pragma once
#include "afxwin.h"

// CConfigDlg dialog

class CConfigDlg : public CDialog
{
	DECLARE_DYNAMIC(CConfigDlg)

public:
	CConfigDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CConfigDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CString mHost;
	afx_msg void OnBnClickedOk();
	CString mPassword;
	CString mUser;
	int mPort;
	CString mProtocol;
	CString mContact;
	CString mKey;
	CString mOutbound;
	int udpPort;
	int tlsPort;
	CString mCertPath;
};
