
#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#include "sip2/util/Socket.hxx"
//#include "sip2/util/Logger.hxx"

#include "sip2/sipstack/SipStack.hxx"
//#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/TuIM.hxx"
#include "buddydlg.h"
//#include "sip2/sipstack/Security.hxx"

// SipImpApp:
// See limp.cpp for the implementation of this class
//

class SipImpApp : public CWinApp
{
public:
	SipImpApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation
	DECLARE_MESSAGE_MAP()

public:
	Vocal2::TuIM* tuIM;
	Vocal2::SipStack* sipStack;

private:
	void imInit();

public:
	void process(void);
	void sendPage(CString text,CString destination);
	void setStatus(bool online, CString note);
	CString addBuddy(CString name);

	BuddyDlg* dlg;
	bool encryp;
	bool sign;
};

extern SipImpApp theApp;
