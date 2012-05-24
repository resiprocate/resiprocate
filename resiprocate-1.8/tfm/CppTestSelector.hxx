#pragma once

#ifndef CPP_TEST_SELECTOR_H 
#define CPP_TEST_SELECTOR_H 

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestRunner.h>
#include <cppunit/Test.h>


class UISelector
{
   public:
      virtual void GetUserSelection(std::vector<std::string>& names, 
                                    std::vector<int> &result,
                                    int &numTimes) = 0;
};

class CommandLineSelector : public UISelector
{
   public:
      CommandLineSelector(int argc=0, char** argv=NULL);
      void GetUserSelection(std::vector<std::string>& names, std::vector<int> &result,int &numTimes);

   private:
      void DisplayTestCases(std::vector<std::string>& names);
      void DisplayTestCases2(std::vector<std::string>& names);
      void ReadLineInput(std::vector<int> &result,int maxSelection, int &numTimes);

      std::vector<int> mArgResult;
      int mArgNumTimes;

      std::string mPattern;
};

class CppTestSelector
{
   public:
      CppTestSelector(void);
      virtual ~CppTestSelector(void);
      static int SelectTests(CppUnit::Test *suite, 
                             CppUnit::TestRunner &testRunner,
                             UISelector &uiSelector, 
                             int &numTimes );

   protected:
      static void GetTestCases(CppUnit::Test *test, std::vector<std::string>& names);
      
};


#endif // CPP_TEST_SELECTOR_H
