#if !defined(REPRO_ABSTRACTROUTESTORE_HXX)
#define REPRO_ABSTRACTROUTESTORE_HXX

#ifdef WIN32
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include <set>

#include "rutil/Data.hxx"
#include "resip/stack/Uri.hxx"



namespace repro
{

class AbstractRouteStore
{
   public:
      typedef std::vector<resip::Uri> UriList;
      typedef resip::Data Key;
      
      virtual ~AbstractRouteStore(){};
      
      virtual void addRoute(const resip::Data& method,
                    const resip::Data& event,
                    const resip::Data& matchingPattern,
                    const resip::Data& rewriteExpression,
                    const int order ) = 0;
      
      virtual void eraseRoute(const resip::Data& method,
                      const resip::Data& event,
                      const resip::Data& matchingPattern) = 0;
      virtual void eraseRoute( const resip::Data& key ) = 0;

      virtual void updateRoute( const resip::Data& originalKey,
                        const resip::Data& method,
                        const resip::Data& event,
                        const resip::Data& matchingPattern,
                        const resip::Data& rewriteExpression,
                        const int order ) = 0;
      
      virtual resip::Data getRouteMethod( const resip::Data& key ) = 0;
      virtual resip::Data getRouteEvent( const resip::Data& key ) = 0;
      virtual resip::Data getRoutePattern( const resip::Data& key ) = 0;
      virtual resip::Data getRouteRewrite( const resip::Data& key ) = 0;
      virtual int         getRouteOrder( const resip::Data& key ) = 0;
      
      virtual Key getFirstKey() = 0;// return empty if no more
      virtual Key getNextKey(Key& key) = 0; // return empty if no more 
      
      virtual UriList process(const resip::Uri& ruri, 
                      const resip::Data& method, 
                      const resip::Data& event ) = 0;

};

 }
#endif  

