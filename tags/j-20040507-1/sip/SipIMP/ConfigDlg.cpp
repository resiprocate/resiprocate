
#include "stdafx.h"
#include "SipIMP.h"
#include "ConfigDlg.h"
#include "Reboot.h"


// CConfigDlg dialog

IMPLEMENT_DYNAMIC(CConfigDlg, CDialog)
CConfigDlg::CConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigDlg::IDD, pParent)
	, mPassword(_T(""))
	, mUser(_T(""))
	, mPort(0)
	, mProtocol(_T(""))
	, mContact(_T(""))
	, mKey(_T(""))
	, mOutbound(_T(""))
	, udpPort(0)
	, tlsPort(0)
	, mCertPath(_T(""))
{
	CWinApp* app = AfxGetApp();
	assert( app );
	mHost = app->GetProfileString("Proxy","host","example.com");
	mProtocol = app->GetProfileString("Proxy","protocol","UDP");
	mPort = app->GetProfileInt("Proxy","port",5060);
	mUser = app->GetProfileString("Proxy","user","");
	mPassword = app->GetProfileString("Proxy","password",""); // !cj! could do better than this 
	mContact = app->GetProfileString("UA","contact","");  
	mKey = app->GetProfileString("PKI","key",""); // !cj! could do *way* better than this 
	mCertPath = app->GetProfileString("PKI","CertPath","C:\\certs"); // !cj! could do *way* better than this 
	mOutbound = app->GetProfileString("Proxy","outbound","");  
	udpPort = app->GetProfileInt("UA","udpPort",5060);
	tlsPort = app->GetProfileInt("UA","tlsPort",5061);
}

CConfigDlg::~CConfigDlg()
{
}

void CConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//	DDX_Control(pDX, IDC_HOST, mHost);
	DDX_Text(pDX, IDC_HOST, mHost);
	DDX_Text(pDX, IDC_PASSWORD, mPassword);
	DDX_Text(pDX, IDC_USER, mUser);
	DDX_Text(pDX, IDC_PORT, mPort);
	DDV_MinMaxInt(pDX, mPort, 0, 65536);
	DDX_CBString(pDX, IDC_PROTOCOL, mProtocol);
	DDV_MaxChars(pDX, mProtocol, 5);
	DDX_Text(pDX, IDC_EDIT_CONTACT, mContact);
	DDV_MaxChars(pDX, mContact, 256);
	DDX_Text(pDX, IDC_EDIT_KEY, mKey);
	DDX_Text(pDX, IDC_EDIT_OUTBOUND, mOutbound);
	DDV_MaxChars(pDX, mOutbound, 256);
	DDX_Text(pDX, IDC_UDP_TCP_PORT, udpPort);
	DDV_MinMaxInt(pDX, udpPort, 0, 65536);
	DDX_Text(pDX, IDC_TLS_PORT, tlsPort);
	DDV_MinMaxInt(pDX, tlsPort, 0, 65536);
	DDX_Text(pDX, IDC_EDIT_CERT_PATH, mCertPath);
	DDV_MaxChars(pDX, mCertPath, 1024);
}


BEGIN_MESSAGE_MAP(CConfigDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CConfigDlg message handlers

void CConfigDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();

	CWinApp* app = AfxGetApp();
	assert( app );
	app->WriteProfileString("Proxy","host",mHost);
	app->WriteProfileString("Proxy","protocol",mProtocol);
	app->WriteProfileInt("Proxy","port",mPort);
	app->WriteProfileString("Proxy","user",mUser);
	app->WriteProfileString("Proxy","password",mPassword); // !cj! could do better than this 
	app->WriteProfileString("UA","contact",mContact); // !cj! could do better than this 
	app->WriteProfileString("PKI","key",mKey); // !cj! could do better than this 
	app->WriteProfileString("PKI","CertPath",mCertPath);  
	app->WriteProfileString("Proxy","outbound",mOutbound); // !cj! could do better than this 
	app->WriteProfileInt("UA","udpPort",udpPort); // !cj! could do better than this 
	app->WriteProfileInt("UA","tlsPort",tlsPort); // !cj! could do better than this 

		CReboot dlg;
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
}

/* ====================================================================
 * The Vovida Software License, Version 1.0  *  * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved. *  * Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met: *  * 1. Redistributions of source code must retain the above copyright *    notice, this list of conditions and the following disclaimer. *  * 2. Redistributions in binary form must reproduce the above copyright *    notice, this list of conditions and the following disclaimer in *    the documentation and/or other materials provided with the *    distribution. *  * 3. The names "VOCAL", "Vovida Open Communication Application Library", *    and "Vovida Open Communication Application Library (VOCAL)" must *    not be used to endorse or promote products derived from this *    software without prior written permission. For written *    permission, please contact vocal@vovida.org. * * 4. Products derived from this software may not be called "VOCAL", nor *    may "VOCAL" appear in their name, without prior written *    permission of Vovida Networks, Inc. *  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH * DAMAGE. *  * ==================================================================== *  * This software consists of voluntary contributions made by Vovida * Networks, Inc. and many individuals on behalf of Vovida Networks, * Inc.  For more information on Vovida Networks, Inc., please see * <http://www.vovida.org/>. *
 */

