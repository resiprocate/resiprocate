// Reboot.cpp : implementation file
//

#include "stdafx.h"
#include "SipIMP.h"
#include "Reboot.h"


// CReboot dialog

IMPLEMENT_DYNAMIC(CReboot, CDialog)
CReboot::CReboot(CWnd* pParent /*=NULL*/)
	: CDialog(CReboot::IDD, pParent)
{
}

CReboot::~CReboot()
{
}

void CReboot::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CReboot, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CReboot message handlers

void CReboot::OnBnClickedOk()
{
	OnOK();
}
