#ifndef PARSEEXCEPTION_HXX
#define PARSEEXCEPTION_HXX

#include <exception>

namespace Vocal2
{

class Data;

class ParseException : public std::exception
{
   public:
      ParseException(const Data& msg, const Data& file, const int line) {}
      virtual const char* what() const throw() { return 0; }
};
 
}

#endif
