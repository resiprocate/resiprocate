/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_XMLCURSOR_HXX)
#define RESIP_XMLCURSOR_HXX 

#include <iosfwd>
#include <vector>

#include "rutil/ParseBuffer.hxx"
#include "rutil/HashMap.hxx"

namespace resip
{

/**
@brief XML tree traversal.

XMLCursor starts at the root.
The attributes and value of the cursor are those of the root.
- To descend to the first child of the root, call firstChild().
- To traverse the children of root from root, call firstChild();nextSibling();nextSibling();...
- To descend into the first child of the current element, call firstChild.
- To return to the parent of the current element, call parent.

The traversal state among the siblings of the parent is maintained.

<pre>
                          root
                        /     \
                       P1     P2
                      /  \   /  \
                     A1  A2 B1  B2

  atRoot() == true;
  firstChild();  // P1
  firstChild();  // A1
  nextSibling(); // A2
  parent();      // P1
  nextSibling(); // P2
  firstChild();  // B1
  nextSibling(); // B2
  parent();      // P2
  nextSibling(); // false, stay at P2
  parent();      // root
  nextSibling(); // false, stay at root
  parent();      // false, stay at root
  firstChild();  // P1
</pre>

 Depth first traversal example:

<pre>
void traverse(XMLCursor& c)
 {
    if (c.firstChild())
    {
       traverse(c);
       c.parent();
    }
 
    process(c);
    
    if (c.nextSibling())
    {
       traverse(c);
    }
 }
</pre>

Lexical Order traversal example:

<pre>
 void
 traverse(XMLCursor& c, int i = 0)
 {
    for (int ii = 0; ii < i; ++ii)
    {
       cerr << " ";
    }

    cerr << c.getTag();
    if (c.atLeaf())
    {
       cerr << "[" << c.getValue() << "]" << endl;
    }
    else
    {
       cerr << endl;
    }

    if (c.firstChild())
    {
       traverse(c, i+2);
       c.parent();
    }
   
    if (c.nextSibling())
    {
       traverse(c, i+2);
    }
 }
</pre>
*/

class XMLCursor
{
   public:
      /// @todo !dlb! should be determined by the document
      /// see http://www.w3.org/TR/1998/REC-xml-19980210#sec-white-space
      enum {WhitespaceSignificant = false};

      /**
        @brief constructor to initialize the tree structure described in 
         the class description 
        @param pb the xml document to be parsed

         <b>Whitespace handling:</b> Are the following XML fragments equivalent?
		 
		 @code
         --->
           <root><child>child content</child></root>
         <--
		 @endcode
           vs.
		 @code
         --->
           <root>
              <child>child content</child>
           </root>
         <--
		 @endcode

         Strictly interpreted, the root of the first XML document has one
         child while the root of the second XML doucment has three children.
         The line breaks and spaces after the &lt;root&gt; and before 
		 &lt;/root&gt; are tagless children.

         Treating whitespace as children is consistent with the spec but not 
         usually convenient. 
         <pre><!ATTLIST poem   xml:space (default|preserve) 'preserve'> </pre>
		 is used to
         control whitespace handling. Supporting this switch is painful. 
         For now, treat whitespace as non-significant.
         
        @note An alternative to removing comments in preparse
         is to deal with them in the parse; ignore when after non-leaf element
         put a leaf after a comment after a leaf in the first leaf's children
         getValue() needs to copy first leaf and all 'child' leaves to mValue
         has the advantage of allowing:
         - lazier parsing
         - embedded wierdnesses like &lt;! &gt; and &lt;?  &gt;
		 
		 @todo clean up the information in the note.
        */
      XMLCursor(const ParseBuffer& pb);

      /**
        @brief destructor is empty. 
        */
      ~XMLCursor();

      /**
        @brief check if you have a sibling and move the cursor to your sibling
        @return true if successful, false if you do not have a sibling
        */
      bool nextSibling();

      /**
        @brief check if you have a child and move the cursor to your child
        @return true if successful, false if you do not have a child
        */
      bool firstChild();

      /**
        @brief check if you have a parent and move the cursor to your parent
        @return true if successful, false if you do not have a parent
        */
      bool parent();

      /**
        @brief reposition yourself at the root
        */
      void reset();

      /**
        @brief check if you are at the root node
        @return true if you are, false if you're not
        */
      bool atRoot() const;

      /**
        @brief check if you are a leaf
        @return true if you are, false if you're not
        */
      bool atLeaf() const;

      /**
        @brief retrieve the XML tag of where you are at present in the tree
        @return Data& the tag ie. "child" in the case: &lt;child&gt; child content &lt;/child&gt;
        */
      const Data& getTag() const;
      typedef HashMap<Data, Data> AttributeMap;
      /**
        @brief create a set of attribute value pairs
        @return AttributeMap ie. a HashMap of value using attributes as keys
        @details examples of types of cases covered in parsing are
		@code
         <foo>
         <foo>
         <foo/>
         <foo attr = 'value'   attr="value">
         <foo attr = 'value'   attr="value" >
         <foo attr = 'value'   attr="value" />
		 @endcode
        */
      const AttributeMap& getAttributes() const;
      /**
        @brief retrieve the XML tag of where you are at present in the tree
        @return Data& the value ie. "child content" in the case: &lt;child&gt; child content &lt;/child&gt;
        */
      const Data& getValue() const;

      /**
        @brief converts the tree structure to an xml document format 
         puts it to ostream and returns it
        @return an ostream with the xml document
        */
      static std::ostream& encode(std::ostream& strm, const AttributeMap& attrs);

      class Node;

      /**
         @internal
      */
      class AttributeValueEqual {
         Data data_;
         public:
            AttributeValueEqual(const Data& data) : data_(data) {};
            bool operator()(const std::pair<Data, Data>& data) { return data.second == data_; }
      };

   private:
      static void skipProlog(ParseBuffer& pb);
      static void decode(Data&);
      static void decodeName(Data&);

      void parseNextRootChild();


      Node* mRoot;
      Node* mCursor;

      //bool isEmpty;

      /// store for undecoded root tag
      Data mTag;

      /// store for copy of input if commented
      Data mData;

      /// store date for decoding
      mutable Data mValue;
      /// store attributes for reference
      mutable AttributeMap mAttributes;
      mutable bool mAttributesSet;

public:
      /**
         @deprecated 
		 @brief Deprecated
      */
      class Node
      {
         public:
            Node(const ParseBuffer& pb);
            ~Node();

            void addChild(Node*);
            /// return true if &lt;foo/&gt;
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

            friend std::ostream& operator<<(std::ostream& str, const XMLCursor& cursor);
            // friend std::ostream& operator<<(std::ostream& str, const XMLCursor::Node& cursor); // this line won't compile in windows 
      };
   private:
      friend std::ostream& operator<<(std::ostream&, const XMLCursor&);
      friend std::ostream& operator<<(std::ostream&, const XMLCursor::Node&);

      /// no value semantics
      XMLCursor(const XMLCursor&);
      XMLCursor& operator=(const XMLCursor&);
      friend class Node;
};

std::ostream&
operator<<(std::ostream& str, const XMLCursor& cursor);

std::ostream&
operator<<(std::ostream& str, const XMLCursor::Node& cursor);

}

#endif

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
