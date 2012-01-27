#include "rutil/CompactStringArray.hxx"


namespace resip
{
int runTests()
{
   {
      CompactStringArray<2> array;
      assert(array.getString(0)=="");
      assert(array.getString(1)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="");

      array.setString(1, "cioeuwygfc", 10);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cioeuwygfc");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cioeuwygfc");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");

      array.setString(1, "cbg", 3);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cbg");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cbg");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");

      array.setString(1, "cnqioeruyvbpq;eircjnv", 21);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");

      array.setString(0,"",0);
      array.setString(1,"",0);

      assert(array.getString(0)=="");
      assert(array.getString(1)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="");

      array.setString(1, "cioeuwygfc", 10);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cioeuwygfc");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cioeuwygfc");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");

      array.setString(1, "cbg", 3);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cbg");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cbg");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");

      array.setString(1, "cnqioeruyvbpq;eircjnv", 21);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");

      array.setString(1,"",0);
      array.setString(0,"",0);

      assert(array.getString(0)=="");
      assert(array.getString(1)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="");

      array.setString(1, "cioeuwygfc", 10);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cioeuwygfc");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cioeuwygfc");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");

      array.setString(1, "cbg", 3);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cbg");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cbg");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");

      array.setString(1, "cnqioeruyvbpq;eircjnv", 21);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
   }

   {
      CompactStringArray<3> array;
      assert(array.getString(0)=="");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");

      array.setString(1, "cioeuwygfc", 10);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");

      array.setString(1, "cbg", 3);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");

      array.setString(1, "cnqioeruyvbpq;eircjnv", 21);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");

      array.setString(0,"",0);
      array.setString(1,"",0);

      assert(array.getString(0)=="");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");

      array.setString(1, "cioeuwygfc", 10);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");

      array.setString(1, "cbg", 3);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");

      array.setString(1, "cnqioeruyvbpq;eircjnv", 21);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");

      array.setString(1,"",0);
      array.setString(0,"",0);

      assert(array.getString(0)=="");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="");
      assert(array.getString(2)=="");

      array.setString(1, "cioeuwygfc", 10);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cioeuwygfc");
      assert(array.getString(2)=="");

      array.setString(1, "cbg", 3);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cbg");
      assert(array.getString(2)=="");

      array.setString(1, "cnqioeruyvbpq;eircjnv", 21);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");

      array.setString(0, "foobajooba", 10);
      assert(array.getString(0)=="foobajooba");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");
      // smaller
      array.setString(0, "oibcdnv", 7);
      assert(array.getString(0)=="oibcdnv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");
      // larger
      array.setString(0, "c iabqerfioacv", 14);
      assert(array.getString(0)=="c iabqerfioacv");
      assert(array.getString(1)=="cnqioeruyvbpq;eircjnv");
      assert(array.getString(2)=="");
   }
   return 0;
}
}

int main()
{
   return resip::runTests();
}
