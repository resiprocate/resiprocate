#ifndef _TEST_LIST
#define _TEST_LIST 1

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

#include <memory>
#include <vector>
#include <list>

#define TEST_ASSERT(x) if (!(x)) { \
      std::cout << __FILE__ << ':' << __LINE__ \
        << ": Assertion failed: " << #x \
        << std::endl; return false;}

#define TEST_ASSERT_EQUAL(x,y) if (!((x) == (y))) { \
      std::cout << __FILE__ << ':' << __LINE__ \
        << ": Assertion failed: " << #x " != " #y \
        << " (" << (unsigned long int)(x) << " != " \
        << (unsigned long int)(y) << ")" \
        << std::endl; return false;}

#define TEST_ASSERT_EQUAL_BUFFERS(x,y,size)\
      { \
        size_t _i;\
        for (_i = 0; _i < (size_t)(size); _i++)\
        { \
          if (!(((x)[_i]) == ((y)[_i])))\
          { \
            std::cout << __FILE__ << ':' << __LINE__ \
              << ": Assertion failed: (" \
              << #x << ")[" << _i << ']' \
              << " != (" \
              << #y << ")[" << _i << ']' \
              << " (" << (unsigned long int)((x)[_i]) << " != " \
              << (unsigned long int)((y)[_i]) << ")" \
              << std::endl; return false;\
          }\
        }\
      }

namespace osc
{
  /**
    @todo Fill in description of TestList
  */
  void print_running(std::string name);
  bool print_status(std::string name, bool status);
  bool print_subtest(std::string name, bool status);
  class TestList
  {
    public:
      static TestList *instance();

      typedef bool (*test_signature_t)();
      ~TestList();
 
      TestList * operator &(){ return this; }
      TestList const * operator &() const { return this; }

      bool addTest(test_signature_t, const std::string &);
      bool runTests(std::list<std::string> match = std::list<std::string>());

    protected:
      TestList();
      TestList(TestList const &);
      TestList& operator=(TestList const &);

    private:
      /// Container for the TestList singleton
      static TestList *theTestList;

      typedef std::vector<std::pair<test_signature_t,std::string> > 
        test_list_t;

      test_list_t tests;
  };
}

#endif
