#include <sipstack/ParserCategories.hxx>
#include <iostream>


using namespace Vocal2;
using namespace std;

Data DefaultSipVersion("SIP/2.0");

//====================
// CSeqComponent:
//====================
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
      mSequence = number.convertInt();
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
   str << int(mMethod) << " " << mSequence;
   return str;
}

//====================
// Integer:
//====================
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
  if (!mComment.empty())
    {
      str << "(" << mComment << ")";
    }
  
  // call encode on the list to get params
  return str;
}

//====================
// String:
//====================
ParserCategory* 
StringComponent::clone(HeaderFieldValue*) const
{
   assert(0);
   return 0;
}

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

//====================
// NameAddr:
//====================
ParserCategory *
URI::clone(HeaderFieldValue*) const
{
  assert(0);
}

void
NameAddrBase::parse()
{
   assert(0);
}

ostream&
NameAddrBase::encode(ostream& str) const
{
   assert(0);
}

void
NameAddr::parse()
{
   assert(0);
}

ostream&
NameAddr::encode(ostream& str) const
{
   assert(0);
}

//====================
// RequestLine:
//====================
ParserCategory *
RequestLineComponent::clone(HeaderFieldValue*) const
{
  assert(0);
}

void 
RequestLineComponent::parse()
{
   assert(0);
}

ostream&
RequestLineComponent::encode(ostream& str) const
{
   assert(0);
}


//====================
// StatusLine:
//====================

ParserCategory *
StatusLineComponent::clone(HeaderFieldValue*) const
{
  assert(0);
}

void
StatusLineComponent::parse()
{
   assert(0);
}

ostream&
StatusLineComponent::encode(ostream& str) const
{
   assert(0);
}
