
#pragma once


#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/Security.hxx"


// buddyDlg dialog
class BuddyDlg : public CDialog
{
// Construction
public:
	BuddyDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SIPIMP_DIALOG };
	
	void receivedPage( const resip::Data& msg, 
		const resip::Uri& from,
		const resip::Data& signedBy,  
		const resip::Security::SignatureStatus sigStatus,
		const bool wasEncryped  );
	void presenseUpdate(const resip::Uri& uri, bool open, const resip::Data& status );

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

/* ====================================================================
 * The Vovida Software License, Version 1.0  *  * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved. *  * Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met: *  * 1. Redistributions of source code must retain the above copyright *    notice, this list of conditions and the following disclaimer. *  * 2. Redistributions in binary form must reproduce the above copyright *    notice, this list of conditions and the following disclaimer in *    the documentation and/or other materials provided with the *    distribution. *  * 3. The names "VOCAL", "Vovida Open Communication Application Library", *    and "Vovida Open Communication Application Library (VOCAL)" must *    not be used to endorse or promote products derived from this *    software without prior written permission. For written *    permission, please contact vocal@vovida.org. * * 4. Products derived from this software may not be called "VOCAL", nor *    may "VOCAL" appear in their name, without prior written *    permission of Vovida Networks, Inc. *  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH * DAMAGE. *  * ==================================================================== *  * This software consists of voluntary contributions made by Vovida * Networks, Inc. and many individuals on behalf of Vovida Networks, * Inc.  For more information on Vovida Networks, Inc., please see * <http://www.vovida.org/>. *
 */

