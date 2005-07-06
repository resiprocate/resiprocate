#include <iostream>

#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/Aor.hxx"

using namespace resip;

Aor::Aor()
{
}

Aor::Aor(const Data& value)
{
   ParseBuffer pb(value);
   
   pb.skipWhitespace();
   const char* start = pb.position();
   pb.skipToOneOf(":@"); // make sure the colon precedes

   pb.assertNotEof();

   pb.data(mScheme, start);
   pb.skipChar(Symbols::COLON[0]);
   mScheme.lowercase();

   if (isEqualNoCase(mScheme, Symbols::Tel))
   {
      const char* anchor = pb.position();
      pb.skipToOneOf(ParseBuffer::Whitespace, ";>");
      pb.data(mUser, anchor);
      if (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0])
      {
         anchor = pb.skipChar();
         pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::RA_QUOTE);
      }
      return;
   }
   
   start = pb.position();
   pb.skipToChar(Symbols::AT_SIGN[0]);
   if (!pb.eof())
   {
      pb.reset(start);
      start = pb.position();
      pb.skipToOneOf(":@");
      pb.data(mUser, start);
      if (!pb.eof() && *pb.position() == Symbols::COLON[0])
      {
         start = pb.skipChar();
         pb.skipToChar(Symbols::AT_SIGN[0]);
      }
      start = pb.skipChar();
   }
   else
   {
      pb.reset(start);
   }
   
   if (*start == '[')
   {
      start = pb.skipChar();
      pb.skipToChar(']');
      pb.data(mHost, start);
      DnsUtil::canonicalizeIpV6Address(mHost);
      pb.skipChar();
   }
   else
   {
      pb.skipToOneOf(ParseBuffer::Whitespace, ":;?>");
      pb.data(mHost, start);
   }

   pb.skipToOneOf(ParseBuffer::Whitespace, ":;?>");
   if (!pb.eof() && *pb.position() == ':')
   {
      start = pb.skipChar();
      mPort = pb.integer();
      pb.skipToOneOf(ParseBuffer::Whitespace, ";?>");
   }
   else
   {
      mPort = 0;
   }
}

Aor::Aor(const Uri& uri) : 
   mScheme(uri.scheme()),
   mUser(uri.user()),
   mHost(uri.host()),
   mPort(uri.port())
{
   
}

Aor::Aor(const Aor& aor)
{
   *this = aor;
}

Aor& 
Aor::operator=(const Aor& aor)
{
   if (this != &aor)
   {
      mScheme = aor.mScheme;
      mUser = aor.mUser;
      mHost = aor.mHost;
      mPort = aor.mPort;
   }
   return *this;
}

      
bool 
Aor::operator==(const Aor& other) const
{
   return value() == other.value();
}

bool 
Aor::operator!=(const Aor& other) const
{
   return value() != other.value();
}

bool 
Aor::operator<(const Aor& other) const
{
   return value() < other.value();
}

const Data& 
Aor::value() const
{
   if (mOldScheme != mScheme || 
       mOldUser != mUser ||
       mOldHost != mHost ||
       mOldPort != mPort)
   {
      mOldHost = mHost;
      if (DnsUtil::isIpV6Address(mHost))
      {
         mCanonicalHost = DnsUtil::canonicalizeIpV6Address(mHost);
      }
      else
      {
         mCanonicalHost = mHost;
         mCanonicalHost.lowercase();
      }

      mOldScheme = mScheme;
      mOldUser = mUser;
      mOldPort = mPort;

      mValue.reserve(mUser.size() + mCanonicalHost.size() + 10);

      DataStream strm(mValue);
      strm << mScheme;
      strm << Symbols::COLON;
      strm << mUser;
      if (!mCanonicalHost.empty())
      {
         strm << Symbols::AT_SIGN;
         strm << mCanonicalHost;

         if (mPort != 0)
         {
            strm << Symbols::COLON;
            strm << Data(mPort);
         }
      }
   }

   return mValue;
}

Data& 
Aor::scheme()
{
   return mScheme;
}

const Data& 
Aor::scheme() const
{
   return mScheme;
}

Data& 
Aor::host()
{
   return mHost;
}

const Data& 
Aor::host() const
{
   return mHost;
}

Data& 
Aor::user()
{
   return mUser;
}

const Data& 
Aor::user() const
{
   return mUser;
}

int& 
Aor::port()
{
   return mPort;
}

int 
Aor::port() const
{
   return mPort;
}
      
std::ostream& 
Aor::operator<<(std::ostream& str) const
{
   str << value();
   return str;
}

