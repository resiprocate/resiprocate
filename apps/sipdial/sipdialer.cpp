
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"

#include "DialerConfiguration.hxx"
#include "DialInstance.hxx"

using namespace resip;
using namespace std;

string getFullFilename()
{
#ifdef WIN32
   char *home_drive = getenv("HOMEDRIVE");
   assert(home_drive); // FIXME
   char *home_path = getenv("HOMEPATH");
   assert(home_path); // FIXME
   string full_filename(string(home_drive) + string(home_dir) + string("\sipdial\sipdial.cfg"));
   return full_filename;
#else   
   char *home_dir = getenv("HOME");
   assert(home_dir); // FIXME
   string full_filename(string(home_dir) + string("/.sipdial/sipdial.cfg"));
   return full_filename;
#endif
}

int main(int argc, char *argv[]) 
{
   if(argc != 2)
      // FIXME
      assert(0);
   
   DialerConfiguration *dc = new DialerConfiguration();
   ifstream in(getFullFilename().c_str());
   if(!in.is_open())
      assert(0); // FIXME

   dc->loadStream(in);
   in.close();
   
   DialInstance di(*dc, Uri(Data(argv[1])));
   di.execute();

}

