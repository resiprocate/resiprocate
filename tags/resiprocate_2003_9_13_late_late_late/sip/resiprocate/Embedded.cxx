#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif


#include <cassert>

#include "resiprocate/Embedded.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/Symbols.hxx"

using namespace resip;

char fromHex(char h1, char h2)
{
   h1 = toupper(h1);
   h2 = toupper(h2);

   int i1;
   int i2;

   if (isdigit(h1))
   {
      i1 = h1 - '0';
   }
   else
   {
      i1 = h1 - 'A' + 10;
   }

   if (isdigit(h2))
   {
      i2 = h2 - '0';
   }
   else
   {
      i2 = h2 - 'A' + 10;
   }
   
   return i1*16+i2;
}

char*
Embedded::decode(const Data& in, unsigned int& count)
{
   const char *get = in.data();
   const char *end = get + in.size();
   char *ret = new char[in.size()];
   char *put = ret;

   count = 0;
   while (get != end)
   {
      if (*get == Symbols::PERCENT[0])
      {
         *put = fromHex(*(get+1), *(get+2));
         get += 3;
         assert(get <= end);
      }
      else
      {
         *put = *get;
         get++;
      }
      count++;
      put++;
   }

   return ret;
}

static char hexMap[] = "0123456789ABCDEF";

Data
Embedded::encode(const Data& dat)
{
   Data out((int)(dat.size()*1.1), true);
   
   {
      DataStream str(out);
      for (Data::size_type i = 0; i < dat.size(); i++)
      {
         switch (dat[i])
         {
            case ';' :
            case '@' :
            case '&' :
            case '=' :
            case ' ' :
            case ',' :
            case '%' : 
            {
               str << Symbols::PERCENT;

               unsigned char temp = dat[i];
               int hi = (temp & 0xf0)>>4;
               int low = (temp & 0xf);

               str << hexMap[hi];
               str << hexMap[low];
               break;
            }
            default :
               str << dat[i];
         }
      }
   }

   return out;
}
