
#include "stdafx.h"


#include "resiprocate/os/Socket.hxx"
//#include "sip2/util/Logger.hxx"

#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"


using namespace resip;
using namespace std;


#include "SipIMP.h"
#include "BuddyDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// SipImpApp

BEGIN_MESSAGE_MAP(SipImpApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// SipImpApp construction

SipImpApp::SipImpApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only SipImpApp object
SipImpApp theApp;


// SipImpApp initialization
BOOL SipImpApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	CWinApp::InitInstance();

#if 1 // !cj! TODO - should fix 
	int tmpFlag = _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG );
	tmpFlag &= ~_CRTDBG_CHECK_ALWAYS_DF;
	tmpFlag &= ~_CRTDBG_ALLOC_MEM_DF;
	tmpFlag &= ~_CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag( tmpFlag );
#endif

#if 0
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
#endif

	AfxEnableControlContainer();

	SetRegistryKey(_T(""));

	imInit();

	dlg = new BuddyDlg;
	m_pMainWnd = dlg;
	INT_PTR nResponse = dlg->DoModal();
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

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


class ImCallback: public TuIM::Callback
{
public:
	virtual void receivedPage( const Data& msg, const Uri& from ,
		const Data& signedBy,  Security::SignatureStatus sigStatus,
		bool wasEncryped  );
	virtual void sendPageFailed( const Uri& dest,int num  );
	virtual void registrationFailed( const Uri& dest, int num );
   virtual void registrationWorked( const Uri& dest );
	virtual void presenseUpdate(const Uri& dest, bool open, const Data& status );
	virtual void receivePageFailed(const Uri& sender);

};


void 
ImCallback::receivedPage( const Data& msg, const Uri& from,
						 const Data& signedBy,  Security::SignatureStatus sigStatus,
						 bool wasEncryped  )
{  
	assert( theApp.m_pMainWnd );
	BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
	assert(buddy);
	buddy->receivedPage(  msg, from,signedBy,  sigStatus, wasEncryped  );
}


void 
ImCallback::presenseUpdate(const Uri& dest, bool open, const Data& status )
{
	assert( theApp.m_pMainWnd );
	BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
	assert(buddy);
	buddy->presenseUpdate( dest, open, status );
}


void 
ImCallback::sendPageFailed( const Uri& dest, int num )
{
	//InfoLog(<< "In TestErrCallback");  
	//cerr << "Message to " << dest << " failed" << endl;
	CString cNum;
	cNum.Format("%d",num);
	CString msg = _T("Message to ");
	msg += CString(dest.getAor().c_str()); 
	msg += _T(" failed (");
	msg += cNum;
	msg += _T(")");

	assert( theApp.m_pMainWnd );
	BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
	assert(buddy);
	buddy->message( msg );
}

void 
ImCallback::registrationFailed( const Uri& dest,int num )
{
	//InfoLog(<< "In TestErrCallback");  
	//cerr << "Message to " << dest << " failed" << endl;
	CString cNum;
	cNum.Format("%d",num);
	CString msg = _T("Registration to ");
	msg += CString(dest.getAor().c_str()); 
	msg += _T(" failed (");
	msg += cNum;
	msg += _T(")");

	assert( theApp.m_pMainWnd );
	BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
	assert(buddy);
	buddy->message( msg );
}

void 
ImCallback::registrationWorked( const Uri& dest )
{
	CString msg = _T("Registration to ");
	msg += CString(dest.getAor().c_str()); 
	msg += _T(" worked");

	assert( theApp.m_pMainWnd );
	BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
	assert(buddy);
	buddy->message( msg );
}

void 
ImCallback::receivePageFailed(const Uri& sender)
{
	CString msg = _T("Can not understand message from ");
	msg += CString(sender.getAor().c_str()); 

	assert( theApp.m_pMainWnd );
	BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
	assert(buddy);
	buddy->message( msg );
}

void 
SipImpApp::imInit()
{  
	sipStack = new SipStack;  
	assert(sipStack);

	encryp = false;
	sign = false;

	CString	mHost     = this->GetProfileString("Proxy","host","example.com");
	CString mProtocol = this->GetProfileString("Proxy","protocol","UDP");
	int     mPort     = this->GetProfileInt("Proxy","port",5060);
	CString mUser     = this->GetProfileString("Proxy","user","");
	CString mPassword = this->GetProfileString("Proxy","password",""); 
	CString mKey      = this->GetProfileString("PKI","key",""); 
	CString mCertPath = this->GetProfileString("PKI","CertPath","C:\\certs"); 
	CString mOutbound = this->GetProfileString("Proxy","outbound",""); 
	CString mContact  = this->GetProfileString("UA","contact",""); 

	int port    = this->GetProfileInt("UA","udpPort",5060); 
	int tlsPort = this->GetProfileInt("UA","tlsPort",5061); 

	Uri aor;
	Uri contact;

	char* envPort = getenv("SIPPORT");
	if ( envPort )
	{
		port = atoi( envPort );
	}

	char* envTlsPort = getenv("SIPTLSPORT");
	if ( envTlsPort )
	{
		tlsPort = atoi( envTlsPort );
	}

	if ( port == 5060 )
	{
		if ( tlsPort == 0 )
		{
			tlsPort = 5061;
		}
	}

	bool haveContact=false;

	if ( !mContact.IsEmpty() )
	{
		try 
		{
			contact = Uri( Data( mContact ) );
			haveContact = true;
		}
		catch ( resip::BaseException& )
		{
		}
	}

	if ( !haveContact )
	{
		contact.scheme() = Data("sip");
		contact.user() = Data("user");
		contact.port() = port;
		contact.host() = sipStack->getHostAddress();
	}
	
	if ( !mHost.IsEmpty() )
	{
		aor.host() = mHost;
		aor.user() = mUser;
		aor.port() = mPort;
		if ( mProtocol == "UDP" )
		{
			aor.scheme() = "sip";
		}
		if ( mProtocol == "TCP" )
		{
			aor.scheme() = "sip";
			aor.param(p_transport) = "tcp";
		}
		if ( mProtocol == "TLS" )
		{
			aor.scheme() = "sips";
		}

		if (!haveContact)
		{
			contact.user() = aor.user();
			if ( aor.scheme() == "sips" )
			{
				contact.scheme() = aor.scheme();
				contact.port() = tlsPort;
			}
		}
	}
	else
	{
		aor = contact;
	}

#ifdef USE_SSL
	assert( sipStack->security );
	try
	{
		resip::Data key( mKey );
		resip::Data path(mCertPath);

		bool okSec = sipStack->security->loadAllCerts( key , path  );
		if ( !okSec )
		{
			//ErrLog( << "Could not load the certificates" );
			assert( 0 );
		}
	}
	catch ( resip::Security::Exception e )
	{
		Data error = Data::from( e );
		const char* problem = error.c_str();
	}
#endif

	try
	{
		// add the transports
		if (port!=0)
		{
			sipStack->addTransport(Transport::UDP, port);
			sipStack->addTransport(Transport::TCP, port);
		}
		if ( tlsPort != 0 )
		{
#ifdef USE_SSL
			sipStack->addTransport(Transport::TLS, tlsPort);
#endif
		}
	}
	catch (resip::BaseException* e )
	{
		Data foo = Data::from( *e );
		tuIM = NULL;
		return;
	}

	// build the TU 
	ImCallback* callback = new ImCallback;
	assert( callback );
	tuIM = new TuIM(sipStack,aor,contact,callback);
	assert(tuIM);

	tuIM->setUAName( Data("SIPimp.org (win32) ver 0.3.0") );

	if ( !mOutbound.IsEmpty() )
	{
		try
		{
			resip::Data dOutbound( mOutbound );
			Uri outbound( dOutbound );

			tuIM->setOutboundProxy( outbound );
		}
		catch ( resip::BaseException&  )
		{	
		}
	}

	// registter 
	if ( !mHost.IsEmpty() )
	{
		assert(tuIM);
		tuIM->registerAor( aor , Data(mPassword) );
	}

	int n = this->GetProfileInt("buddyList","size",0);
	for ( int i=0; i<n; i++ )
	{
		CString item;
		item.Format("%d",i);
		CString name = this->GetProfileString("buddyList",item,"");

		resip::Data dName( name );
		resip::Uri uri(dName);
		assert( tuIM );
		tuIM->addBuddy( uri , resip::Data::Empty );
	}
}

void SipImpApp::process(void)
{
	assert(sipStack);
	
	if ( !tuIM )
	{
		return;
	}
	
	assert(tuIM);

	FdSet fdset; 
	sipStack->buildFdSet(fdset);

	int  err = fdset.selectMilliSeconds( 0 );
	if ( err == -1 )
	{
		int e = errno;
		assert(0);
		//InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
	}
	//InfoLog(<< "Select returned");

	//DebugLog ( << "Try TO PROCESS " );
	sipStack->process(fdset);

	tuIM->process();
}

void 
SipImpApp::sendPage(CString text, CString destiation)
{
	if ( !tuIM )
	{
		return;
	}

	Data d(destiation);
	Uri dest;
	try 
	{
		dest = Uri(d);
	}
	catch ( resip::BaseException& )
	{
		return;
	}

	resip::Data foo(text);
	Data encFor = dest.getAorNoPort();
	if (!encryp)
	{
		encFor = Data::Empty;
	}

#ifndef USE_SSL
	if ( sign || encryp )
	{
		assert( theApp.m_pMainWnd );
		BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
		assert(buddy);
		buddy->message( CString(_T("no support for sign or encryption in this version")) );
		return;
	}
#endif

	if ( !tuIM->haveCerts(sign,encFor) )
	{
		assert( theApp.m_pMainWnd );
		BuddyDlg* buddy = dynamic_cast<BuddyDlg*>( theApp.m_pMainWnd );
		assert(buddy);
		buddy->message( CString(_T("Don't have the aproperate certificates to send this message")) );
		return;
	}

	tuIM->sendPage( foo , dest, sign,  encFor /*encryptFor*/ );
}


CString
SipImpApp::addBuddy(CString name)
{
	if ( !theApp.tuIM )
	{
		return name;
	}
	assert(theApp.tuIM);
	name.Trim();

	CString uName;

	if ( (name.Find("sip:")==-1) && (name.Find("sips:")==-1) )
	{
		uName += "sip:";
	}
	uName += name;

	Uri buddy;
	resip::Data dName( uName);
	try 
	{
		buddy = Uri( dName );
	}
	catch ( resip::BaseException& )
	{
		return CString(_T(""));
	}

	theApp.tuIM->addBuddy(buddy, resip::Data::Empty /*group*/ );

	assert( dlg );
	resip::Data sName = Data::from(buddy);
	dlg->addBuddy( sName.c_str() );

	return CString( sName.c_str() );
}

void 
SipImpApp::setStatus(bool online, CString note)
{
	if ( !theApp.tuIM )
	{
		return;
	}
	assert(theApp.tuIM);
	resip::Data status( note );
	theApp.tuIM->setMyPresense( online, status );
}

/* ====================================================================
 * The Vovida Software License, Version 1.0  *  * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved. *  * Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met: *  * 1. Redistributions of source code must retain the above copyright *    notice, this list of conditions and the following disclaimer. *  * 2. Redistributions in binary form must reproduce the above copyright *    notice, this list of conditions and the following disclaimer in *    the documentation and/or other materials provided with the *    distribution. *  * 3. The names "VOCAL", "Vovida Open Communication Application Library", *    and "Vovida Open Communication Application Library (VOCAL)" must *    not be used to endorse or promote products derived from this *    software without prior written permission. For written *    permission, please contact vocal@vovida.org. * * 4. Products derived from this software may not be called "VOCAL", nor *    may "VOCAL" appear in their name, without prior written *    permission of Vovida Networks, Inc. *  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH * DAMAGE. *  * ==================================================================== *  * This software consists of voluntary contributions made by Vovida * Networks, Inc. and many individuals on behalf of Vovida Networks, * Inc.  For more information on Vovida Networks, Inc., please see * <http://www.vovida.org/>. *
 */

