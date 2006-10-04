#if !defined(AFX_TESTRUNNERDLG_H)
#define AFX_TESTRUNNERDLG_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TestRunnerDlg.h : header file
//

/* Refer to MSDN documentation:
   mk:@MSITStore:h:\DevStudio\MSDN\98VSa\1036\vcmfc.chm::/html/_mfcnotes_tn033.htm#_mfcnotes_how_to_write_an_mfc_extension_dll
   to know how to write and use MFC extension DLL
   Can be found in the index with "mfc extension"
   =>
   Using:
   - your application must link  Multithreaded MFC DLL
   - memory allocation is done using the same heap
   - you must define the symbol _AFX_DLL
   Building:
   - you must define the symbol _AFX_DLL and _AFX_EXT
 */

// Define the folowing symbol to subclass TestRunnerDlg
#ifndef CPPUNIT_SUBCLASSING_TESTRUNNERDLG_BUILD
#include "resource.h"
#else
#define IDD_DIALOG_TESTRUNNER 0
#endif

#include <vector>
#include <cppunit/TestSuite.h>
#include <cppunit/Exception.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestListener.h>
#include <cppunit/TestResultCollector.h>

#include "ActiveTest.h"
#include "MsDevCallerListCtrl.h"
#include "TestRunnerModel.h"

class ProgressBar;
class TestRunnerModel;


/////////////////////////////////////////////////////////////////////////////
// TestRunnerDlg dialog

class AFX_EXT_CLASS TestRunnerDlg : public CDialog,
                                    public CppUnit::TestListener
{
public:
  TestRunnerDlg( TestRunnerModel *model,
                int nDialogResourceId = -1,
                CWnd* pParent = NULL);
  ~TestRunnerDlg();

  // overrided from TestListener;
  void startTest( CppUnit::Test *test );
  void addFailure( const CppUnit::TestFailure &failure );
  void endTest( CppUnit::Test *test );

  // IDD is not use, it is just there for the wizard.
  //{{AFX_DATA(TestRunnerDlg)
  enum { IDD = IDD_DIALOG_TESTRUNNER };
	MsDevCallerListCtrl	m_listCtrl;
  CButton	m_buttonClose;
  CButton	m_buttonStop;
  CButton	m_buttonRun;
  BOOL	m_bAutorunAtStartup;
  //}}AFX_DATA

  //{{AFX_VIRTUAL(TestRunnerDlg)
public:
  virtual BOOL PreTranslateMessage(MSG* pMsg);
protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  //}}AFX_VIRTUAL

protected:

  //{{AFX_MSG(TestRunnerDlg)
  virtual BOOL OnInitDialog();
  afx_msg void OnRun();
  afx_msg void OnStop();
  virtual void OnOK();
  afx_msg void OnSelchangeComboTest();
  afx_msg void OnPaint();
  afx_msg void OnBrowseTest();
  afx_msg void OnQuitApplication();
  afx_msg void OnClose();
  afx_msg void OnSize(UINT nType, int cx, int cy);
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()

  typedef std::vector<CppUnit::Test *> Tests;
  ProgressBar *m_testsProgress;
  CppUnit::Test *m_selectedTest;
  ActiveTest *m_activeTest;
  CppUnit::TestResult *m_testObserver;
  CppUnit::TestResultCollector *m_result;
  int m_testsRun;
  int m_errors;
  int m_failures;
  DWORD m_testStartTime;
  DWORD m_testEndTime;
  static const CString ms_cppunitKey;
  HACCEL m_hAccelerator;
  bool m_bIsRunning;
  TestRunnerModel *m_model;
  CImageList m_errorListBitmap;

  enum ErrorTypeBitmaps
  {
    errorTypeFailure =0,
    errorTypeError
  };

  void addListEntry( const CppUnit::TestFailure &failure );
  void beIdle();
  void beRunning();
  void beRunDisabled();
  void reset();
  void freeState();
  void updateCountsDisplay();
  void setupHistoryCombo();
  CppUnit::Test *findTestByName( std::string name ) const;
  CppUnit::Test *findTestByNameFor( const std::string &name, 
                                    CppUnit::Test *test ) const;
  void addNewTestToHistory( CppUnit::Test *test );
  void addTestToHistoryCombo( CppUnit::Test *test, 
                              int idx =-1 );
  void removeTestFromHistory( CppUnit::Test *test );
  CComboBox *getHistoryCombo();
  void updateSelectedItem();
  void saveHistory();
  void loadSettings();
  void saveSettings();
  TestRunnerModel &model();
  void updateHistoryCombo();

  void updateTopButtonPosition( unsigned int buttonId,
                                int xButtonLeft,
                                int xButtonRight );
  void updateBottomButtonPosition( unsigned int buttonId,
                                   int xButtonLeft,
                                   int xButtonRight,
                                   int yButtonBottom );

  // layout management
  void updateLayoutInfo();

  CRect getItemWindowRect( unsigned int itemId );
  CRect getDialogBounds();
  void updateListPosition( int xButtonLeft );

private:

  int m_margin;

  /// distance from bottom of ListView
  int m_listViewDelta;

  /// distance from timing edit box
  int m_editDelta;

  TestRunnerModel::Settings m_settings;
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TESTRUNNERDLG_H)
