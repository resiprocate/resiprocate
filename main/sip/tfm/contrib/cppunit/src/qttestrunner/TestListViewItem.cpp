// //////////////////////////////////////////////////////////////////////////
// Implementation file TestListViewItem.cpp for class TestListViewItem
// (c)Copyright 2000, Baptiste Lepilleur.
// Created: 2001/09/19
// //////////////////////////////////////////////////////////////////////////

#include "TestListViewItem.h"


TestListViewItem::TestListViewItem( CppUnit::Test *test,
                                    QListViewItem *parent ) : 
    QListViewItem( parent ),
    _test( test )
{
}


TestListViewItem::~TestListViewItem()
{
}


CppUnit::Test *
TestListViewItem::test() const
{
  return _test;
}
