#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <time.h>
#include <iomanip>

#include "resip/stack/DialogInfoContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/XMLCursor.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const static Data BaseDialogInfoNamespaceUri("urn:ietf:params:xml:ns:dialog-info");

const static Data DefaultEncodeIndent("  ");

bool
DialogInfoContents::init()
{
   static ContentsFactory<DialogInfoContents> factory;
   (void)factory;
   return true;
}

static const char* DialogInfoStateStrings[] =
{
   "full",
   "partial"
};

const char* 
DialogInfoContents::dialogInfoStateToString(const DialogInfoState& dialogInfoState)
{
   return DialogInfoStateStrings[dialogInfoState];
}

DialogInfoContents::DialogInfoState 
DialogInfoContents::dialogInfoStateStringToEnum(const Data& dialogInfoStateString)
{
   for (int i = 0; i < MaxDialogInfoState; i++)
   {
      if (isEqualNoCase(DialogInfoStateStrings[i], dialogInfoStateString))
      {
         return (DialogInfoState)i;
      }
   }
   return MaxDialogInfoState;
}

static const char* DialogStateStrings[] =
{
   "trying",
   "proceeding",
   "early",
   "confirmed",
   "terminated"
};

const char* 
DialogInfoContents::dialogStateToString(const DialogState& dialogState)
{
   return DialogStateStrings[dialogState];
}

DialogInfoContents::DialogState 
DialogInfoContents::dialogStateStringToEnum(const Data& dialogStateString)
{
   for (int i = 0; i < MaxDialogState; i++)
   {
      if (isEqualNoCase(DialogStateStrings[i], dialogStateString))
      {
         return (DialogState)i;
      }
   }
   return MaxDialogState;
}

static const char* DialogStateEventStrings[] =
{
   "cancelled",
   "rejected",
   "replaced",
   "local-bye",
   "remote-bye",
   "error",
   "timeout"
};

const char* 
DialogInfoContents::dialogStateEventToString(const DialogStateEvent& dialogStateEvent)
{
   return DialogStateEventStrings[dialogStateEvent];
}

DialogInfoContents::DialogStateEvent
DialogInfoContents::dialogStateEventStringToEnum(const Data& dialogStateEventString)
{
   for (int i = 0; i < MaxOrUnsetDialogStateEvent; i++)
   {
      if (isEqualNoCase(DialogStateEventStrings[i], dialogStateEventString))
      {
         return (DialogStateEvent)i;
      }
   }
   return MaxOrUnsetDialogStateEvent;
}

static const char* DirectionStrings[] =
{
   "initiator",
   "recipient"
};

const char*
DialogInfoContents::directionToString(const Direction& direction)
{
   return DirectionStrings[direction];
}

DialogInfoContents::Direction
DialogInfoContents::directionStringToEnum(const Data& directionString)
{
   for (int i = 0; i < MaxOrUnsetDirection; i++)
   {
      if (isEqualNoCase(DirectionStrings[i], directionString))
      {
         return (Direction)i;
      }
   }
   return MaxOrUnsetDirection;
}

const DialogInfoContents DialogInfoContents::Empty;

DialogInfoContents::DialogInfoContents()
   : Contents(getStaticType()), mIndent(DefaultEncodeIndent), mVersion(0), mDialogInfoState(Partial)
{
}

DialogInfoContents::DialogInfoContents(const Mime& contentType)
   : Contents(getStaticType()), mIndent(DefaultEncodeIndent), mVersion(0), mDialogInfoState(Partial)
{
}

DialogInfoContents::DialogInfoContents(const HeaderFieldValue& hfv, const Mime& contentsType)
   : Contents(hfv, contentsType), mIndent(DefaultEncodeIndent), mVersion(0), mDialogInfoState(Partial)
{
}

DialogInfoContents::~DialogInfoContents()
{
}

Contents* 
DialogInfoContents::clone() const
{
   return new DialogInfoContents(*this);
}

const Mime& 
DialogInfoContents::getStaticType() 
{
   static Mime type("application","dialog-info+xml");
   return type;
}

EncodeStream& 
DialogInfoContents::encodeParsed(EncodeStream& str) const
{
   str << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << Symbols::CRLF;
   str << "<dialog-info xmlns=\"" << BaseDialogInfoNamespaceUri << "\"" << Symbols::CRLF;
   str << "             version=\"" << mVersion << "\" state=\"" << dialogInfoStateToString(mDialogInfoState) << "\"" << Symbols::CRLF;
   str << "             entity=\"" << Data::from(mEntity).xmlCharDataEncode() << "\">" << Symbols::CRLF;

   for (DialogList::const_iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
   {
      it->encodeParsed(str, mIndent);
   }

   str << "</dialog-info>" << Symbols::CRLF;

   return str;
}

bool 
DialogInfoContents::Dialog::getDialogElement(const Data& childElementName, Data& elementValue, int instance) const
{
   bool found = false;
   elementValue = "";
   std::pair <std::multimap<Data, Data>::const_iterator, std::multimap<Data, Data>::const_iterator> filteredList;
   filteredList = mExtraDialogElements.equal_range(childElementName);
   int matchCount = 0;
   for (std::multimap<Data, Data>::const_iterator matchListIterator = filteredList.first; matchListIterator != filteredList.second; ++matchListIterator)
   {
      if(instance == matchCount)
      {
         elementValue = matchListIterator->second;
         found = true;
         break;
      }
      matchCount++;
   }

   return(found);
}

EncodeStream& 
DialogInfoContents::Dialog::encodeParsed(EncodeStream& str, const Data& indent) const
{
   // Encode dialog attributes
   str << indent << "<dialog id=\"" << mId.xmlCharDataEncode() << "\"";
   if (!mCallId.empty())
   {
      str << " call-id=\"" << mCallId.xmlCharDataEncode() << "\"";
   }
   if (!mLocalTag.empty())
   {
      str << Symbols::CRLF << indent << "        local-tag=\"" << mLocalTag.xmlCharDataEncode() << "\"";
   }
   if (!mRemoteTag.empty())
   {
      str << Symbols::CRLF << indent << "        remote-tag=\"" << mRemoteTag.xmlCharDataEncode() << "\"";
   }
   if (mDirection != MaxOrUnsetDirection)
   {
      str << " direction=\"" << directionToString(mDirection) << "\"";
   }
   str << ">" << Symbols::CRLF;

   // Encode state element
   str << indent << indent << "<state";
   if (mStateEvent != MaxOrUnsetDialogStateEvent)
   {
      str << " event=\"" << dialogStateEventToString(mStateEvent) << "\"";
   }
   if (mStateCode != 0)
   {
      str << " code=\"" << mStateCode << "\"";
   }
   str << ">" << dialogStateToString(mState) << "</state>" << Symbols::CRLF;

   // Encode duration element
   if (mHasDuration)
   {
      str << indent << indent << "<duration>" << mDuration << "</duration>" << Symbols::CRLF;
   }

   // Encode replaces element
   if (!mReplacesCallId.empty())
   {
      str << indent << indent << "<replaces call-id=\"" << mReplacesCallId.xmlCharDataEncode() << "\"" << Symbols::CRLF;
      str << indent << indent << "          local-tag=\"" << mReplacesLocalTag.xmlCharDataEncode() << "\" remote-tag=\"" << mReplacesRemoteTag.xmlCharDataEncode() << "\"/>" << Symbols::CRLF;
   }

   // Encode referred by element
   if (!mReferredBy.uri().host().empty())
   {
      str << indent << indent;
      encodeNameAddrElement(str, "referred-by", mReferredBy);
      str << Symbols::CRLF;
   }

   // Encode route set
   if (!mRouteSet.empty())
   {
      str << indent << indent << "<route-set>" << Symbols::CRLF;
      for (NameAddrs::const_iterator itNA = mRouteSet.begin(); itNA != mRouteSet.end(); itNA++)
      {
         str << indent << indent << indent << "<hop>" << Data::from(itNA->uri()).xmlCharDataEncode() << "</hop>" << Symbols::CRLF;
      }
      str << indent << indent << "</route-set>" << Symbols::CRLF;
   }

   // Encode local participant (if set)
   mLocalParticipant.encode(str, "local", indent);

   // Encode remote participant (if set)
   mRemoteParticipant.encode(str, "remote", indent);

   // User specific/non-standard Dialog elements
   for(std::multimap<Data, Data>::const_iterator elementIterator = mExtraDialogElements.begin(); 
       elementIterator != mExtraDialogElements.end(); 
       elementIterator++)
   {
      DebugLog(<< "Dialog child element Name: \"" << elementIterator->first << "\" Value: \"" << elementIterator->second << "\"");
      str << indent << indent << '<' << elementIterator->first << '>'
          << elementIterator->second.xmlCharDataEncode()
          << "</" << elementIterator->first << '>' << Symbols::CRLF;
   }

   str << indent << "</dialog>" << Symbols::CRLF;

   return str;
}

EncodeStream& 
DialogInfoContents::encodeNameAddrElement(EncodeStream& str, const char* elementName, const NameAddr& nameAddr)
{
   str << "<" << elementName;
   if (!nameAddr.displayName().empty())
   {
      str << " display=\"" << nameAddr.displayName().xmlCharDataEncode() << "\"";
   }
   str << ">" << Data::from(nameAddr.uri()).xmlCharDataEncode() << "</" << elementName << ">";
   return str;
}

void 
DialogInfoContents::Dialog::Participant::setTarget(const NameAddr& targetWithContactParams) 
{ 
   mTarget = targetWithContactParams.uri(); 
   Data paramsData;
   {  // scope so that stream flushs
      DataStream dstr(paramsData);
      targetWithContactParams.encodeParameters(dstr);
   }
   // If there are parameters present - parse them
   if (!paramsData.empty())
   {
      ParseBuffer pb(paramsData);
      pb.skipChar();  // Skip first ; that is always present
      pb.skipWhitespace();

      do
      {
         const char* anchor = pb.position();
         pb.skipToOneOf("=;");
         if (*pb.position() == '=')
         {
            Data name;
            Data value;
            pb.data(name, anchor);
            pb.skipChar();
            pb.skipWhitespace();
            if (*pb.position() == '"')
            {
               // Quoted Value
               pb.skipChar();
               anchor = pb.position();
               pb.skipToChar('"');
               value = pb.data(anchor);
               pb.skipChar();
               pb.skipToChar(';');
            }
            else
            {
               // Unquoted value
               anchor = pb.position();
               pb.skipToChar(';');
               value = pb.data(anchor);
            }
            mTargetParams[name] = value;
         }
         else // ';' or eof
         {
            // No equals operator - this is a boolean param
            mTargetParams[pb.data(anchor)] = "true";
            pb.skipToChar(';');
         }
         if (!pb.eof())
         {
            // Skip ;
            pb.skipChar();
         }
      } while (!pb.eof());
   }
}

bool 
DialogInfoContents::Dialog::Participant::getTargetParam(const Data& name, Data& value) const
{
   TargetParams::const_iterator it = mTargetParams.find(name);
   if (it != mTargetParams.end())
   {
      value = it->second;
      return true;
   }
   return false;
}

EncodeStream& 
DialogInfoContents::Dialog::Participant::encode(EncodeStream& str, const char* baseElementName, const Data& indent) const
{
   // Only encode if any of the optional sub elements are actually set
   if (!mIdentity.uri().host().empty() || !mTarget.host().empty() || !mSessionDescription.empty() || mHasCSeq)
   {
      str << indent << indent << "<" << baseElementName << ">" << Symbols::CRLF;
      if (!mIdentity.uri().host().empty())
      {
         str << indent << indent << indent;
         encodeNameAddrElement(str, "identity", mIdentity);
         str << Symbols::CRLF;
      }
      if (!mTarget.host().empty())
      {
         str << indent << indent << indent << "<target uri=\"" << Data::from(mTarget).xmlCharDataEncode() << "\"";
         if (mTargetParams.empty())
         {
            // no params - just close tag
            str << "/>" << Symbols::CRLF;
         }
         else
         {
            str << ">" << Symbols::CRLF;
            for (TargetParams::const_iterator itParm = mTargetParams.begin(); itParm != mTargetParams.end(); itParm++)
            {
               str << indent << indent << indent << indent << "<param pname=\"" << itParm->first.xmlCharDataEncode() << "\" pval=\"" << itParm->second.xmlCharDataEncode() << "\"/>" << Symbols::CRLF;
            }
            str << indent << indent << indent << "</target>" << Symbols::CRLF;
         }
      }
      if (!mSessionDescription.empty())
      {
         // SDP is multiline, so whitespace is important - don't format for "pretty" output
         str << indent << indent << indent << "<session-description type=\"" << mSessionDescriptionType << "\">" << mSessionDescription.xmlCharDataEncode() << "</session-description>" << Symbols::CRLF;
      }
      if (mHasCSeq)
      {
         str << indent << indent << indent << "<cseq>" << mCSeq << "</cseq>" << Symbols::CRLF;
      }
      str << indent << indent << "</" << baseElementName << ">" << Symbols::CRLF;
   }
   return str;
}

void
DialogInfoContents::parse(ParseBuffer& pb)
{
   XMLCursor xml(pb);
   const XMLCursor::AttributeMap& attr = xml.getAttributes();
   XMLCursor::AttributeMap::const_iterator itAttr = attr.begin();
   bool baseDialogInfoNamespaceUriFound = false;
   for (; itAttr != attr.end(); itAttr++)
   {
      if (itAttr->first.prefix("xmlns"))
      {
         Data prefix;
         ParseBuffer pb(itAttr->first);
         pb.skipToChar(Symbols::COLON[0]);
         if (!pb.eof())
         {
            pb.skipChar();
            const char* anchor = pb.position();
            pb.skipToEnd();
            pb.data(prefix, anchor);
            prefix += Symbols::COLON;
         }
         if (isEqualNoCase(itAttr->second, BaseDialogInfoNamespaceUri))
         {
            baseDialogInfoNamespaceUriFound = true;
         }
      }
      else if (itAttr->first == "version")
      {
         mVersion = itAttr->second.convertUnsignedLong();
      }
      else if (itAttr->first == "state")
      {
         mDialogInfoState = dialogInfoStateStringToEnum(itAttr->second);
      }
      else if (itAttr->first == "entity")
      {
         mEntity = Uri(itAttr->second.xmlCharDataDecode());  // can throw!
      }
      else
      {
         DebugLog(<< "Unknown root attribute: " << itAttr->first << "=" << itAttr->second);
      }
   }

   if (!baseDialogInfoNamespaceUriFound)
   {
      WarningLog(<< "Base xmlns from RFC4235 was not found, expected: " << BaseDialogInfoNamespaceUri);
      // ?slg? - throw or be tolerant?
   }

   if (xml.firstChild())
   {
      do
      {
         if (xml.getTag() == "dialog")
         {
            parseDialog(xml);
         }
         else
         {
            DebugLog(<< "Unknown root element: " << xml.getTag());
         }
      } while (xml.nextSibling());
      xml.parent();
   }
}

void 
DialogInfoContents::parseDialog(XMLCursor& xml)
{
   const XMLCursor::AttributeMap& attr = xml.getAttributes();
   XMLCursor::AttributeMap::const_iterator itAttr = attr.begin();
   Dialog dialog;
   for (; itAttr != attr.end(); itAttr++)
   {
      if (itAttr->first == "id")
      {
         dialog.mId = itAttr->second.xmlCharDataDecode();
      }
      else if (itAttr->first == "call-id")
      {
         dialog.mCallId = itAttr->second.xmlCharDataDecode();
      }
      else if (itAttr->first == "local-tag")
      {
         dialog.mLocalTag = itAttr->second.xmlCharDataDecode();
      }
      else if (itAttr->first == "remote-tag")
      {
         dialog.mRemoteTag = itAttr->second.xmlCharDataDecode();
      }
      else if (itAttr->first == "direction")
      {
         dialog.mDirection = directionStringToEnum(itAttr->second);
      }
      else
      {
         DebugLog(<< "Unknown dialog attribute: " << itAttr->first << "=" << itAttr->second);
      }
   }

   if (dialog.mId.empty())
   {
      WarningLog(<< "Dialog Id was not found for dialog element");
      // ?slg? - throw or be tolerant?
   }

   // Clear out any remnent parsed dialog child elements
   dialog.mExtraDialogElements.clear();

   if (xml.firstChild())
   {
      do
      {
         if (xml.getTag() == "state")
         {
            const XMLCursor::AttributeMap& attr = xml.getAttributes();
            for (itAttr = attr.begin(); itAttr != attr.end(); itAttr++)
            {
               if (itAttr->first == "event")
               {
                  dialog.mStateEvent = dialogStateEventStringToEnum(itAttr->second);
               }
               else if (itAttr->first == "code")
               {
                  dialog.mStateCode = itAttr->second.convertInt();
               }
               else
               {
                  DebugLog(<< "Unknown state attribute: " << itAttr->first << "=" << itAttr->second);
               }
            }
            if (xml.firstChild())
            {
               dialog.mState = dialogStateStringToEnum(xml.getValue());
               xml.parent();
            }
         }
         else if (xml.getTag() == "duration")
         {
            if (xml.firstChild())
            {
               dialog.mDuration = xml.getValue().convertUnsignedLong();
               dialog.mHasDuration = true;
               xml.parent();
            }
         }
         else if (xml.getTag() == "replaces")
         {
            const XMLCursor::AttributeMap& attr = xml.getAttributes();
            for (itAttr = attr.begin(); itAttr != attr.end(); itAttr++)
            {
               if (itAttr->first == "call-id")
               {
                  dialog.mReplacesCallId = itAttr->second.xmlCharDataDecode();
               }
               else if (itAttr->first == "local-tag")
               {
                  dialog.mReplacesLocalTag = itAttr->second.xmlCharDataDecode();
               }
               else if (itAttr->first == "remote-tag")
               {
                  dialog.mReplacesRemoteTag = itAttr->second.xmlCharDataDecode();
               }
               else
               {
                  DebugLog(<< "Unknown dialog/replaces attribute: " << itAttr->first << "=" << itAttr->second);
               }
            }
         }
         else if (xml.getTag() == "referred-by")
         {
            parseNameAddrElement(xml, dialog.mReferredBy);
         }
         else if (xml.getTag() == "route-set")
         {
            if (xml.firstChild())
            {
               do
               {
                  if (xml.getTag() == "hop")
                  {
                     NameAddr nameAddr;
                     if (parseUriValue(xml, nameAddr.uri()))
                     {
                        dialog.mRouteSet.push_back(nameAddr);
                     }
                  }
                  else
                  {
                     DebugLog(<< "Unknown dialog/route-set element: " << xml.getTag());
                  }
               } while (xml.nextSibling());
               xml.parent();
            }
         }
         else if (xml.getTag() == "local")
         {
            dialog.mLocalParticipant.parse(xml);
         }
         else if (xml.getTag() == "remote")
         {
            dialog.mRemoteParticipant.parse(xml);
         }
         else
         {
            Data elementName = xml.getTag();
            if (xml.firstChild())
            {
               DebugLog(<< "Unknown dialog element: " << elementName << " value: " << xml.getValue().xmlCharDataDecode());
               dialog.addDialogElement(elementName, xml.getValue().xmlCharDataDecode());
               xml.parent();
            }
            else
            {
               DebugLog(<< "Unknown dialog element: " << elementName);
            }
         }
      } while (xml.nextSibling());
      xml.parent();
   }

   //DebugLog(<< "Pushing " << mDialogs.size() << "'th dialog " << dialog.mId << " parsed into DialogInfoCOntents::mDialogs");
   mDialogs.push_back(dialog);
}

bool
DialogInfoContents::parseUriValue(XMLCursor& xml, Uri& uri)
{
   bool parseGood = false;
   if (xml.firstChild())
   {
      try
      {
         uri = Uri(xml.getValue().xmlCharDataDecode());
         parseGood = true;
      }
      catch (BaseException& ex)  // ?slg? - catch here or not - let exception bubble up? or rethrow?
      {
         DebugLog(<< "Could not parse NameAddr value: " << xml.getValue().xmlCharDataDecode() << ": " << ex);
      }
      xml.parent();
   }
   return parseGood;
}

bool 
DialogInfoContents::parseNameAddrElement(XMLCursor& xml, NameAddr& nameAddr)
{
   const XMLCursor::AttributeMap& attr = xml.getAttributes();
   XMLCursor::AttributeMap::const_iterator itAttr = attr.begin();
   for (; itAttr != attr.end(); itAttr++)
   {
      if (itAttr->first == "display")
      {
         nameAddr.displayName() = itAttr->second.xmlCharDataDecode();
      }
      else
      {
         DebugLog(<< "Unknown NameAddr attribute: " << itAttr->first << "=" << itAttr->second);
      }
   }
   return parseUriValue(xml, nameAddr.uri());
}

void
DialogInfoContents::Dialog::Participant::parse(XMLCursor& xml)
{
   if (xml.firstChild())
   {
      do
      {
         if (xml.getTag() == "identity")
         {
            parseNameAddrElement(xml, mIdentity);
         }
         else if (xml.getTag() == "target")
         {
            try
            {
               const XMLCursor::AttributeMap& attr = xml.getAttributes();
               XMLCursor::AttributeMap::const_iterator itAttr = attr.begin();
               for (; itAttr != attr.end(); itAttr++)
               {
                  if (itAttr->first == "uri")
                  {
                     mTarget = Uri(itAttr->second.xmlCharDataDecode());
                  }
                  else
                  {
                     DebugLog(<< "Unknown dialog/participant/target attribute: " << itAttr->first << "=" << itAttr->second);
                  }
               }

               if (xml.firstChild())
               {
                  do
                  {
                     if (xml.getTag() == "param")
                     {
                        parseParam(xml);
                     }
                     else
                     {
                        DebugLog(<< "Unknown dialog/particpant/target element: " << xml.getTag());
                     }
                  } while (xml.nextSibling());
                  xml.parent();
               }
            }
            catch (BaseException& ex)  // ?slg? - catch here or not - let exception bubble up? or rethrow?
            {
               DebugLog(<< "Could not parse dialog/participatn/target uri value: " << xml.getValue().xmlCharDataDecode() << ": " << ex);
            }
         }
         else if (xml.getTag() == "session-description")
         {
            const XMLCursor::AttributeMap& attr = xml.getAttributes();
            XMLCursor::AttributeMap::const_iterator itAttr = attr.begin();
            for (; itAttr != attr.end(); itAttr++)
            {
               if (itAttr->first == "type")
               {
                  mSessionDescriptionType = itAttr->second.xmlCharDataDecode();
               }
               else
               {
                  DebugLog(<< "Unknown dialog/participant/session-description attribute: " << itAttr->first << "=" << itAttr->second);
               }
            }
            if (xml.firstChild())
            {
               mSessionDescription = xml.getValue().xmlCharDataDecode();
               xml.parent();
            }
         }
         else if (xml.getTag() == "cseq")
         {
            if (xml.firstChild())
            {
               mCSeq = xml.getValue().convertUnsignedLong();
               mHasCSeq = true;
               xml.parent();
            }
         }
         else
         {
            DebugLog(<< "Unknown dialog participant element: " << xml.getTag());
         }
      } while (xml.nextSibling());
      xml.parent();
   }
}

void 
DialogInfoContents::Dialog::Participant::parseParam(XMLCursor& xml)
{
   const XMLCursor::AttributeMap& attr = xml.getAttributes();
   XMLCursor::AttributeMap::const_iterator itAttr = attr.begin();
   Data name;
   Data value;
   for (; itAttr != attr.end(); itAttr++)
   {
      if (itAttr->first == "pname")
      {
         name = itAttr->second.xmlCharDataDecode();
      }
      else if (itAttr->first == "pval")
      {
         value = itAttr->second.xmlCharDataDecode();
      }
      else
      {
         DebugLog(<< "Unknown dialog/participant/target/param attribute: " << itAttr->first << "=" << itAttr->second);
      }
   }
   if (!name.empty())
   {
      mTargetParams[name] = value;
   }
}

void 
DialogInfoContents::addDialog(const Dialog& dialog)
{
   checkParsed();
   // Ensure id doesn't already exist
   removeDialog(dialog.mId);
   mDialogs.push_back(dialog);
}

bool 
DialogInfoContents::removeDialog(const Data& id) 
{ 
   checkParsed();
   for (DialogList::iterator it = mDialogs.begin(); it != mDialogs.end(); it++)
   {
      if (it->mId == id)
      {
         mDialogs.erase(it);
         return true;
      }
   }
   return false;
}


/* ====================================================================
*
* Copyright (c) 2016 SIP Spectrum, Inc.  All rights reserved.
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
