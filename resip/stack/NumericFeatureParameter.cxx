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
   mMin(DBL_MIN),
   mMax(DBL_MAX),
   mNegated(false),
   mMinPrecision(3),
   mMaxPrecision(3)
{}

NumericPredicate::~NumericPredicate()
{}

bool 
NumericPredicate::matches(double test) const
{
   // !bwc! Need to make this consistent with the precision reqs...
   return mNegated ^ ((test <= mMax) && (test >= mMin));
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
         mValue.setMin(pb.floatVal());
         mValue.setMinPrecision(countDecimals(pb));
         pb.skipChar(':');
         mValue.setMax(pb.floatVal());
         // .bwc. Different from countDecimals since we might grab the decimal
         // from the _lower_ bound.
         const char* end=pb.position();
         pb.skipBackToOneOf(".:");
         if(pb.bof() || *pb.position()==':')
         {
            mValue.setMaxPrecision(0);
         }
         else
         {
            mValue.setMaxPrecision(end-pb.position());
         }
         pb.reset(end);
         break;
      case '>':
         // Greater than or equal to type
         pb.skipChars(">=");
         mValue.setMin(pb.floatVal());
         mValue.setMinPrecision(countDecimals(pb));
         break;
      case '<':
         // Less than or equal to type
         pb.skipChars("<=");
         mValue.setMax(pb.floatVal());
         mValue.setMaxPrecision(countDecimals(pb));
         break;
      case '=':
         // Equal to type
         pb.skipChar();
         float value = pb.floatVal();
         UInt8 precision=countDecimals(pb);
         mValue.setMax(value);
         mValue.setMin(value);
         mValue.setMaxPrecision(precision);
         mValue.setMinPrecision(precision);
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
   stream << getName() << "=\"#";
   std::ios_base::fmtflags oldFlags(stream.flags());
   if(mValue.getMin()==DBL_MIN)
   {
      if(mValue.getMax()==DBL_MAX)
      {
         ErrLog(<< "Accessing defaulted NumericFeatureParam: '" 
                  << getName() << "'");
         // ?bwc? Bail?
      }
      else
      {
         // LTE type
         stream << "<=" << std::setprecision(mValue.getMaxPrecision()) << mValue.getMax();
      }
   }
   else
   {
      if(mValue.getMax()==DBL_MAX)
      {
         // GTE type
         stream << ">=" << std::setprecision(mValue.getMinPrecision()) << mValue.getMin();
      }
      else
      {
         if(mValue.getMax() == mValue.getMin())
         {
            // Equal type
            stream << "=" << std::setprecision(mValue.getMaxPrecision()) << mValue.getMax();
         }
         else
         {
            // Range type
            stream << std::setprecision(mValue.getMinPrecision()) 
                  << mValue.getMin() 
                  << ":" 
                  << std::setprecision(mValue.getMaxPrecision()) 
                  << mValue.getMax();
         }
      }
   }
   stream.flags(oldFlags);
   return stream << "\"";
}

UInt8 
NumericFeatureParameter::countDecimals(ParseBuffer& pb) const
{
   const char* const end=pb.position();
   pb.skipBackToChar('.');
   if(!pb.bof())
   {
      const char* const begin=pb.position();
      pb.reset(end);
      return end - begin;
   }

   pb.reset(end);
   return 0;
}

} // of namespace resip
