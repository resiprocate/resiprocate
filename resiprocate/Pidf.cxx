#include "resiprocate/sipstack/Pidf.hxx"
#include "resiprocate/sipstack/SipMessage.hxx"
#include "resiprocate/sipstack/Symbols.hxx"
#include "resiprocate/util/Logger.hxx"

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
   for( unsigned int i=0; i < rhs.mTuple.size(); i++)
   {
      Tuple t = rhs.mTuple[i];
      mTuple.push_back( t );
   }
   assert(  mTuple.size() ==  rhs.mTuple.size() );
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
      clear();
      
      mNote = rhs.mNote;
      mEntity = rhs.mEntity;
      for( unsigned int i=0; i < rhs.mTuple.size(); i++)
      {
         Tuple t = rhs.mTuple[i];
         mTuple.push_back( t );
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
Pidf::getStaticType() 
{
   static Mime type("application","cpim-pidf+xml");
   return type;
}


std::ostream& 
Pidf::encodeParsed(std::ostream& str) const
{
   //DebugLog(<< "Pidf::encodeParsed " << mText);
   //str << mText;

   str       << "<?xml version\"1.0\" encoding=\"UTF-8\"?>" << Symbols::CRLF;;
   str       << "<presence xmlns=\"urn:ietf:params:xml:ns:cpim-pidf\"" << Symbols::CRLF;;
   str       << "           entity=\""<<mEntity<<"\">" << Symbols::CRLF;;
   for( unsigned int i=0; i<mTuple.size(); i++)
   {
      Data status( (char*)( (mTuple[i].status)?"open":"close" ) );
      str    << "  <tuple id=\""<<mTuple[i].id<<"\">" << Symbols::CRLF;;
      str    << "     <status><basic>"<<status<<"</basic></status>" << Symbols::CRLF;;
      if ( !mTuple[i].contact.empty() )
      {
         str << "     <contact priority=\""<<mTuple[i].contactPriority<<"\">"<<mTuple[i].contact<<"</contact>" << Symbols::CRLF;;
      }
      if ( !mTuple[i].timeStamp.empty() )
      {
         str << "     <timestamp>"<<mTuple[i].timeStamp<<"</timestamp>" << Symbols::CRLF;;
      }
      if ( !mTuple[i].note.empty() )
      {
         str << "     <note>"<<mTuple[i].note<<"</note>" << Symbols::CRLF;;
      }
      str    << "  </tuple>" << Symbols::CRLF;;
   }
   str       << "</presence>" << Symbols::CRLF;;
   
   return str;
}


void 
Pidf::parse(ParseBuffer& pb)
{
   const char* anchor = pb.position();

   Tuple t;
   
   mTuple.push_back( t );
   mTuple[0].status = true;
   mTuple[0].note = Data::Empty;

   const char* close = pb.skipToChars("close");
   if ( close != pb.end() )
   {
      DebugLog ( << "found a close" );
      mTuple[0].status = false;
   }

   pb.reset(anchor);
   const char* open = pb.skipToChars("open");
   if ( open != pb.end() )
   {
      DebugLog ( << "found an open" );
      mTuple[0].status = true;
   }

   pb.reset(anchor);
   pb.skipToChars("<note");
   pb.skipToChars(">");
   if (!pb.eof() )
   {
      const char* startNote = pb.skipChar();
      pb.data(mTuple[0].note, startNote);
      DebugLog ( << "found a note of" << mTuple[0].note);
   }
}


void 
Pidf::setSimpleId( const Data& id )
{
   if ( mTuple.size() == 0 )
   {
      Tuple t;
      mTuple.push_back( t );
   }
   assert( mTuple.size() > 0 );

   mTuple[0].id = id;
}


void 
Pidf::setSimpleStatus( bool online, const Data& note, const Data& contact )
{
   if ( mTuple.size() == 0 )
   {
      Tuple t;
      mTuple.push_back( t );
   }
   assert( mTuple.size() > 0 );

   mTuple[0].status = online;
   mTuple[0].contact = contact;
   mTuple[0].contactPriority = 1.0;
   mTuple[0].note = note;
   mTuple[0].timeStamp = Data::Empty;
}


bool 
Pidf::getSimpleStatus( Data* note )
{
   checkParsed();

   assert( mTuple.size() > 0 );

   if ( note )
   {
      *note = mTuple[0].note;
   }
   
   return mTuple[0].status;
}

void
Pidf::clear()
{
   mTuple.clear();
}   
