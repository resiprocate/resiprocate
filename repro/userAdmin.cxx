
#include <cassert>
#include <iostream>

#include "repro/UserDb.hxx"
#include "resiprocate/os/Data.hxx"

using namespace resip;
using namespace repro;
using namespace std;


void 
add( char* pUsername, char* pRealm, char* pPasswd )
{
   UserDb db;  

   Data fullName = Data::Empty;
   Data email = Data::Empty;
   
   db.addUser( Data(pUsername), 
               Data(pRealm), 
               Data(pPasswd),
               fullName, 
               email );
}


void
remove( char* pAor )
{ 
   UserDb db;
   
   db.removeUser( Data(pAor) );
}


void
list()
{
   UserDb db; 
   
   Data key = db.getFirstKey();
   while ( !key.empty() )
   {
      clog << "Key: " << key << endl;
      clog << "  passwordHash = " << db.getUserAuthInfo(key) << endl;
      
      key = db.getNextKey();
   } 
}


void 
usage()
{
   clog << "Command line options are" << endl
        << "  -list" << endl
        << "  -add user realm password" << endl
        << "  -remove aor" << endl;
}


int 
main(int argc, char* argv[])
{
   for ( int i=1; i<argc; i++ )
   {
      if ( !strcmp(argv[i],"-list" ) )
      {
         list();
      }
      else if (!strcmp(argv[i],"-add"))
      {
         i++;
         assert( i<argc );
         char* username = argv[i];

         i++;
         assert( i<argc );
         char* realm = argv[i];

         i++;
         assert( i<argc );
         char* passwd = argv[i];

         add( username, realm, passwd );
      }  
      else if (!strcmp(argv[i],"-remove"))
      {
         i++;
         assert( i<argc );
         char* aor = argv[i];
         remove( aor );
      }  
      else
      {
         usage();
         exit(1);
      }
   }

   return 0;
}
