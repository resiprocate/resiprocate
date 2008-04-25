#ifndef NumericFeatureParameter_Include_Guard
#define NumericFeatureParameter_Include_Guard

#include "resip/stack/Parameter.hxx"

#include "resip/stack/ParameterTypeEnums.hxx"

#include <ostream>

namespace resip
{
class NumericPredicate
{
   public:
      NumericPredicate();
      ~NumericPredicate();
      bool matches(double test) const;

      inline double getMin() const { return mMin;} 
      inline void setMin(double pMin) { mMin = pMin;}
      inline double getMax() const { return mMax;} 
      inline void setMax(double pMax) { mMax = pMax;}
      inline bool getNegated() const { return mNegated;} 
      inline void setNegated(bool pNegated) { mNegated = pNegated;}
      inline UInt8 getMinPrecision() const { return mMinPrecision;} 
      inline void setMinPrecision(UInt8 pMinPrecision) { mMinPrecision = pMinPrecision;}
      inline UInt8 getMaxPrecision() const { return mMaxPrecision;} 
      inline void setMaxPrecision(UInt8 pMaxPrecision) { mMaxPrecision = pMaxPrecision;}

   private:
      double mMin;
      double mMax;
      bool mNegated;
      UInt8 mMinPrecision;
      UInt8 mMaxPrecision;
};

class NumericFeatureParameter : public Parameter
{
   public:
      typedef NumericPredicate Type;
      NumericFeatureParameter(ParameterTypes::Type, 
                              ParseBuffer& pb, 
                              const char* terminators);
      explicit NumericFeatureParameter(ParameterTypes::Type);
      NumericFeatureParameter(const NumericFeatureParameter& orig);
      virtual ~NumericFeatureParameter();

      static Parameter* decode(ParameterTypes::Type type, 
                                 ParseBuffer& pb, 
                                 const char* terminators)
      {
         return new NumericFeatureParameter(type, pb, terminators);
      }

/////////////////// Must implement unless abstract ///

      virtual Parameter* clone() const ;
      virtual std::ostream& encode(std::ostream& stream) const ;

/////////////////// May override ///

      virtual bool isQuoted() const {return true;}
      Type& value() {return mValue;}

   private:
      NumericPredicate mValue;
      UInt8 countDecimals(ParseBuffer& pb) const;
}; // class NumericFeatureParameter

} // namespace resip

#endif // include guard
