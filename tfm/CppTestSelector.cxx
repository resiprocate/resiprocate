#include "CppTestSelector.hxx"
#include <iostream>
#include <boost/algorithm/string.hpp>


using namespace std;



CommandLineSelector::CommandLineSelector(int argc, char** argv) : mArgNumTimes(0)
{
   // Needs to be reworked for many selections and number of repeats
	for(int i = 1; i < argc; i++)
	{
		std::string userInput = argv[i];

		if(isdigit(*userInput.c_str()))
		{
		   int res = atoi(userInput.c_str());

		   if(res < 0)
			   continue;
   	    
         if(i + 1 < argc)
         {
	         mArgResult.push_back(res);  
         }
         else
         {
            mArgNumTimes = res;
         }
      }
      else
      {
         mPattern = userInput;
      }
	}

   if(mArgNumTimes == 0 && (mArgResult.size() > 0 || mPattern.size() > 0) )
   {
      mArgNumTimes = 1;
   }
}

void CommandLineSelector::DisplayTestCases(std::vector<std::string>& names)
{
   int i;
   int count = (int) names.size();
   
   std::cout << std::endl;
   for(i=0; i< count; i++)
      std::cout << i << "   " << names[i] << std::endl;
   std::cout << i << "   Cancel" << std::endl;
   std::cout << std::endl;
}

void CommandLineSelector::DisplayTestCases2(std::vector<std::string>& names)
{
   int i;
   int off   = 0;
   int off1  = 0;
   int off2  = 0;
   const char *sep = "::";
   const char *indent0 = "  ";
   const char *indent1 = "    ";
   const char *indent2 = "    ...  ";
   const char *indent;

   int count = (int) names.size();
   std::cout << std::endl;
   for(i=0; i< count; i++)
   {
	  off  = 0;
	  indent = indent0;
     
     if((off1 = names[i].find(sep, 0)) >= 0) 
	  {
		  if((off2 = names[i].find(sep, off1+strlen(sep))) >= 0)  //child test
		  {
           off = off2+strlen(sep);
			  indent = indent2;
		  }
		  else
		  {
           off = off1+strlen(sep);
			  indent = indent1;
		  }
	  }
	  std::cout << i << indent << names[i].c_str()+off << std::endl;
   }
   std::cout << i << indent0 << "Cancel" << std::endl;
   std::cout << std::endl;
}



void CommandLineSelector::ReadLineInput(std::vector<int> &result, int maxSelection, int &numTimes)
{
   int res = 0;
   int repeat = 1;
   std::string userInput; 
   std::string repeatStr; 
   //long index = 0;

   numTimes = 1;
   while(true)
   {
      std::cout << std::endl;
      std::cout << "Enter a test number (or pattern) followed by number of repetitions: ";
      std::cin >> userInput;
      std::cin >> repeatStr;
      if(isdigit(*userInput.c_str()))
      {
         res = atoi(userInput.c_str());
         if(res == maxSelection)  
         {
            break;  // user wants to cancel
         }
         if((res < 0) || (res >= maxSelection))
         {
            break;
         }
         result.push_back(res);
      }
      else
      {
         mPattern = userInput;
      }

      // see if user specified number of repetitions
      if((repeat = atoi(repeatStr.c_str())) > 0)
      {
         numTimes = repeat;
         break;
      }
   }
}

void CommandLineSelector::GetUserSelection(std::vector<std::string>& names, std::vector<int> &result, int &numTimes)
{
   if(names.size() == 0)
   {
      std::cout << std::endl << "No tests specified" <<  std::endl;
	   return;
   }
  
   if(mArgNumTimes > 0)
   {
      result = mArgResult;
      numTimes = mArgNumTimes;
   }
   else
   {
      DisplayTestCases2(names);
      ReadLineInput(result, (int)names.size(), numTimes);
   }

   if(mPattern.size() == 0)
   {
      return;
   }

   size_t startFrom = 0;

   if(boost::istarts_with(mPattern, L">="))
   {
      string pattern = mPattern;
      boost::algorithm::ireplace_all(pattern, ">=", "");

      startFrom = atoi(pattern.c_str());
   }
   else if(boost::istarts_with(mPattern, L">"))
   {
      string pattern = mPattern;
      boost::algorithm::ireplace_all(pattern, ">", "");

      startFrom = atoi(pattern.c_str());
      if(startFrom > 0)
      {
         startFrom++;
      }
   }

   if(startFrom > 0 && startFrom < names.size())
   {
      // Match by > or >=
      for(unsigned int i = startFrom; i < names.size(); i++)
      {
         result.push_back(i);
      }
      return;
   }


   bool matchedTopLevel = false;

   for(unsigned int i = 0; i < names.size(); i++)
   {
      // Match by pattern
      string name = names[i];

      bool topLevel = (name.find_first_of("::") == (name.find_last_of("::") - 1));

      if(boost::contains(name, mPattern))
      {
         if(topLevel)
         {
            matchedTopLevel = true;
         }

         if(matchedTopLevel && topLevel)
         {
            result.push_back(i);
         }
         else if(matchedTopLevel == false && topLevel == false)
         {
            result.push_back(i);
         }
      }
      else
      {
         matchedTopLevel = false;
      }
   }

}




CppTestSelector::CppTestSelector(void)
{
}

CppTestSelector::~CppTestSelector(void)
{
    
}

// may throw a memory exception if push fails
void CppTestSelector::GetTestCases(CppUnit::Test *test, std::vector<std::string>& names)
{
  int childCount = 0;
  names.push_back (test->getName());
  
  if ((childCount = test->getChildTestCount()) == 0)
     return;
  for (int i = 0; i < childCount; i++)
    GetTestCases (test->getChildTestAt (i), names);
}


// return number of selected tests or suites
// may throw memory exception 
int CppTestSelector::SelectTests(CppUnit::Test *suite, CppUnit::TestRunner &testRunner, UISelector &uiSelector, int &numTimes)
{
	CppUnitVector<std::string>    testNames ;
	std::vector<int>   selectedTests;
	size_t i = 0;
	numTimes = 1;

   GetTestCases(suite, testNames);
   uiSelector.GetUserSelection(testNames, selectedTests, numTimes);

   CppUnit::Test *testToRun = NULL;
   for(i = 0; i< selectedTests.size(); i++)
   {
      try
	   {
	      testToRun = suite->findTest(testNames[selectedTests[i]]);
	      if(testToRun)
         {
             testRunner.addTest(testToRun);
         }
	   }
	   catch (...)
      {   
		   //std::cout << "test not found:   " << testNames[selectedTests[i]] << std::endl;
		   // test not found, but this should never happen
		  // because the test names were found from the given suite
      }
   }

   return (int) i;
}





