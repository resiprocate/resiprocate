#ifndef SysLogStream_hxx
#define SysLogStream_hxx

#include <sipstack/Log.hxx>
#include <sipstack/SysLogBuf.hxx>

namespace Vocal2
{

class SysLogStream : public std::ostream
{
   public:
      SysLogStream() : std::ostream (_buf = new SysLogBuf) 
      {
      }

      virtual ~SysLogStream()
      {
         // need to clean up the buf
         delete _buf;
      }

   private:
      SysLogBuf* _buf;
      
      SysLogStream(const SysLogStream& );
      SysLogStream& operator=(const SysLogStream&);
};

}


#endif
