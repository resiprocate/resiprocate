#include "resiprocate/MessageWaitingContents.hxx"
#include "resiprocate/HeaderFieldValue.hxx"
#include "resiprocate/os/FileSystem.hxx"
#include <iostream>

using namespace resip;
using namespace std;

int
main()
{
   FileSystem::Directory dir("/usr/bin");
   for (FileSystem::Directory::iterator it = dir.begin(); it != dir.end(); ++it)
   {
      cerr << *it << endl;
   }
}



 
