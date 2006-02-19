// MsDevCallerListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include <atlbase.h>

#include "MsDevCallerListCtrl.h"
#include <msvc6/testrunner/TestRunner.h>
#include <msvc6/DSPlugin/TestRunnerDSPlugin.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// MsDevCallerListCtrl

const int MsDevCallerListCtrl::s_lineNumberSubItem = 3;
const int MsDevCallerListCtrl::s_fileNameSubItem = 4;

MsDevCallerListCtrl::MsDevCallerListCtrl()
{
}

MsDevCallerListCtrl::~MsDevCallerListCtrl()
{
}


BEGIN_MESSAGE_MAP(MsDevCallerListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(MsDevCallerListCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblclk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// MsDevCallerListCtrl message handlers

void MsDevCallerListCtrl::OnDblclk(NMHDR* pNMHDR, LRESULT* pResult) 
{
  HRESULT hr = S_OK;
  int hotItem = 0;

  CComPtr< ITestRunnerDSPlugin> pIDSPlugin;
 
  hr = CoCreateInstance( CLSID_DSAddIn, NULL, CLSCTX_LOCAL_SERVER, IID_ITestRunnerDSPlugin, 
                          reinterpret_cast< void**>(&pIDSPlugin));
  
  if ( SUCCEEDED( hr))
  {
    CPoint pt;
    UINT flags = 0;
    CString lineNumber, fileName;

    GetCursorPos( &pt);
    ScreenToClient( &pt);

    // some dirty hack to get some selection
    // should get the border-width + 1, but WINDOWINFO
    // is not supported in Win95
    pt.x = 5;

    hotItem = HitTest( pt, &flags);

    lineNumber = GetItemText( hotItem, s_lineNumberSubItem);
    fileName = GetItemText( hotItem, s_fileNameSubItem);
    
    pIDSPlugin->goToLineInSourceCode( CComBSTR( fileName), _ttoi( lineNumber));

  }

	*pResult = 0;
}
