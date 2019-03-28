#if !defined(RESIP_PUBLICATIONPERSISTENCEMANAGER_HXX)
#define RESIP_PUBLICATIONPERSISTENCEMANAGER_HXX

#include <map>

#include "rutil/XMLCursor.hxx"
#include "resip/stack/Contents.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "rutil/Data.hxx"
#include "rutil/Timer.hxx"
#include "rutil/SharedPtr.hxx"

namespace resip
{

/** Abstract interface of a datastore of all publication documents processed by DUM.  Derived classes implement the
    actual storage of publication documents.  resip::InMemorySyncPubDb is an example of a local datastore.
  */
class PublicationPersistenceManager
{
public:

   struct PubDocument
   {
      PubDocument() : mExpirationTime(0), mLastUpdated(0), mLingerTime(0), mSyncPublication(false) {}
      PubDocument(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, const Contents* contents, const SecurityAttributes* securityAttributes, bool syncPublication = false) :
         mEventType(eventType), mDocumentKey(documentKey), mETag(eTag), mExpirationTime(expirationTime), mLastUpdated(Timer::getTimeSecs()), mLingerTime(expirationTime), mSyncPublication(syncPublication)
      {
         if (contents)
         {
            mContents.reset(contents->clone());
         }
         if (securityAttributes)
         {
            mSecurityAttributes.reset(new SecurityAttributes);
            *mSecurityAttributes = *securityAttributes;
         }
      }

      void stream(std::iostream& s) const
      {
          UInt64 now = Timer::getTimeSecs();

          // Note: for compatibility with RegSyncServer/Client, using pubinfo for tag name instead of pubdocument
          s << "<pubinfo>" << Symbols::CRLF;
          s << "  <eventtype>" << mEventType.xmlCharDataEncode() << "</eventtype>" << Symbols::CRLF;
          s << "  <documentkey>" << mDocumentKey.xmlCharDataEncode() << "</documentkey>" << Symbols::CRLF;
          s << "  <etag>" << mETag.xmlCharDataEncode() << "</etag>" << Symbols::CRLF;
          s << "  <expires>" << (((mExpirationTime == 0) || (mExpirationTime <= now)) ? 0 : (mExpirationTime - now)) << "</expires>" << Symbols::CRLF;
          s << "  <lastupdate>" << now - mLastUpdated << "</lastupdate>" << Symbols::CRLF;
          if (mExpirationTime != 0 && mContents.get())  // lingering records will have expirationTime as 0 - don't need to send contents - refreshes also have no body
          {

              Mime mimeType = mContents->getType();

              // There is an assumption that the contenttype and contentsubtype tags will always occur
              // before the content tag in the deserialization
              s << "  <contentstype>" << mimeType.type().xmlCharDataEncode() << "</contentstype>" << Symbols::CRLF;
              s << "  <contentssubtype>" << mimeType.subType().xmlCharDataEncode() << "</contentssubtype>" << Symbols::CRLF;
              s << "  <contents>" << mContents->getBodyData().xmlCharDataEncode() << "</contents>" << Symbols::CRLF;

              if(mSecurityAttributes.get())
              {
                  resip_assert(mSecurityAttributes);
                  s << "   <isencrypted>" << (mSecurityAttributes->isEncrypted() ? "true" : "false") << "</isencrypted>" << Symbols::CRLF;
                  if (mSecurityAttributes->isEncrypted())
                  {
                     s << "   <sigstatus>";
                     switch (mSecurityAttributes->getSignatureStatus())
                     {
                     case SignatureNone:
                        s << "none";
                        break;
                     case SignatureIsBad:
                        s << "bad";
                        break;
                     case SignatureTrusted:
                        s << "trusted";
                        break;
                     case SignatureCATrusted:
                        s << "catrusted";
                        break;
                     case SignatureNotTrusted:
                        s << "nottrusted";
                        break;
                     case SignatureSelfSigned:
                        s << "selfsigned";
                        break;
                     default:
                        resip_assert(false);
                        s << "unknown";
                        break;
                     }
                     s << "</sigstatus>" << Symbols::CRLF;
                     if (!mSecurityAttributes->getSigner().empty())
                     {
                        s << "   <signer>" << mSecurityAttributes->getSigner().xmlCharDataEncode() << "</signer>" << Symbols::CRLF;
                     }
                     if (!mSecurityAttributes->getIdentity().empty())
                     {
                        s << "   <identity>" << mSecurityAttributes->getIdentity().xmlCharDataEncode() << "</identity>" << Symbols::CRLF;
                        s << "   <identitystrength>";
                        switch (mSecurityAttributes->getIdentityStrength())
                        {
                        case SecurityAttributes::From:
                           s << "from";
                           break;
                        case SecurityAttributes::FailedIdentity:
                           s << "failedidentity";
                           break;
                        case SecurityAttributes::Identity:
                           s << "identity";
                           break;
                        default:
                           resip_assert(false);
                           s << "unknown";
                           break;
                        }
                        s << "</identitystrength>" << Symbols::CRLF;
                     }
                     // Note:  intentionally not syncing mLevel and mEncryptionPerformed from SecurityAttributes since they are for outbound messages only
                  }
              }
          }
          s << "</pubinfo>" << Symbols::CRLF;
      }
      
      bool deserialize(resip::XMLCursor& xml, UInt64 now = 0)
      {
          bool success = false;
          *this = PubDocument();
          if(now <= 0)
          {
              now = Timer::getTimeSecs();
          }
          Data mimeTypeString;
          Data mimeSubtypeString;

          if(isEqualNoCase(xml.getTag(), "pubinfo"))
          {
             if(xml.firstChild())
             {
                do
                {
                    if(isEqualNoCase(xml.getTag(), "eventtype"))
                    {
                        if(xml.firstChild())
                        {
                            mEventType = xml.getValue().xmlCharDataDecode();
                            xml.parent();
                        }
                    }
                    else if(isEqualNoCase(xml.getTag(), "documentkey"))
                    {
                        if(xml.firstChild())
                        {
                            mDocumentKey = xml.getValue().xmlCharDataDecode();
                            xml.parent();
                        }
                    }
                    else if(isEqualNoCase(xml.getTag(), "etag"))
                    {
                        if(xml.firstChild())
                        {
                            mETag = xml.getValue().xmlCharDataDecode();
                            xml.parent();
                        }
                    }
                    else if(isEqualNoCase(xml.getTag(), "expires"))
                    {
                        if(xml.firstChild())
                        {
                           UInt64 expires = xml.getValue().convertUInt64();
                           mExpirationTime = (expires == 0 ? 0 : now + expires);
                           mLingerTime = mExpirationTime;
                           xml.parent();
                        }
                    }
                    else if(isEqualNoCase(xml.getTag(), "lastupdated"))
                    {
                        if(xml.firstChild())
                        {
                           mLastUpdated = now - xml.getValue().convertUInt64();
                           xml.parent();
                        }
                    }
                    else if(isEqualNoCase(xml.getTag(), "contentstype"))
                    {
                        if(xml.firstChild())
                        {
                            mimeTypeString = xml.getValue().xmlCharDataDecode();
                            xml.parent();
                        }
                    }
                    else if(isEqualNoCase(xml.getTag(), "contentssubtype"))
                    {
                        if(xml.firstChild())
                        {
                            mimeSubtypeString = xml.getValue().xmlCharDataDecode();
                            xml.parent();
                        }
                    }
                    else if(isEqualNoCase(xml.getTag(), "contents"))
                    {
                        if(xml.firstChild())
                        {
                            // There is an assumption that the contenttype and contentsubtype tags will always occur
                            // before the content tag
                            Mime mimeType(mimeTypeString, mimeSubtypeString);
                            // Need explilcit Data decodedBody as serializedContents refers to it in parser
                            Data decodedBody(xml.getValue().xmlCharDataDecode());
                            Contents* serializedContents = Contents::createContents(mimeType, decodedBody);
                            // Need to clone, as serializedContents is still referring to decodedBody, via the LazyParser,
                            // which will go out of scope.
                            mContents.reset(serializedContents->clone());
                            delete serializedContents;
                            serializedContents = 0;
                            xml.parent();
                            success = true;
                        }
                    }
                    else if (isEqualNoCase(xml.getTag(), "isencrypted"))
                    {
                       if (xml.firstChild())
                       {
                          if (mSecurityAttributes.get() == 0)
                          {
                             mSecurityAttributes.reset(new SecurityAttributes);
                          }
                          if (isEqualNoCase(xml.getValue(), "true"))
                          {
                             mSecurityAttributes->setEncrypted();
                          }
                          xml.parent();
                       }
                    }
                    else if (isEqualNoCase(xml.getTag(), "sigstatus"))
                    {
                       if (xml.firstChild())
                       {
                          if (mSecurityAttributes.get() == 0)
                          {
                             mSecurityAttributes.reset(new SecurityAttributes);
                          }
                          if (isEqualNoCase(xml.getValue(), "none"))
                          {
                             mSecurityAttributes->setSignatureStatus(SignatureNone);
                          }
                          else if (isEqualNoCase(xml.getValue(), "bad"))
                          {
                             mSecurityAttributes->setSignatureStatus(SignatureIsBad);
                          }
                          else if (isEqualNoCase(xml.getValue(), "trusted"))
                          {
                             mSecurityAttributes->setSignatureStatus(SignatureTrusted);
                          }
                          else if (isEqualNoCase(xml.getValue(), "catrusted"))
                          {
                             mSecurityAttributes->setSignatureStatus(SignatureCATrusted);
                          }
                          else if (isEqualNoCase(xml.getValue(), "nottrusted"))
                          {
                             mSecurityAttributes->setSignatureStatus(SignatureNotTrusted);
                          }
                          else if (isEqualNoCase(xml.getValue(), "selfsigned"))
                          {
                             mSecurityAttributes->setSignatureStatus(SignatureSelfSigned);
                          }
                          xml.parent();
                       }
                    }
                    else if (isEqualNoCase(xml.getTag(), "signer"))
                    {
                       if (xml.firstChild())
                       {
                          if (mSecurityAttributes.get() == 0)
                          {
                             mSecurityAttributes.reset(new SecurityAttributes);
                          }
                          mSecurityAttributes->setSigner(xml.getValue().xmlCharDataDecode());
                          xml.parent();
                       }
                    }
                    else if (isEqualNoCase(xml.getTag(), "identity"))
                    {
                       if (xml.firstChild())
                       {
                          if (mSecurityAttributes.get() == 0)
                          {
                             mSecurityAttributes.reset(new SecurityAttributes);
                          }
                          mSecurityAttributes->setIdentity(xml.getValue().xmlCharDataDecode());
                          xml.parent();
                       }
                    }
                    else if (isEqualNoCase(xml.getTag(), "identitystrength"))
                    {
                       if (xml.firstChild())
                       {
                          if (mSecurityAttributes.get() == 0)
                          {
                             mSecurityAttributes.reset(new SecurityAttributes);
                          }
                          if (isEqualNoCase(xml.getValue(), "from"))
                          {
                             mSecurityAttributes->setIdentityStrength(SecurityAttributes::From);
                          }
                          else if (isEqualNoCase(xml.getValue(), "failedidentity"))
                          {
                             mSecurityAttributes->setIdentityStrength(SecurityAttributes::FailedIdentity);
                          }
                          else if (isEqualNoCase(xml.getValue(), "identity"))
                          {
                             mSecurityAttributes->setIdentityStrength(SecurityAttributes::Identity);
                          }
                          xml.parent();
                       }
                    }

                } while(xml.nextSibling());
                xml.parent();
             }
          }

          return(success);
      }


      Data mEventType;
      Data mDocumentKey;
      Data mETag;
      UInt64 mExpirationTime;
      UInt64 mLastUpdated;
      UInt64 mLingerTime;      // No need to sync this - this is essentially the latest expiration time we have seen for this ETag
      SharedPtr<Contents> mContents;
      SharedPtr<SecurityAttributes> mSecurityAttributes;
      bool mSyncPublication;   // No need to sync this
   };

   typedef std::map<resip::Data, PubDocument> ETagToDocumentMap;
   typedef std::map<resip::Data, ETagToDocumentMap> KeyToETagMap;

   class ETagMerger
   {
   public:
       virtual ~ETagMerger() {}
       virtual bool mergeETag(Contents* eTagDest, Contents* eTagSrc, bool isFirst) = 0;
   };

   PublicationPersistenceManager() {}
   virtual ~PublicationPersistenceManager() {}

   virtual void addUpdateDocument(const PubDocument& document) = 0;
   virtual void addUpdateDocument(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 expirationTime, const Contents* contents, const SecurityAttributes* securityAttributes, bool syncPublication = false)
   {
      addUpdateDocument(PubDocument(eventType, documentKey, eTag, expirationTime, contents, securityAttributes, syncPublication));
   }
   virtual bool removeDocument(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated, bool syncPublication = false) = 0;
   virtual bool getMergedETags(const Data& eventType, const Data& documentKey, ETagMerger& merger, Contents* destination) = 0;
   virtual bool documentExists(const Data& eventType, const Data& documentKey, const Data& eTag) = 0;
   virtual bool getDocument(const resip::Data& eventType, const resip::Data& documentKey, const resip::Data& eTag, PubDocument& document) { return false; }
   virtual bool checkExpired(const Data& eventType, const Data& documentKey, const Data& eTag, UInt64 lastUpdated) = 0;
   virtual void lockDocuments() = 0;
   virtual KeyToETagMap& getDocuments() = 0;  // Ensure you lock before calling this and unlock when done
   virtual void unlockDocuments() = 0;
};
}

#endif

/* ====================================================================
*
* Copyright (c) 2015 SIP Spectrum, Inc.  All rights reserved.
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
* 3. Neither the name of the author(s) nor the names of any contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* ====================================================================
*
*/
/*
* vi: set shiftwidth=3 expandtab:
*/
