#include <sipstack/Uri.hxx>
#include <util/ParseBuffer.hxx>

using namespace Vocal2;

Uri::Uri(const Uri& rhs)
   : ParserCategory(rhs),
     mScheme(rhs.mScheme),
     mHost(rhs.mHost),
     mUser(rhs.mUser),
     mAor(rhs.mAor),
     mPort(rhs.mPort),
     mPassword(rhs.mPassword)
{
}

const Data&
Uri::getAor() const
{
   // could keep last mUser, mHost, mPort around and compare
   // rather than always reallocate...
   mAor = mUser + Symbols::AT_SIGN + mHost + Symbols::COLON + Data(mPort);
   return mAor;
}

void
Uri::parse(ParseBuffer& pb)
{
   const char* start = pb.position();
   pb.skipToChar(Symbols::COLON[0]);
   mScheme = Data(start, pb.position() - start);
   pb.skipChar();
   if (mScheme == Symbols::Sip || mScheme == Symbols::Sips)
   {
      start = pb.position();
      pb.skipToChar(Symbols::AT_SIGN[0]);
      if (!pb.eof())
      {
         pb.reset(start);
         start = pb.position();
         pb.skipToOneOf(":@");
         mUser = Data(start, pb.position() - start);
         if (*pb.position() == Symbols::COLON[0])
         {
            start = pb.skipChar();
            pb.skipToChar(Symbols::AT_SIGN[0]);
            mPassword = Data(start, pb.position() - start);
         }
         start = pb.skipChar();
      }
      else
      {
         pb.reset(start);
      }

      if (*start == '[')
      {
         pb.skipToChar(']');
      }
      else
      {
         pb.skipToOneOf(ParseBuffer::Whitespace, ":;>");
      }
      mHost = Data(start, pb.position() - start);
      pb.skipToOneOf(ParseBuffer::Whitespace, ":;>");
      if (*pb.position() == ':')
      {
         start = pb.skipChar();
         pb.skipToOneOf(ParseBuffer::Whitespace, ";>");
         mPort = atoi(start);
      }
      else
      {
         mPort = Symbols::DefaultSipPort;
      }
   }
   else
   {
      // generic URL
      assert(0);
   }
}

ParserCategory*
Uri::clone() const
{
   return new Uri(*this);
}
 
std::ostream& 
Uri::encode(std::ostream& str) const
{
   str << mScheme << Symbols::COLON;
   if (!mUser.empty())
   {
      str << mUser;
      if (!mPassword.empty())
      {
         str << Symbols::COLON << mPassword;
      }
      str << Symbols::AT_SIGN;
   }
   str << mHost << Symbols::COLON << mPort;
   encodeParameters(str);
   return str;
}

void
Uri::parseEmbeddedHeaders()
{
   assert(0);
}
