#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/XMLCursor.hxx"
#include "resiprocate/Symbols.hxx"
#if !defined(DISABLE_RESIP_LOG)
#include "resiprocate/os/Logger.hxx"
#endif
#include "resiprocate/os/WinLeakCheck.hxx"
#include "resiprocate/os/Win32Export.hxx"

//#ifndef   `_WIN32 // !cj! TODO FIX 
#if 1 

using namespace resip;
using namespace std;

#if !defined(DISABLE_RESIP_LOG)
#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS
#endif
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
//http://www.w3.org/TR/1998/REC-xml-19980210
static const Data COMMENT_START("<!--");
static const Data COMMENT_END("-->");

// An alternative to stripping comments out in preparse
// is to deal with them in the parse; ignore when after non-leaf element
// put a leaf after a comment after a leaf in the first leaf's children
// getValue() needs to copy first leaf and all 'child' leaves to mValue
//
// has the advantage of allowing
// 1. lazier parsing
// 2. embedded wierdnesses like <! > and <? >
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
#if !defined(DISABLE_RESIP_LOG)
      StackLog(<< "removing comments");
#endif
      lPb.reset(start);
      mData.reserve(lPb.end() - lPb.start());

      const char* anchor = lPb.position();
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
#if !defined(DISABLE_RESIP_LOG)
      InfoLog(<< "XML: empty element no a legal root");
#endif
      mRoot->mPb.fail(__FILE__, __LINE__);
   }

   mTag = mRoot->mTag;
   decodeName(mRoot->mTag);

   // check for # & and note -- make decode, decodeName do stuff if set

   //<top></top> // no children
   lPb.reset(lPb.start());
   lPb.skipToChar(Symbols::RA_QUOTE[0]);
   lPb.skipChar();
   if (!WhitespaceSignificant)
   {
      lPb.skipWhitespace();
   }
   if (*lPb.position() == Symbols::LA_QUOTE[0] &&
      *(lPb.position()+1) == Symbols::SLASH[0])
   {
      lPb.skipChar();
      lPb.skipChar();
      if (strncmp(mRoot->mTag.data(), lPb.position(), mRoot->mTag.size()) == 0)
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

static const Data QUESTION_RA_QUOTE("?>");
void
XMLCursor::skipProlog(ParseBuffer& pb)
{
   //'<?xml' VersionInfo '<xml?' EncodingDecl '?>'? '<?xml' SDDecl '?>'? S? '?>

   // !dlb! much more complicated than this.. can contain comments
   pb.skipToChars(QUESTION_RA_QUOTE);
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
      mRoot->mPb.skipToChar(Symbols::RA_QUOTE[0]);
      mRoot->mPb.skipChar();
   }

   if (!WhitespaceSignificant)
   {
      mRoot->mPb.skipWhitespace();
   }

   // root end tag?
   if (*mRoot->mPb.position() == Symbols::LA_QUOTE[0])
   {
      ParseBuffer pb(mRoot->mPb.position(), 
         mRoot->mPb.end() - mRoot->mPb.position());
      pb.skipChar();
      if (!pb.eof() && *pb.position() == Symbols::SLASH[0])
      {
         pb.skipChar();
         // CodeWarrior isn't helpful enough to pick the "obvious" operator definition
         // so we add volatile here so CW is completely unconfused what to do.
         // second note - MSVC 7.0 won't compile the volatile - tried the following to fix
         const char* end = pb.position();
         if ( pb.end() < end + mTag.size() )
         {
#if !defined(DISABLE_RESIP_LOG)
            InfoLog(<< "XML: unexpected end");
#endif
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
   if (*mRoot->mPb.position() != Symbols::LA_QUOTE[0])
   {
      const char* anchor = mRoot->mPb.position();
      mRoot->mPb.skipToChar(Symbols::LA_QUOTE[0]);
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
#if !defined(DISABLE_RESIP_LOG)
      StackLog(<< "XMLCursor::nextSibling" << *this->mCursor << " <<root>>");
#endif
      return false;
   }
#if !defined(DISABLE_RESIP_LOG)
   StackLog(<< "XMLCursor::nextSibling" << *this->mCursor << " " << *this->mCursor->mParent);
#endif
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

      static const Data term(">/");
      pb.skipToOneOf(ParseBuffer::Whitespace, term);

      while (!pb.eof() && 
         *pb.position() != Symbols::RA_QUOTE[0] &&
         *pb.position() != Symbols::SLASH[0])
      {
         attribute.clear();
         value.clear();

         const char* anchor = pb.skipWhitespace();
         pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::EQUALS);
         pb.data(attribute, anchor);
         XMLCursor::decodeName(attribute);
#if !defined(DISABLE_RESIP_LOG)
         StackLog(<< "attribute: " << attribute);
#endif
         pb.skipWhitespace();
         pb.skipToChar(Symbols::EQUALS[0]);
         pb.skipChar();
         pb.skipWhitespace();
         if (!pb.eof())
         {
            const char quote = *pb.position();
#if !defined(DISABLE_RESIP_LOG)
            StackLog(<< "quote is <" << quote << ">");
#endif
            if (quote != Symbols::DOUBLE_QUOTE[0] &&
               quote != '\'')
            {
#if !defined(DISABLE_RESIP_LOG)
               InfoLog(<< "XML: badly quoted attribute value");
#endif
               pb.fail(__FILE__, __LINE__);
            }
            anchor = pb.skipChar();
            pb.skipToChar(quote);
            pb.data(value, anchor);
            XMLCursor::decode(value);
            pb.skipChar();
            mAttributes[attribute] = decodeXMLCompatible(value);
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
      mValue = pb.data(pb.start());
      XMLCursor::decode(mValue);
   }
   else
   {
      mValue.clear();
   }
   return mValue;
}

Data
XMLCursor::encodeXMLCompatible(const Data& strData)
{
   Data encodedStr(strData.size() << 1, true);
   int charCount = strData.size();
   for (register int i = 0; i < charCount; ++i)
   {
      switch (strData[i])
      {
      case '"':
         encodedStr.append("&quot;", strlen("&quot;"));
         break;
      case '<':
         encodedStr.append("&lt;", strlen("&lt;"));
         break;
      case '>':
         encodedStr.append("&gt;", strlen("&gt;"));
         break;
      default:
         encodedStr.append(strData.c_str() + i, 1);
         break;
      }
   }
   return encodedStr;
}

class RESIP_API FindStruct
{
public:
   FindStruct(const char* findStr, const char* replaceStr)
      :mFoundPos(Data::npos)
      ,mFindStr(findStr)
      ,mFindStrLen(::strlen(findStr))
      ,mReplaceStr(replaceStr)
   {
   }


public: // members
   int         mFoundPos;
   const char* mFindStr;
   const int   mFindStrLen;
   const char* mReplaceStr;
};

Data
XMLCursor::decodeXMLCompatible(const Data& strData)
{
   FindStruct findDatas[] = 
      { 
         FindStruct("&quot;", "\""), 
         FindStruct("&lt;", "<"), 
         FindStruct("&gt;", ">"),
         FindStruct("&nbsp;", " ") 
      };

   Data decodedStr(strData.size(), true);
   Data lowerStrData(strData);
   lowerStrData.lowercase();
   int lastIdx = 0;
   while (true)
   {
      int minIdx = Data::npos;
      int foundIdx = -1;
      for (int i = 0; i < (sizeof(findDatas) / sizeof(findDatas[0])); ++i)
      {
         findDatas[i].mFoundPos = lowerStrData.find(findDatas[i].mFindStr, lastIdx);
         if (findDatas[i].mFoundPos != Data::npos)
         {
            if (foundIdx == -1)
            {
               minIdx = findDatas[i].mFoundPos;
               foundIdx = i;
            }
            else if (findDatas[i].mFoundPos < minIdx)
            {
               minIdx = findDatas[i].mFoundPos;
               foundIdx = i;
            }
         }
      }

      if (foundIdx != -1)
      {
         decodedStr += strData.substr(lastIdx, findDatas[foundIdx].mFoundPos - lastIdx);
         decodedStr += findDatas[foundIdx].mReplaceStr;
         lastIdx = findDatas[foundIdx].mFoundPos + findDatas[foundIdx].mFindStrLen;
      }
      else
      {
         break;
      }
   }
   decodedStr += strData.substr(lastIdx);
   return decodedStr;
}

std::ostream&
XMLCursor::encode(std::ostream& str, const AttributeMap& attrs)
{
   for(AttributeMap::const_iterator i = attrs.begin();
      i != attrs.end(); ++i)
   {
      if (i != attrs.begin())
      {
         str << " ";
      }
      // !dlb! some sort of character encoding required here
      str << i->first << "=\"" << encodeXMLCompatible(i->second) << "\"";
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
#if !defined(DISABLE_RESIP_LOG)
   StackLog(<< "XMLCursor::Node::Node" << *this);
#endif
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
bool
XMLCursor::Node::extractTag()
{
   ParseBuffer pb(mPb);
   const char* anchor = pb.skipChar();
   static Data SLASH_RA_QUOTE("/>");
   pb.skipToOneOf(ParseBuffer::Whitespace, SLASH_RA_QUOTE);
   pb.assertNotEof();
   pb.data(mTag, anchor);

   return !pb.eof() && *pb.position() == Symbols::SLASH[0];
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
#if !defined(DISABLE_RESIP_LOG)
   StackLog(<< "XMLCursor::Node::skipToEndTag(" <<  mTag << ")");
#endif
   //StackLog(<< "XMLCursor::Node::skipToEndTag(" << Data(mPb.position(), mPb.end() - mPb.position()) << ")");

   //<foo />
   mPb.skipToChar(Symbols::RA_QUOTE[0]);
   if (*(mPb.position()-1) == Symbols::SLASH[0])
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
      if (*mPb.position() != Symbols::LA_QUOTE[0])
      {
         const char* anchor = mPb.position();
         mPb.skipToChar(Symbols::LA_QUOTE[0]);
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
      if (*mPb.position() == Symbols::SLASH[0])
      {
         mPb.skipChar();
         // CodeWarrior isn't helpful enough to pick the "obvious" operator definition
         // so we add volatile here so CW is completely unconfused what to do.
         // second note - MSVC 7.0 won't compile the volatile - tried the following to fix
         const char* end = mPb.position();
         if ( mPb.end() < end + mTag.size() )
         {
#if !defined(DISABLE_RESIP_LOG)
            InfoLog(<< "XML: unexpected end");
#endif
            mPb.fail(__FILE__, __LINE__);
         }

         if (strncmp(mTag.data(), mPb.position(), mTag.size()) == 0)
         {
            mPb.skipToChar(Symbols::RA_QUOTE[0]);
            mPb.skipChar();
            mPb = ParseBuffer(mPb.start(), mPb.position() - mPb.start());
            return;
         }
         else
         {
#if !defined(DISABLE_RESIP_LOG)
            InfoLog(<< "Badly formed XML: unexpected endtag");
#endif
            mPb.fail(__FILE__, __LINE__);
         }
      }

      //<child>...
      // ^
      if (mPb.position() == mPb.start())
      {
#if !defined(DISABLE_RESIP_LOG)
         InfoLog(<< "XML: badly formed element");
#endif
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
#if !defined(DISABLE_RESIP_LOG)
      StackLog(<< mTag << "(" << child->mTag << ")");
#endif
   }
}

//<!-- declarations for <head> & <body> -->
const char*
XMLCursor::Node::skipComments(ParseBuffer& pb)
{
   while (*pb.position() == Symbols::LA_QUOTE[0] &&
      *(pb.position()+1) == BANG[0] &&
      *(pb.position()+2) == HYPHEN[0] &&
      *(pb.position()+3) == HYPHEN[0])
   {
      pb.skipToChars(COMMENT_END);
      pb.assertNotEof();
   }

   return pb.position();
}

RESIP_API std::ostream&
resip::operator<<(std::ostream& str, const XMLCursor::Node& node)
{
   Data::size_type size = node.mPb.end() - node.mPb.start();

   static const Data::size_type showSize(35);

   str << &node << "[" 
      << Data(node.mPb.start(), 
      min(showSize, size))
      << "]" << (size ? "" : "...");

   return str;
}

RESIP_API std::ostream&
resip::operator<<(std::ostream& str, const XMLCursor& cursor)
{
   str << "XMLCursor " << *cursor.mCursor;
   return str;
}

#endif // _WIN32
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
