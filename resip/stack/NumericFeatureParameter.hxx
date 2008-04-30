#ifndef NumericFeatureParameter_Include_Guard
#define NumericFeatureParameter_Include_Guard

#include "resip/stack/Parameter.hxx"

#include "resip/stack/ParameterTypeEnums.hxx"

#include "rutil/LameFloat.hxx"

#include <ostream>

namespace resip
{
class NumericPredicate
{
   public:
      NumericPredicate();
      NumericPredicate(const LameFloat& min, const LameFloat& max, bool negate);
      ~NumericPredicate();

      bool matches(int test) const;
#ifndef RESIP_FIXED_POINT
      inline double getMinAsDouble() const {return double(mMin);}
      inline double getMaxAsDouble() const {return double(mMax);}
#endif

      bool matches(const LameFloat& num) const;
      bool matches(const NumericPredicate& pred) const;
      inline const LameFloat& getMin() const { return mMin;} 
      inline void setMin(const LameFloat& pMin) { mMin = pMin;}
      inline const LameFloat& getMax() const { return mMax;} 
      inline void setMax(const LameFloat& pMax) { mMax = pMax;}

      inline bool getNegated() const { return mNegated;} 
      inline void setNegated(bool pNegated) { mNegated = pNegated;}

   private:
      LameFloat mMin;
      LameFloat mMax;
      bool mNegated;
};

class NumericPredicateDisjunction
{
   public:
      NumericPredicateDisjunction();
      ~NumericPredicateDisjunction();

      bool matches(int test) const;
      bool matches(const LameFloat& test) const;
      bool matches(const NumericPredicate& pred) const;
      inline const std::vector<NumericPredicate>& getPredicates() const { return mPredicates;} 
      inline std::vector<NumericPredicate>& getPredicates() { return mPredicates;} 
      void addPredicate(const NumericPredicate& pred);

   private:
      std::vector<NumericPredicate> mPredicates;
};

class NumericFeatureParameter : public Parameter
{
   public:
      typedef NumericPredicateDisjunction Type;
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
      NumericPredicateDisjunction mValue;
}; // class NumericFeatureParameter

} // namespace resip

#endif // include guard
