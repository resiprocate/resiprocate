#if !defined(RESIP_XMLCURSOR_HXX)
#define RESIP_XMLCURSOR_HXX 

#include <iosfwd>
#include <vector>

#include "rutil/ParseBuffer.hxx"
#include "rutil/HashMap.hxx"

namespace resip
{

/*
// XML tree traversal.
// XMLCursor starts at the root.
// The attributes and value of the cursor are those of the root.
// To descend to the first child of the root, call firstChild().
// To traverse the children of root from root, call firstChild();nextSibling();nextSibling();...
// To descend into the first child of the current element, call firstChild.
// To return to the parent of the current element, call parent.
// The traversal state among the siblings of the parent is maintained.
//
//                          root
//                        /     \
//                       P1     P2
//                      /  \   /  \
//                     A1  A2 B1  B2
//
//  atRoot() == true;
//  firstChild(); // P1
//  firstChild(); // A1
//  nextSibling(); // A2
//  parent();   // P1
//  nextSibling(); // P2
//  firstChild(); // B1
//  nextSibling(); // B2
//  parent();   // P2
//  nextSibling(); // false, stay at P2
//  parent();   // root
//  nextSibling(); // false, stay at root
//  parent();   // false, stay at root
//  firstChild(); // P1

// E.g.: Depth first traversal
//
// void traverse(XMLCursor& c)
// {
//    if (c.firstChild())
//    {
//       traverse(c);
//       c.parent();
//    }
// 
//    process(c);
//    
//    if (c.nextSibling())
//    {
//       traverse(c);
//    }
// }
//
// E.g.: Lexical Order traversal
//
// void
// traverse(XMLCursor& c, int i = 0)
// {
//    for (int ii = 0; ii < i; ++ii)
//    {
//       cerr << " ";
//    }

//    cerr << c.getTag();
//    if (c.atLeaf())
//    {
//       cerr << "[" << c.getValue() << "]" << endl;
//    }
//    else
//    {
//       cerr << endl;
//    }

//    if (c.firstChild())
//    {
//       traverse(c, i+2);
//       c.parent();
//    }
   
//    if (c.nextSibling())
//    {
//       traverse(c, i+2);
//    }
// }

*/

class XMLCursor
{
   public:
      // !dlb! should be determined by the document
      // see http://www.w3.org/TR/1998/REC-xml-19980210#sec-white-space
      enum {WhitespaceSignificant = false};

      XMLCursor(const ParseBuffer& pb);
      ~XMLCursor();

      bool nextSibling();
      bool firstChild();
      bool parent();
      void reset();

      bool atRoot() const;
      bool atLeaf() const;

      const Data& getTag() const;
      typedef HashMap<Data, Data> AttributeMap;
      const AttributeMap& getAttributes() const;
      const Data& getValue() const;

      static EncodeStream& encode(EncodeStream& strm, const AttributeMap& attrs);
      class Node;

      class AttributeValueEqual 
      {
         Data data_;
         public:
            AttributeValueEqual(const Data& data) : data_(data) {};
            bool operator()(const std::pair<const Data, Data>& data) { return data.second == data_; }
      };

   private:
      static void skipProlog(ParseBuffer& pb);
      static void decode(Data&);
      static void decodeName(Data&);

      void parseNextRootChild();


      Node* mRoot;
      Node* mCursor;

      //bool isEmpty;

      // store for undecoded root tag
      Data mTag;

      // store for copy of input if commented
      Data mData;

      // store date for decoding
      mutable Data mValue;
      // store attributes for reference
      mutable AttributeMap mAttributes;
      mutable bool mAttributesSet;

public:
      class Node
      {
         public:
            Node(const ParseBuffer& pb);
            ~Node();

            void addChild(Node*);
            // return true if <foo/>
            bool extractTag();
            void skipToEndTag();
            static const char* skipComments(ParseBuffer& pb);

            ParseBuffer mPb;
            Node* mParent;
            std::vector<Node*> mChildren;
            std::vector<Node*>::const_iterator mNext;

            bool mIsLeaf;
            Data mTag;

         private:
            Node(const Node&);
            Node& operator=(const Node&);

            friend EncodeStream& operator<<(EncodeStream& str, const XMLCursor& cursor);
            // friend EncodeStream& operator<<(EncodeStream& str, const XMLCursor::Node& cursor); // this line won't compile in windows 
      };
   private:
      friend EncodeStream& operator<<(EncodeStream&, const XMLCursor&);
      friend EncodeStream& operator<<(EncodeStream&, const XMLCursor::Node&);

      // no value semantics
      XMLCursor(const XMLCursor&);
      XMLCursor& operator=(const XMLCursor&);
      friend class Node;
};

EncodeStream&
operator<<(EncodeStream& str, const XMLCursor& cursor);

EncodeStream&
operator<<(EncodeStream& str, const XMLCursor::Node& cursor);

}

#endif

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
