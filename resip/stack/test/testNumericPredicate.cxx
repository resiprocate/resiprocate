#include <memory>
#include <iostream>

#include "TestSupport.hxx"

#include "rutil/Logger.hxx"
#include "resip/stack/NumericFeatureParameter.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;

int
main(int argc, char* argv[])
{
   Log::Level l = Log::Debug;
   Log::initialize(Log::Cerr, l, argv[0]);

   {
      // [-1, 1]
      NumericPredicate superset(LameFloat("-1"),LameFloat("1"),false);
      assert(superset.matches(superset));
      // [-0.5, 0.5]
      assert(superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),false)));
      // [0, 0.5]
      assert(superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),false)));
      // [-0.5, 0]
      assert(superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),false)));
      // [-0.5, -0.5]
      assert(superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),false)));
      // [-1, 1]
      assert(superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("1"),false)));
      // [0, 1]
      assert(superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1"),false)));
      // [-1, 0]
      assert(superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("0"),false)));

      // [-0.5, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),false)));
      // [0, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),false)));
      // [-1.5, 0]
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),false)));
      // [-1.5, -1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),false)));
      // [1.5, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),false)));

      // [-0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,false)));
      // (-inf, 0.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),false)));
      // (-inf, -1.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),false)));
      // [1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,false)));



      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),true)));

      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),true)));

      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,true)));
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),true)));
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,true)));
   }

   {
      // (-inf, -1) + (1, inf)
      NumericPredicate superset(LameFloat("-1"),LameFloat("1"),true);
      assert(superset.matches(superset));
      // [-0.5, 0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),false)));
      // [0, 0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),false)));
      // [-0.5, 0]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),false)));
      // [-0.5, -0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),false)));
      // [-1, 1]
      assert(!superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("1"),false)));
      // [0, 1]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1"),false)));
      // [-1, 0]
      assert(!superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("0"),false)));
      // [1, 2]
      assert(!superset.matches(NumericPredicate(LameFloat("1"),LameFloat("2"),false)));
      // [-2, -1]
      assert(!superset.matches(NumericPredicate(LameFloat("-2"),LameFloat("-1"),false)));
      // [1.001, 2]
      assert(superset.matches(NumericPredicate(LameFloat("1.001"),LameFloat("2"),false)));
      // [-2, -1.001]
      assert(superset.matches(NumericPredicate(LameFloat("-2"),LameFloat("-1.001"),false)));

      // [-0.5, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),false)));
      // [0, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),false)));
      // [-1.5, 0]
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),false)));
      // [-1.5, -1.5]
      assert(superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),false)));
      // [1.5, 1.5]
      assert(superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),false)));

      // [-0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,false)));
      // (-inf, 0.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),false)));
      // (-inf, -1.5]
      assert(superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),false)));
      // [1.5, inf)
      assert(superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,false)));
      // (-inf, 1.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("1.5"),false)));
      // [-1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat::lf_max,false)));



      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),true)));

      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),true)));
      assert(superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("1.5"),true)));

      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,true)));
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),true)));
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),true)));
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,true)));
   }

   {
      // (-inf, 0]
      NumericPredicate superset(-LameFloat::lf_max,LameFloat("0"),false);
      assert(superset.matches(superset));
      // [-0.5, 0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),false)));
      // [0, 0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),false)));
      // [-0.5, 0]
      assert(superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),false)));
      // [-0.5, -0.5]
      assert(superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),false)));
      // [-1, 1]
      assert(!superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("1"),false)));
      // [0, 1]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1"),false)));
      // [-1, 0]
      assert(superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("0"),false)));
      // [1, 2]
      assert(!superset.matches(NumericPredicate(LameFloat("1"),LameFloat("2"),false)));
      // [-2, -1]
      assert(superset.matches(NumericPredicate(LameFloat("-2"),LameFloat("-1"),false)));
      // [1.001, 2]
      assert(!superset.matches(NumericPredicate(LameFloat("1.001"),LameFloat("2"),false)));
      // [-2, -1.001]
      assert(superset.matches(NumericPredicate(LameFloat("-2"),LameFloat("-1.001"),false)));

      // [-0.5, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),false)));
      // [0, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),false)));
      // [-1.5, 0]
      assert(superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),false)));
      // [-1.5, -1.5]
      assert(superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),false)));
      // [1.5, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),false)));

      // [-0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,false)));
      // (-inf, 0.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),false)));
      // (-inf, -1.5]
      assert(superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),false)));
      // [1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,false)));
      // (-inf, 1.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("1.5"),false)));
      // [-1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat::lf_max,false)));



      // (-inf, -0.5) + (0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),true)));
      // (-inf, 0) + (0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),true)));
      // (-inf, -0.5) + (0, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),true)));
      // (-inf, -0.5) + (-0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),true)));
      // (-inf, 0.5) + (0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("0.5"),LameFloat("0.5"),true)));

      // (-inf, -0.5) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),true)));
      // (-inf, 0) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),true)));
      // (-inf, -1.5) + (0, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),true)));
      // (-inf, -1.5) + (-1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),true)));
      // (-inf, 1.5) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),true)));
      // (-inf, -1.5) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("1.5"),true)));

      // (-inf, -0.5)
      assert(superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,true)));
      // (0.5, inf)
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),true)));
      // (-1.5, inf)
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),true)));
      // (-inf, 1.5)
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,true)));
   }

   {
      // [1,1]
      NumericPredicate superset(LameFloat("1"),LameFloat("1"),false);
      assert(superset.matches(superset));
      // [-0.5, 0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),false)));
      // [0, 0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),false)));
      // [-0.5, 0]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),false)));
      // [-0.5, -0.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),false)));
      // [-1, 1]
      assert(!superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("1"),false)));
      // [0, 1]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1"),false)));
      // [-1, 0]
      assert(!superset.matches(NumericPredicate(LameFloat("-1"),LameFloat("0"),false)));
      // [1, 2]
      assert(!superset.matches(NumericPredicate(LameFloat("1"),LameFloat("2"),false)));
      // [-2, -1]
      assert(!superset.matches(NumericPredicate(LameFloat("-2"),LameFloat("-1"),false)));
      // [1.001, 2]
      assert(!superset.matches(NumericPredicate(LameFloat("1.001"),LameFloat("2"),false)));
      // [-2, -1.001]
      assert(!superset.matches(NumericPredicate(LameFloat("-2"),LameFloat("-1.001"),false)));

      // [-0.5, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),false)));
      // [0, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),false)));
      // [-1.5, 0]
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),false)));
      // [-1.5, -1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),false)));
      // [1.5, 1.5]
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),false)));

      // [-0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,false)));
      // (-inf, 0.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),false)));
      // (-inf, -1.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),false)));
      // [1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,false)));
      // (-inf, 1.5]
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("1.5"),false)));
      // [-1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat::lf_max,false)));



      // (-inf, -0.5) + (0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0.5"),true)));
      // (-inf, 0) + (0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("0.5"),true)));
      // (-inf, -0.5) + (0, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("0"),true)));
      // (-inf, -0.5) + (-0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("-0.5"),true)));
      // (-inf, 0.5) + (0.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("0.5"),LameFloat("0.5"),true)));

      // (-inf, -0.5) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat("1.5"),true)));
      // (-inf, 0) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("0"),LameFloat("1.5"),true)));
      // (-inf, -1.5) + (0, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("0"),true)));
      // (-inf, -1.5) + (-1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("-1.5"),true)));
      // (-inf, 1.5) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat("1.5"),true)));
      // (-inf, -1.5) + (1.5, inf)
      assert(!superset.matches(NumericPredicate(LameFloat("-1.5"),LameFloat("1.5"),true)));

      // (-inf, -0.5)
      assert(!superset.matches(NumericPredicate(LameFloat("-0.5"),LameFloat::lf_max,true)));
      // (0.5, inf)
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("0.5"),true)));
      // (-1.5, inf)
      assert(!superset.matches(NumericPredicate(-LameFloat::lf_max,LameFloat("-1.5"),true)));
      // (-inf, 1.5)
      assert(!superset.matches(NumericPredicate(LameFloat("1.5"),LameFloat::lf_max,true)));
   }

   return 0;
}
