// //////////////////////////////////////////////////////////////////////////
// Header file TestListViewItem.h for class TestListViewItem
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/09/19
// //////////////////////////////////////////////////////////////////////////
#ifndef TESTLISTVIEWITEM_H
#define TESTLISTVIEWITEM_H

#include <qlistview.h>
#include <cppunit/Test.h>


/*! \class TestListViewItem
 * \brief This class represents an list item pointing to a Test.
 */
class TestListViewItem : public QListViewItem
{
public:
  /*! Constructs a TestListViewItem object.
   */
  TestListViewItem( CppUnit::Test *test,
                    QListViewItem *parent );

  /*! Destructor.
   */
  virtual ~TestListViewItem();

  CppUnit::Test *test() const;

private:
  /// Prevents the use of the copy constructor.
  TestListViewItem( const TestListViewItem &copy );

  /// Prevents the use of the copy operator.
  void operator =( const TestListViewItem &copy );

private:
  CppUnit::Test *_test;
};



// Inlines methods for TestListViewItem:
// -------------------------------------



#endif  // TESTLISTVIEWITEM_H
