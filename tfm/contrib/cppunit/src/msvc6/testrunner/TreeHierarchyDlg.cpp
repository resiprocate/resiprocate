// TreeHierarchyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TreeHierarchyDlg.h"
#include <algorithm>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// TreeHierarchyDlg dialog


TreeHierarchyDlg::TreeHierarchyDlg(CWnd* pParent )
	: CDialog(TreeHierarchyDlg::IDD, pParent),
    m_selectedTest( NULL )
{
	//{{AFX_DATA_INIT(TreeHierarchyDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void 
TreeHierarchyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(TreeHierarchyDlg)
	DDX_Control(pDX, IDC_TREE_TEST, m_treeTests);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(TreeHierarchyDlg, CDialog)
	//{{AFX_MSG_MAP(TreeHierarchyDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



void 
TreeHierarchyDlg::setRootTest( CppUnit::Test *test )
{
  m_rootTest = test;
}


BOOL 
TreeHierarchyDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  fillTree();
  	
	return TRUE;
}


void 
TreeHierarchyDlg::fillTree()
{
  VERIFY( m_imageList.Create( IDB_TEST_TYPE, 16, 1, RGB( 255,0,255 ) ) );

  m_treeTests.SetImageList( &m_imageList, TVSIL_NORMAL );

  HTREEITEM hSuite = addTest( m_rootTest, TVI_ROOT );
  m_treeTests.Expand( hSuite, TVE_EXPAND );
}


HTREEITEM
TreeHierarchyDlg::addTest( CppUnit::Test *test, 
                           HTREEITEM hParent )
{
  int testType = isTestSuite( test ) ? imgSuite : imgUnitTest;
  HTREEITEM hItem = m_treeTests.InsertItem( CString(test->getName().c_str()),
                                            testType,
                                            testType,
                                            hParent );
  if ( hItem != NULL )
  {
    VERIFY( m_treeTests.SetItemData( hItem, (DWORD)test ) );
    if ( isTestSuite( test ) )
      addTestSuiteChildrenTo( static_cast<CppUnit::TestSuite *>(test),
                              hItem );
  }
  return hItem;
}


void 
TreeHierarchyDlg::addTestSuiteChildrenTo( CppUnit::TestSuite *suite,
                                          HTREEITEM hItemSuite )
{
  Tests tests( suite->getTests() );
  sortByName( tests );

  for ( Tests::const_iterator it = tests.begin(); it != tests.end(); ++it )
  {
    addTest( *it, hItemSuite );
  }
}


bool 
TreeHierarchyDlg::isTestSuite( CppUnit::Test *test )
{
  CppUnit::TestSuite *suite = dynamic_cast<CppUnit::TestSuite *>(test);
  return suite != NULL;
}


struct PredSortTest
{
  bool operator()( CppUnit::Test *test1, CppUnit::Test *test2 ) const
  {
    bool isTest1Suite = TreeHierarchyDlg::isTestSuite( test1 );
    bool isTest2Suite = TreeHierarchyDlg::isTestSuite( test2 );
    if ( isTest1Suite  &&  !isTest2Suite )
      return true;
    if ( isTest1Suite  &&  isTest2Suite )
      return test1->getName() < test2->getName();
    return false;
  }
};

void 
TreeHierarchyDlg::sortByName( Tests &tests ) const
{
  std::stable_sort( tests.begin(), tests.end(), PredSortTest() );
}


void 
TreeHierarchyDlg::OnOK() 
{
  CppUnit::Test *test = findSelectedTest();
  if ( test == NULL )
  {
    AfxMessageBox( IDS_ERROR_SELECT_TEST, MB_OK );
    return;
  }

  m_selectedTest = test;
  CDialog::OnOK();
}


CppUnit::Test *
TreeHierarchyDlg::findSelectedTest()
{
  HTREEITEM hItem = m_treeTests.GetSelectedItem();
  if ( hItem != NULL )
  {
    DWORD data;
    VERIFY( data = m_treeTests.GetItemData( hItem ) );
    return reinterpret_cast<CppUnit::Test *>( data );
  }
  return NULL;
}


CppUnit::Test *
TreeHierarchyDlg::getSelectedTest() const
{
  return m_selectedTest;
}
