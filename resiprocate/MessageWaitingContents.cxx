#include "resiprocate/MessageWaitingContents.hxx"

using namespace Vocal2;
using namespace std;

ContentsFactory<MessageWaitingContents> MessageWaitingContents::Factory;

Vocal2::MessageWaitingContents::AccountHeader Vocal2::mw_account;
const char* MessageHeaders[MW_MAX] = {"voice-message", 
                                      "fax-message", 
                                      "pager-message", 
                                      "multimedia-message", 
                                      "text-message",
                                      "none"};

MessageWaitingContents::MessageWaitingContents()
   : Contents(getStaticType()),
     mHasMessages(false),
     mAccountUri(0)
{
   for(int i = 0; i < (int)MW_MAX; i++)
   {
      mHeaders[i] = 0;
   }
}

MessageWaitingContents::MessageWaitingContents(HeaderFieldValue* hfv, const Mime& contentType)
   : Contents(hfv, contentType),
     mHasMessages(false),
     mAccountUri(0)
{
   for(int i = 0; i < (int)MW_MAX; i++)
   {
      mHeaders[i] = 0;
   }
}

MessageWaitingContents::MessageWaitingContents(const Data& data, const Mime& contentType)
   : Contents(contentType),
     mHasMessages(false),
     mAccountUri(0)
{
   for(int i = 0; i < (int)MW_MAX; i++)
   {
      mHeaders[i] = 0;
   }
   assert(0);
}

MessageWaitingContents::MessageWaitingContents(const MessageWaitingContents& rhs)
   : Contents(getStaticType()),
     mHasMessages(rhs.mHasMessages),
     mAccountUri(rhs.mAccountUri ? new Uri(*rhs.mAccountUri) : 0),
     mExtensions(rhs.mExtensions)
{
   for(int i = 0; i < (int)MW_MAX; i++)
   {
      if (rhs.mHeaders[i] != 0)
      {
         mHeaders[i] = new Header(*rhs.mHeaders[i]);
      }
      else
      {
         mHeaders[i] = 0;
      }
   }
}   

MessageWaitingContents::~MessageWaitingContents()
{
   clear();
}

void
MessageWaitingContents::clear()
{
   mHasMessages = false;

   delete mAccountUri;
   mAccountUri = 0;
   
   for (int i = 0; i < (int)MW_MAX; i++)
   {
      delete mHeaders[i];
   }
}

MessageWaitingContents&
MessageWaitingContents::operator=(const MessageWaitingContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      clear();

      mHasMessages = rhs.mHasMessages;
      mAccountUri = rhs.mAccountUri ? new Uri(*rhs.mAccountUri) : 0;
      mExtensions = rhs.mExtensions;

      for(int i = 0; i < (int)MW_MAX; i++)
      {
         if (rhs.mHeaders[i] != 0)
         {
            mHeaders[i] = new Header(*rhs.mHeaders[i]);
         }
         else
         {
            mHeaders[i] = 0;
         }
      }
   }
   return *this;
}

const Mime& 
MessageWaitingContents::getStaticType() 
{
   //static Mime type("application", "simple-message-summary");
   static Mime type("text", "data");
   return type;
}

Contents*
MessageWaitingContents::clone() const
{
   return new MessageWaitingContents(*this);
}

ostream& 
MessageWaitingContents::encodeParsed(ostream& s) const
{
   s << "Messages-Waiting" << Symbols::COLON[0] << Symbols::SPACE[0]
     << (mHasMessages ? "yes" : "no") << Symbols::CRLF;

   if (exists(mw_account))
   {
      header(mw_account).encode(s);
   }

   for(int i = 0; i < (int)MW_MAX; i++)
   {
      if (mHeaders[i] != 0)
      {
         s << MessageHeaders[i] << Symbols::COLON[0] << Symbols::SPACE[0]
           << mHeaders[i]->mNew << Symbols::SLASH[0] 
           << mHeaders[i]->mOld;
         if (mHeaders[i]->mHasUrgent)
         {
            s << Symbols::LPAREN[0]
              << mHeaders[i]->mUrgentNew << Symbols::SLASH[0] 
              << mHeaders[i]->mUrgentOld;
         }
         s << Symbols::CRLF;
      }
   }

   if (!mExtensions.empty())
   {
      s << Symbols::CRLF;
      for (map<Data, Data>::const_iterator i = mExtensions.begin();
           i != mExtensions.end(); i++)
      {
         s << i->first << Symbols::COLON[0] << Symbols::SPACE[0]
           << i->second << Symbols::CRLF;
      }
   }

   return s;
}

inline
bool
isWhite(char c)
{
   switch (c)
   {
      case ' ' :
      case '\t' : 
      case '\r' : 
      case '\n' : 
         return true;
      default:
         return false;
   }
}

const char*
Vocal2::skipSipLWS(ParseBuffer& pb)
{
   enum {WS, CR, LF, CR1};

   int state = WS;

   while (!pb.eof())
   {
      if (!isWhite(*pb.position()))
      {
         if (state == LF)
         {
            pb.reset(pb.position() - 2);
         }
         return pb.position();
      }
      if (!pb.eof())
      {
	 switch (state)
	 {
	    case WS:
	       if (*pb.position() == Symbols::CR[0])
	       {
		  state = CR;
	       }
	       break;
	    case CR:
	       if (*pb.position() == Symbols::CR[0])
	       {
		  state = CR;
	       }
	       else if (*pb.position() == Symbols::LF[0])
	       {
		  state = LF;
	       }
	       else
	       {
		  state = WS;
	       }
	       break;
	    case LF:
	       if (*pb.position() == Symbols::CR[0])
	       {
		  state = CR1;
	       }
	       else if (!pb.eof() && *pb.position() == Symbols::LF[0])
	       {
		  state = WS;
	       }
	       break;
	    case CR1:
	       if (*pb.position() == Symbols::CR[0])
	       {
		  state = CR;
	       }
	       else if (*pb.position() == Symbols::LF[0])
	       {
		  pb.reset(pb.position() - 3);
		  return pb.position();
	       }
	       else
	       {
		  state = WS;
	       }
	       break;
	    default:
	       assert(false);
	 }
      }
      pb.skipChar();
   }

   if (state == LF)
   {
      pb.reset(pb.position() - 2);
   }
   return pb.position();
}

void
MessageWaitingContents::parse(ParseBuffer& pb)
{
   pb.skipChars("Messages-Waiting");
   pb.skipWhitespace();
   pb.skipChar(Symbols::COLON[0]);
   const char* anchor = pb.skipWhitespace();
   pb.skipNonWhitespace();
   
   Data has;
   pb.data(has, anchor);
   if (has == "yes")
   {
      mHasMessages = true;
   }
   else if (has == "no")
   {
      mHasMessages = false;
   }
   else
   {
      pb.fail(__FILE__, __LINE__);
   }

   anchor = pb.skipWhitespace();
   if (pb.eof())
   {
      return;
   }

   Data accountHeader;
   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::COLON);
   pb.data(accountHeader, anchor);
   static const Data AccountMessage("message-account");
   if (isEqualNoCase(accountHeader, AccountMessage))
   {
      pb.skipWhitespace();
      pb.skipChar(Symbols::COLON[0]);
      pb.skipWhitespace();
      
      mAccountUri = new Uri();
      mAccountUri->parse(pb);
      pb.skipChars(Symbols::CRLF);
   }
   else
   {
      pb.reset(anchor);
   }

   while (!pb.eof() && *pb.position() != Symbols::CR[0])
   {
      int ht = -1;
      switch (tolower(*pb.position()))
      {
         case 'v' :
            ht = mw_voice;
            break;
         case 'f' :
            ht = mw_fax;
            break;
         case 'p' :
            ht = mw_pager;
            break;
         case 'm' :
            ht = mw_multimedia;
            break;
         case 't' :
            ht = mw_text;
            break;
         case 'n' :
            ht = mw_none;
            break;
         default :
            pb.fail(__FILE__, __LINE__);
      }
      assert(ht != -1);

      pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::COLON);
      pb.skipWhitespace();
      pb.skipChar(Symbols::COLON[0]);
      pb.skipWhitespace();

      unsigned int numNew = pb.integer();
      pb.skipWhitespace();
      pb.skipChar(Symbols::SLASH[0]);
      pb.skipWhitespace();

      unsigned int numOld = pb.integer();
      skipSipLWS(pb);

      if (!pb.eof() && *pb.position() != Symbols::LPAREN[0])
      {
         if (mHeaders[ht] != 0)
         {
            pb.fail(__FILE__, __LINE__);
         }
         mHeaders[ht] = new Header(numNew, numOld);
      }
      else
      {
         pb.skipChar();
         pb.skipWhitespace();

         unsigned int numUrgentNew = pb.integer();
         pb.skipWhitespace();
         pb.skipChar(Symbols::SLASH[0]);
         pb.skipWhitespace();

         unsigned int numUrgentOld = pb.integer();
         pb.skipWhitespace();
         pb.skipChar(Symbols::RPAREN[0]);
         // skip LWS as specified in rfc3261
         skipSipLWS(pb);

         if (mHeaders[ht] != 0)
         {
            pb.fail(__FILE__, __LINE__);
         }
         mHeaders[ht] = new Header(numNew, numOld, numUrgentNew, numUrgentOld);
      }
      
      pb.skipChars(Symbols::CRLF);
   }

   if (!pb.eof() && *pb.position() == Symbols::CR[0])
   {
      pb.skipChars(Symbols::CRLF);
      
      while (!pb.eof())
      {
         anchor = pb.position();
         Data header;
         pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::COLON);
         pb.data(header, anchor);

         pb.skipWhitespace();
         pb.skipChar(Symbols::COLON[0]);
         anchor = pb.skipWhitespace();

         while (true)
         {
            const char* pos = pb.skipToChar(Symbols::CR[0]);
            skipSipLWS(pb);
            if (pb.position() == pos)
            {
               Data content;
               pb.data(content, anchor);
               mExtensions[header] = content;

               pb.skipChars(Symbols::CRLF);
               break;
            }
         }
      }
   }
}

MessageWaitingContents::Header::Header(unsigned int numNew,
                                       unsigned int numOld)
   : mNew(numNew),
     mOld(numOld),
     mHasUrgent(false),
     mUrgentNew(0),
     mUrgentOld(0)
{}

MessageWaitingContents::Header::Header(unsigned int numNew,
                                       unsigned int numOld,
                                       unsigned int numUrgentNew,
                                       unsigned int numUrgentOld)
   : mNew(numNew),
     mOld(numOld),
     mHasUrgent(true),
     mUrgentNew(numUrgentNew),
     mUrgentOld(numUrgentOld)
{}

MessageWaitingContents::Header& 
MessageWaitingContents::header(HeaderType ht) const
{
   checkParsed();
   MessageWaitingContents& ncthis = *const_cast<MessageWaitingContents*>(this);
   if (mHeaders[ht] == 0)
   {
      ncthis.mHeaders[ht] = new Header(0, 0);
   }
   return *ncthis.mHeaders[ht];
}

bool 
MessageWaitingContents::exists(HeaderType ht) const
{
   checkParsed();
   return mHeaders[ht] != 0;
}

void
MessageWaitingContents::remove(HeaderType ht)
{
   checkParsed();
   delete mHeaders[ht];
   mHeaders[ht] = 0;
}

Uri& 
MessageWaitingContents::header(const AccountHeader& ht) const
{
   checkParsed();
   MessageWaitingContents& ncthis = *const_cast<MessageWaitingContents*>(this);
   if (mAccountUri == 0)
   {
      ncthis.mAccountUri = new Uri();
   }
   return *ncthis.mAccountUri;
}

bool 
MessageWaitingContents::exists(const AccountHeader& ht) const
{
   checkParsed();
   return mAccountUri != 0;
}

void
MessageWaitingContents::remove(const AccountHeader& ht)
{
   checkParsed();
   delete mAccountUri;
   mAccountUri = 0;
}

Data&
MessageWaitingContents::header(const Data& hn) const
{
   checkParsed();
   return mExtensions[hn];
}

bool
MessageWaitingContents::exists(const Data& hn) const
{
   checkParsed();
   return mExtensions.find(hn) != mExtensions.end();
}

void
MessageWaitingContents::remove(const Data& hn)
{
   checkParsed();
   mExtensions.erase(hn);
}

