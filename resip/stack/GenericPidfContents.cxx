#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <time.h>
#include <iomanip>

#include "resip/stack/GenericPidfContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/XMLCursor.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const static Data BasePidfNamespaceUri("urn:ietf:params:xml:ns:pidf");

bool
GenericPidfContents::init()
{
   static ContentsFactory<GenericPidfContents> factory;
   (void)factory;
   return true;
}

const GenericPidfContents GenericPidfContents::Empty;

GenericPidfContents::GenericPidfContents()
   : Contents(getStaticType()), mOnline(false), mSimplePresenceExtracted(false)
{
}

GenericPidfContents::GenericPidfContents(const Mime& contentType)
   : Contents(getStaticType()), mOnline(false), mSimplePresenceExtracted(false)
{
}

GenericPidfContents::GenericPidfContents(const HeaderFieldValue& hfv, const Mime& contentsType)
   : Contents(hfv, contentsType), mOnline(false), mSimplePresenceExtracted(false)
{
}

GenericPidfContents&
GenericPidfContents::operator=(const GenericPidfContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);

      // clear any data then merge in new stuff
      reset();
      mergeNoCheckParse(rhs);
   }
   return *this;
}
 
GenericPidfContents::GenericPidfContents(const GenericPidfContents& rhs)
   : Contents(rhs), mOnline(false), mSimplePresenceExtracted(false)
{
   // merge in new stuff
   mergeNoCheckParse(rhs);
}

GenericPidfContents::~GenericPidfContents()
{
   reset();
}

void 
GenericPidfContents::reset()
{
   // Cleanup Node Memory recursively
   cleanupNodeMemory(mRootNodes);

   // Clear namespace map
   mNamespaces.clear();

   mRootPidfNamespacePrefix.clear();
   mEntity.host().clear();
   mEntity.user().clear();

   clearSimplePresenceInfo();
}

void GenericPidfContents::clearSimplePresenceInfo()
{
   mTupleId.clear();
   mOnline = false;
   mTimestamp.clear();
   mNote.clear();
   mContact.clear();
   mContactPriority.clear();
   mSimplePresenceExtracted = false;
}

void
GenericPidfContents::cleanupNodeMemory(NodeList& nodeList)
{
   // Cleanup Node Memory recursively
   NodeList::iterator itNode = nodeList.begin();
   for (; itNode != nodeList.end(); itNode++)
   {
      cleanupNodeMemory((*itNode)->mChildren);
      delete *itNode;
   }
   nodeList.clear();
}

Contents* 
GenericPidfContents::clone() const
{
   return new GenericPidfContents(*this);
}

const Mime& 
GenericPidfContents::getStaticType() 
{
   static Mime type("application","pidf+xml");
   return type;
}

EncodeStream& 
GenericPidfContents::encodeParsed(EncodeStream& str) const
{
   str << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << Symbols::CRLF;
   str << "<" << mRootPidfNamespacePrefix << "presence ";
   NamespaceMap::const_iterator itNs = mNamespaces.begin();
   bool first = true;
   for (; itNs != mNamespaces.end(); itNs++)
   {
      if (first)
      {
         first = false;
         str << "xmlns";
      }
      else
      {
         str << "          xmlns";
      }
      if (!itNs->second.empty())  // Check if prefix is not-empty
      {
         str << ":" << itNs->second.substr(0, itNs->second.size() - 1);  // remove trailing ":"
      }
      str << "=\"" << itNs->first << "\"" << Symbols::CRLF;
   }
   str << "        entity=\"" << mEntity << "\">" << Symbols::CRLF;
   NodeList::const_iterator itNode = mRootNodes.begin();
   Data indent("  ");
   for (; itNode != mRootNodes.end(); itNode++)
   {
      (*itNode)->encode(str, indent);
   }
   str << "</" << mRootPidfNamespacePrefix << "presence>" << Symbols::CRLF;

   return str;
}

void
GenericPidfContents::parse(ParseBuffer& pb)
{
   mSimplePresenceExtracted = false;

   XMLCursor xml(pb);
   const XMLCursor::AttributeMap& attr = xml.getAttributes();
   XMLCursor::AttributeMap::const_iterator itAttr = attr.begin();
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
         if (isEqualNoCase(itAttr->second, BasePidfNamespaceUri))
         {
            mRootPidfNamespacePrefix = prefix;
         }

         mNamespaces[itAttr->second] = prefix;
      }
      else if (itAttr->first == "entity")
      {
         mEntity = Uri(itAttr->second);  // can throw!
      }
      else
      {
         DebugLog(<< "Unknown root attribute: " << itAttr->first << "=" << itAttr->second);
      }
   }

   // Ensure root presence node is present
   if (xml.getTag() == mRootPidfNamespacePrefix + Symbols::Presence)
   {
      if (xml.firstChild())
      {
         do
         {
            parseChildren(xml, mRootNodes);
         } while (xml.nextSibling());
         xml.parent();
      }
   }
   else
   {
      // TODO Throw?
      DebugLog(<< "Aborting parse, root presence node missing: " << mRootPidfNamespacePrefix + Symbols::Presence);
   }
}

void 
GenericPidfContents::parseChildren(XMLCursor& xml, NodeList& nodeList)
{
   Node* node = new Node();
   node->mAttributes = xml.getAttributes(); // !slg! yuck - yes we are copying memory for attributes
   node->mValue.duplicate(xml.getValue());  // use Data::duplicate to avoid copying memory
   ParseBuffer pb(xml.getTag());
   const char* anchor = pb.position();
   pb.skipToChar(Symbols::COLON[0]);
   if (!pb.eof())
   {
      pb.skipChar();
      pb.data(node->mNamespacePrefix, anchor);
      anchor = pb.position();
      pb.skipToEnd();
      pb.data(node->mTag, anchor);
   }
   else
   {
      // No namespace prefix
      node->mTag.duplicate(xml.getTag()); // use Data::duplicate to avoid copying memory
   }

   if (node->mValue.empty() && xml.firstChild())
   {
      do
      {
         if (!xml.getValue().empty())
         {
            node->mValue.duplicate(xml.getValue()); // use Data::duplicate to avoid copying memory
         }
         else
         {
            parseChildren(xml, node->mChildren);
         }
      } while (xml.nextSibling());
      xml.parent();
   }
   nodeList.push_back(node);
}

EncodeStream&
GenericPidfContents::Node::encodeAttributes(EncodeStream& str) const
{
   AttributeMap::const_iterator itAttrib = mAttributes.begin();
   for (; itAttrib != mAttributes.end(); itAttrib++)
   {
      str << " " << itAttrib->first << "=\"" << itAttrib->second << "\"";
   }
   return str;
}

EncodeStream&
GenericPidfContents::Node::encode(EncodeStream& str, Data indent) const
{
   if (!mTag.empty())
   {
      if (mChildren.size() == 0)
      {
         if (mValue.empty())
         {
            str << indent << "<" << mNamespacePrefix << mTag;
            encodeAttributes(str);
            str << "/>" << Symbols::CRLF;
         }
         else
         {
            str << indent << "<" << mNamespacePrefix << mTag;
            encodeAttributes(str);
            str << ">" << mValue << "</" << mNamespacePrefix << mTag << ">" << Symbols::CRLF;
         }
      }
      // The following else collapses simple single nodes (no attributes or values) to one line: ie:
      // <outernode></innernode></outernode>
      else if (mChildren.size() == 1 && mAttributes.empty() &&
               mChildren.front()->mValue.empty() && mChildren.front()->mAttributes.empty() && mChildren.front()->mChildren.size() == 0)
      {
         str << indent << "<" << mNamespacePrefix << mTag << "><" << mChildren.front()->mNamespacePrefix;
         str << mChildren.front()->mTag << "/></" << mNamespacePrefix << mTag << ">" << Symbols::CRLF;
      }
      else
      {
         str << indent << "<" << mNamespacePrefix << mTag;
         encodeAttributes(str);
         str << ">" << Symbols::CRLF;
         NodeList::const_iterator itNode = mChildren.begin();
         for (; itNode != mChildren.end(); itNode++)
         {
            (*itNode)->encode(str, indent + "  ");
         }
         str << indent << "</" << mNamespacePrefix << mTag << ">" << Symbols::CRLF;
      }
   }
   return str;
}

void 
GenericPidfContents::Node::copy(const Node& rhs, HashMap<Data, Data>* namespacePrefixCorrections)
{
   if (namespacePrefixCorrections)
   {
      // Check if namespace should be corrected
      HashMap<Data, Data>::iterator itNsCorr = namespacePrefixCorrections->find(rhs.mNamespacePrefix);
      if (itNsCorr != namespacePrefixCorrections->end())
      {
         mNamespacePrefix = itNsCorr->second;
      }
      else
      {
         mNamespacePrefix = rhs.mNamespacePrefix;
      }
   }
   else
   {
      mNamespacePrefix = rhs.mNamespacePrefix;
   }
   mTag = rhs.mTag;
   mAttributes = rhs.mAttributes;
   mValue = rhs.mValue;
   NodeList::const_iterator itNode = rhs.mChildren.begin();
   for (; itNode != rhs.mChildren.end(); itNode++)
   {
      Node* node = new Node();
      node->copy(*(*itNode), namespacePrefixCorrections);
      mChildren.push_back(node);
   }
}

bool 
GenericPidfContents::merge(const GenericPidfContents& other)
{
   // Ensure both sides are parsed if not already
   checkParsed();
   other.checkParsed();
   return mergeNoCheckParse(other);
}

const Data&
GenericPidfContents::getSubNodeValue(Node* node, const Data& tag)
{
   NodeList::iterator it = node->mChildren.begin();
   for (; it != node->mChildren.end(); it++)
   {
      if ((*it)->mTag == tag)
      {
         return (*it)->mValue;
      }
   }
   return Data::Empty;
}

bool 
GenericPidfContents::mergeNoCheckParse(const GenericPidfContents& other)
{
   mSimplePresenceExtracted = false;

   // Validate entity user and host - we allow mismatched schemes
   if (mEntity.host().empty())
   {
      mEntity = other.mEntity;
   }
   else if(mEntity.user() != other.mEntity.user() ||
           mEntity.host() != other.mEntity.host())
   {
      DebugLog(<< "Merge failed, entities do not match: " << mEntity << ", other=" << other.mEntity);
      return false;
   }

   HashMap<Data, Data> namespacePrefixCorrections;  // other/old prefix name, new/dest prefix name

   // Copy over namespaces - looking for mismatched prefixes
   bool checkNamespaceMismatches = mNamespaces.size() > 0;
   NamespaceMap::const_iterator itOtherNs = other.mNamespaces.begin();
   for(; itOtherNs != other.mNamespaces.end(); itOtherNs++)
   {
      // Check if namespace is already in list and if so - verify prefix will match
      bool found = false;
      if (checkNamespaceMismatches)
      {
         NamespaceMap::iterator itNs = mNamespaces.find(itOtherNs->first);
         if (itNs != mNamespaces.end())
         {
            if (itNs->second != itOtherNs->second)
            {
               // Prefix used for same namespace does not match
               namespacePrefixCorrections[itOtherNs->second] = itNs->second;
            }
            found = true;
         }
      }
      if(!found)
      {
         mNamespaces[itOtherNs->first] = itOtherNs->second;  // Copy over
      }
   }
   // If we didn't check for namespace mismatches then we didn't have any namespaces
   // to start with, which means we didn't have a root namespace - set it now
   if (!checkNamespaceMismatches)
   {
      mRootPidfNamespacePrefix = other.mRootPidfNamespacePrefix;
   }

   // Merge root presence nodes
   bool checkMatches = mRootNodes.size() > 0;
   NodeList::const_iterator itOtherNode = other.mRootNodes.begin();
   for(; itOtherNode != other.mRootNodes.end(); itOtherNode++)
   {
      // If there is an ID attribute then see if tag/id combo lives already in local list
      bool matchFound = false;
      if (checkMatches)
      {
         Node::AttributeMap::iterator itOtherAttrib = (*itOtherNode)->mAttributes.find("id");
         if (itOtherAttrib != (*itOtherNode)->mAttributes.end())
         {
            NodeList::iterator itNode = mRootNodes.begin();
            for (; itNode != mRootNodes.end(); itNode++)
            {
               if ((*itNode)->mTag == (*itOtherNode)->mTag)
               {
                  // Node found - check for id match
                  Node::AttributeMap::iterator itAttrib = (*itNode)->mAttributes.find("id");
                  if (itAttrib != (*itNode)->mAttributes.end())
                  {
                     if (itOtherAttrib->second == itAttrib->second)  // Check if Id's match
                     {
                        // compare timestamps
                        const Data& ts1 = getSubNodeValue((*itNode), "timestamp");
                        const Data& ts2 = getSubNodeValue((*itOtherNode), "timestamp");
                        // Note: to compare timestamps we use a string compare and rely on properties from RFC3339 (section 5.1) with
                        // the assumption that same id items will generate timestamps in the same timezone and timezone format
                        if (ts1.empty() || ts2.empty() || ts2 >= ts1)  
                        {
                           cleanupNodeMemory((*itNode)->mChildren);
                           (*itNode)->copy(*(*itOtherNode), &namespacePrefixCorrections);
                        }
                        matchFound = true;
                        break;
                     }
                  }
               }
            }
         }
      }
      if (!matchFound)
      {
         Node* node = new Node();
         node->copy(*(*itOtherNode), namespacePrefixCorrections.size() > 0 ? &namespacePrefixCorrections : 0);
         mRootNodes.push_back(node);
      }
   }
   return true;
}

void
GenericPidfContents::setEntity(const Uri& entity)
{
   checkParsed();
   mEntity = entity;
}

const Uri&
GenericPidfContents::getEntity() const
{
   checkParsed();
   return mEntity;
}

void 
GenericPidfContents::addNamespace(const Data& uri, const Data& prefix)
{
   checkParsed();
   Data adjustedPrefix(prefix);
   // Add colon to end if missing if prefix was provided
   if (!prefix.empty() && !prefix.postfix(Symbols::COLON))
   {
      adjustedPrefix += Symbols::COLON;
   }
   if (isEqualNoCase(uri, BasePidfNamespaceUri))
   {
      mRootPidfNamespacePrefix = adjustedPrefix;
   }
   mNamespaces[uri] = adjustedPrefix;
}

void 
GenericPidfContents::setSimplePresenceTupleNode(const Data& id,
                                                bool online,
                                                const Data& timestamp,
                                                const Data& note,
                                                const Data& contact,
                                                const Data& contactPriority)
{
   if (mNamespaces.empty())
   {
      // Add default namespace
      addNamespace(BasePidfNamespaceUri, Data::Empty);  // no custom prefix
   }

   // See if node exists and if so delete it - otherwise add new
   bool foundExisting = false;
   NodeList::iterator it = mRootNodes.begin();
   for(; it != mRootNodes.end(); it++)
   {
       if((*it)->mTag == "tuple")
       {
           Node::AttributeMap::iterator itAttrib = (*it)->mAttributes.find("id");
           if(itAttrib != (*it)->mAttributes.end())
           {
               if(itAttrib->second == id)
               {
                   foundExisting = true;
                   break;
               }
           }
       }
   }

   Node* tupleNode;
   if(foundExisting)
   {
      // Remove all children and add back, below
      cleanupNodeMemory((*it)->mChildren);
      tupleNode = (*it);
   }
   else
   {
      tupleNode = new Node();
      tupleNode->mNamespacePrefix = mRootPidfNamespacePrefix;
      tupleNode->mTag = "tuple";
      tupleNode->mAttributes["id"] = id;
   }

   // Add Status Node with Basic subnode
   Node* statusNode = new Node();
   statusNode->mNamespacePrefix = mRootPidfNamespacePrefix;
   statusNode->mTag = "status";
   Node* basicNode = new Node();
   basicNode->mNamespacePrefix = mRootPidfNamespacePrefix;
   basicNode->mTag = "basic";
   basicNode->mValue = online ? "open" : "closed";
   statusNode->mChildren.push_back(basicNode);
   tupleNode->mChildren.push_back(statusNode);

   // Add Note node if required
   if (!note.empty())
   {
      Node* noteNode = new Node();
      noteNode->mNamespacePrefix = mRootPidfNamespacePrefix;
      noteNode->mTag = "note";
      noteNode->mValue = note;
      tupleNode->mChildren.push_back(noteNode);
   }

   // Add Contact node if required
   if (!contact.empty())
   {
      Node* contactNode = new Node();
      contactNode->mNamespacePrefix = mRootPidfNamespacePrefix;
      contactNode->mTag = "contact";
      contactNode->mValue = contact;
      if (!contactPriority.empty())
      {
         contactNode->mAttributes["priority"] = contactPriority;
      }
      tupleNode->mChildren.push_back(contactNode);
   }

   // Add Timestamp node if required
   if (!timestamp.empty())
   {
      Node* timestampNode = new Node();
      timestampNode->mNamespacePrefix = mRootPidfNamespacePrefix;
      timestampNode->mTag = "timestamp";
      timestampNode->mValue = timestamp;
      tupleNode->mChildren.push_back(timestampNode);
   }

   if (!foundExisting)
   {
      mRootNodes.push_back(tupleNode);
   }

   // store info in members - note if you add simplePresence with different id's then 
   // detail is lost here
   mTupleId = id;
   mOnline = online;
   mTimestamp = timestamp;
   mNote = note;
   mContact = contact;
   mContactPriority = contactPriority;
   mSimplePresenceExtracted = true;
}

void 
GenericPidfContents::extractSimplePresenceInfo()
{
   if (!mSimplePresenceExtracted)
   {
      clearSimplePresenceInfo();

      // Iterate through root nodes and find first tuple
      NodeList::const_iterator itNode = mRootNodes.begin();
      for (; itNode != mRootNodes.end(); itNode++)
      {
         if ((*itNode)->mTag == "tuple")
         {
            Node::AttributeMap::iterator itAttrib = (*itNode)->mAttributes.find("id");
            if (itAttrib != (*itNode)->mAttributes.end())
            {
               mTupleId = itAttrib->second;
               // iterate through children looking for nodes we want
               NodeList::const_iterator itTupleChild = (*itNode)->mChildren.begin();
               for (; itTupleChild != (*itNode)->mChildren.end(); itTupleChild++)
               {
                  if ((*itTupleChild)->mTag == "status")
                  {
                     // iterate through children looking for basic node
                     NodeList::const_iterator itStatusChild = (*itTupleChild)->mChildren.begin();
                     for (; itStatusChild != (*itTupleChild)->mChildren.end(); itStatusChild++)
                     {
                        if ((*itStatusChild)->mTag == "basic")
                        {
                           mOnline = (*itStatusChild)->mValue == "open";
                           break;
                        }
                     }
                  }
                  else if (mContact.empty() && (*itTupleChild)->mTag == "contact")
                  {
                     mContact = (*itTupleChild)->mValue;
                     Node::AttributeMap::iterator itAttrib = (*itTupleChild)->mAttributes.find("priority");
                     if (itAttrib != (*itTupleChild)->mAttributes.end())
                     {
                        mContactPriority = itAttrib->second;
                     }
                  }
                  else if (mNote.empty() && (*itTupleChild)->mTag == "note")
                  {
                     mNote = (*itTupleChild)->mValue;
                  }
                  else if (mTimestamp.empty() && (*itTupleChild)->mTag == "timestamp")
                  {
                     mTimestamp = (*itTupleChild)->mValue;
                  }
               }
            }
            break;
         }
      }
      mSimplePresenceExtracted = true;
   }
}

Data 
GenericPidfContents::generateNowTimestampData()
{
   time_t now;
   time(&now);
   return generateTimestampData(now);
}

static void pad2(const int x, DataStream& str)
{
    if (x < 10)
    {
        str << Symbols::ZERO[0];
    }
    str << x;
}

Data 
GenericPidfContents::generateTimestampData(time_t datetime)
{
   struct tm gmt;
#if defined(WIN32) || defined(__sun)
   struct tm *gmtp = gmtime(&datetime);
   if (gmtp == 0)
   {
      int e = getErrno();
      DebugLog(<< "Failed to convert to gmt: " << strerror(e));
      return Data::Empty;
   }
   memcpy(&gmt, gmtp, sizeof(gmt));
#else
   if (gmtime_r(&datetime, &gmt) == 0)
   {
      int e = getErrno();
      DebugLog(<< "Failed to convert to gmt: " << strerror(e));
      return Data::Empty;
   }
#endif

   Data timestamp;
   {
      DataStream ds(timestamp);
      ds << gmt.tm_year + 1900 << "-";
      pad2(gmt.tm_mon + 1, ds);
      ds << "-";
      pad2(gmt.tm_mday, ds);
      ds << "T";
      pad2(gmt.tm_hour, ds);
      ds << ":";
      pad2(gmt.tm_min, ds);
      ds << ":";
      pad2(gmt.tm_sec, ds);
      ds << "Z";
   }
   return timestamp;
}

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
