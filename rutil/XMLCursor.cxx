#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/XMLCursor.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

/**
Whitespace handling:
Are the following XML fragments equivalent?

Strictly interpreted, the root of the first XML document has one 
child while the root of the second XML doucment has three children.
The line breaks and spaces after the <root> and before </root> are 
tagless children.

--->
  <root><child>child content</child></root>
<--
  vs.
--->
  <root>
     <child>child content</child>
  </root>
<--

Treating whitespace as children is consistent with the spec but not usually
convenient. <!ATTLIST poem   xml:space (default|preserve) 'preserve'> is used to
control whitespace handling. Supporting this switch is painful. For now, treat
whitespace as non-significant.
*/

static char BANG[] = "!";
static char HYPHEN[] = "-";
static char LA_QUOTE[] = "<";
static char RA_QUOTE[] = ">";
static char SLASH[] = "/";
static char EQUALS[] = "=";
static char DOUBLE_QUOTE[] = "\"";
static char SINGLE_QUOTE[] = "\'";
//http://www.w3.org/TR/1998/REC-xml-19980210
static const Data COMMENT_START("<!--");
static const Data COMMENT_END("-->");
static const Data QUESTION_RA_QUOTE("?>");

// An alternative to stripping comments out in preparse
// is to deal with them in the parse; ignore when after non-leaf element
// put a leaf after a comment after a leaf in the first leaf's children
// getValue() needs to copy first leaf and all 'child' leaves to mValue
//
// has the advantage of allowing
// 1. lazier parsing
// 2. embedded weirdnesses like <! > and <? >
XMLCursor::XMLCursor(const ParseBuffer& pb)
   : mRoot(0),
     mCursor(0),
     mAttributesSet(false)
{
   ParseBuffer lPb(pb);

   skipProlog(lPb);
   const char* start = lPb.position();

   lPb.skipToChars(COMMENT_START);
   if (!lPb.eof())
   {
      StackLog(<< "removing comments");
      lPb.reset(start);
      mData.reserve(lPb.end() - lPb.start());

      const char* anchor = start;
      {
         DataStream str(mData);
         Data temp;
         while (true)
         {
            lPb.skipToChars(COMMENT_START);
            if (!lPb.eof())
            {
               lPb.data(temp, anchor);
               str << temp;
               anchor = Node::skipComments(lPb);
            }
            else
            {
               lPb.data(temp, anchor);
               str << temp;
               break;
            }
         }
      }
      mRoot = new Node(ParseBuffer(mData.data(), mData.size()));
   }
   else
   {
      mRoot = new Node(ParseBuffer(start, pb.end() - start));
   }
   mCursor = mRoot;

   if (mRoot->extractTag())
   {
      InfoLog(<< "XML: empty element no a legal root");
      mRoot->mPb.fail(__FILE__, __LINE__);
   }

   mTag = mRoot->mTag;
   decodeName(mRoot->mTag);

   // check for # & and note -- make decode, decodeName do stuff if set

   //<top></top> // no children
   ParseBuffer pbtemp(mRoot->mPb);
   pbtemp.skipToChar(RA_QUOTE[0]);
   pbtemp.skipChar();
   if (!WhitespaceSignificant)
   {
      pbtemp.skipWhitespace();
   }
   if (*pbtemp.position() == LA_QUOTE[0] &&
       *(pbtemp.position()+1) == SLASH[0])
   {
      pbtemp.skipChar();
      pbtemp.skipChar();
      if (strncmp(mRoot->mTag.data(), pbtemp.position(), mRoot->mTag.size()) == 0)
      {
         // no children ever
         mRoot->mPb.reset(mRoot->mPb.end());
         return;
      }
   }
}

XMLCursor::~XMLCursor()
{
   delete mRoot;
}

void
XMLCursor::skipProlog(ParseBuffer& pb)
{
   //'<?xml' VersionInfo '<xml?' EncodingDecl '?>'? '<?xml' SDDecl '?>'? S? '?>

   // !dlb! much more complicated than this.. can contain comments
   const char* start = pb.position();
   pb.skipToChars(QUESTION_RA_QUOTE);
   if(pb.eof()) 
   {
      // No Prolog
      pb.reset(start);
      return;
   }
   pb.skipN(2);
   pb.skipWhitespace();
}

void
XMLCursor::decode(Data& text)
{
}

void
XMLCursor::decodeName(Data& name)
{
}

void
XMLCursor::parseNextRootChild()
{
   // no next child to parse?
   if (mRoot->mPb.eof())
   {
      return;
   }

   // next child already parsed?
   if (mRoot->mNext != mRoot->mChildren.end())
   {
      return;
   }

   // skip self tag
   if (mRoot->mPb.position() == mRoot->mPb.start())
   {
      mRoot->mPb.skipToChar(RA_QUOTE[0]);
      mRoot->mPb.skipChar();
   }

   if (!WhitespaceSignificant)
   {
      mRoot->mPb.skipWhitespace();
   }

   // root end tag?
   if (*mRoot->mPb.position() == LA_QUOTE[0])
   {
      ParseBuffer pb(mRoot->mPb.position(), 
                     mRoot->mPb.end() - mRoot->mPb.position());
      pb.skipChar();
      if (!pb.eof() && *pb.position() == SLASH[0])
      {
         pb.skipChar();
         // CodeWarrior isn't helpful enough to pick the "obvious" operator definition
         // so we add volatile here so CW is completely unconfused what to do.
         // second note - MSVC 7.0 won't compile the volatile - tried the following to fix
         const char* end = pb.position();
         if ( (const char*)pb.end() < end + mTag.size() )
         {
            InfoLog(<< "XML: unexpected end");
            pb.fail(__FILE__, __LINE__);
         }
         
         if (strncmp(mTag.data(), pb.position(), mRoot->mTag.size()) == 0)
         {
            mRoot->mPb.skipToEnd();
            return;
         }
      }
   }

   // leaf?
   if (*mRoot->mPb.position() != LA_QUOTE[0])
   {
      const char* anchor = mRoot->mPb.position();
      mRoot->mPb.skipToChar(LA_QUOTE[0]);
      Node* leaf = new Node(ParseBuffer(anchor, mRoot->mPb.position() - anchor));
      leaf->mIsLeaf = true;
      mRoot->addChild(leaf);
   }
   else
   {
      Node* child = new Node(mRoot->mPb);
      child->skipToEndTag();

      // leave the parse buffer after the child
      mRoot->mPb.reset(child->mPb.end());

      mRoot->addChild(child);
   }

   // mNext always points at cursored child
   mRoot->mNext = mRoot->mChildren.end();
   mRoot->mNext--;
}

bool
XMLCursor::nextSibling()
{
   if (atRoot())
   {
      StackLog(<< "XMLCursor::nextSibling" << *this->mCursor << " <<root>>");
      return false;
   }

   StackLog(<< "XMLCursor::nextSibling" << *this->mCursor << " " << *this->mCursor->mParent);
   if (mCursor->mParent == mRoot)
   {
      parseNextRootChild();
   }

   if (mCursor->mParent->mNext != mCursor->mParent->mChildren.end())
   {
      mCursor = *((mCursor->mParent->mNext)++);
      mAttributesSet = false;
      return true;
   }
   else
   {
      return false;
   }
}

bool
XMLCursor::firstChild()
{
   if (atRoot() &&
       mRoot->mChildren.empty())
   {
      parseNextRootChild();
   }

   if (mCursor->mChildren.empty())
   {
      return false;
   }
   else
   {
      // mNext always points after cursored child
      mCursor->mNext = mCursor->mChildren.begin();
      mCursor->mNext++;
      mCursor = mCursor->mChildren.front();
      mAttributesSet = false;
      return true;
   }
}

bool
XMLCursor::parent()
{
   if (atRoot())
   {
      return false;
   }

   mCursor = mCursor->mParent;
   mAttributesSet = false;
   return true;
}

void
XMLCursor::reset()
{
   mCursor = mRoot;
   mAttributesSet = false;
}

bool
XMLCursor::atRoot() const
{
   return mCursor == mRoot;
}

bool
XMLCursor::atLeaf() const
{
   return mCursor->mIsLeaf;
}

const Data&
XMLCursor::getTag() const
{
   return mCursor->mTag;
}

//<foo >
//<foo>
//<foo/>
//<foo attr = 'value'   attr="value">
//<foo attr = 'value'   attr="value" >
//
//<foo attr = 'value'   attr="value" />
static const Data RA_QUOTE_SLASH(">/");
const XMLCursor::AttributeMap&
XMLCursor::getAttributes() const
{
   if (!atLeaf() &&
       !mAttributesSet)
   {
      mAttributes.clear();
      mAttributesSet = true;
   
      ParseBuffer pb(mCursor->mPb);
      pb.reset(mCursor->mPb.start());

      Data attribute;
      Data value;

      pb.skipToOneOf(ParseBuffer::Whitespace, RA_QUOTE_SLASH);

      while (!pb.eof() && 
             *pb.position() != RA_QUOTE[0] &&
             *pb.position() != SLASH[0])
      {
         attribute.clear();
         value.clear();

         const char* anchor = pb.skipWhitespace();
         pb.skipToOneOf(ParseBuffer::Whitespace, EQUALS);
         pb.data(attribute, anchor);
         XMLCursor::decodeName(attribute);

         StackLog(<< "attribute: " << attribute);

         pb.skipWhitespace();
         pb.skipToChar(EQUALS[0]);
         pb.skipChar();
         pb.skipWhitespace();
         if (!pb.eof())
         {
            const char quote = *pb.position();

            StackLog(<< "quote is <" << quote << ">");

            if(quote != DOUBLE_QUOTE[0] &&
               quote != SINGLE_QUOTE[0])
            {
               InfoLog(<< "XML: badly quoted attribute value");
               pb.fail(__FILE__, __LINE__);
            }
            anchor = pb.skipChar();
            pb.skipToChar(quote);
            pb.data(value, anchor);
            XMLCursor::decode(value);
            pb.skipChar();
            mAttributes[attribute] = value;
         }
         pb.skipWhitespace();
      }
   }

   return mAttributes;
}

const Data&
XMLCursor::getValue() const
{
   if (atLeaf())
   {
      ParseBuffer pb(mCursor->mPb);
      pb.skipToEnd();
      pb.data(mValue, pb.start());
      XMLCursor::decode(mValue);
   }
   else
   {
      mValue.clear();
   }
   return mValue;
}

EncodeStream&
XMLCursor::encode(EncodeStream& str, const AttributeMap& attrs)
{
   for(AttributeMap::const_iterator i = attrs.begin();
       i != attrs.end(); ++i)
   {
      if (i != attrs.begin())
      {
         str << " ";
      }
      // !dlb! some sort of character encoding required here
      str << i->first << "=\"" << i->second << "\"";
   }

   return str;
}

XMLCursor::Node::Node(const ParseBuffer& pb)
   : mPb(pb.position(), pb.end() - pb.position()),
     mParent(0),
     mChildren(),
     mNext(mChildren.begin()),
     mIsLeaf(false)
{
   mPb.assertNotEof();
   StackLog(<< "XMLCursor::Node::Node" << *this);
}

XMLCursor::Node::~Node()
{
   for (vector<Node*>::iterator i = mChildren.begin();
        i != mChildren.end(); ++i)
   {
      delete *i;
   }
}

// start:
//<foo >
//^
// end:
//<foo >
//      ^
static Data SLASH_RA_QUOTE("/>");
bool
XMLCursor::Node::extractTag()
{
   ParseBuffer pb(mPb);
   if (!WhitespaceSignificant)
   {
       pb.skipWhitespace();
   }
   const char* anchor = pb.skipChar(LA_QUOTE[0]);
   pb.skipToOneOf(ParseBuffer::Whitespace, SLASH_RA_QUOTE);
   pb.assertNotEof();
   pb.data(mTag, anchor);

   return !pb.eof() && *pb.position() == SLASH[0];
}

void
XMLCursor::Node::addChild(Node* child)
{
   mChildren.push_back(child);
   child->mParent = this;
}

//<foo> <bar> </bar> <baz> </baz> </foo>
//^start
//      ^child      
//                   ^child
//                                ^end
//
//<foo> sdfsf sadfsf <bar> asdfdf </bar> sadfsdf </foo>
//^start
//      ^child
//                   ^child sub 
//                                      ^child
void
XMLCursor::Node::skipToEndTag()
{
   extractTag();
   StackLog(<< "XMLCursor::Node::skipToEndTag(" <<  mTag << ")");
   //StackLog(<< "XMLCursor::Node::skipToEndTag(" << Data(mPb.position(), mPb.end() - mPb.position()) << ")");

   //<foo />
   mPb.skipToChar(RA_QUOTE[0]);
   if (*(mPb.position()-1) == SLASH[0])
   {
      mPb.skipChar();
      mPb = ParseBuffer(mPb.start(), mPb.position() - mPb.start());
      return;
   }

   //<foo> ...<child> ... </child> </foo>
   //    ^
   mPb.skipChar();
   //<foo> ...<child> ... </child> </foo>
   //     ^
   while (true)
   {
      if (!WhitespaceSignificant)
      {
         mPb.skipWhitespace();
      }

      // Some text contents ...<
      // ^                     ^
      if (*mPb.position() != LA_QUOTE[0])
      {
         const char* anchor = mPb.position();
         mPb.skipToChar(LA_QUOTE[0]);
         Node* leaf = new Node(ParseBuffer(anchor, mPb.position() - anchor));
         leaf->mIsLeaf = true;
         addChild(leaf);
      }

      //<...
      //^
      mPb.skipChar();
      //<...
      // ^

      // exit condition
      //</foo>
      if (*mPb.position() == SLASH[0])
      {
         mPb.skipChar();
         // CodeWarrior isn't helpful enough to pick the "obvious" operator definition
         // so we add volatile here so CW is completely unconfused what to do.
         // second note - MSVC 7.0 won't compile the volatile - tried the following to fix
         const char* end = mPb.position();
         if ( (const char*)mPb.end() < end + mTag.size() )
         {
            InfoLog(<< "XML: unexpected end");
            mPb.fail(__FILE__, __LINE__);
         }

         if (strncmp(mTag.data(), mPb.position(), mTag.size()) == 0)
         {
            mPb.skipToChar(RA_QUOTE[0]);
            mPb.skipChar();
            mPb = ParseBuffer(mPb.start(), mPb.position() - mPb.start());
            return;
         }
         else
         {
            InfoLog(<< "Badly formed XML: unexpected endtag");
            mPb.fail(__FILE__, __LINE__);
         }
      }

      //<child>...
      // ^
      if (mPb.position() == mPb.start())
      {
         InfoLog(<< "XML: badly formed element");
         mPb.fail(__FILE__, __LINE__);
      }

      mPb.reset(mPb.position()-1);
      //<child>...
      //^
      Node* child = new Node(mPb);
      addChild(child);
      child->skipToEndTag();
      mPb.reset(child->mPb.end());
      XMLCursor::decodeName(child->mTag);
      StackLog(<< mTag << "(" << child->mTag << ")");
    }
}

//<!-- declarations for <head> & <body> -->
const char*
XMLCursor::Node::skipComments(ParseBuffer& pb)
{
   while (*pb.position() == LA_QUOTE[0] &&
          *(pb.position()+1) == BANG[0] &&
          *(pb.position()+2) == HYPHEN[0] &&
          *(pb.position()+3) == HYPHEN[0])
   {
      pb.skipToChars(COMMENT_END);
      pb.skipChars(COMMENT_END);
      pb.skipWhitespace();
      if(pb.eof())
      {
         return pb.end();
      }
   }

   return pb.position();
}

EncodeStream&
resip::operator<<(EncodeStream& str, const XMLCursor::Node& node)
{
   Data::size_type size = node.mPb.end() - node.mPb.start();

   static const Data::size_type showSize(35);

   str << &node << "[" 
       << Data(node.mPb.start(), 
               min(showSize, size))
        << "]" << (size ? "" : "...");

   return str;
}

EncodeStream&
resip::operator<<(EncodeStream& str, const XMLCursor& cursor)
{
   str << "XMLCursor " << *cursor.mCursor;
   return str;
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
