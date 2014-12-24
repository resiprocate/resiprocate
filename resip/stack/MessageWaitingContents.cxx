#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/MessageWaitingContents.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

bool
MessageWaitingContents::init()
{
   static ContentsFactory<MessageWaitingContents> factory;
   (void)factory;
   return true;
}

resip::MessageWaitingContents::AccountHeader resip::mw_account;
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

MessageWaitingContents::MessageWaitingContents(const HeaderFieldValue& hfv, const Mime& contentType)
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
   resip_assert(0);
}

MessageWaitingContents::MessageWaitingContents(const MessageWaitingContents& rhs)
   : Contents(rhs),
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
   static Mime type("application", "simple-message-summary");
   //static Mime type("text", "data");
   return type;
}

Contents*
MessageWaitingContents::clone() const
{
   return new MessageWaitingContents(*this);
}

EncodeStream& 
MessageWaitingContents::encodeParsed(EncodeStream& s) const
{
   s << "Messages-Waiting" << Symbols::COLON[0] << Symbols::SPACE[0]
     << (mHasMessages ? "yes" : "no") << Symbols::CRLF;

   if (exists(mw_account))
   {
      s << "Message-Account" << Symbols::COLON[0] << Symbols::SPACE[0];
      header(mw_account).encode(s);
      s << Symbols::CRLF;
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
            s << Symbols::SPACE[0] << Symbols::LPAREN[0]    
              << mHeaders[i]->mUrgentNew << Symbols::SLASH[0] 
              << mHeaders[i]->mUrgentOld << Symbols::RPAREN[0]; 
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
resip::skipSipLWS(ParseBuffer& pb)
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
	       resip_assert(false);
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
   if (isEqualNoCase(has, "yes"))
   {
      mHasMessages = true;
   }
   else if (isEqualNoCase(has, "no"))
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
      resip_assert(ht != -1);

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
            // CodeWarrior isn't helpful enough to pick the "obvious" operator definition
            // so we add volatile here so CW is completely unconfused what to do.
            const volatile char* pos = pb.skipToChar(Symbols::CR[0]);
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
MessageWaitingContents::header(HeaderType ht)
{
   checkParsed();

   /* this is a trick to allow a const method to update "this" with an empty
      Header in case there wasn't a corresponding header line in the MessageWaiting doc
    */
   if (mHeaders[ht] == 0)
   {
      mHeaders[ht] = new Header(0, 0);
   }
   return *mHeaders[ht];
}

const MessageWaitingContents::Header& 
MessageWaitingContents::header(HeaderType ht) const
{
   checkParsed();

   /* this is a trick to allow a const method to update "this" with an empty
      Header in case there wasn't a corresponding header line in the MessageWaiting doc
    */
   if (mHeaders[ht] == 0)
   {
      ErrLog(<< "You called "
            "MessageWaitingContents::header(HeaderType ht) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      MessageWaitingContents* ncthis = const_cast<MessageWaitingContents*>(this);
      ncthis->mHeaders[ht] = new Header(0, 0);
   }
   return *mHeaders[ht];
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
MessageWaitingContents::header(const AccountHeader& ht)
{
   checkParsed();

   /* this is a trick to allow a const method to update "this" with an empty
      Uri in case there wasn't a Message-Account line in the MessageWaiting doc
    */
   if (mAccountUri == 0)
   {
      mAccountUri = new Uri();
   }
   return *mAccountUri;
}

const Uri& 
MessageWaitingContents::header(const AccountHeader& ht) const
{
   checkParsed();

   /* this is a trick to allow a const method to update "this" with an empty
      Uri in case there wasn't a Message-Account line in the MessageWaiting doc
    */
   if (mAccountUri == 0)
   {
      ErrLog(<< "You called "
            "MessageWaitingContents::header(const AccountHeader& ht) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      MessageWaitingContents* ncthis = const_cast<MessageWaitingContents*>(this);
      ncthis->mAccountUri = new Uri();
   }
   return *mAccountUri;
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
MessageWaitingContents::header(const Data& hn)
{
   checkParsed();
   return mExtensions[hn];
}

const Data&
MessageWaitingContents::header(const Data& hn) const
{
   checkParsed();
   std::map<Data, Data>::const_iterator h=mExtensions.find(hn);
   if(h==mExtensions.end())
   {
      ErrLog(<< "You called "
            "MessageWaitingContents::header(const Data& hn) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      MessageWaitingContents* ncthis = const_cast<MessageWaitingContents*>(this);
      h=ncthis->mExtensions.insert(std::make_pair(hn,Data::Empty)).first;
   }
   return h->second;
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

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
