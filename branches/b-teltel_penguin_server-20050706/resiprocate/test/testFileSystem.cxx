#include "resiprocate/MessageWaitingContents.hxx"
#include "resiprocate/HeaderFieldValue.hxx"
#include "resiprocate/os/FileSystem.hxx"
#include <iostream>

using namespace resip;
using namespace std;

int
main()
{
#ifdef WIN32
   FileSystem::Directory dir("c:\\windows\\");
#else
   FileSystem::Directory dir("/usr/bin/");
#endif
   for (FileSystem::Directory::iterator it = dir.begin(); it != dir.end(); ++it)
   {
      cerr << *it << endl;
   }
   return 0;
}



 
