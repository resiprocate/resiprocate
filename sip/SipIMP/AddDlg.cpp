// AddDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SipIMP.h"
#include "AddDlg.h"

// CAddDlg dialog

IMPLEMENT_DYNAMIC(CAddDlg, CDialog)
CAddDlg::CAddDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAddDlg::IDD, pParent)
	, newBuddyName(_T(""))
{
	//int foo;
	//AfxGetApp()->WriteProfileInt("camera","iris",foo);
}

CAddDlg::~CAddDlg()
{
}

void CAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_BUDDY_NAME, newBuddyName);
	DDV_MaxChars(pDX, newBuddyName, 256);
}


BEGIN_MESSAGE_MAP(CAddDlg, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CAddDlg message handlers

void CAddDlg::OnBnClickedOk()
{
	OnOK();

	CWinApp* app = AfxGetApp();
	assert(app);

	CString uri = theApp.addBuddy(newBuddyName);

	int n = app->GetProfileInt("buddyList","size",0);

	CString item;
	item.Format("%d",n);
	app->WriteProfileString("buddyList",item,uri);
	app->WriteProfileInt("buddyList","size", n+1 );
}
