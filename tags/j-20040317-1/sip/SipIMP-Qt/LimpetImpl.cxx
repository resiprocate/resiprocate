
#include "LimpetImpl.hxx"
#include "SetupDlgImpl.hxx"

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Data.hxx"
#include <qmultilineedit.h>
#include <qtimer.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qlistbox.h>



using namespace resip;
using namespace std;

template<class T>
resip::Data cullenize(const T& x)
{
   resip::Data d;
   {
      resip::DataStream s(d);
      s << x;
   }
   return d;
}

LimpetImpl* theApp;

LimpetImpl::LimpetImpl( QWidget* parent,
		        const char* name,
		        bool modal,
			WFlags f )
   :
   Limpet( parent, name, modal, f ),
   sipStack( NULL )
{
   setUpDlgImpl = new SetupDlgImpl; // memory leak to fix
   theApp = this;
}

void
LimpetImpl::setup()
{

   setUpDlgImpl->show();
}

typedef int HTREEITEM;

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
LimpetImpl::logon()
{  
   if ( sipStack != NULL )
   {
      delete sipStack;
   }
   sipStack = new SipStack;  
   assert(sipStack);

   encryp = false;
   sign = false;

   int port = 5060;
   char* envPort = getenv("SIPPORT");
   if ( envPort )
   {
      port = atoi( envPort );
   }

   int tlsPort = 0;
   char* envTlsPort = getenv("SIPTLSPORT");
   if ( envTlsPort )
   {
      tlsPort = atoi( envTlsPort );
   }
//ASIM this won't work
   QString	mHost     = setUpDlgImpl->mHost;
   QString mProtocol = setUpDlgImpl->mProtocol;
   int     mPort     = setUpDlgImpl->mPort;
   QString mUser     = setUpDlgImpl->mUser;
   QString mPassword = setUpDlgImpl->mPassword;
   QString mContact  = setUpDlgImpl->mContact;
   QString mKey      = setUpDlgImpl->mKey;
   QString mOutbound = setUpDlgImpl->mOutbound;
#if 0
   QString	mHost( "example.com" );
   QString mProtocol("UDP" );
   int     mPort    = 5060 ;
   QString mUser( "me" );
   QString mPassword( "none" );
   QString mContact ( "" );
   QString mKey     ( "" );
   QString mOutbound( "sip:me@1.1.1.1:6001" );
#endif
	
   Uri aor;
   Uri contact;

#ifdef USE_SSL
   assert( sipStack->security );
   bool ok = sipStack->security->loadAllCerts( ".", Data(mKey) );
   if ( !ok )
   {
      //InfoLog( << "Could not load the certificates" );
      // !cj! - needs TODO assert(0);
   }
#endif

   if ( port == 5060 )
   {
      if ( tlsPort == 0 )
      {
         tlsPort = 5061;
      }
   }

   if (mContact.isEmpty())
   {
      contact.scheme() = Data("sip");
      contact.user() = Data("user");
      contact.port() = port;
      contact.host() = sipStack->getHostname();
   }
   else
   {
      try 
      {
         contact = Uri( Data( mContact ) );
         if (contact.port() != 0 )
         {
            port = contact.port();
         }
      }
      catch ( std::exception* )
      {
         mContact = "";
         contact.scheme() = Data("sip");
         contact.user() = Data("user");
         contact.port() = port;
         contact.host() = sipStack->getHostname();
      }
   }

   if ( !mHost.isEmpty() )
   {
      aor.host() = mHost.ascii();
      aor.user() = mUser.ascii();
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

      if (mContact.isEmpty())
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

   cout  << "aor is " << aor;

   //InfoLog( << "aor is " << aor );
   //InfoLog( << "contact is " << contact );

#ifdef USE_SSL
   assert( sipStack->security );
   ok = sipStack->security->loadAllCerts( ".", mKey.data() );
   if ( !ok )
   {
      //ErrLog( << "Could not load the certificates" );
      assert( 0 );
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
   catch (resip::Transport::Exception e )
   {
      Data foo = cullenize( e );

      tuIM = NULL;
      return;
   }

   // build the TU 
   ImCallback* callback = new ImCallback;
   assert( callback );

   tuIM = new TuIM(sipStack,aor,contact,callback);
   assert(tuIM);

   tuIM->setUAName( Data("sip instant maessagin program (Qt) ver 0.2") );

   if ( !mOutbound.isEmpty() )
   {
      try
      {
         resip::Data dOutbound( mOutbound.ascii() );
         Uri outbound( dOutbound );

         tuIM->setOutboundProxy( outbound );
      }
      catch ( exception )
      {	
      }
   }

   // registter 
   if ( !mHost.isEmpty() )
   {
      assert(tuIM);
      tuIM->registerAor( aor , Data(mPassword) );
   }

   // int n = this->GetProfileInt("buddyList","size",0);
   int n = 0;
   for ( int i=0; i<n; i++ )
   {
/*		QString item;
		item.sprintf("%d",i);
		QString name = this->GetProfileString("buddyList",item,"");

		resip::Data dName( name );
		resip::Uri uri(dName);
		assert( tuIM );
		tuIM->addBuddy( uri , resip::Data::Empty ); */
   }

   QTimer* timer = new QTimer( theApp );
   connect( timer, SIGNAL(timeout()), theApp, SLOT(process()) );
   timer->start( 300 );
}

void LimpetImpl::process(void)
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
      //int e = errno;
      assert(0);
      //InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
   }
   //InfoLog(<< "Select returned");

   //DebugLog ( << "Try TO PROCESS " );
   sipStack->process(fdset);

   tuIM->process();
}

void 
LimpetImpl::sendPage(QString text, QString destiation)
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
   catch ( std::exception )
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
      assert(theApp);
      theApp->message( QString("no support for sign or encryption in this version") );
      return;
   }
#endif

   if ( !tuIM->haveCerts(sign,encFor) )
   {
      assert(theApp);
      theApp->message( QString("Don't have the aproperate certificates to send this message") );
      return;
   }
   tuIM->sendPage( foo , dest, sign,  encFor /*encryptFor*/ );
}


void
LimpetImpl::addBuddy()
{
   QString name = Buddy->text();

   if ( !this->tuIM )
   {
      return;
   }
   assert(this->tuIM);
   name.stripWhiteSpace();

   QString uName;

   if ( (name.find("sip:")==-1) && (name.find("sips:")==-1) )
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
   catch ( std::exception* )
   {
      return;
   }

   this->tuIM->addBuddy(buddy, resip::Data::Empty /*group*/ );

   resip::Data sName = cullenize(buddy);
   BuddyListWidget->insertItem( sName.c_str() );

}

void 
LimpetImpl::setStatus(bool online, QString note)
{
   if ( !this->tuIM )
   {
      return;
   }
   assert(this->tuIM);
   resip::Data status( note );
   this->tuIM->setMyPresense( online, status );
}


/////////////////////////////////////////////////////////
// BUDDY STARTS HERE
// //////////////////////////////////////////////////////

#if 0
bool LimpetImpl::OnInitDialog()
{
   CDialog::OnInitDialog();

   buddy = this;

   // Add "About..." menu item to system menu.

   // IDM_ABOUTBOX must be in the system command range.
   ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
   ASSERT(IDM_ABOUTBOX < 0xF000);

   CMenu* pSysMenu = GetSystemMenu(FALSE);
   if (pSysMenu != NULL)
   {
      QString strAboutMenu;
      strAboutMenu.LoadString(IDS_ABOUTBOX);
      if (!strAboutMenu.isEmpty())
      {
         pSysMenu->AppendMenu(MF_SEPARATOR);
         pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
      }
   }

   // Set the icon for this dialog.  The framework does this automatically
   //  when the application's main window is not a dialog
   SetIcon(m_hIcon, TRUE);			// Set big icon
   SetIcon(m_hIcon, FALSE);		// Set small icon

   CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
   assert(tree);

   CImageList          *pImageList;
   CBitmap             bitmap;

   pImageList = new CImageList();
   pImageList->Create(16, 16, ILC_COLOR4, 2, 1);

   bitmap.LoadBitmap(IDB_OFFLINE);
   pImageList->Add(&bitmap, (COLORREF)0x000000);
   bitmap.DeleteObject();

   bitmap.LoadBitmap(IDB_ONLINE);
   pImageList->Add(&bitmap, (COLORREF)0x000000);
   bitmap.DeleteObject();

   tree->SetImageList(pImageList, TVSIL_NORMAL);

   if ( this->tuIM )
   {
      assert( this->tuIM );
      HTREEITEM group = TVI_ROOT;
      //group = tree->InsertItem("Friends", TVI_ROOT, TVI_SORT);
      for ( int i=0; i<this->tuIM->getNumBuddies(); i++)
      {
         resip::Uri budy = this->tuIM->getBuddyUri(i);
         tree->InsertItem( cullenize(budy).c_str(), group, TVI_SORT);
      }
      tree->Expand(group,TVE_EXPAND);

      UINT_PTR timer = SetTimer(1/*timer number*/, 10 /* time ms*/, timerProcCallback/*callback*/);
      assert(timer);
   }
   else
   {
      CListBox* list = (CListBox*)GetDlgItem(IDC_LIST);
      assert(list);
      list->AddString("Could not set up network - likely the port is in use by another program");
   }

   return TRUE;  // return TRUE  unless you set the focus to a control
}

void LimpetImpl::OnSysCommand(UINT nID, LPARAM lParam)
{
   if ((nID & 0xFFF0) == IDM_ABOUTBOX)
   {
      CAboutDlg dlgAbout;
      dlgAbout.DoModal();
   }
   else
   {
      CDialog::OnSysCommand(nID, lParam);
   }
}

void LimpetImpl::OnEnChangeEdit()
{

   CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT);
   assert(edit);

   int numLines = edit->GetLineCount();

   if ( numLines > 1 )
   {
      QString text;
      edit->GetWindowText(text);
      edit->SetWindowText(_T(""));     

      text = text.Trim("\n\r");

      CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
      assert(tree);
      HTREEITEM sel = tree->GetSelectedItem();
      QString dest = tree->GetItemText(sel);

      QString res = _T("To: ");
      res += dest;
      res += _T(" ");
      res += text;

      CListBox* list = (CListBox*)GetDlgItem(IDC_LIST);
      assert(list);
      list->AddString(res);

      this->sendPage(text,dest);
   }
}

void 
CALLBACK EXPORT LimpetImpl::timerProcCallback(
   HWND hWnd,      // handle of CWnd that called SetTimer
   UINT nMsg,      // WM_TIMER
   UINT nIDEvent,   // timer identification
   DWORD dwTime    // system time
   )
{
   //CWnd* wnd = hWnd;
   //buddyDlg* dlg = (buddyDlg*)( wnd );
   //assert(dlg);

   assert( buddy );
   buddy->timerProc();
}

void 
LimpetImpl::timerProc()
{
   static int count=0;
   if ( (count++)%500 == 0 )
   {
      CListBox* list = (CListBox*)GetDlgItem(IDC_LIST);
      assert(list);
      list->AddString(_T("timer"));
   }

   // poll the stack 
   this->process();
}

#endif

void 
LimpetImpl::receivedPage( const resip::Data& msg, 
                          const resip::Uri& from,
                          const resip::Data& signedBy,  
                          const resip::Security::SignatureStatus sigStatus,
                          const bool wasEncryped  )
{
   QString res = "";

   res += from.getAor().c_str();
   res += " says: ";
   res += msg.c_str();

   MultiLineEdit3->insertLine(res);
}

void 
LimpetImpl::message(const QString& msg)
{
   MultiLineEdit3->insertLine(msg);
}

void 
LimpetImpl::presenseUpdate(const resip::Uri& uri, bool open, const resip::Data& status )
{
   QString res = "";

   res += uri.getAor().c_str();
   res += " is now: ";
   if ( open )
   {
      res += "online ";
   }
   else
   {
      res += "offline ";
   }
   res += status.c_str();

   MultiLineEdit3->insertLine(res);


   // go trhough the items in the list and change the icon of the aproperate one 
   for ( unsigned int index = 0; index < BuddyListWidget->count(); index++ )
   {
      QString dest = BuddyListWidget->text( index );
      resip::Data uDest( dest );
      resip::Uri u( uDest );
      if ( uri.getAor() == u.getAor() )
      {
         // found a match
         QString currentString = BuddyListWidget->text( index );
         if ( open )
         {
            QPixmap pixmap( "online.bmp" );
            BuddyListWidget->changeItem( pixmap, currentString, index );
         }
         else
         {
            QPixmap pixmap( "offline.bmp" );
            BuddyListWidget->changeItem( pixmap, currentString, index );
         }

      }

   }
	
   //tree->Invalidate();
}


#if 0
void LimpetImpl::OnBnClickedButtonAdd()
{
   CAddDlg dlg;
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

void LimpetImpl::OnBnClickedButtonClear()
{
   // TODO: Add your control notification handler code here
   CWinApp* app = AfxGetApp();
   app->WriteProfileInt("buddyList","size", 0 );

   CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
   assert(tree);

   tree->DeleteAllItems();
}

void LimpetImpl::OnEnKillfocusEditNote()
{
   LimpetImpl::OnBnClickedCheckOnline();
}

void LimpetImpl::OnBnClickedCheckSign()
{
   CButton* button = (CButton*)GetDlgItem(IDC_CHECK_SIGN);
   assert(button);

   bool checked = (BST_CHECKED == button->GetCheck());

   this->sign = checked;
}

void LimpetImpl::OnBnClickedCheckEncrypt()
{
   CButton* button = (CButton*)GetDlgItem(IDC_CHECK_ENCRYPT);
   assert(button);

   bool checked = (BST_CHECKED == button->GetCheck());

   this->encryp = checked;
}

void LimpetImpl::OnBnClickedButtonDel()
{
   CWinApp* app = AfxGetApp();
   assert(app);

   // TODO: Add your control notification handler code here
   CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
   assert(tree);

   HTREEITEM sel = tree->GetSelectedItem();
   QString dest = tree->GetItemText(sel);

   Data destData( dest );
   Uri uri( destData );

   tree->DeleteItem( sel );

   if ( !this->tuIM )
   {
      return;
   }

   assert( this->tuIM );
   this->tuIM->removeBuddy( uri );

   //tree->DeleteAllItems();

   assert( this->tuIM );
   //HTREEITEM group = TVI_ROOT;
   //group = tree->InsertItem(_T("Friends"), TVI_ROOT, TVI_SORT);
   for ( int i=0; i<this->tuIM->getNumBuddies(); i++)
   {
      resip::Uri budy = this->tuIM->getBuddyUri(i);
      QString name( _T( cullenize(budy).c_str() ) );
      //tree->InsertItem( name , group, TVI_SORT);

      // write buddy to registry 
      QString item;
      item.sprintf("%d",i);
      app->WriteProfileString("buddyList",item,name);
   }
   //tree->Expand(group,TVE_EXPAND);
   app->WriteProfileInt("buddyList","size",this->tuIM->getNumBuddies());
}

#endif

void LimpetImpl::OnBnClickedCheckOnline()
{
   bool online = !(OfflineCheckbox->isChecked());

   QString text = OfflineText->text();
//	text = text.Trim("\n\r");

   this->setStatus(online,text);
}

void LimpetImpl::send()
{

   QString text = MultiLineEdit4->text();
   MultiLineEdit4->setText("");     

//		text = text.Trim("\n\r");

   assert( BuddyListWidget );
   QString dest = BuddyListWidget->currentText();
   if ( dest == "" )
   {
      return;
   }

   QString res = "To: ";
   res += dest;
   res += " ";
   res += text;

   MultiLineEdit3->insertLine(res);

   this->sendPage(text,dest);
}


////////////////////////////////////////
////////////////////////////////////////
//// CALLBACK stuff



void 
ImCallback::receivedPage( const Data& msg, const Uri& from,
                          const Data& signedBy,  Security::SignatureStatus sigStatus,
                          bool wasEncryped  )
{  
   assert(theApp);
   theApp->receivedPage(  msg, from,signedBy,  sigStatus, wasEncryped  );
}


void 
ImCallback::presenseUpdate(const Uri& dest, bool open, const Data& status )
{
   assert(theApp);
   theApp->presenseUpdate( dest, open, status );
}


void 
ImCallback::sendPageFailed( const Uri& dest, int num )
{
   //InfoLog(<< "In TestErrCallback");  
   //cerr << "Message to " << dest << " failed" << endl;
   QString cNum;
   cNum.sprintf("%d",num);
   QString msg = "Message to ";
   msg += QString(dest.getAor().c_str()); 
   msg += " failed (";
   msg += cNum;
   msg += ")";

   assert(theApp);
   theApp->message( msg );
}

void 
ImCallback::registrationFailed( const Uri& dest,int num )
{
   //InfoLog(<< "In TestErrCallback");  
   //cerr << "Message to " << dest << " failed" << endl;
   QString cNum;
   cNum.sprintf("%d",num);
   QString msg = "Registration to ";
   msg += QString(dest.getAor().c_str()); 
   msg += " failed (";
   msg += cNum;
   msg += ")";

   assert(theApp);
   cout << "Registration failed\n";
   theApp->message( msg );
}

void 
ImCallback::registrationWorked( const Uri& dest )
{
   QString msg = "Registration to ";
   msg += QString(dest.getAor().c_str()); 
   msg += " worked";

   assert(theApp);
   cout << "Registration worked\n";
   theApp->message( msg );
}

void 
ImCallback::receivePageFailed(const Uri& sender)
{
   QString msg = "Can not understand message from ";
   msg += QString(sender.getAor().c_str()); 

   assert(theApp);
   theApp->message( msg );
}


/* ====================================================================
 * The Vovida Software License, Version 1.0  * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

