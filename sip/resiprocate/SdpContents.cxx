#include "sip2/sipstack/SdpContents.hxx"
#include "sip2/util/ParseBuffer.hxx"
#include "sip2/sipstack/Symbols.hxx"

using namespace Vocal2;
using namespace std;

ContentsFactory<SdpContents> SdpContents::Factory;

const char* NetworkType[] = {"???", "IP4", "IP6"};

bool 
AttributeHelper::exists(const Data& key) const
{
   return mAttributes.find(key) != mAttributes.end();
}

const list<Data>& 
AttributeHelper::getValue(const Data& key) const
{
   return mAttributes.find(key)->second;
}

ostream& 
AttributeHelper::encode(ostream& s) const
{
   for (std::map< Data, std::list<Data> >::const_iterator i = mAttributes.begin();
        i != mAttributes.end(); i++)
   {
      for (list<Data>::const_iterator j = i->second.begin();
           j != i->second.end(); j++)
      {
         s << "a="
           << i->first;
         if (!j->empty())
         {
            s << Symbols::COLON[0] << *j;
         }
         s << Symbols::CRLF;
      }
   }
   return s;
}
      
void 
AttributeHelper::parse(ParseBuffer& pb)
{
   while (!pb.eof() && *pb.position() == 'a')
   {
      Data key;
      Data value;
      
      pb.skipChar('a');
      const char* anchor = pb.skipChar(Symbols::EQUALS[0]);
      pb.skipToOneOf(Symbols::COLON, Symbols::CR);
      pb.data(key, anchor);
      if (*pb.position() == Symbols::COLON[0])
      {
         anchor = pb.skipChar(Symbols::COLON[0]);
         pb.skipToChar(Symbols::CR[0]);
         pb.data(value, anchor);
      }
            
      pb.skipChar(Symbols::CR[0]);
      pb.skipChar(Symbols::LF[0]);
            
      mAttributes[key].push_back(value);
   }
}

void 
AttributeHelper::addAttribute(const Data& key, const Data& value)
{
   mAttributes[key].push_back(value);
}

SdpContents::SdpContents()
   : Contents(getStaticType())
{}

SdpContents::SdpContents(HeaderFieldValue* hfv, const Mime& contentTypes)
   : Contents(hfv, contentTypes)
{}

SdpContents::SdpContents(const Data& data, const Mime& contentTypes)
   : Contents(contentTypes)
{
   assert(0);
}

Contents*
SdpContents::clone() const
{
   return new SdpContents(*this);
}

void
SdpContents::parse(ParseBuffer& pb)
{
   mSession.parse(pb);
}

ostream&
SdpContents::encodeParsed(ostream& s) const
{
   mSession.encode(s);
   return s;
}

const Mime& 
SdpContents::getStaticType() 
{
   static Mime type("application", "sdp");
   return type;
}

SdpContents::Session::Origin::Origin()
   : mUser(Data::Empty),
     mSessionId(0),
     mVersion(0),
     mAddrType(IP4),
     mAddress(Data::Empty)
{}

SdpContents::Session::Origin::Origin(const Data& user,
                                     const Data& sesionId,
                                     const Data& version,
                                     AddrType addr,
                                     const Data& address)
   : mUser(user),
     mSessionId(sesionId),
     mVersion(version),
     mAddrType(addr),
     mAddress(address)
{}

ostream& 
SdpContents::Session::Origin::encode(ostream& s) const
{
   s << "o=" 
     << mUser << Symbols::SPACE[0]
     << mSessionId << Symbols::SPACE[0]
     << mVersion << Symbols::SPACE[0]
     << "IN "
     << NetworkType[mAddrType] << Symbols::SPACE[0]
     << mAddress << Symbols::CRLF;
   return s;
}

void
SdpContents::Session::Origin::parse(ParseBuffer& pb)
{
   pb.skipChar('o');
   const char* anchor = pb.skipChar(Symbols::EQUALS[0]);

   pb.skipToChar(Symbols::SPACE[0]);
   pb.data(mUser, anchor);

   anchor = pb.skipChar(Symbols::SPACE[0]);
   pb.skipToChar(Symbols::SPACE[0]);
   pb.data(mSessionId, anchor);
   
   anchor = pb.skipChar(Symbols::SPACE[0]);
   pb.skipToChar(Symbols::SPACE[0]);
   pb.data(mVersion, anchor);

   pb.skipChar(Symbols::SPACE[0]);
   pb.skipChar('I');
   pb.skipChar('N');

   anchor = pb.skipChar(Symbols::SPACE[0]);
   pb.skipToChar(Symbols::SPACE[0]);
   Data addrType;
   pb.data(addrType, anchor);
   if (addrType == NetworkType[IP4])
   {
      mAddrType = IP4;
   }
   else if (addrType == NetworkType[IP6])
   {
      mAddrType = IP6;
   }
   else
   {
      mAddrType = static_cast<AddrType>(0);
   }

   anchor = pb.skipChar(Symbols::SPACE[0]);
   pb.skipToChar(Symbols::CR[0]);
   pb.data(mAddress, anchor);

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);
}

SdpContents::Session::Email::Email(const Data& address,
                                   const Data& freeText)
   : mAddress(address),
     mFreeText(freeText)
{}

ostream& 
SdpContents::Session::Email::encode(ostream& s) const
{
   s << "e="
     << mAddress;
   if (!mFreeText.empty())
   {
      s << Symbols::LPAREN[0] << mFreeText << Symbols::RPAREN[0];
   }
   s << Symbols::CRLF;
   
   return s;
}

// helper to parse email and phone numbers with display name
void parseEorP(ParseBuffer& pb, Data& eOrp, Data& freeText)
{
   // =mjh@isi.edu (Mark Handley)
   // =mjh@isi.edu
   // =Mark Handley <mjh@isi.edu>
   // =<mjh@isi.edu>

   const char* anchor = pb.skipChar(Symbols::EQUALS[0]);

   pb.skipToChar(Symbols::LPAREN[0]);
   if (!pb.eof())
   {
      // mjh@isi.edu (Mark Handley)
      //             ^
      pb.data(eOrp, anchor);

      anchor = pb.skipChar();
      pb.skipToEndQuote(Symbols::RPAREN[0]);
      pb.data(freeText, anchor);
      pb.skipChar(Symbols::RPAREN[0]);
   }
   else
   {
      pb.reset(anchor);
      pb.skipToChar(Symbols::LA_QUOTE[0]);
      if (!pb.eof())
      {
         // Mark Handley <mjh@isi.edu>
         //              ^
         pb.data(freeText, anchor);
         pb.skipChar();

         // <mjh@isi.edu>
         // ^
         anchor = pb.skipChar();
         pb.skipToEndQuote(Symbols::RA_QUOTE[0]);
         pb.data(eOrp, anchor);
         pb.skipChar(Symbols::RA_QUOTE[0]);
      }
      else
      {
         // mjh@isi.edu
         //            ^
         pb.data(eOrp, anchor);
      }
   }

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);
}

void
SdpContents::Session::Email::parse(ParseBuffer& pb)
{
   pb.skipChar('e');
   parseEorP(pb, mAddress, mFreeText);
}

SdpContents::Session::Phone::Phone(const Data& number,
                                   const Data& freeText)
   : mNumber(number),
     mFreeText(freeText)
{}

ostream&
SdpContents::Session::Phone::encode(ostream& s) const
{
   s << "e="
     << mNumber << Symbols::SPACE[0];
   if (!mFreeText.empty())
   {
      s << Symbols::LPAREN[0] << mFreeText << Symbols::RPAREN[0];
   }
   s << Symbols::CRLF;
   
   return s;
}

void
SdpContents::Session::Phone::parse(ParseBuffer& pb)
{
   pb.skipChar('p');
   parseEorP(pb, mNumber, mFreeText);
}

SdpContents::Session::Connection::Connection(AddrType addType,
                                             const Data& address,
                                             unsigned int ttl)
   : mAddrType(addType),
     mAddress(address),
     mTTL(ttl)
{}

SdpContents::Session::Connection::Connection()
   : mAddrType(IP4),
     mAddress(Data::Empty),
     mTTL(0)
{}

ostream& 
SdpContents::Session::Connection::encode(ostream& s) const
{
   s << "c=IN "
     << NetworkType[mAddrType] << Symbols::SPACE[0]
     << mAddress << Symbols::SPACE[0]
     << Symbols::SLASH[0] << mTTL 
     << Symbols::CRLF;
   return s;
}


void
SdpContents::Session::Connection::parse(ParseBuffer& pb)
{
   pb.skipChar('c');
   pb.skipChar(Symbols::EQUALS[0]);
   pb.skipChar('I');
   pb.skipChar('N');

   const char* anchor = pb.skipChar(Symbols::SPACE[0]);
   pb.skipToChar(Symbols::SPACE[0]);
   Data addrType;
   pb.data(addrType, anchor);
   if (addrType == NetworkType[IP4])
   {
      mAddrType = IP4;
   }
   else if (addrType == NetworkType[IP6])
   {
      mAddrType = IP6;
   }
   else
   {
      mAddrType = static_cast<AddrType>(0);
   }

   anchor = pb.skipChar();
   pb.skipToOneOf(Symbols::SLASH, Symbols::CR);
   pb.data(mAddress, anchor);

   mTTL = 0;
   if (*pb.position() == Symbols::SLASH[0])
   {
      pb.skipChar();
      mTTL = pb.integer();
   }

   // multicast dealt with above this parser
   if (*pb.position() != Symbols::SLASH[0])
   {
      pb.skipChar(Symbols::CR[0]);
      pb.skipChar(Symbols::LF[0]);
   }
}

SdpContents::Session::Bandwidth::Bandwidth(const Data& modifier,
                                           unsigned int kbPerSecond)
   : mModifier(modifier),
     mKbPerSecond(kbPerSecond)
{}
  
ostream& 
SdpContents::Session::Bandwidth::encode(ostream& s) const
{
   s << "b="
     << mModifier
     << Symbols::COLON[0] << mKbPerSecond
     << Symbols::CRLF;
   return s;
}

void
SdpContents::Session::Bandwidth::parse(ParseBuffer& pb)
{
   pb.skipChar('b');
   const char* anchor = pb.skipChar(Symbols::EQUALS[0]);
   
   pb.skipToChar(Symbols::COLON[0]);
   pb.data(mModifier, anchor);

   anchor = pb.skipChar(Symbols::COLON[0]);
   mKbPerSecond = pb.integer();

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);
}

SdpContents::Session::Time::Time(unsigned int start,
                                 unsigned int stop)
   : mStart(start),
     mStop(stop)
{}

ostream& 
SdpContents::Session::Time::encode(ostream& s) const
{
   s << "t=" << mStart << Symbols::SPACE[0] 
     << mStop 
     << Symbols::CRLF;

   for (list<Repeat>::const_iterator i = mRepeats.begin();
        i != mRepeats.end(); i++)
   {
      i->encode(s);
   }
   return s;
}

void
SdpContents::Session::Time::parse(ParseBuffer& pb)
{
   pb.skipChar('t');
   pb.skipChar(Symbols::EQUALS[0]);

   mStart = pb.integer();
   pb.skipChar(Symbols::SPACE[0]);
   mStop = pb.integer();

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);

   while (!pb.eof() && *pb.position() == 'r')
   {
      addRepeat(Repeat());
      mRepeats.back().parse(pb);
   }
}

void 
SdpContents::Session::Time::addRepeat(const Repeat& repeat)
{
   mRepeats.push_back(repeat);
}

SdpContents::Session::Time::Repeat::Repeat(unsigned int interval,
                                           unsigned int duration,
                                           list<int> offsets)
   : mInterval(interval),
     mDuration(duration),
     mOffsets(offsets)
{}

ostream&
SdpContents::Session::Time::Repeat::encode(ostream& s) const
{
   s << "r="
     << mInterval << Symbols::SPACE[0]
     << mDuration << 's';
   for (list<int>::const_iterator i = mOffsets.begin();
        i != mOffsets.end(); i++)
   {
      s << Symbols::SPACE[0] << *i << 's';
   }

   s << Symbols::CRLF;
   return s;
}

int
parseTypedTime(ParseBuffer& pb)
{
   int v = pb.integer();
   switch (*pb.position())
   {
      case 's' :
         pb.skipChar();
         break;
      case 'm' :
         v *= 60;
         pb.skipChar();
         break;
      case 'h' :
         v *= 3600;
         pb.skipChar();
      case 'd' :
         v *= 3600*24;
         pb.skipChar();
   }
   
   return v;
}
   
void
SdpContents::Session::Time::Repeat::parse(ParseBuffer& pb)
{
   pb.skipChar('r');
   pb.skipChar(Symbols::EQUALS[0]);
   
   mInterval = parseTypedTime(pb);
   pb.skipChar(Symbols::SPACE[0]);

   mDuration = parseTypedTime(pb);

   while (*pb.position() != Symbols::CR[0])
   {
      pb.skipChar(Symbols::SPACE[0]);
      
      mOffsets.push_back(parseTypedTime(pb));
   }

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);
}

SdpContents::Session::Timezones::Adjustment::Adjustment(unsigned int _time,
                                                        int _offset)
   : time(_time),
     offset(_offset)
{
}

SdpContents::Session::Timezones::Timezones()
   : mAdjustments()
{
}

ostream& 
SdpContents::Session::Timezones::encode(ostream& s) const
{
   if (!mAdjustments.empty())
   {
      s << "z=";
      bool first = true;
      for (list<Adjustment>::const_iterator i = mAdjustments.begin();
           i != mAdjustments.end(); i++)
      {
         if (!first)
         {
            s << Symbols::SPACE[0];
         }
         first = false;
         s << i->time << Symbols::SPACE[0]
           << i->offset << 's';
      }
      
      s << Symbols::CRLF;
   }
   return s;
}

void
SdpContents::Session::Timezones::parse(ParseBuffer& pb)
{
   pb.skipChar('z');
   pb.skipChar(Symbols::EQUALS[0]);

   while (*pb.position() != Symbols::CR[0])
   {
      Adjustment adj(0, 0);
      adj.time = pb.integer();
      pb.skipChar(Symbols::SPACE[0]);
      adj.offset = parseTypedTime(pb);
      addAdjustment(adj);

      if (*pb.position() == Symbols::SPACE[0])
      {
         pb.skipChar();
      }
   }

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);
}

void
SdpContents::Session::Timezones::addAdjustment(const Adjustment& adjust)
{
   mAdjustments.push_back(adjust);
}

SdpContents::Session::Encryption::Encryption()
   : mMethod(NoEncryption),
     mKey(Data::Empty)
{}

SdpContents::Session::Encryption::Encryption(const KeyType& method,
                                             const Data& key)
   : mMethod(method),
     mKey(key)
{}
  
const char* KeyTypes[] = {"????", "prompt", "clear", "base64", "uri"};

ostream&
SdpContents::Session::Encryption::encode(ostream& s) const
{
   s << "k="
     << KeyTypes[mMethod];
   if (mMethod != Prompt)
   {
      s << Symbols::COLON[0] << mKey;
   }
   s << Symbols::CRLF;

   return s;
}

void
SdpContents::Session::Encryption::parse(ParseBuffer& pb)
{
   pb.skipChar('k');
   const char* anchor = pb.skipChar(Symbols::EQUALS[0]);
   
   pb.skipToChar(Symbols::COLON[0]);
   if (!pb.eof())
   {
      Data p;
      pb.data(p, anchor);
      if (p == KeyTypes[Clear])
      {
         mMethod = Clear;
      }
      else if (p == KeyTypes[Base64])
      {
         mMethod = Base64;
      }
      else if (p == KeyTypes[UriKey])
      {
         mMethod = UriKey;
      }

      anchor = pb.skipChar(Symbols::COLON[0]);
      pb.skipToChar(Symbols::CR[0]);
      pb.data(mKey, anchor);
   }
   else
   {
      pb.reset(anchor);
      pb.skipToChar(Symbols::CR[0]);

      Data p;
      pb.data(p, anchor);
      if (p == KeyTypes[Prompt])
      {
         mMethod = Prompt;
      }
   }

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);
}

SdpContents::Session::Session(int version,
                              const Origin& origin,
                              const Data& name)
   : mVersion(version),
     mOrigin(origin),
     mName(name)
{}

void
SdpContents::Session::parse(ParseBuffer& pb)
{
   pb.skipChar('v');
   pb.skipChar(Symbols::EQUALS[0]);
   mVersion = pb.integer();
   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);

   mOrigin.parse(pb);

   pb.skipChar('s');
   const char* anchor = pb.skipChar(Symbols::EQUALS[0]);
   pb.skipToChar(Symbols::CR[0]);
   pb.data(mName, anchor);
   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);

   if (*pb.position() == 'u')
   {
      mUri.parse(pb);
      pb.skipChar(Symbols::CR[0]);
      pb.skipChar(Symbols::LF[0]);
   }

   while (*pb.position() == 'e')
   {
      addEmail(Email());
      mEmails.back().parse(pb);
   }

   while (*pb.position() == 'p')
   {
      addPhone(Phone());
      mPhones.back().parse(pb);
   }

   if (*pb.position() == 'c')
   {
      mConnection.parse(pb);
   }

   while (*pb.position() == 'b')
   {
      addBandwidth(Bandwidth());
      mBandwidths.back().parse(pb);
   }

   while (*pb.position() == 't')
   {
      addTime(Time());
      mTimes.back().parse(pb);
   }

   if (!pb.eof() && *pb.position() == 'z')
   {
      mTimezones.parse(pb);
   }

   if (!pb.eof() && *pb.position() == 'k')
   {
      mEncryption.parse(pb);
   }

   mAttributeHelper.parse(pb);

   while (!pb.eof() && *pb.position() == 'm')
   {
      addMedium(Medium());
      mMedia.back().parse(pb);
   }
}

ostream& 
SdpContents::Session::encode(ostream& s) const
{
   s << "v=" << mVersion << Symbols::CRLF;
   mOrigin.encode(s);
   s << "s=" << mName << Symbols::CRLF;
   
   if (!mInformation.empty())
   {
      s << "i=" << mInformation << Symbols::CRLF;
   }

   if (!mUri.host().empty())
   {
      mUri.encode(s);
      s << Symbols::CRLF;
   }

   for (list<Email>::const_iterator i = mEmails.begin();
        i != mEmails.end(); i++)
   {
      i->encode(s);
   }

   for (list<Phone>::const_iterator i = mPhones.begin();
        i != mPhones.end(); i++)
   {
      i->encode(s);
   }
   
   if (!mConnection.getAddress().empty())
   {
      mConnection.encode(s);
   }

   for (list<Bandwidth>::const_iterator i = mBandwidths.begin();
        i != mBandwidths.end(); i++)
   {
      i->encode(s);
   }

   for (list<Time>::const_iterator i = mTimes.begin();
        i != mTimes.end(); i++)
   {
      i->encode(s);
   }

   mTimezones.encode(s);

   if (mEncryption.getMethod() != Encryption::NoEncryption)
   {
      mEncryption.encode(s);
   }

   mAttributeHelper.encode(s);

   for (list<Medium>::const_iterator i = mMedia.begin();
        i != mMedia.end(); i++)
   {
      i->encode(s);
   }

   return s;
}

void
SdpContents::Session::addEmail(const Email& email)
{
   mEmails.push_back(email);
}

void
SdpContents::Session::addTime(const Time& t)
{
   mTimes.push_back(t);
}

void
SdpContents::Session::addPhone(const Phone& phone)
{
   mPhones.push_back(phone);
}

void
SdpContents::Session::addBandwidth(const Bandwidth& bandwidth)
{
   mBandwidths.push_back(bandwidth);
}

void
SdpContents::Session::addMedium(const Medium& medium)
{
   mMedia.push_back(medium);
   mMedia.back().setSession(this);
}

void
SdpContents::Session::addAttribute(const Data& key, const Data& value)
{
   mAttributeHelper.addAttribute(key, value);
}

bool
SdpContents::Session::exists(const Data& key) const
{
   return mAttributeHelper.exists(key);
}

const list<Data>&
SdpContents::Session::getValue(const Data& key) const
{
   return mAttributeHelper.getValue(key);
}

SdpContents::Session::Medium::Medium(const Data& name,
                                     unsigned int port,
                                     unsigned int multicast,
                                     const Data& protocol)
   : mName(name),
     mPort(port),
     mMulticast(multicast),
     mProtocol(protocol)
{}

SdpContents::Session::Medium::Medium()
   : mMulticast(1)
{}
  
void
SdpContents::Session::Medium::setSession(Session* session)
{
   mSession = session;
}

void
SdpContents::Session::Medium::parse(ParseBuffer& pb)
{
   pb.skipChar('m');
   const char* anchor = pb.skipChar(Symbols::EQUALS[0]);

   pb.skipToChar(Symbols::SPACE[0]);
   pb.data(mName, anchor);
   pb.skipChar(Symbols::SPACE[0]);

   mPort = pb.integer();

   if (*pb.position() == Symbols::SLASH[0])
   {
      pb.skipChar();
      mMulticast = pb.integer();
   }

   anchor = pb.skipChar(Symbols::SPACE[0]);
   pb.skipToOneOf(Symbols::SPACE, Symbols::CR);
   pb.data(mProtocol, anchor);

   while (*pb.position() != Symbols::CR[0])
   {
      anchor = pb.skipChar(Symbols::SPACE[0]);
      pb.skipToOneOf(Symbols::SPACE, Symbols::CR);
      Data format;
      pb.data(format, anchor);
      addFormat(format);
   }

   pb.skipChar(Symbols::CR[0]);
   pb.skipChar(Symbols::LF[0]);

   if (!pb.eof() && *pb.position() == 'i')
   {
      pb.skipChar('i');
      anchor = pb.skipChar(Symbols::EQUALS[0]);
      pb.skipToChar(Symbols::CR[0]);
      pb.data(mInformation, anchor);
      
      pb.skipChar(Symbols::CR[0]);
      pb.skipChar(Symbols::LF[0]);
   }

   while (!pb.eof() && *pb.position() == 'c')
   {
      addConnection(Connection());
      mConnections.back().parse(pb);
      if (*pb.position() == Symbols::SLASH[0])
      {
         pb.skipChar();
         int num = pb.integer();

         Connection& con = mConnections.back();
         const Data& addr = con.getAddress();
         int i = addr.size() - 1;
         for (; i; i--)
         {
            if (addr[i] == '.')
            {
               break;
            }
         }

         if (addr[i] == '.')
         {
            Data before(addr.data(), i);
            ParseBuffer subpb(addr.data()+i+1, addr.size()-i-1);
            int after = subpb.integer();

            for (int i = 0; i < num-1; i++)
            {
               addConnection(con);
               mConnections.back().mAddress = before + Data(after+i);               
            }
         }

         pb.skipChar(Symbols::CR[0]);
         pb.skipChar(Symbols::LF[0]);
      }
   }

   while (!pb.eof() && *pb.position() == 'b')
   {
      addBandwidth(Bandwidth());
      mBandwidths.back().parse(pb);
   }

   if (!pb.eof() && *pb.position() == 'k')
   {
      mEncryption.parse(pb);
   }
   
   mAttributeHelper.parse(pb);
}

ostream& 
SdpContents::Session::Medium::encode(ostream& s) const
{
   s << "m="
     << mName << Symbols::SPACE[0]
     << mPort;
   if (mMulticast > 1)
   {
      s << Symbols::SLASH[0] << mMulticast;
   }
   s << Symbols::SPACE[0] 
     << mProtocol;

   for (list<Data>::const_iterator i = mFormats.begin();
        i != mFormats.end(); i++)
   {
      s << Symbols::SPACE[0] << *i;
   }
   s << Symbols::CRLF;

   if (!mInformation.empty())
   {
      s << "i=" << mInformation << Symbols::CRLF;
   }

   for (list<Connection>::const_iterator i = mConnections.begin();
        i != mConnections.end(); i++)
   {
      i->encode(s);
   }

   for (list<Bandwidth>::const_iterator i = mBandwidths.begin();
        i != mBandwidths.end(); i++)
   {
      i->encode(s);
   }

   if (mEncryption.getMethod() != Encryption::NoEncryption)
   {
      mEncryption.encode(s);
   }

   mAttributeHelper.encode(s);
   
   return s;
}

void 
SdpContents::Session::Medium::addFormat(const Data& format)
{
   mFormats.push_back(format);
}

void
SdpContents::Session::Medium::addConnection(const Connection& connection)
{
   mConnections.push_back(connection);
}

void
SdpContents::Session::Medium::addBandwidth(const Bandwidth& bandwidth)
{
   mBandwidths.push_back(bandwidth);
}

void
SdpContents::Session::Medium::addAttribute(const Data& key, const Data& value)
{
   mAttributeHelper.addAttribute(key, value);
}

bool
SdpContents::Session::Medium::exists(const Data& key) const
{
   if (mAttributeHelper.exists(key))
   {
      return true;
   }
   return mSession->exists(key);
}

const list<Data>&
SdpContents::Session::Medium::getValue(const Data& key) const
{
   if (exists(key))
   {
      return mAttributeHelper.getValue(key);
   }
   return mSession->getValue(key);
}

