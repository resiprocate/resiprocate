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

NumericPredicate::NumericPredicate(const LameFloat& min, 
                                    const LameFloat& max, 
                                    bool negate) :
   mMin(min),
   mMax(max),
   mNegated(negate)
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

bool 
NumericPredicate::matches(const NumericPredicate& pred) const
{
   if(mNegated)
   {
      if(pred.mNegated)
      {
         return !(mMin < pred.mMin) && !(mMax > pred.mMax);
      }
      else
      {
         return mMin > pred.mMax || mMax < pred.mMin;
      }
   }
   else
   {
      if(pred.mNegated)
      {
         // This is more complicated... this _only_ works if:
         // 1) pred is a <= type, and we are a >= type, and our min is less than 
         //    or equal to pred's max.
         // 2) pred is a >= type, and we are a <= type, and our max is greater 
         //    than or equal to pred's min.
         return (mMax == LameFloat::lf_max && 
                  pred.mMin == -LameFloat::lf_max &&
                  !(pred.mMax < mMin)) 
                     || 
                  (mMin == -LameFloat::lf_max && 
                  pred.mMax == LameFloat::lf_max &&
                  !(pred.mMin > mMax));
      }
      else
      {
         return !(mMin > pred.mMin) && !(mMax < pred.mMax);
      }
   }
}

NumericPredicateDisjunction::NumericPredicateDisjunction()
{}

NumericPredicateDisjunction::~NumericPredicateDisjunction()
{}

bool 
NumericPredicateDisjunction::matches(int t) const
{
   LameFloat test(t,0);
   return matches(test);
}

bool 
NumericPredicateDisjunction::matches(const LameFloat& test) const
{
   for(std::vector<NumericPredicate>::const_iterator i=mPredicates.begin();
         i!=mPredicates.end(); ++i)
   {
      if(i->matches(test))
      {
         return true;
      }
   }
   return false;
}

bool 
NumericPredicateDisjunction::matches(const NumericPredicate& pred) const
{
   for(std::vector<NumericPredicate>::const_iterator i=mPredicates.begin();
         i!=mPredicates.end(); ++i)
   {
      if(i->matches(pred))
      {
         return true;
      }
   }
   return false;
}

void 
NumericPredicateDisjunction::addPredicate(const NumericPredicate& pred)
{
   mPredicates.push_back(pred);
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
   while(true)
   {
      NumericPredicate pred;
      if(*pb.position()=='!')
      {
         pred.setNegated(true);
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
            pred.setMin(pb.lameFloat());
            pb.skipChar(':');
            pred.setMax(pb.lameFloat());
            break;
         case '>':
            // Greater than or equal to type
            pb.skipChars(">=");
            pred.setMin(pb.lameFloat());
            break;
         case '<':
            // Less than or equal to type
            pb.skipChars("<=");
            pred.setMax(pb.lameFloat());
            break;
         case '=':
            // Equal to type
            pb.skipChar();
            pred.setMin(pb.lameFloat());
            pred.setMax(pred.getMin());
            break;
         default:
            throw ParseException("Illegal starting character for BNF element "
                                 " <numeric-relation> : " + *pb.position(),
                                 "NumericFeatureParameter",
                                 __FILE__,
                                 __LINE__);
      }

      mValue.addPredicate(pred);

      if(*pb.position() != ',')
      {
         break;
      }
      pb.skipChar();
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
   const std::vector<NumericPredicate>& preds(mValue.getPredicates());
   bool first=true;
   for(std::vector<NumericPredicate>::const_iterator p=preds.begin();
         p!=preds.end(); ++p)
   {
      if(!first)
      {
         stream << ',';
      }
      first=false;

      if(p->getNegated())
      {
         stream << "!";
      }
      stream << "#";
   
      if(p->getMin()==-LameFloat::lf_max)
      {
         if(p->getMax()==LameFloat::lf_max)
         {
            ErrLog(<< "Accessing defaulted NumericFeatureParam: '" 
                     << getName() << "'");
            // ?bwc? Bail?
         }
         else
         {
            // LTE type
            stream << "<=" << p->getMax();
         }
      }
      else
      {
         if(p->getMax()==LameFloat::lf_max)
         {
            // GTE type
            stream << ">=" << p->getMin();
         }
         else
         {
            if(p->getMax() == p->getMin())
            {
               // Equal type
               stream << "=" << p->getMax();
            }
            else
            {
               // Range type
               stream << p->getMin() << ":" << p->getMax();
            }
         }
      }
   }
   return stream << "\"";
}

} // of namespace resip
