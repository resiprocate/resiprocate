
#pragma once


#include "sip2/util/Socket.hxx"
#include "sip2/util/Data.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/Security.hxx"


// buddyDlg dialog
class BuddyDlg : public CDialog
{
// Construction
public:
	BuddyDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SIPIMP_DIALOG };
	
	void receivedPage( const Vocal2::Data& msg, 
		const Vocal2::Uri& from,
		const Vocal2::Data& signedBy,  
		const Vocal2::Security::SignatureStatus sigStatus,
		const bool wasEncryped  );
	void presenseUpdate(const Vocal2::Uri& uri, bool open, const Vocal2::Data& status );

	void message(const CString& msg);

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;
	static BuddyDlg* buddy;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnLbnSelchangeList();
	afx_msg void OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeEdit();
	static void CALLBACK EXPORT timerProcCallback(
			  HWND hWnd,      // handle of CWnd that called SetTimer
			  UINT nMsg,      // WM_TIMER
			  UINT nIDEvent,   // timer identification
	          DWORD dwTime    // system time
		);
    void timerProc();
	afx_msg void OnBnClickedButtonConfig();
	afx_msg void OnBnClickedButtonAdd();
    void addBuddy(CString name);
	afx_msg void OnBnClickedButtonClear();
	afx_msg void OnBnClickedCheckOnline();
	afx_msg void OnEnKillfocusEditNote();
	afx_msg void OnBnClickedCheckSign();
	afx_msg void OnBnClickedCheckEncrypt();
	afx_msg void OnBnClickedButtonDel();
	afx_msg void OnBnClickedButtonSend();

	void display(const CString& text);
};
