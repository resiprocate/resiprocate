#include "sip2/sipstack/XMLCursor.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/util/Logger.hxx"

// !dlb! prolog
//<?xml' VersionInfo EncodingDecl? SDDecl? S? '?>
//<? ... ?>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::CONTENTS

static char* BANG = "!";
static char* HYPHEN = "-";
static const Data COMMENT_START("<!--");
static const Data COMMENT_END("-->");

XMLCursor::XMLCursor(const ParseBuffer& pb)
   : mRoot(0),
     mCursor(0)
{
   ParseBuffer lPb(pb);
   bool needsPreparse;
   lPb.skipToChars(COMMENT_START);
   needsPreparse = !lPb.eof();
   lPb.reset(lPb.start());
   lPb.skipToChars(Symbols::CRLF);
   needsPreparse |= !lPb.eof();

   if (needsPreparse)
   {
      DebugLog(<< "removing comments and cannonicalzing endlines");
      lPb.reset(lPb.start());
      mData.reserve(lPb.end() - lPb.start());

      const char* anchor = lPb.position();
      Data temp;
      while (!lPb.eof())
      {
         ParseBuffer alt(lPb);
         alt.skipToChars(Symbols::CRLF);
         lPb.skipToChars(COMMENT_START);
         if (lPb.position() < alt.position())
         {
            lPb.data(temp, anchor);
            mData += temp;
            Node::skipComments(lPb);
         }
         else
         {
            lPb.reset(alt.position()+2);
            mData += Symbols::LF[0];
         }
      }
      mRoot = new Node(ParseBuffer(mData.data(), mData.size()));
   }
   else
   {
      mRoot = new Node(ParseBuffer(pb));
   }
   mCursor = mRoot;

   if (mRoot->extractTag())
   {
      InfoLog("XML: empty element no a legal root");
      mRoot->mPb.fail(__FILE__, __LINE__);
   }

   mTag = mRoot->mTag;
   decodeName(mRoot->mTag);

   //<top></top> // no children
   lPb.reset(lPb.start());
   lPb.skipToChar(Symbols::RA_QUOTE[0]);
   lPb.skipChar();
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
   // no next child to parse
   if (mRoot->mPb.eof())
   {
      return;
   }

   // next child already parsed
   if (mRoot->mNext != mCursor->mChildren.end())
   {
      return;
   }

   // skip self tag
   if (mRoot->mPb.position() == mRoot->mPb.start())
   {
      mRoot->mPb.skipToChar(Symbols::RA_QUOTE[0]);
      mRoot->mPb.skipChar();
      mRoot->mPb.skipToChar(Symbols::LA_QUOTE[0]);
      mRoot->mPb.assertNotEof();
   }

   // root end tag?
   ParseBuffer pb(mRoot->mPb.position(), 
                  mRoot->mPb.end() - mRoot->mPb.position());
   pb.skipToChar(Symbols::LA_QUOTE[0]);
   pb.skipChar();

   if (*pb.position() == Symbols::SLASH[0])
   {
      pb.skipChar();
      if (mTag.size() + pb.position() > pb.end())
      {
         InfoLog("XML: unexpected end");
         pb.fail(__FILE__, __LINE__);
      }
         
      if (strncmp(mTag.data(), pb.position(), mRoot->mTag.size()) == 0)
      {
         mRoot->mPb.skipToEnd();
         return;
      }
   }

   Node* child = new Node(mRoot->mPb);
   child->skipToEndTag();

   // leave the parse buffer after the child
   mRoot->mPb.reset(child->mPb.end());

   mRoot->addChild(child);
}

bool
XMLCursor::nextSibling()
{
   if (atRoot())
   {
      return false;
   }

   if (mCursor->mParent == mRoot)
   {
      parseNextRootChild();
   }

   if (mCursor->mParent->mNext != mCursor->mParent->mChildren.end())
   {
      mCursor = *(++(mCursor->mParent->mNext));
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
   if (atRoot())
   {
      parseNextRootChild();
   }

   if (mCursor->mChildren.empty())
   {
      return false;
   }
   else
   {
      mCursor->mNext = ++(mCursor->mChildren.begin());
      mCursor = mCursor->mChildren.front();
   }
   return true;
}

bool
XMLCursor::parent()
{
   if (atRoot())
   {
      return false;
   }

   mCursor = mCursor->mParent;
   return true;
}

void
XMLCursor::reset()
{
   mCursor = mRoot;
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

Data 
XMLCursor::getTag() const
{
   return mCursor->mTag;
}

//<foo >
//<foo>
//<foo/>
//<foo attr = 'value'   attr="value">
//<foo attr = 'value'   attr="value" >
std::map<Data, Data>
XMLCursor::getAttributes() const
{
   std::map<Data, Data> attributes;

   ParseBuffer pb(mCursor->mPb);
   pb.reset(mCursor->mPb.start());

   Data attribute;
   Data value;

   pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::RA_QUOTE);

   while (*pb.position() != Symbols::RA_QUOTE[0])
   {
      attribute.clear();
      value.clear();

      const char* anchor = pb.skipWhitespace();
      pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::EQUALS);
      pb.data(attribute, anchor);
      XMLCursor::decodeName(attribute);

      CerrLog(<< "attribute: " << attribute);

      pb.skipWhitespace();
      pb.skipToChar(Symbols::EQUALS[0]);
      pb.skipChar();
      pb.skipWhitespace();
      const char quote = *pb.position();

      CerrLog(<< "quote is <" << quote << ">");

      if (quote != Symbols::DOUBLE_QUOTE[0] &&
          quote != '\'')
      {
         InfoLog("XML: badly quoted attribute value");
         pb.fail(__FILE__, __LINE__);
      }
      anchor = pb.skipChar();
      pb.skipToChar(quote);
      pb.data(value, anchor);
      XMLCursor::decode(value);
      pb.skipChar();

      attributes[attribute] = value;
      pb.skipWhitespace();
   }

   return attributes;
}

Data
XMLCursor::getValue() const
{
   if (atLeaf())
   {
      Data value;
      ParseBuffer pb(mCursor->mPb);
      pb.skipToEnd();
      value = pb.data(pb.start());
      XMLCursor::decode(value);
      return value;
   }
   else
   {
      return Data::Empty;
   }
}

XMLCursor::Node::Node(const ParseBuffer& pb)
   : mPb(pb.position(), pb.end() - pb.position()),
     mParent(0),
     mChildren(),
     mNext(mChildren.begin()),
     mIsLeaf(false)
{
   mPb.assertNotEof();
   CerrLog(<< "XMLCursor::Node::Node[" << Data(mPb.position(), mPb.end() - mPb.start()) << "]");
}

XMLCursor::Node::~Node()
{
   for (list<Node*>::iterator i = mChildren.begin();
        i != mChildren.end(); i++)
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

   return *pb.position() == Symbols::SLASH[0];
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

   CerrLog(<< "XMLCursor::Node::skipToEndTag(" << Data(mPb.position(), mPb.end() - mPb.position()) << ")");

   extractTag();

   //<foo />
   mPb.skipToChar(Symbols::RA_QUOTE[0]);
   if (*(mPb.position()-1) == Symbols::SLASH[0])
   {
      mPb.skipChar();
      return;
   }

   //<foo> ...<child> ... </child> </foo>
   //    ^
   mPb.skipChar();
   //<foo> ...<child> ... </child> </foo>
   //     ^
   while (true)
   {
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
         if (mTag.size() + mPb.position() > mPb.end())
         {
            InfoLog("XML: unexpected end");
            mPb.fail(__FILE__, __LINE__);
         }

         if (strncmp(mTag.data(), mPb.position(), mTag.size()) == 0)
         {
            //...</foo>
            //   ^
            mPb = ParseBuffer(mPb.start(), mPb.position() - mPb.start() - 2 - mTag.size());
            return;
         }
         else
         {
            InfoLog("Badly formed XML: unexpected endtag");
            mPb.fail(__FILE__, __LINE__);
         }
      }

      //<child>...
      // ^
      if (mPb.position() == mPb.start())
      {
         InfoLog("XML: badly formed element");
         mPb.fail(__FILE__, __LINE__);
      }

      mPb.reset(mPb.position()-1);
      //<child>...
      //^
      Node* child = new Node(mPb);
      addChild(child);
      child->skipToEndTag();
      XMLCursor::decodeName(child->mTag);
   }
}

//<!-- declarations for <head> & <body> -->
void
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
}
