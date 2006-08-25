// TestRunnerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "mmsystem.h"
#include "TestRunnerApp.h"
#include "TestRunnerDlg.h"
#include "Resource.h"
#include "ActiveTest.h"
#include "ProgressBar.h"
#include "TreeHierarchyDlg.h"
#include "ListCtrlFormatter.h"
#include "ListCtrlSetter.h"
#include "MfcSynchronizationObject.h"
#include <cppunit/TestFailure.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* Notes:
 - code duplication between OnOK() and OnQuitApplication()
 - the threading need to be rewrite, so that GUI update occures in the original
 thread, not in the thread that is running the tests. This slow down the time
 needed to run the test much...
 */


/////////////////////////////////////////////////////////////////////////////
// TestRunnerDlg dialog

const CString TestRunnerDlg::ms_cppunitKey( "CppUnit" );


TestRunnerDlg::TestRunnerDlg( TestRunnerModel *model,
                              int nDialogResourceId,
                              CWnd* pParent ) :
    CDialog( nDialogResourceId == -1 ? IDD_DIALOG_TESTRUNNER :
                                       nDialogResourceId, 
             pParent),
    m_model( model ),
    m_margin( 0 ),
    m_listViewDelta( 0 ),
    m_editDelta( 0 )
{
  //{{AFX_DATA_INIT(TestRunnerDlg)
    m_bAutorunAtStartup = FALSE;
  //}}AFX_DATA_INIT

  m_testsProgress     = 0;
  m_selectedTest      = 0;
  m_bAutorunAtStartup = true;
  m_bIsRunning = false;
}


void 
TestRunnerDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(TestRunnerDlg)
    DDX_Control(pDX, IDC_LIST, m_listCtrl);
    DDX_Control(pDX, IDOK, m_buttonClose);
    DDX_Control(pDX, ID_STOP, m_buttonStop);
    DDX_Control(pDX, ID_RUN, m_buttonRun);
    DDX_Check(pDX, IDC_CHECK_AUTORUN, m_bAutorunAtStartup);
  //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TestRunnerDlg, CDialog)
  //{{AFX_MSG_MAP(TestRunnerDlg)
  ON_BN_CLICKED(ID_RUN, OnRun)
  ON_BN_CLICKED(ID_STOP, OnStop)
  ON_CBN_SELCHANGE(IDC_COMBO_TEST, OnSelchangeComboTest)
  ON_WM_PAINT()
  ON_BN_CLICKED(IDC_BROWSE_TEST, OnBrowseTest)
  ON_COMMAND(ID_QUIT_APPLICATION, OnQuitApplication)
  ON_WM_CLOSE()
  ON_WM_SIZE()
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// TestRunnerDlg message handlers

BOOL 
TestRunnerDlg::OnInitDialog() 
{
  CDialog::OnInitDialog();

//    m_hAccelerator = ::LoadAccelerators( AfxGetResourceHandle(),
  m_hAccelerator = ::LoadAccelerators( g_testRunnerResource,
                                       MAKEINTRESOURCE( IDR_ACCELERATOR_TEST_RUNNER ) );
// It always fails!!! I don't understand why. Complain about not finding the resource name!
  ASSERT( m_hAccelerator !=NULL );
  
  CListCtrl   *listCtrl = (CListCtrl *)GetDlgItem (IDC_LIST);
  CComboBox   *comboBox = (CComboBox *)GetDlgItem (IDC_COMBO_TEST);

  ASSERT (listCtrl);
  ASSERT (comboBox);
  ListCtrlFormatter formatter( *listCtrl );

  VERIFY( m_errorListBitmap.Create( IDB_ERROR_TYPE, 16, 1, 
                                    RGB( 255,0,255 ) ) );

  loadSettings();
      
  listCtrl->SetImageList( &m_errorListBitmap, LVSIL_SMALL );
  listCtrl->SetExtendedStyle( listCtrl->GetExtendedStyle() | LVS_EX_FULLROWSELECT );

  int total_col_1_4 = m_settings.col_1 + m_settings.col_2 + 
		      m_settings.col_3 + m_settings.col_4;

  CRect listBounds;
  listCtrl->GetClientRect(&listBounds);
  int col_5_width = listBounds.Width() - total_col_1_4; // 5th column = rest of listview space
  formatter.AddColumn( IDS_ERRORLIST_TYPE, m_settings.col_1, LVCFMT_LEFT, 0 );
  formatter.AddColumn( IDS_ERRORLIST_NAME, m_settings.col_2, LVCFMT_LEFT, 1 );
  formatter.AddColumn( IDS_ERRORLIST_FAILED_CONDITION, m_settings.col_3, LVCFMT_LEFT, 2 );
  formatter.AddColumn( IDS_ERRORLIST_LINE_NUMBER, m_settings.col_4, LVCFMT_LEFT, 3 );
  formatter.AddColumn( IDS_ERRORLIST_FILE_NAME, col_5_width, LVCFMT_LEFT, 4 );

  m_testsProgress = new ProgressBar (this, CRect (50, 85, 50 + 425, 85 + 25));

  reset ();

  updateLayoutInfo();
      
  updateHistoryCombo();

  UpdateData( FALSE );

  RECT & r = m_settings.dlgBounds;
  if ( r.right-r.left > 0  &&  r.bottom-r.top > 0 )
      MoveWindow( &r );

  m_buttonRun.SetFocus();

  if ( m_bAutorunAtStartup )
    OnRun();
  
  return FALSE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


TestRunnerDlg::~TestRunnerDlg ()
{ 
  freeState ();
  delete m_testsProgress;
}


void 
TestRunnerDlg::OnRun() 
{
  if ( m_bIsRunning )
    return;

  m_selectedTest = m_model->selectedTest();

  if ( m_selectedTest == 0 )
    return;

  freeState(); 
  reset();

  beRunning();

  int numberOfTests = m_selectedTest->countTestCases();

  m_testsProgress->start( numberOfTests );

  
  m_result = new CppUnit::TestResultCollector( new MfcSynchronizationObject() );
  m_testObserver = new CppUnit::TestResult( new MfcSynchronizationObject() );
  m_testObserver->addListener( m_result );
  m_testObserver->addListener( this );
  m_activeTest = new ActiveTest( m_selectedTest );

  m_testStartTime = timeGetTime();

  m_activeTest->run( m_testObserver );

  m_testEndTime = timeGetTime();
}


void 
TestRunnerDlg::addListEntry( const CppUnit::TestFailure &failure )
{
  CListCtrl *listCtrl = (CListCtrl *)GetDlgItem (IDC_LIST);
  int currentEntry = m_result->testErrors() + 
                     m_result->testFailures() -1;

  ErrorTypeBitmaps errorType;
  if ( failure.isError() )
    errorType = errorTypeError;
  else
    errorType = errorTypeFailure;

  ListCtrlSetter setter( *listCtrl );
  setter.insertLine( currentEntry );
  setter.addSubItem( failure.isError() ? _T("Error") : _T("Failure"), errorType );

  // Set test name
  setter.addSubItem( failure.failedTestName().c_str(), errorType );

  // Set the asserted text
  setter.addSubItem( failure.thrownException()->what() );

  // Set the line number
  if ( failure.sourceLine().isValid() )
  {
    CString lineNumber;
    lineNumber.Format( _T("%ld"), failure.sourceLine().lineNumber() );
    setter.addSubItem( lineNumber );
  }
  else
    setter.addSubItem( _T("") );

  // Set the file name
  setter.addSubItem( failure.sourceLine().fileName().c_str() );

/* In place of the missing detail field...
  std::string dump = "Test: " + test->getName() + "\n";
  dump += e->what();
  dump += "\n";
  ::OutputDebugString( dump.c_str() );
*/

  listCtrl->RedrawItems (currentEntry, currentEntry);
  listCtrl->UpdateWindow ();
}


void 
TestRunnerDlg::startTest( CppUnit::Test *test )
{
}


void 
TestRunnerDlg::addFailure( const CppUnit::TestFailure &failure )
{
  addListEntry( failure );
  if ( failure.isError() )
    m_errors++;
  else
    m_failures++;

  updateCountsDisplay();
}


void 
TestRunnerDlg::endTest( CppUnit::Test *test )
{
  if ( m_selectedTest == 0 )
    return;

  m_testsRun++;
  updateCountsDisplay();
  m_testsProgress->step( m_failures == 0  &&  m_errors == 0 );

  m_testEndTime = timeGetTime();

  updateCountsDisplay();

  if ( m_testsRun >= m_selectedTest->countTestCases() )
    beIdle ();
}


void 
TestRunnerDlg::beRunning()
{
  m_bIsRunning = true;
  m_buttonRun.EnableWindow( FALSE );
  m_buttonClose.EnableWindow( FALSE );

//    m_buttonRun.SetButtonStyle( m_buttonRun.GetButtonStyle() & ~BS_DEFPUSHBUTTON );
//    m_buttonStop.SetButtonStyle( m_buttonStop.GetButtonStyle() | BS_DEFPUSHBUTTON );
}


void 
TestRunnerDlg::beIdle()
{
  m_bIsRunning = false;
  m_buttonRun.EnableWindow( TRUE );
  m_buttonClose.EnableWindow( TRUE );

  m_buttonRun.SetButtonStyle( m_buttonRun.GetButtonStyle() | BS_DEFPUSHBUTTON );
//    m_buttonStop.SetButtonStyle( m_buttonStop.GetButtonStyle() & ~BS_DEFPUSHBUTTON );
}


void 
TestRunnerDlg::beRunDisabled()
{
  m_bIsRunning = false;
  m_buttonRun.EnableWindow( FALSE );
  m_buttonStop.EnableWindow( FALSE );
  m_buttonClose.EnableWindow( TRUE );

//    m_buttonRun.SetButtonStyle( m_buttonRun.GetButtonStyle() | BS_DEFPUSHBUTTON );
//    m_buttonStop.SetButtonStyle( m_buttonStop.GetButtonStyle() & ~BS_DEFPUSHBUTTON );
}


void 
TestRunnerDlg::freeState()
{
  delete m_activeTest;
  delete m_result;
  delete m_testObserver;
}


void 
TestRunnerDlg::reset()
{
  m_testsRun = 0;
  m_errors = 0;
  m_failures = 0;
  m_testEndTime = m_testStartTime;

  updateCountsDisplay();

  m_activeTest = NULL;
  m_result = NULL;
  m_testObserver = NULL;

  CListCtrl *listCtrl = (CListCtrl *)GetDlgItem (IDC_LIST);

  listCtrl->DeleteAllItems();
  m_testsProgress->reset();
}


void 
TestRunnerDlg::updateCountsDisplay()
{
  CStatic *statTestsRun = (CStatic *)GetDlgItem( IDC_STATIC_RUNS );
  CStatic *statErrors = (CStatic *)GetDlgItem( IDC_STATIC_ERRORS );
  CStatic *statFailures = (CStatic *)GetDlgItem( IDC_STATIC_FAILURES );
  CEdit *editTime = (CEdit *)GetDlgItem( IDC_EDIT_TIME );

  CString argumentString;

  argumentString.Format( _T("%d"), m_testsRun );
  statTestsRun->SetWindowText (argumentString);

  argumentString.Format( _T("%d"), m_errors );
  statErrors->SetWindowText( argumentString );

  argumentString.Format( _T("%d"), m_failures );
  statFailures->SetWindowText( argumentString );

  argumentString.Format( _T("Execution time: %3.3lf seconds"), 
                         (m_testEndTime - m_testStartTime) / 1000.0 );
  editTime->SetWindowText( argumentString );
}


void 
TestRunnerDlg::OnStop() 
{
  if ( m_testObserver )
    m_testObserver->stop ();

  beIdle ();
}


void 
TestRunnerDlg::OnOK() 
{
  if ( m_testObserver )
    m_testObserver->stop ();

  UpdateData();
  saveSettings();

  CDialog::OnOK ();
}


void 
TestRunnerDlg::OnSelchangeComboTest() 
{
  int currentSelection = getHistoryCombo()->GetCurSel ();

  if ( currentSelection >= 0  &&
       currentSelection < m_model->history().size() )
  {
    CppUnit::Test *selectedTest = m_model->history()[currentSelection];
    m_model->selectHistoryTest( selectedTest );
    updateHistoryCombo();
    beIdle();
  }
  else
    beRunDisabled();
}


void
TestRunnerDlg::updateHistoryCombo()
{
  getHistoryCombo()->LockWindowUpdate();

  getHistoryCombo()->ResetContent();

  const TestRunnerModel::History &history = m_model->history();
  for ( TestRunnerModel::History::const_iterator it = history.begin(); 
        it != history.end(); 
        ++it )
  {
    CppUnit::Test *test = *it;
    getHistoryCombo()->AddString( CString(test->getName().c_str()) );
  }

  if ( history.size() > 0 )
  {
    getHistoryCombo()->SetCurSel( 0 );
    beIdle();
  }
  else
    beRunDisabled();

  getHistoryCombo()->UnlockWindowUpdate();
}


void 
TestRunnerDlg::OnPaint() 
{
  CPaintDC dc (this); 
  
  m_testsProgress->paint(dc);
}


void 
TestRunnerDlg::OnBrowseTest() 
{
  TreeHierarchyDlg dlg;
  dlg.setRootTest( m_model->rootTest() );
  if ( dlg.DoModal() == IDOK )
  {
    m_model->selectHistoryTest( dlg.getSelectedTest() );
    updateHistoryCombo();
  }
}


BOOL 
TestRunnerDlg::PreTranslateMessage(MSG* pMsg) 
{
  if ( ::TranslateAccelerator( m_hWnd,
                               m_hAccelerator,
                               pMsg ) )
  {
    return TRUE;
  }
  return CDialog::PreTranslateMessage(pMsg);
}


CComboBox *
TestRunnerDlg::getHistoryCombo()
{
  CComboBox   *comboBox = (CComboBox *)GetDlgItem (IDC_COMBO_TEST);
  ASSERT (comboBox);
  return comboBox;
}


void
TestRunnerDlg::loadSettings()
{
  m_model->loadSettings(m_settings);

  m_bAutorunAtStartup = m_settings.autorunOnLaunch;
}


void
TestRunnerDlg::saveSettings()
{
  m_settings.autorunOnLaunch = ( m_bAutorunAtStartup != 0 );
  GetWindowRect(&m_settings.dlgBounds);
  
  m_settings.col_1 = m_listCtrl.GetColumnWidth(0);
  m_settings.col_2 = m_listCtrl.GetColumnWidth(1);
  m_settings.col_3 = m_listCtrl.GetColumnWidth(2);
  m_settings.col_4 = m_listCtrl.GetColumnWidth(3);

  m_model->saveSettings(m_settings);
}


void 
TestRunnerDlg::OnQuitApplication() 
{
  if ( m_testObserver )
    m_testObserver->stop();

  UpdateData();
  saveSettings();
  
  CWinApp *app = AfxGetApp();
  ASSERT( app != NULL );
  app->PostThreadMessage( WM_QUIT, 0, 0 );
}


TestRunnerModel &
TestRunnerDlg::model()
{
  ASSERT( m_model != NULL );
  return *m_model;
}


void 
TestRunnerDlg::OnClose() 
{
	OnOK();
}


void 
TestRunnerDlg::updateLayoutInfo()
{
  // get dialog in screen co-ordinates
  RECT dlgBounds;
  GetWindowRect( &dlgBounds );

  // Set general margin according to distance of browse button from the right
  m_margin = dlgBounds.right - getItemWindowRect(IDC_BROWSE_TEST).right;
  m_listViewDelta = dlgBounds.bottom - getItemWindowRect( IDC_LIST ).bottom;
  m_editDelta = dlgBounds.bottom - getItemWindowRect( IDC_EDIT_TIME ).bottom;
}


void 
TestRunnerDlg::OnSize(UINT nType, int cx, int cy) 
{
  CDialog::OnSize(nType, cx, cy);
  
  // Layout controls according to new size	
  CRect r;
  CRect dlgBounds = getDialogBounds();
  
  int buttonLeft  = 0;
  int buttonRight = 0;
  
  CWnd * pItem = GetDlgItem(IDC_BROWSE_TEST);
  if (pItem)
  {
    pItem->GetWindowRect( &r );
    // adjust to dialog units
    r.OffsetRect(-dlgBounds.left, -dlgBounds.top);
    
    int buttonWidth = r.Width();
    buttonLeft  = cx - buttonWidth - m_margin; 
    buttonRight = buttonLeft + buttonWidth;
    r.left =  buttonLeft;
    r.right = buttonRight;
    
    pItem->MoveWindow( &r );
    pItem->Invalidate();
  }

  updateTopButtonPosition( ID_RUN, buttonLeft, buttonRight );
  updateTopButtonPosition( IDC_CHECK_AUTORUN, buttonLeft, buttonRight );
  updateBottomButtonPosition( IDOK, buttonLeft, buttonRight, cy - m_editDelta );
  updateBottomButtonPosition( ID_STOP, buttonLeft, buttonRight, cy - m_listViewDelta );
  
  pItem = GetDlgItem(IDC_COMBO_TEST);
  if (pItem)
  {
    pItem->GetClientRect( &r );
    pItem->ClientToScreen( &r );
    
    // adjust to dialog units
    r.OffsetRect(-dlgBounds.left, -dlgBounds.top);
    r.right = buttonLeft - m_margin;
    
    pItem->MoveWindow( &r );
    pItem->Invalidate();
  }

  updateListPosition( buttonLeft );
  
  if (m_testsProgress)
  {
    m_testsProgress->setWidth(r.right - r.left);
  }
  
  pItem = GetDlgItem(IDC_EDIT_TIME);
  if (pItem)
  {
    pItem->GetWindowRect( &r );
    
    // adjust to dialog units
    r.OffsetRect(-dlgBounds.left, -dlgBounds.top);
    
    r.right =  buttonLeft - m_margin;
    
    int height = r.Height();
    // order matters here ( r.top uses new r.bottom )
    r.bottom = cy - m_editDelta;
    r.top = r.bottom - height;
    
    pItem->MoveWindow( &r );
    pItem->Invalidate();
  }
}


void 
TestRunnerDlg::updateTopButtonPosition( unsigned int buttonId,
                                        int xButtonLeft,
                                        int xButtonRight )
{
  CWnd *pItem = GetDlgItem( buttonId );
  if ( !pItem )
    return;
  
  CRect r;
  pItem->GetClientRect( &r );
  pItem->ClientToScreen( &r );
  
  // adjust to dialog units
  CRect dlgBounds = getDialogBounds();
  r.OffsetRect( -dlgBounds.left, -dlgBounds.top );
  r.left = xButtonLeft;
  r.right = xButtonRight;
  
  pItem->MoveWindow( &r );
  pItem->Invalidate();
}


void 
TestRunnerDlg::updateBottomButtonPosition( unsigned int buttonId,
                                           int xButtonLeft,
                                           int xButtonRight,
                                           int yButtonBottom )
{
  CWnd *pItem = GetDlgItem( buttonId );
  if ( !pItem )
    return;
  
  CRect r;
  pItem->GetClientRect( &r );
  pItem->ClientToScreen( &r );
  
  // adjust to dialog units
  CRect dlgBounds = getDialogBounds();
  r.OffsetRect(-dlgBounds.left, -dlgBounds.top);
  r.left =  xButtonLeft;
  r.right = xButtonRight;
  
  int height = r.Height();
  // order matters here ( r.top uses new r.bottom )
  r.bottom = yButtonBottom;
  r.top = r.bottom - height;
  
  pItem->MoveWindow( &r );
  pItem->Invalidate();
}


void 
TestRunnerDlg::updateListPosition( int xButtonLeft )
{
  CWnd *pItem = GetDlgItem( IDC_LIST );
  if ( !pItem )
    return;
  CRect r;
  pItem->GetWindowRect( &r );
  
  // adjust to dialog units
  CRect dlgBounds = getDialogBounds();
  r.OffsetRect(-dlgBounds.left, -dlgBounds.top);
  
  r.right = xButtonLeft - m_margin;
  
  // resize to fit last column
  
  LVCOLUMN lv;
  lv.mask = LVCF_WIDTH;
  int width_1_4 = 0;
  for (int i = 0; i < 4; ++i)
  {
    m_listCtrl.GetColumn(i, &lv);
    width_1_4 += lv.cx;
  }
  
  // the 4 offset is so no horiz scroll bar will appear
  m_listCtrl.SetColumnWidth(4, r.Width() - width_1_4 - 4); 
  
  // order matters here ( r.top uses new r.bottom )
  r.bottom = getDialogBounds().Height() - m_listViewDelta;
  
  pItem->MoveWindow( &r );
  pItem->Invalidate();
}


CRect 
TestRunnerDlg::getItemWindowRect( unsigned int itemId )
{
  CWnd * pItem = GetDlgItem( itemId );
  CRect rect;
  if ( pItem )
    pItem->GetWindowRect( &rect );
  return rect;
}


CRect 
TestRunnerDlg::getDialogBounds()
{
  CRect dlgBounds;
  GetClientRect( &dlgBounds );
  ClientToScreen( &dlgBounds );
  return dlgBounds;
}
