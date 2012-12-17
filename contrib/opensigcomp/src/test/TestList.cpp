/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 021110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

#include <iostream>
#include <assert.h>
#include <deque>
#include "TestList.h"

#ifdef USING_COLOR
  #define ANSI_GREEN     "\033[32;1m"
  #define ANSI_BLUE    "\033[34m"
  #define ANSI_RED       "\033[31;1m"
  #define ANSI_DEFAULT     "\033[39;22m"
#else
  #define ANSI_GREEN     ""
  #define ANSI_BLUE      ""
  #define ANSI_RED       ""
  #define ANSI_DEFAULT     ""
#endif

namespace osc
{
  struct TestListKiller
  {
    ~TestListKiller() { delete osc::TestList::instance(); }
  } testListKiller;
}

void osc::print_running(std::string name)
{
  std::cout<<"  - Running test: "<<ANSI_BLUE<<name.c_str()<<ANSI_DEFAULT<<std::endl;
}

bool osc::print_status(std::string name, bool status)
{
  std::cout<<"  - Test : "<<name.c_str()<<" [";
  if(status)
  {
    std::cout<<ANSI_GREEN<<"PASSED"<<ANSI_DEFAULT;

  } 
  else 
  {
    std::cout<<ANSI_RED<<"FAILED"<<ANSI_DEFAULT;
  }
  std::cout<<"]"<<std::endl;
  return status;
}

bool osc::print_subtest(std::string name, bool status)
{
  std::cout<<"    -["<<(status?ANSI_GREEN:ANSI_RED)<<(status?"+":"x")
  <<ANSI_DEFAULT<<"] Subtest: "<<name.c_str()<<std::endl;
  return status;

}
//std::auto_ptr<osc::TestList> osc::TestList::theTestList;
osc::TestList *osc::TestList::theTestList;

osc::TestList *
osc::TestList::instance()
{
  if (theTestList == 0)
  {
    theTestList = new TestList();
  }

  return theTestList;
}

/**
  Constructor for osc::TestList.
 */
osc::TestList::TestList()
{
}

/**
  Copy constructor for osc::TestList.
 */
osc::TestList::TestList(TestList const &r)
  : tests(r.tests)
{
}

/**
  Destructor for osc::TestList.
 */
osc::TestList::~TestList()
{
  theTestList = 0;
}

/**
  Assignment operator for osc::TestList.
 */
osc::TestList &
osc::TestList::operator=(TestList const &r)
{
  if (&r == this)
  {
    return *this;
  }

  /* Assign attributes */
  tests = r.tests;
  return *this;
}

bool
osc::TestList::addTest(test_signature_t testFunction, const std::string &name)
{
  tests.push_back(std::make_pair<test_signature_t,std::string>
                                (testFunction, name));
  return true;
}

/**
  Run all the tests in this list until one fails.

  @retval true   All tests succeeded
  @retval false  A test failed, and subsequent tests were not run.
 */
bool
osc::TestList::runTests(std::list<std::string> matchList)
{
  bool success;
  bool allPassed = true;
  bool runTest;
  int numPassed = 0;
  int numTotal = 0;
  int numSkipped = 0;
  test_list_t::iterator i;
  std::list<std::string>::iterator j;
  std::deque<std::string> failed;
  std::deque<std::string> passed;
  for(i = tests.begin(); i != tests.end(); i++)
  {
    if (matchList.empty())
    {
      runTest = true;
    }
    else
    {
      runTest = false;
      for (j = matchList.begin(); j != matchList.end(); j++)
      {
        if (i->second.find(*j) != std::string::npos)
        {
          runTest = true;
        }
      }
    }
    if (runTest)
    {
      osc::print_running(i->second);
      success = (i->first)();
      #ifdef USING_COLOR
      osc::print_status(i->second,success);
      #endif
      if (success)
      {
        passed.push_back(i->second);
        numPassed++;
      }
      else
      {
        #ifndef USING_COLOR
        std::cout << "!!! Failure: " << (i->second).c_str() << std::endl;
        #endif
        failed.push_back(i->second);
        allPassed = false;
      }
      numTotal++;
    }
    else
    {
      numSkipped++;
    }
  }
  std::cout << std::endl << ">> Unit testing: Passed " << numPassed 
            << " tests of " << numTotal 
            << " total";
  if (numSkipped)
  {
    std::cout << " (" << numSkipped << " skipped)";
  }
  #ifdef USING_COLOR
  
  if(numPassed > 0)
  {
    std::cout<<std::endl<<"--- Tests Passed ---"<<std::endl;
  }
  while(!passed.empty())
  {
    std::cout<<"["<<ANSI_GREEN<<"PASSED"<<ANSI_DEFAULT<<"]"<<passed.front()<<std::endl;
    passed.pop_front();
  }
  if(numPassed < numTotal)
  {
    std::cout<<std::endl<<"--- Tests Failed ---"<<std::endl;
  }
  while(!failed.empty())
  {
    std::cout<<"["<<ANSI_RED<<"FAILED"<<ANSI_DEFAULT<<"]"<<failed.front()<<std::endl;
    failed.pop_front();
  }
  #endif
  std::cout << std::endl;
  return allPassed;
}
