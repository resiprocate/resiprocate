
#include "stdafx.h"

#include <cassert>

#include "SipIMP.h"
#include "BuddyDlg.h"
#include "ConfigDlg.h"
#include "AddDlg.h"

#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/Uri.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace resip;

//#define VOCAL_SUBSYSTEM Subsystem::SIP

BuddyDlg* BuddyDlg::buddy=NULL;

// CBuddyDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// buddyDlg dialog



BuddyDlg::BuddyDlg(CWnd* pParent /*=NULL*/)
: CDialog(BuddyDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void BuddyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(BuddyDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnTvnSelchangedTree)
	ON_EN_CHANGE(IDC_EDIT, OnEnChangeEdit)
	//	ON_EN_UPDATE(IDC_EDIT, OnEnUpdateEdit)
	ON_BN_CLICKED(IDC_BUTTON_CONFIG, OnBnClickedButtonConfig)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_CLEAR, OnBnClickedButtonClear)
	//ON_NOTIFY(BCN_HOTITEMCHANGE, IDC_CHECK_ONLINE, OnBnHotItemChangeCheckOnline)
	ON_BN_CLICKED(IDC_CHECK_ONLINE, OnBnClickedCheckOnline)
	//ON_EN_CHANGE(IDC_EDIT_NOTE, OnEnChangeEditNote)
	//ON_EN_UPDATE(IDC_EDIT_NOTE, OnEnUpdateEditNote)
	ON_EN_KILLFOCUS(IDC_EDIT_NOTE, OnEnKillfocusEditNote)
	ON_BN_CLICKED(IDC_CHECK_SIGN, OnBnClickedCheckSign)
	ON_BN_CLICKED(IDC_CHECK_ENCRYPT, OnBnClickedCheckEncrypt)
	ON_BN_CLICKED(IDC_BUTTON_DEL, OnBnClickedButtonDel)
	ON_BN_CLICKED(IDC_BUTTON_SEND, OnBnClickedButtonSend)
END_MESSAGE_MAP()


// BuddyDlg message handlers

BOOL BuddyDlg::OnInitDialog()
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
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
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

	if ( theApp.tuIM )
	{
		assert( theApp.tuIM );
		HTREEITEM group = TVI_ROOT;
		//group = tree->InsertItem(_T("Friends"), TVI_ROOT, TVI_SORT);
		for ( int i=0; i<theApp.tuIM->getNumBuddies(); i++)
		{
			resip::Uri budy = theApp.tuIM->getBuddyUri(i);
			tree->InsertItem( Data::from(budy).c_str(), group, TVI_SORT);
		}
		tree->Expand(group,TVE_EXPAND);

			UINT_PTR timer = SetTimer(1/*timer number*/, 10 /* time ms*/, timerProcCallback/*callback*/);
	assert(timer);
	}
	else
	{
		display( CString(_T("Could not set up network - likely the port is in use by another program")) );
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void BuddyDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void BuddyDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR BuddyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void BuddyDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	//OnOK();
}

void BuddyDlg::OnLbnSelchangeList()
{
	// TODO: Add your control notification handler code here
}

void BuddyDlg::OnTvnSelchangedTree(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

void BuddyDlg::OnEnChangeEdit()
{

	CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT);
	assert(edit);

	int numLines = edit->GetLineCount();

	if ( numLines > 1 )
	{
		CString text;
		edit->GetWindowText(text);
		edit->SetWindowText(_T(""));     

		text = text.Trim("\n\r");

		CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
		assert(tree);
		HTREEITEM sel = tree->GetSelectedItem();
		CString dest = tree->GetItemText(sel);

		CString res = _T("To: ");
		res += dest;
		res += _T(" ");
		res += text;

		display(res);

		theApp.sendPage(text,dest);
	}
}

void 
CALLBACK EXPORT BuddyDlg::timerProcCallback(
	HWND hWnd,      // handle of CWnd that called SetTimer
	UINT nMsg,      // WM_TIMER
	UINT nIDEvent,   // timer identification
	DWORD dwTime    // system time
	)
{
	//CWnd* wnd = hWnd;
	//BuddyDlg* dlg = (BuddyDlg*)( wnd );
	//assert(dlg);

	assert( buddy );
	buddy->timerProc();
}

void 
BuddyDlg::timerProc()
{
	// poll the stack 
	theApp.process();
}


void 
BuddyDlg::receivedPage( const resip::Data& msg, 
					   const resip::Uri& from,
					   const resip::Data& signedBy,  
					   const resip::Security::SignatureStatus sigStatus,
					   const bool wasEncryped  )
{
	CString res = _T("");

	res += from.getAor().c_str();
	res += _T(" says: ");
	res += msg.c_str();

	display(res);
}

void 
BuddyDlg::message(const CString& msg)
{
	display(msg);
}

void 
BuddyDlg::presenseUpdate(const resip::Uri& uri, bool open, const resip::Data& status )
{
	CString res = _T("");

	res += uri.getAor().c_str();
	res += _T(" is now: ");
	if ( open )
	{
		res += _T("online ");
	}
	else
	{
		res += _T("offline ");
	}
	res += status.c_str();

	display(res);

	// go trhough the items in the list and change the icon of the aproperate one 
		CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
	assert(tree);

	HTREEITEM item = tree->GetRootItem();

	// Delete all of the children of hmyItem.
	while (item )
	{
		if (tree->ItemHasChildren(item))
		{
			HTREEITEM sub = tree->GetChildItem(item);

			while (sub != NULL)
			{
				CString dest = tree->GetItemText(sub);
				resip::Data uDest( dest );
				resip::Uri u( uDest );
				if ( uri.getAor() == u.getAor() )
				{
					// found a match 
					int imageNum = open?1:0;
					tree->SetItemImage(sub, imageNum, imageNum );
				}

				sub = tree->GetNextItem(sub, TVGN_NEXT);
			}
		}

		CString dest = tree->GetItemText(item);
		resip::Data uDest( dest );
		resip::Uri u( uDest );
		if ( uri.getAor() == u.getAor() )
		{
			// found a match 
			int imageNum = open?1:0;
			tree->SetItemImage(item, imageNum, imageNum );
		}

		item = tree->GetNextItem(item, TVGN_NEXT);
	}

	//tree->Invalidate();
}


void 
BuddyDlg::OnBnClickedButtonConfig()
{
	CConfigDlg dlg;
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

void BuddyDlg::OnBnClickedButtonAdd()
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

void BuddyDlg::addBuddy(CString name)
{
	CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
	assert(tree);

	HTREEITEM group = TVI_ROOT;
	//group = tree->InsertItem(_T("Friends"), TVI_ROOT, TVI_SORT);
	tree->InsertItem( name, TVI_ROOT, TVI_SORT);

	tree->Expand(group,TVE_EXPAND);
}

void BuddyDlg::OnBnClickedButtonClear()
{
	// TODO: Add your control notification handler code here
	CWinApp* app = AfxGetApp();
	app->WriteProfileInt("buddyList","size", 0 );

	CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
	assert(tree);

	tree->DeleteAllItems();
}

void BuddyDlg::OnBnClickedCheckOnline()
{
	CButton* button = (CButton*)GetDlgItem(IDC_CHECK_ONLINE);
	assert(button);

	bool online = (BST_CHECKED != button->GetCheck());

	CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT_NOTE);
	assert(edit);

	CString text;
	edit->GetWindowText(text);
	text = text.Trim("\n\r");

	theApp.setStatus(online,text);
}

void BuddyDlg::OnEnKillfocusEditNote()
{
	BuddyDlg::OnBnClickedCheckOnline();
}

void BuddyDlg::OnBnClickedCheckSign()
{
	CButton* button = (CButton*)GetDlgItem(IDC_CHECK_SIGN);
	assert(button);

	bool checked = (BST_CHECKED == button->GetCheck());

	theApp.sign = checked;
}

void BuddyDlg::OnBnClickedCheckEncrypt()
{
	CButton* button = (CButton*)GetDlgItem(IDC_CHECK_ENCRYPT);
	assert(button);

	bool checked = (BST_CHECKED == button->GetCheck());

	theApp.encryp = checked;
}

void BuddyDlg::OnBnClickedButtonDel()
{
	CWinApp* app = AfxGetApp();
	assert(app);

	// TODO: Add your control notification handler code here
	CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
	assert(tree);

	HTREEITEM sel = tree->GetSelectedItem();
	CString dest = tree->GetItemText(sel);

	Data destData( dest );
	Uri uri( destData );

	tree->DeleteItem( sel );

	if ( !theApp.tuIM )
	{
		return;
	}

	assert( theApp.tuIM );
	theApp.tuIM->removeBuddy( uri );

	//tree->DeleteAllItems();

	assert( theApp.tuIM );
	//HTREEITEM group = TVI_ROOT;
	//group = tree->InsertItem(_T("Friends"), TVI_ROOT, TVI_SORT);
	for ( int i=0; i<theApp.tuIM->getNumBuddies(); i++)
	{
		resip::Uri budy = theApp.tuIM->getBuddyUri(i);
		CString name( _T( Data::from(budy).c_str() ) );
		//tree->InsertItem( name , group, TVI_SORT);

		// write buddy to registry 
		CString item;
		item.Format("%d",i);
		app->WriteProfileString("buddyList",item,name);
	}
	//tree->Expand(group,TVE_EXPAND);
	app->WriteProfileInt("buddyList","size",theApp.tuIM->getNumBuddies());
}

void BuddyDlg::OnBnClickedButtonSend()
{
	CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT);
	assert(edit);

	CString text;
	edit->GetWindowText(text);
	edit->SetWindowText(_T(""));     

	text = text.Trim("\n\r");

	CTreeCtrl* tree = (CTreeCtrl*) GetDlgItem(IDC_TREE);
	assert(tree);
	HTREEITEM sel = tree->GetSelectedItem();
	CString dest = tree->GetItemText(sel);

	CString res = _T("To: ");
	res += dest;
	res += _T(" ");
	res += text;

	display( res );

	theApp.sendPage(text,dest);
}

void BuddyDlg::display(const CString& text)
{
    CEdit* display = (CEdit*)GetDlgItem(IDC_DISPLAY);
	assert(display);
	display->ReplaceSel( text + CString( _T("\r\n")) );
}

/* ====================================================================
 * The Vovida Software License, Version 1.0  *  * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved. *  * Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met: *  * 1. Redistributions of source code must retain the above copyright *    notice, this list of conditions and the following disclaimer. *  * 2. Redistributions in binary form must reproduce the above copyright *    notice, this list of conditions and the following disclaimer in *    the documentation and/or other materials provided with the *    distribution. *  * 3. The names "VOCAL", "Vovida Open Communication Application Library", *    and "Vovida Open Communication Application Library (VOCAL)" must *    not be used to endorse or promote products derived from this *    software without prior written permission. For written *    permission, please contact vocal@vovida.org. * * 4. Products derived from this software may not be called "VOCAL", nor *    may "VOCAL" appear in their name, without prior written *    permission of Vovida Networks, Inc. *  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH * DAMAGE. *  * ==================================================================== *  * This software consists of voluntary contributions made by Vovida * Networks, Inc. and many individuals on behalf of Vovida Networks, * Inc.  For more information on Vovida Networks, Inc., please see * <http://www.vovida.org/>. *
 */

