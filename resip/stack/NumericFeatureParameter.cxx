#include "resip/stack/NumericFeatureParameter.hxx"

#include "resip/stack/Symbols.hxx"

#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ParseException.hxx"

#include <float.h>
#include <iomanip>
#include <iostream>

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{
NumericPredicate::NumericPredicate() :
   mMin(-LameFloat::lf_max),
   mMax(LameFloat::lf_max),
   mNegated(false)
{}

NumericPredicate::~NumericPredicate()
{}

bool 
NumericPredicate::matches(int t) const
{
   LameFloat test(t, 0);
   return matches(test);
}

bool 
NumericPredicate::matches(const LameFloat& test) const
{
   return mNegated ^ (!(test > mMax) && !(test < mMin));
}

NumericFeatureParameter::NumericFeatureParameter(ParameterTypes::Type type, 
                                             ParseBuffer& pb, 
                                             const char* terminators) :
   Parameter(type)
{
   pb.skipWhitespace();
   pb.skipChar('=');
   pb.skipWhitespace();
   pb.skipChar('\"');
   if(*pb.position()=='!')
   {
      mValue.setNegated(true);
      pb.skipChar();
   }
   pb.skipChar('#');

   switch(*pb.position())
   {
      case '-':
      case '+':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
         // range type.
         mValue.setMin(pb.lameFloat());
         pb.skipChar(':');
         mValue.setMax(pb.lameFloat());
         break;
      case '>':
         // Greater than or equal to type
         pb.skipChars(">=");
         mValue.setMin(pb.lameFloat());
         break;
      case '<':
         // Less than or equal to type
         pb.skipChars("<=");
         mValue.setMax(pb.lameFloat());
         break;
      case '=':
         // Equal to type
         pb.skipChar();
         mValue.setMin(pb.lameFloat());
         mValue.setMax(mValue.getMin());
         break;
      default:
         throw ParseException("Illegal starting character for BNF element "
                              " <numeric-relation> : " + *pb.position(),
                              "NumericFeatureParameter",
                              __FILE__,
                              __LINE__);
   }
   pb.skipChar('\"');
}

NumericFeatureParameter::NumericFeatureParameter(ParameterTypes::Type type) :
   Parameter(type)
{}

NumericFeatureParameter::NumericFeatureParameter(const NumericFeatureParameter& orig) :
   Parameter(orig),
   mValue(orig.mValue)
{}

NumericFeatureParameter::~NumericFeatureParameter()
{}

Parameter* 
NumericFeatureParameter::clone() const 
{
   return new NumericFeatureParameter(*this);
}

std::ostream& 
NumericFeatureParameter::encode(std::ostream& stream) const 
{
   stream << getName() << "=\"";
   if(mValue.getNegated())
   {
      stream << "!";
   }
   stream << "#";

   if(mValue.getMin()==-LameFloat::lf_max)
   {
      if(mValue.getMax()==LameFloat::lf_max)
      {
         ErrLog(<< "Accessing defaulted NumericFeatureParam: '" 
                  << getName() << "'");
         // ?bwc? Bail?
      }
      else
      {
         // LTE type
         stream << "<=" << mValue.getMax();
      }
   }
   else
   {
      if(mValue.getMax()==LameFloat::lf_max)
      {
         // GTE type
         stream << ">=" << mValue.getMin();
      }
      else
      {
         if(mValue.getMax() == mValue.getMin())
         {
            // Equal type
            stream << "=" << mValue.getMax();
         }
         else
         {
            // Range type
            stream << mValue.getMin() 
                  << ":" 
                  << mValue.getMax();
         }
      }
   }
   return stream << "\"";
}

} // of namespace resip
