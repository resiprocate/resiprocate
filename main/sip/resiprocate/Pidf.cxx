#include "sip2/sipstack/Pidf.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/util/Logger.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<Pidf> Pidf::Factory;


Pidf::Pidf()
   : Contents(getStaticType())
{}

#if 0
Pidf::Pidf(const Data& txt)
   : Contents(getStaticType())
{}
#endif


Pidf::Pidf(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType)
{
}
 

#if 0
Pidf::Pidf(const Data& txt, const Mime& contentsType)
   : Contents(contentsType)
{
}
#endif


Pidf::Pidf(const Pidf& rhs)
   : Contents(rhs),
     mEntity(rhs.mEntity),
     mNote(rhs.mNote)
{
   for( unsigned int i=0; i < rhs.mTupple.size(); i++)
   {
      Tupple t = rhs.mTupple[i];
      mTupple.push_back( t );
   }
   assert(  mTupple.size() ==  rhs.mTupple.size() );
}


Pidf::~Pidf()
{
}


Pidf&
Pidf::operator=(const Pidf& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      
      mNote = rhs.mNote;
      mEntity = rhs.mEntity;
      mTupple.clear();
      for( unsigned int i=0; i < rhs.mTupple.size(); i++)
      {
         Tupple t = rhs.mTupple[i];
         mTupple.push_back( t );
      }
   }
   return *this;
}


Contents* 
Pidf::clone() const
{
   return new Pidf(*this);
}


const Mime& 
Pidf::getStaticType() const
{
   static Mime type("application","cpim-pidf+xml");
   return type;
}


std::ostream& 
Pidf::encodeParsed(std::ostream& str) const
{
   encodeHeaders(str);

   //DebugLog(<< "Pidf::encodeParsed " << mText);
   //str << mText;

   str       << "<?xml version\"1.0\" encoding=\"UTF-8\"?>" << Symbols::CRLF;;
   str       << "<presence xmlns=\"urn:ietf:params:xml:ns:cpim-pidf\"" << Symbols::CRLF;;
   str       << "           entity=\""<<mEntity<<"\">" << Symbols::CRLF;;
   for( unsigned int i=0; i<mTupple.size(); i++)
   {
      Data status( (char*)( (mTupple[i].status)?"open":"close" ) );
      str    << "  <tuple id=\""<<mTupple[i].id<<"\">" << Symbols::CRLF;;
      str    << "     <status><basic>"<<status<<"</basic></status>" << Symbols::CRLF;;
      if ( !mTupple[i].contact.empty() )
      {
         str << "     <contact priority=\""<<mTupple[i].contactPriority<<"\">"<<mTupple[i].contact<<"</contact>" << Symbols::CRLF;;
      }
      if ( !mTupple[i].timeStamp.empty() )
      {
         str << "     <timestamp>"<<mTupple[i].timeStamp<<"</timestamp>" << Symbols::CRLF;;
      }
      if ( !mTupple[i].note.empty() )
      {
         str << "     <note>"<<mTupple[i].note<<"</note>" << Symbols::CRLF;;
      }
      str    << "  </tuple>" << Symbols::CRLF;;
   }
   str       << "</presence>" << Symbols::CRLF;;
   
   return str;
}


void 
Pidf::parse(ParseBuffer& pb)
{
   parseHeaders(pb);

   const char* anchor = pb.position();

   Tupple t;
   
   mTupple.push_back( t );
   mTupple[0].status = true;
   mTupple[0].note = Data::Empty;

   pb.reset(anchor);
   const char* close = pb.skipToChars("close");
   if ( close != pb.end() )
   {
      DebugLog ( << "found an close" );
      mTupple[0].status = false;
   }

   pb.reset(anchor);
   const char* open = pb.skipToChars("open");
   if ( open != pb.end() )
   {
      DebugLog ( << "found an open" );
      mTupple[0].status = true;
   }

   pb.reset(anchor);
   pb.skipToChars("<note");
   const char* startNote = pb.skipToChars(">");
   startNote++;
   if ( startNote < pb.end() )
   {
      const char* endNote = pb.skipToChars("</note>");
      Data blueNote( startNote, endNote-startNote );
      DebugLog ( << "found a note of" << blueNote );
      mTupple[0].note = blueNote;
   }
}


void 
Pidf::setSimpleId( const Data& id )
{
   if ( mTupple.size() == 0 )
   {
      Tupple t;
      mTupple.push_back( t );
   }
   assert( mTupple.size() > 0 );

   mTupple[0].id = id;
}


void 
Pidf::setSimpleStatus( bool online, const Data& note, const Data& contact )
{
   if ( mTupple.size() == 0 )
   {
      Tupple t;
      mTupple.push_back( t );
   }
   assert( mTupple.size() > 0 );

   mTupple[0].status = online;
   mTupple[0].contact = contact;
   mTupple[0].contactPriority = 1.0;
   mTupple[0].note = note;
   mTupple[0].timeStamp = Data::Empty;
}


bool 
Pidf::getSimpleStatus( Data* note )
{
   checkParsed();

   assert( mTupple.size() > 0 );

   if ( note )
   {
      *note = mTupple[0].note;
   }
   
   return mTupple[0].status;
}

   
