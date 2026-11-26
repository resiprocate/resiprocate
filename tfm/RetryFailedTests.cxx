#include "tfm/RetryFailedTests.hxx"

#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestListener.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TextOutputter.h>

#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

bool
retryFailedTests(CppUnit::Test& suite,
                 const CppUnit::TestResultCollector& firstRun,
                 CppUnit::TestListener* progress)
{
   // A test can record more than one failure; retry it once.
   std::vector<std::string> names;
   std::set<std::string> seen;
   const CppUnit::TestResultCollector::TestFailures& failures = firstRun.failures();
   for (CppUnit::TestResultCollector::TestFailures::const_iterator i = failures.begin();
        i != failures.end(); ++i)
   {
      const std::string name = (*i)->failedTestName();
      if (seen.insert(name).second)
      {
         names.push_back(name);
      }
   }
   if (names.empty())
   {
      return firstRun.wasSuccessful();
   }

   std::cerr << std::endl << "Retrying " << names.size()
             << " failed test(s) once, in case they failed on timing:" << std::endl;

   CppUnit::TestResult controller;
   CppUnit::TestResultCollector retryRun;
   controller.addListener(&retryRun);
   if (progress)
   {
      controller.addListener(progress);
   }

   for (std::vector<std::string>::const_iterator name = names.begin(); name != names.end(); ++name)
   {
      CppUnit::Test* test = 0;
      try
      {
         test = suite.findTest(*name);
      }
      catch (std::invalid_argument&)
      {
      }
      if (!test)
      {
         std::cerr << "Cannot find " << *name << " to retry it." << std::endl;
         return false;
      }
      controller.runTest(test);
   }

   std::cerr << std::endl << "Retry results:" << std::endl;
   CppUnit::TextOutputter outputter(&retryRun, std::cerr);
   outputter.write();
   return retryRun.wasSuccessful();
}

/* ====================================================================

 Copyright (c) 2026, SIP Spectrum, Inc. http://www.sipspectrum.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of SIP Spectrum nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
