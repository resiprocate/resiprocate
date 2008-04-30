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

   assert(LameFloat("0.101") == LameFloat("0.101"));
   assert(!(LameFloat("0.101") < LameFloat("0.101")));
   assert(!(LameFloat("0.101") > LameFloat("0.101")));

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

   assert(LameFloat("-9223372.0368547758085").getBase()==-922337203685477581LL);
   assert(LameFloat("-9223372.0368547758085").getNegExp()==11);

   assert(Data::from(LameFloat("1.0"))=="1.0");
   assert(Data::from(LameFloat("-1.0"))=="-1.0");
   assert(Data::from(LameFloat("1"))=="1");
   assert(Data::from(LameFloat("-1"))=="-1");
   assert(Data::from(LameFloat("0"))=="0");
   assert(Data::from(LameFloat("-0"))=="0");
   assert(Data::from(LameFloat("0.000101"))=="0.000101");
   assert(Data::from(LameFloat("0.0000"))=="0.0000");
   assert(Data::from(LameFloat("-0.0000"))=="0.0000");
   assert(Data::from(LameFloat("0.101"))=="0.101");
   assert(Data::from(LameFloat("231.9082645"))=="231.9082645");
   assert(Data::from(LameFloat("9223372.0368547758085"))=="9223372.03685477581");
   assert(Data::from(LameFloat("-9223372.0368547758085"))=="-9223372.03685477581");

   assert(LameFloat(LameFloat::lf_min)<LameFloat(LameFloat::lf_max));
   assert((-LameFloat(LameFloat::lf_min))>(-LameFloat(LameFloat::lf_max)));

   assert(!(LameFloat(LameFloat::lf_min)>LameFloat(LameFloat::lf_max)));
   assert(!((-LameFloat(LameFloat::lf_min))<(-LameFloat(LameFloat::lf_max))));

   assert((-LameFloat("-1"))==LameFloat("1"));
   assert((-LameFloat(LameFloat::lf_min))<LameFloat(LameFloat::lf_min));
   assert((-LameFloat(LameFloat::lf_max))<LameFloat(LameFloat::lf_max));

#ifndef RESIP_FIXED_POINT
   assert(double(LameFloat("1.0"))==1.0f);
   assert(double(LameFloat("-1.0"))==-1.0f);
   assert(double(LameFloat("1"))==1);
   assert(double(LameFloat("-1"))==-1);
   assert(double(LameFloat("0"))==0);
   assert(double(LameFloat("-0"))==0);
#define checkRange(_quoted, _num) \
   assert(double(LameFloat(_quoted)) > 0.99999f*_num); \
   assert(double(LameFloat(_quoted)) < 1.00001f*_num);
   checkRange("0.000101",0.000101f);
   checkRange("0.101",0.101f);
   checkRange("231.9082645",231.9082645f);
   checkRange("9223372.03685477581",9223372.03685477581f);
#undef checkRange
#endif

}
