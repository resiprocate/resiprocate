#include "rutil/ParseBuffer.hxx"
#include <string.h>
#include <assert.h>

#include "rutil/LameFloat.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, argc > 1 ? Log::toLevel(argv[1]) :  Log::Info, argv[0]);

   assert(LameFloat("1.0")==LameFloat("1"));
   assert(LameFloat("-1.0")==LameFloat("-1"));
   assert(LameFloat("1")==LameFloat("1.0"));
   assert(LameFloat("-1")==LameFloat("-1.0"));
   assert(!(LameFloat("1.0")<LameFloat("1")));
   assert(!(LameFloat("-1.0")<LameFloat("-1")));
   assert(!(LameFloat("1")<LameFloat("1.0")));
   assert(!(LameFloat("-1")<LameFloat("-1.0")));
   assert(!(LameFloat("1.0")>LameFloat("1")));
   assert(!(LameFloat("-1.0")>LameFloat("-1")));
   assert(!(LameFloat("1")>LameFloat("1.0")));
   assert(!(LameFloat("-1")>LameFloat("-1.0")));

   assert(!(LameFloat("1.0")==LameFloat("-1")));
   assert(!(LameFloat("-1.0")==LameFloat("1")));
   assert(!(LameFloat("1")==LameFloat("-1.0")));
   assert(!(LameFloat("-1")==LameFloat("1.0")));
   assert(!(LameFloat("1.0")<LameFloat("-1")));
   assert(LameFloat("-1.0")<LameFloat("1"));
   assert(!(LameFloat("1")<LameFloat("-1.0")));
   assert(LameFloat("-1")<LameFloat("1.0"));
   assert(LameFloat("1.0")>LameFloat("-1"));
   assert(!(LameFloat("-1.0")>LameFloat("1")));
   assert(LameFloat("1")>LameFloat("-1.0"));
   assert(!(LameFloat("-1")>LameFloat("1.0")));

   assert(LameFloat("0") == LameFloat("0.0000"));
   assert(LameFloat("-0") == LameFloat("-0.0000"));
   assert(LameFloat("0") == LameFloat("-0.0000"));
   assert(LameFloat("-0") == LameFloat("0.0000"));
   assert(!(LameFloat("0") < LameFloat("0.0000")));
   assert(!(LameFloat("-0") < LameFloat("-0.0000")));
   assert(!(LameFloat("0") < LameFloat("-0.0000")));
   assert(!(LameFloat("-0") < LameFloat("0.0000")));
   assert(!(LameFloat("0") > LameFloat("0.0000")));
   assert(!(LameFloat("-0") > LameFloat("-0.0000")));
   assert(!(LameFloat("0") > LameFloat("-0.0000")));
   assert(!(LameFloat("-0") > LameFloat("0.0000")));

   assert(LameFloat("0.000101") == LameFloat("0.000101"));
   assert(!(LameFloat("0.000101") < LameFloat("0.000101")));
   assert(!(LameFloat("0.000101") > LameFloat("0.000101")));

   assert(LameFloat("231.9082645")==LameFloat("231.9082645"));
   assert(!(LameFloat("231.9082645")<LameFloat("231.9082645")));
   assert(!(LameFloat("231.9082645")>LameFloat("231.9082645")));

   assert(!(LameFloat("0.000101") == LameFloat("231.9082645")));
   assert(!(LameFloat("231.9082645") == LameFloat("0.000101")));
   assert(LameFloat("0.000101") < LameFloat("231.9082645"));
   assert(!(LameFloat("0.000101") > LameFloat("231.9082645")));
   assert(!(LameFloat("231.9082645") < LameFloat("0.000101")));
   assert(LameFloat("231.9082645") > LameFloat("0.000101"));

   // Test pushing the limits of a 64-bit signed int
   assert(LameFloat("9223372.0368547758085")==LameFloat("9223372.03685477581"));
   assert(LameFloat("-9223372.0368547758085")==LameFloat("-9223372.03685477581"));

   assert(LameFloat("9223372.0368547758085").getBase()==922337203685477581LL);
   assert(LameFloat("9223372.0368547758085").getNegExp()==11);
}
