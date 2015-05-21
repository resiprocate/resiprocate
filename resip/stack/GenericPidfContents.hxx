#if !defined(RESIP_GENERICPIDFCONTENTS_HXX)
#define RESIP_GENERICPIDFCONTENTS_HXX 

#include <map>
#include <list>

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/HeapInstanceCounter.hxx"
#include "resip/stack/QValue.hxx"

namespace resip
{

class XMLCursor;

/**
   SIP body type for holding PIDF contents (MIME content-type application/pidf+xml).
   This version is used to be able to extract and manipulate all Pidf contents and
   contained extensions (ie: rpid, data-model, cipid, etc.) in a generic manner.
*/
class GenericPidfContents : public Contents
{
public:

   static const GenericPidfContents Empty;

   RESIP_HeapCount(GenericPidfContents);
   GenericPidfContents(const Mime& contentType);
   GenericPidfContents();
   GenericPidfContents(const HeaderFieldValue& hfv, const Mime& contentType);
   GenericPidfContents(const GenericPidfContents& rhs);
   virtual ~GenericPidfContents();
   virtual GenericPidfContents& operator=(const GenericPidfContents& rhs);

   /** @brief duplicate an GenericPidfContents object
       @return pointer to a new GenericPidfContents object
       **/
   virtual Contents* clone() const;
   static const Mime& getStaticType();
   virtual EncodeStream& encodeParsed(EncodeStream& str) const;
   virtual void parse(ParseBuffer& pb);

   void setEntity(const Uri& entity);
   const Uri& getEntity() const;

   void addNamespace(const Data& uri, const Data& prefix);
   typedef HashMap<Data, Data> NamespaceMap;  // first is Uri, second is prefix (which includes a trailing ":")
   const NamespaceMap& getNamespaces() const { checkParsed(); return mNamespaces; }
   // Note:  you set the RootPidfNamesapces prefix by adding the generic Pidf 
   //        namespace urn:ietf:params:xml:ns:pidf via addNamespace
   const Data& getRootPidfNamespacePrefix() const { checkParsed(); return mRootPidfNamespacePrefix; }

   class Node;
   typedef std::list<Node*> NodeList;
   class Node
   {
   public:
      Data mNamespacePrefix;
      Data mTag;
      typedef HashMap<Data, Data> AttributeMap;
      AttributeMap mAttributes;
      Data mValue;
      NodeList mChildren;

      void copy(const Node& rhs, HashMap<Data, Data>* namespacePrefixCorrections);

      EncodeStream& encodeAttributes(EncodeStream& str) const;
      EncodeStream& encode(EncodeStream& str, Data indent) const;
   };
   const NodeList& getRootNodes() const { checkParsed(); return mRootNodes; }

   // Helpers for users of this class
   static const Data& getSubNodeValue(Node* node, const Data& tag);
   static Data generateNowTimestampData();
   static Data generateTimestampData(time_t datetime);

   // You should be adding the namespace first manually if you want a custom prefix
   void setSimplePresenceTupleNode(const Data& id,
                                   bool online,
                                   const Data& timestamp = Data::Empty,
                                   const Data& note = Data::Empty,
                                   const Data& contact = Data::Empty,
                                   const Data& contactPriority = Data::Empty);

   const Data& getSimplePresenceTupleId() { checkParsed(); extractSimplePresenceInfo(); return mTupleId; }
   const bool getSimplePresenceOnline() { checkParsed(); extractSimplePresenceInfo(); return mOnline; }
   const Data& getSimplePresenceTimestamp() { checkParsed(); extractSimplePresenceInfo(); return mTimestamp; }
   const Data& getSimplePresenceNote() { checkParsed(); extractSimplePresenceInfo(); return mNote; }
   const Data& getSimplePresenceContact() { checkParsed(); extractSimplePresenceInfo(); return mContact; }
   const Data& getSimplePresenceContactPriority() { checkParsed(); extractSimplePresenceInfo(); return mContactPriority; }

   static bool init();

   // combine pidfs
   bool merge(const GenericPidfContents& other);

private:

   NamespaceMap mNamespaces;
   Data mRootPidfNamespacePrefix; // includes trailing ":"
   Uri mEntity;

   // Simple Presence Info
   Data mTupleId;
   bool mOnline;
   Data mTimestamp;
   Data mNote;
   Data mContact;
   Data mContactPriority;
   bool mSimplePresenceExtracted;

   NodeList mRootNodes;
   void parseChildren(XMLCursor& xml, NodeList& nodeList);
   void cleanupNodeMemory(NodeList& nodeList);
   void reset();
   bool mergeNoCheckParse(const GenericPidfContents& other);
   void extractSimplePresenceInfo();
   void clearSimplePresenceInfo();
};

static bool invokeGenericPidfContentsInit = GenericPidfContents::init();

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
