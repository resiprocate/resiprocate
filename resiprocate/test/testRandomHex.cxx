
#include <iostream>
#include <set>

#include <util/Data.hxx>
#include <util/RandomHex.hxx>

using namespace std;
using namespace Vocal2;

int main(int argc, char** argv)
{
   if(argc != 3)
   {
      cerr << "usage: testRandomHex number_of_tries string_length" << endl;
      exit(-1);
   }

   int runs = atoi(argv[1]);
   int length = atoi(argv[2]);

   if (runs <= 0 || length <= 0)
   {
      cerr << "usage: testRandomHex number_of_tries string_length" << endl
           << "number_of_tries and string_length must be a positive integers." << endl;
      exit(-1);
   }

   cerr << "Generating " << runs << " random " << length << " byte strings and checking for uniqueness." << endl;

   set<Data> randomDatas;
   
   for (int i = 0; i < runs; i++)
   {
      Data foo = RandomHex::get(length);
//      cerr << foo << endl;
      if (randomDatas.insert(foo).second == false)
      {
         cerr << "RandomHex produced a duplicate" << length << "byte string after " << i << " runs. " << endl;
         exit(-1);
      }
   }
   cerr << "Success." << endl;
}


