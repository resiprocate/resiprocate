#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/XMLCursor.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/os/Logger.hxx"

#ifndef WIN32 // !cj! TODO FIX 


using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

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
     mCursor(0)
{
   ParseBuffer lPb(pb);

   skipProlog(lPb);
   const char* start = lPb.position();

   bool needsPreparse;
   lPb.skipToChars(COMMENT_START);
   needsPreparse = !lPb.eof();
   lPb.reset(lPb.start());
   lPb.skipToChars(Symbols::CRLF);
   needsPreparse |= !lPb.eof();

   if (needsPreparse)
   {
      DebugLog(<< "removing comments and cannonicalzing endlines");
      lPb.reset(start);
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

static const Data QUESTION_RA_QUOTE("?>");
void
XMLCursor::skipProlog(ParseBuffer& pb)
{
   //'<?xml' VersionInfo '<xml?' EncodingDecl '?>'? '<?xml' SDDecl '?>'? S? '?>

   // !dlb! much more complicated than this.. can contain comments
   pb.skipToChars(QUESTION_RA_QUOTE);
   pb.skipN(2);
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

   // root end tag?
   if (*mRoot->mPb.position() == Symbols::LA_QUOTE[0])
   {
      ParseBuffer pb(mRoot->mPb.position(), 
                     mRoot->mPb.end() - mRoot->mPb.position());
      pb.skipChar();
      if (!pb.eof() && *pb.position() == Symbols::SLASH[0])
      {
         pb.skipChar();
         if (mTag.size() + pb.position() > pb.end())
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
   DebugLog(<< "XMLCursor::nextSibling" << this->mCursor << " " << this->mCursor->mParent);

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
      mCursor = *((mCursor->mParent->mNext)++);
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
const std::map<Data, Data>&
XMLCursor::getAttributes() const
{
   mAttributes.clear();

   if (!atLeaf())
   {
      ParseBuffer pb(mCursor->mPb);
      pb.reset(mCursor->mPb.start());

      Data attribute;
      Data value;

      pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::RA_QUOTE);

      while (!pb.eof() && *pb.position() != Symbols::RA_QUOTE[0])
      {
         attribute.clear();
         value.clear();

         const char* anchor = pb.skipWhitespace();
         pb.skipToOneOf(ParseBuffer::Whitespace, Symbols::EQUALS);
         pb.data(attribute, anchor);
         XMLCursor::decodeName(attribute);

         DebugLog(<< "attribute: " << attribute);

         pb.skipWhitespace();
         pb.skipToChar(Symbols::EQUALS[0]);
         pb.skipChar();
         pb.skipWhitespace();
	 if (!pb.eof())
	 {
	    const char quote = *pb.position();

	    DebugLog(<< "quote is <" << quote << ">");
	    
	    if (quote != Symbols::DOUBLE_QUOTE[0] &&
		quote != '\'')
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
      mValue = pb.data(pb.start());
      XMLCursor::decode(mValue);
   }
   else
   {
      mValue.clear();
   }
   return mValue;
}

XMLCursor::Node::Node(const ParseBuffer& pb)
   : mPb(pb.position(), pb.end() - pb.position()),
     mParent(0),
     mChildren(),
     mNext(mChildren.begin()),
     mIsLeaf(false)
{
   mPb.assertNotEof();
   DebugLog(<< "XMLCursor::Node::Node" << this << "[" << Data(mPb.position(), mPb.end() - mPb.start()) << "]");
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

   DebugLog(<< "XMLCursor::Node::skipToEndTag(" << Data(mPb.position(), mPb.end() - mPb.position()) << ")");

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
            InfoLog(<< "XML: unexpected end");
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
      DebugLog(<< mTag << "(" << child->mTag << ")");
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

#endif // WIN32
