// //////////////////////////////////////////////////////////////////////////
// Header file TestRunnerModel.h for class TestRunnerModel
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/04/26
// //////////////////////////////////////////////////////////////////////////
#ifndef TESTRUNNERMODEL_H
#define TESTRUNNERMODEL_H

#include <deque>
#include <cppunit/Test.h>


/*! \class TestRunnerModel
 * \brief This class represents a model for the test runner.
 */
class AFX_CLASS_EXPORT TestRunnerModel
{
public:
  struct Settings
  {
    bool autorunOnLaunch;
    RECT dlgBounds;	
    int  col_1; // 1st column width in list view
    int  col_2; // 2nd column width in list view
    int  col_3; // 3rd column width in list view
    int  col_4; // 4th column width in list view  
  };

  typedef std::deque<CppUnit::Test *> History;

  /*! Constructs a TestRunnerModel object.
   */
  TestRunnerModel( CppUnit::Test *rootTest );

  /*! Destructor.
   */
  virtual ~TestRunnerModel();

  virtual void setRootTest( CppUnit::Test *rootTest );

  void loadSettings(Settings & s);
  void saveSettings(const Settings & s);

  const History &history() const;
  void selectHistoryTest( CppUnit::Test *test );
  CppUnit::Test *selectedTest() const;

  CppUnit::Test *rootTest();

protected:
  void loadHistory();
  CString loadHistoryEntry( int idx );
  CppUnit::Test *findTestByName( CString name ) const;
  CppUnit::Test *findTestByNameFor( const CString &name, 
                                    CppUnit::Test *test ) const;

  void saveHistoryEntry( int idx, 
                         CString testName );

  CString getHistoryEntryName( int idx ) const;

private:
  /// Prevents the use of the copy constructor.
  TestRunnerModel( const TestRunnerModel &copy );

  /// Prevents the use of the copy operator.
  TestRunnerModel &operator =( const TestRunnerModel &copy );

private:
  History m_history;

  CppUnit::Test *m_rootTest;
};



// Inlines methods for TestRunnerModel:
// ------------------------------------



#endif  // TESTRUNNERMODEL_H
