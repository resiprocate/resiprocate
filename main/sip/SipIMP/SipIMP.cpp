
#include "stdafx.h"


#include "sip2/util/Socket.hxx"
//#include "sip2/util/Logger.hxx"

#include "sip2/sipstack/SipStack.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/TuIM.hxx"
#include "sip2/sipstack/Security.hxx"


using namespace Vocal2;
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
		catch ( Vocal2::BaseException )
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
		bool okSec = sipStack->security->loadAllCerts( Vocal2::Data( mKey ), Data(mCertPath) );
		if ( !okSec )
		{
			//ErrLog( << "Could not load the certificates" );
			assert( 0 );
		}
	}
	catch ( Vocal2::Security::Exception e )
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
	catch (Vocal2::BaseException* e )
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

	tuIM->setUAName( Data("SIPimp.org (win32) ver 0.2.2") );

	if ( !mOutbound.IsEmpty() )
	{
		try
		{
			Vocal2::Data dOutbound( mOutbound );
			Uri outbound( dOutbound );

			tuIM->setOutboundProxy( outbound );
		}
		catch ( Vocal2::BaseException  )
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

		Vocal2::Data dName( name );
		Vocal2::Uri uri(dName);
		assert( tuIM );
		tuIM->addBuddy( uri , Vocal2::Data::Empty );
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
	catch ( Vocal2::BaseException )
	{
		return;
	}

	Vocal2::Data foo(text);
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
	Vocal2::Data dName( uName);
	try 
	{
		buddy = Uri( dName );
	}
	catch ( Vocal2::BaseException )
	{
		return CString(_T(""));
	}

	theApp.tuIM->addBuddy(buddy, Vocal2::Data::Empty /*group*/ );

	assert( dlg );
	Vocal2::Data sName = Data::from(buddy);
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
	Vocal2::Data status( note );
	theApp.tuIM->setMyPresense( online, status );
}
