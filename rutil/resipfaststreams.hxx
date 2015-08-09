#if !defined(RESIP_RESIPFASTSTREAMS_HXX)
#define RESIP_RESIPFASTSTREAMS_HXX
/*! \file resipfaststreams.hxx
    \brief Replaces STL streams for general encoding purposes.

    #define RESIP_USE_STL_STREAMS will use the STL for stream encoding (std::ostream).  Undefining RESIP_USE_STL_STREAMS will
      cause resip to use the alternative stream handling defined in this file for encoding objects.
*/
#define RESIP_USE_STL_STREAMS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream> //for std::endl, std::cerr, etc.
#include <stdio.h> //for snprintf

#include "rutil/ResipAssert.h"
#include "rutil/compat.hxx"

namespace resip
{

class ResipStreamBuf
{
   public:
      ResipStreamBuf(void)
      {}
      virtual ~ ResipStreamBuf(void)
      {}
      virtual size_t writebuf(const char *s, size_t count) = 0;
      virtual size_t readbuf(char *buf, size_t count) = 0;
      virtual size_t putbuf(char ch) = 0;
      virtual void flushbuf(void)=0;
      virtual UInt64 tellpbuf(void)=0;
};

class ResipBasicIOStream
{
   public:
      ResipBasicIOStream(void):good_(false),eof_(true)
      {}
      ~ResipBasicIOStream(void)
      {}

      bool good(void) const
      {
         return good_;
      }
      bool eof(void) const
      {
         return eof_;
      }
      void clear(void) const
      {}

   protected:
      bool good_;
      bool eof_;
};

/** std::ostream replacement.
*/
class ResipFastOStream : public ResipBasicIOStream
{
   public:
      ResipFastOStream(ResipStreamBuf *buf):buf_(buf)
      {
         good_ = true;
      }
      virtual ~ResipFastOStream(void)
      {}

      virtual UInt64 tellp(void)
      {
         if (rdbuf())
         {
            return rdbuf()->tellpbuf();
         }
         return 0;
      }

      ResipStreamBuf * rdbuf(void) const
      {
         return buf_;
      }

      void rdbuf(ResipStreamBuf *buf)
      {
         buf_ = buf;
      }

      ResipFastOStream & flush(void)
      {
         if (rdbuf())
         {
            rdbuf()->flushbuf();
         }
         return *this;
      }

      ResipFastOStream &write(const char *s, size_t count)
      {
         if (rdbuf())
         {
            rdbuf()->writebuf(s,count);
         }
         return *this;
      }

      ResipFastOStream &put(char ch)
      {
         if (rdbuf())
         {
            rdbuf()->putbuf(ch);
         }
         return *this;
      }

      ResipFastOStream& operator<<(bool b)
      {
         //int i = (b == true) ? (1):(0);
         *this<<(static_cast<Int32>(b));
         return *this;
      }

      ResipFastOStream& operator<<(Int16 i)
      {
         *this<<(static_cast<Int32>(i));
         return *this;
      }

      ResipFastOStream& operator<<(UInt16 ui)
      {
         *this<<(static_cast<UInt32>(ui));
         return *this;
      }

      ResipFastOStream& operator<<(Int32 l)
      {
         if (!buf_)
         {
            return *this;
         }
         char buf[33];
         //snprintf(buf,33,"%" PRId32,l);
         LTOA((long int)l,buf,33,10);
         size_t count = strlen(buf);
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }

      ResipFastOStream& operator<<(UInt32 ul)
      {
         if (!buf_)
         {
            return *this;
         }
         char buf[33];
         //snprintf(buf,33,"%" PRIu32,ul);
         ULTOA((unsigned long int)ul,buf,33,10);
         size_t count = strlen(buf);
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }

      ResipFastOStream& operator<<(long l)
      {
         if (!buf_)
         {
            return *this;
         }

         char buf[66];
         LTOA(l,buf,66,10);

         size_t count = strlen(buf);
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }

      ResipFastOStream& operator<<(unsigned long ul)
      {
         if (!buf_)
         {
            return *this;
         }

         char buf[66];
         ULTOA(ul,buf,66,10);

         size_t count = strlen(buf);
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }

      ResipFastOStream& operator<<(Int64 i64)
      {
         if (!buf_)
         {
            return *this;
         }

         char buf[66];
         //snprintf(buf,66,"%" PRId64,i64);
         I64TOA(i64,buf,66,10);

         size_t count = strlen(buf);
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }

      ResipFastOStream& operator<<(UInt64 ui64)
      {
         if (!buf_)
         {
            return *this;
         }

         char buf[66];
         //snprintf(buf,66,"%" PRIu64,ui64);
         UI64TOA(ui64,buf,66,10);

         size_t count = strlen(buf);
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }

      ResipFastOStream& operator<<(float f)
      {
         *this<< (static_cast<double>(f));

         return *this;
      }

      ResipFastOStream& operator<<(double d)
      {
         if (!buf_)
         {
            return *this;
         }

         char buf[_CVTBUFSIZE];
         GCVT(d,6,buf,_CVTBUFSIZE);//6 significant digits is the default for %f
         size_t count = strlen(buf);
#ifndef WIN32 
         //not using the non-standard microsoft conversion functions
         //remove any trailing zeros.  Note that resipfastreams does not support STL stream width or precision
         //modifiers
         size_t idx=0;
         for (; count > 1; count--)
         {
            idx = count-1;
            if (buf[idx] != '0' && buf[idx] != '.')
            {
               break;
            }
         }
#endif
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }

      ResipFastOStream& operator<<(const void *vp)
      {
         if (!buf_)
         {
            return *this;
         }

         char buf[32];
         SNPRINTF_1(buf,32,_TRUNCATE,"%p",vp);
         size_t count = strlen(buf);
         if (buf_->writebuf(buf,count) < count)
         {
            good_ = false;
         }

         return *this;
      }
#ifdef WIN32
      ResipFastOStream& operator<<(std::ostream& (__cdecl *_Pfn)(std::ostream&))
#else
      ResipFastOStream& operator<<(std::ostream& (*_Pfn)(std::ostream &))
#endif
      {
         if (!buf_)
         {
            return *this;
         }

         if (_Pfn == static_cast<std::ostream& (*)(std::ostream&)>(std::endl))
         {
            if (buf_->writebuf("\n",1) < 1)
            {
               good_ = false;
            }
         }
         else
         {
            resip_assert(0);
         }
         return *this;
      }

   private:
      ResipStreamBuf *buf_;
};


inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr, const char *str)
{
   ostr.write(str,strlen(str));

   return ostr;
}

inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr, char ch)
{
   ostr.put(ch);

   return ostr;
}

inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr, unsigned char ch)
{
   ostr.put((char)ch);

   return ostr;
}

inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr, const unsigned char *str)
{
   ostr.write((const char *)str,strlen((const char *)str));

   return ostr;
}

inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr, signed char ch)
{
   ostr.put((char)ch);

   return ostr;
}

inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr, const signed char *str)
{
   ostr.write((const char *)str,strlen((const char *)str));

   return ostr;
}

inline resip::ResipFastOStream & operator<<(resip::ResipFastOStream &ostr, const std::string &str)
{
   ostr.write(str.c_str(),str.size());

   return ostr;
}

/** std::istream replacement
*/
class ResipFastIStream : public ResipBasicIOStream
{
   public:
      ResipFastIStream(ResipStreamBuf *buf):buf_(buf)
      {}
      virtual ~ResipFastIStream(void)
      {}

      ResipStreamBuf * rdbuf(void) const
      {
         return buf_;
      }

      ResipFastIStream &read(char *s, size_t count)
      {
         if (rdbuf())
         {
            rdbuf()->readbuf(s,count);
         }
         return *this;
      }

   private:
      ResipStreamBuf *buf_;
};

/** Used to replace std::cerr, std::cout, etc.
*/
class ResipStdBuf : public ResipStreamBuf
{
   public:
      typedef enum BufType
      {
         null,
         stdCerr,
         stdCout
      } BufType;

      ResipStdBuf(BufType type)
            :type_(type)
      {}

      ~ResipStdBuf(void)
      {}

      virtual size_t writebuf(const char *s, size_t count)
      {
         switch (type_)
         {
            case stdCerr:
            {
               std::cerr << s;
               break;
            }
            case stdCout:
            {
               std::cout << s;
               break;
            }
            default:
               break;
         }
         return count;
      }
      virtual size_t readbuf(char *buf, size_t count)
      {
         return 0;
      }
      virtual size_t putbuf(char ch)
      {
         return writebuf(&ch,1);
      }
      virtual void flushbuf(void)
      {}
      virtual UInt64 tellpbuf(void)
      {
         return 0;
      }

   private:
      BufType type_;
};

/** A direct replacement for std::cout, std::cerr, etc.
*/
class ResipStdCOStream: public ResipFastOStream
{
   public:
      ResipStdCOStream(ResipStdBuf::BufType type)
            :ResipFastOStream(0),buf_(type)
      {
         rdbuf(&buf_);
      }

      ~ResipStdCOStream(void)
      {}

   private:
      ResipStdBuf buf_;
};

#ifdef  RESIP_USE_STL_STREAMS
#define EncodeStream std::ostream
#define DecodeStream std::istream
#define resipCerr std::cerr
#define resipCout std::cout
#else
#define EncodeStream resip::ResipFastOStream
#define DecodeStream resip::ResipFastIStream
extern ResipStdCOStream resipFastCerr;
extern ResipStdCOStream resipFastCout;
#define resipCerr resip::resipFastCerr
#define resipCout resip::resipFastCout
#endif
extern ResipStdCOStream resipFastNull;

} //namespace resip

#endif //RESIP_RESIPFASTSTREAMS_HXX

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2005.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */




