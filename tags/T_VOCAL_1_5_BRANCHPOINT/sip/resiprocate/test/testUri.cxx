#include <memory>

#include "sip2/util/Logger.hxx"
#include "sip2/util/DataStream.hxx"

#include "sip2/sipstack/Uri.hxx"

#include <iostream>

#include "TestSupport.hxx"
#include "tassert.h"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::APP


int
main(int argc, char* argv[])
{
  Log::Level l = Log::DEBUG;
  Log::initialize(Log::COUT, l, argv[0]);

   tassert_init(12);
   
   {
      tassert_reset();      
      Uri uri( "sip:fluffy@iii.ca:666" );
      tassert( uri.scheme() == "sip" );
      tassert( uri.user() == "fluffy" );
      tassert( uri.host() == "iii.ca" );
      tassert( uri.port() == 666 );
      
      tassert_verify(1);
   }
   
   {
      tassert_reset();
      Uri uri( "sip:fluffy@iii.ca;transport=tcp" );
      tassert( uri.param(p_transport) == "tcp"  );
      
      tassert_verify(2);
   }
   
   {
      tassert_reset();
      Uri uri( "sips:fluffy@iii.ca;transport=tls" );
      tassert( uri.scheme() == "sips" );
      tassert( uri.param(p_transport) == "tls"  );
      tassert_verify(3);
   }
   
   {
      tassert_reset();
      Uri uri( "sip:fluffy@iii.ca;transport=sctp" );
      tassert( uri.param(p_transport) == "sctp"  );
      tassert_verify(4);
   }
   
   {
      tassert_reset();
      Uri uri( "sip:fluffy:password@iii.ca" );
      tassert( uri.password() == "password" );
      tassert_verify(5);
   }

  {
      tassert_reset();
      Uri uri( "sip:fluffy@iii.ca;user=phone;ttl=5;lr;maddr=1.2.3.4" );
      tassert( uri.param(p_ttl) == 5 );
      tassert( uri.exists(p_lr) == true );
      tassert( uri.param(p_maddr) == "1.2.3.4" );
      tassert( uri.param(p_user) == "phone" );

      tassert_verify(6);
   }
 
   {
      tassert_reset();
      Uri uri( "sip:fluffy@iii.ca;x-fluffy=foo" );
      tassert( uri.exists("x-fluffy") == true );
      tassert( uri.exists("x-fufu") == false );
      tassert( uri.param("x-fluffy") == "foo" );
      tassert_verify(7);
   }
 
   {
      tassert_reset();
      Uri uri( "sip:fluffy@iii.ca;method=MESSAGE" );
      tassert( uri.param(p_method) == "MESSAGE" );
      tassert_verify(8);
   }

#if 0
   {
      tassert_reset();
      try
      {
         Uri uri( "sip:fluffy@iii.ca?Subject=foo&Call-Info=<http://www.foo.com>" );
         tassert( false /* need test here */  );
      }
      catch(std::exception e)
      { 
         tassert( false /* got an exception*/  );
      }
      
      tassert_verify(9);
   }
#endif
              
  {
      tassert_reset();
      Uri uri( "sip:+1(408) 444-1212:666@gw1" );
      tassert( uri.user() == "+1(408) 444-1212"  );
      tassert( uri.password() == "666"  );
      tassert( uri.host() == "gw1"  );
      tassert_verify(10);
   }
 
  {
      tassert_reset();
      Uri uri( "sip:fluffy;x-utag=foo@iii.ca" );
      tassert( uri.user() == "fluffy;x-utag=foo"  );
      tassert( uri.host() == "iii.ca"  );
      tassert_verify(11);
   }

#if 0 
  {
      tassert_reset();
      Uri uri( "tel:+1 (408) 555-1212" );
      tassert( uri.scheme() == "tel"  );
      tassert( 0  );
      tassert_verify(12);
   }
#endif
                
  tassert_report();
}
