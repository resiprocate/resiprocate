#ifndef Embedded_hxx
#define Embedded_hxx

namespace resip
{

class Data;

class Embedded
{
   public:
      static char* decode(const Data& input, unsigned int& decodedLength);
      static Data encode(const Data& input);

   private:
      Embedded();
};

}

#endif
