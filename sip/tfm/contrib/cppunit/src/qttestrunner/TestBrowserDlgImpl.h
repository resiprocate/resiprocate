#ifndef TESTBROWSER_H
#define TESTBROWSER_H

#include <cppunit/Test.h>
#include "TestBrowserDlg.h"

class QListViewItem;

class TestBrowser : public TestBrowserBase
{ 
    Q_OBJECT

public:
  TestBrowser( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
  ~TestBrowser();

  void setRootTest( CppUnit::Test *rootTest );

  CppUnit::Test *selectedTest();

protected slots:
  void accept();

private:
  void insertItemFor( CppUnit::Test *test,
                       QListViewItem *parentItem );

private:
  CppUnit::Test *_selectedTest;
};

#endif // TESTBROWSER_H
