#include <sipstack/ParserCategories.hxx>
#include <iostream>


using namespace Vocal2;
using namespace std;

ParserCategory* 
StringComponent::clone(HeaderFieldValue*) const
{
   assert(0);
   return 0;
}

//====================
// String:
//====================
void 
StringComponent::parse()
{
   mValue = Data(getHeaderField().mField, getHeaderField().mFieldLength);
}

std::ostream& 
StringComponent::encode(std::ostream& str) const
{
   str << mValue;
   return str;
}

Data& 
StringComponent::value() 
{ 
   checkParsed();
   return mValue; 
}

MethodTypes
RequestLineComponent::getMethod() const
{
   assert(0);
   return INVITE;
}

//====================
// CSeqComponent:
//====================
CSeqComponent::CSeqComponent(HeaderFieldValue& hfv) 
  :ParserCategory(&hfv)
{
  // need to call base constructor when it exists?
}


ParserCategory* CSeqComponent::clone(HeaderFieldValue*) const
{
  
  cerr << "No body for CSeqComponent::clone()" << endl;
  assert(0);
  
}


void
CSeqComponent::parse()
{

  Data number;
  Data method = Data(mHeaderField->mField, mHeaderField->mFieldLength);
  int ret = method.match(" ", &number, true);
  if (ret == FOUND)
    {
      mCSeq = number.convertInt();
      mMethod = getMethodType(method);
    }
  else if (ret == NOT_FOUND)
    {
      ParseException except;
      throw except;
    }
  else if (ret == FIRST)
    {
       ParseException except;
      throw except;
    }

}

std::ostream& 
CSeqComponent::encode(std::ostream& str) const
{
  // NEED TO FIX THIS!!!
  // method needs to print out a string, not the enum type
  str << mMethod << " " << mCSeq;
  return str;
}


enum MethodTypes
CSeqComponent::getMethod()
{
  checkParsed();
  return mMethod;
}


int&
CSeqComponent::cSeq()
{
  checkParsed();
  return mCSeq;
}


//====================
// Integer:
//====================
IntegerComponent::IntegerComponent(HeaderFieldValue& hfv) 
  :ParserCategory(&hfv), mHasComment(false)
{
  // need to call base constructor when it exists?
}


ParserCategory* IntegerComponent::clone(HeaderFieldValue*) const
{
  
  cerr << "No body for IntegerComponent::clone()" << endl;
  assert(0);
  
}


void
IntegerComponent::parse()
{

  Data rawdata = Data(mHeaderField->mField, mHeaderField->mFieldLength);

  // look for a comment
  Data comment;
  int retn = rawdata.match("(", &comment, true);
  Data params;

  // Starts with a comment, bad
  if (retn == FIRST)
    {
      ParseException except;
      throw except;
    }

  // we have a comment, handle it
  else if (retn == FOUND)
    {

      mHasComment = true;

      // right now only look to verify that they close
      // we really need to also look for escaped \) parens (or anything
      // else for that matter) and also replace cr lf with two spaces
      
      Data remainder;
      retn = comment.match(")", &remainder, true);
      
      // if it is not found, they never close, if it is first, the comment
      // is empty. Either is illegal
      if (retn != FOUND)
	{
	  ParseException except;
	  throw except;
	}

      mComment = comment;

      // we should immediately see a ; or nothing, so see if there 
      // is anything after the )
      
      if (remainder.size() != 0)
	{
	  
	  retn = remainder.match(";", &params, true);
	  if (retn != FIRST)
	    {
	      // something between the comment and the ;, or something
	      // after the ) and no ;. Both are bad.
	      ParseException except;
	      throw except;
	    }
	  else
	    {
	      // walk params and populate list
	    }
	}
      
      mValue = rawdata.convertInt();

    }
  
  // no comment
  else if (retn == NOT_FOUND)
    {
      
      int ret = rawdata.match(";", &params, true);
      if (ret == FOUND)
        {
	  // walk params and populate list
	}

      mValue = rawdata.convertInt();

    }
} 


std::ostream& 
IntegerComponent::encode(std::ostream& str) const
{
  
  str << mValue;
  if (mHasComment)
    {
      str << "(" << mComment << ")";
    }
  
  // call encode on the list to get params
  return str;
}


Data&
IntegerComponent::comment()
{
  checkParsed();
  return mComment;
}


int&
IntegerComponent::value()
{
  checkParsed();
  return mValue;
}


int
StatusLineComponent::getResponseCode() const
{
   assert(0);
   return 200;
}
