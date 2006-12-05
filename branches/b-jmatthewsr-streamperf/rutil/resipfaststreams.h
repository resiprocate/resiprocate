#pragma once

#include <ostream>
#include <cassert>
#include "rutil/compat.hxx"

namespace resip { 

class  ResipStreamBuf
{
public:
	 ResipStreamBuf(void){}
	virtual ~ ResipStreamBuf(void){}

	virtual size_t writebuf(const char *s, size_t count) = 0;
	virtual size_t putbuf(char ch) = 0;
	virtual void flushbuf(void)=0;
	virtual UInt64 tellpbuf(void)=0;
};

class ResipBasicIOStream
{
public:
	ResipBasicIOStream(void):good_(false),eof_(true){}
	~ResipBasicIOStream(void){}

	bool good(void) const { return good_; }
	bool eof(void) const { return eof_; }
	void clear(void) const {};

protected:
	bool good_;
	bool eof_;

};


#if (defined(WIN32) || defined(_WIN32_WCE))

	#if (defined(_MSC_VER) && _MSC_VER >= 1400 )
		#define SNPRINTF_1(buffer,sizeofBuffer,count,format,var1) _snprintf_s(buffer,sizeofBuffer,_TRUNCATE,format,var1)
		#define LTOA(a,b,c,d) _ltoa_s(a,b,c,d)
		#define ULTOA(a,b,c,d) _ultoa_s(a,b,c,d)
		#define GCVT(val,num,buffer,buffersize) _gcvt_s(buffer,buffersize,val,num)
	#else
		#define SNPRINTF_1(buffer,sizeofBuffer,count,format,var1) _snprintf(buffer,count,format,var1)
		#define LTOA(a,b,c,d) _ltoa(a,b,d)
		#define ULTOA(a,b,c,d) _ultoa(a,b,d)
		#define GCVT(val,sigdigits,buffer,buffersize) _gcvt(val,num,buffer)
		#define _CVTBUFSIZE 309+40
	#endif

#else //non-windows
#define _TRUNCATE -1
#define SNPRINTF_1(buffer,sizeofBuffer,count,format,var1) snprintf(buffer,sizeofBuffer,format,var1)
#define LTOA(l,buffer,bufferlen,radix) SNPRINTF_1(buffer,bufferlen,bufferlen,"%li",l)/*ltoa(l,buffer,radix)*/
#define ULTOA(ul,buffer,bufferlen,radix) SNPRINTF_1(buffer,bufferlen,bufferlen,"%lu",ul)/*ultoa(a,b,d)*/
#define GCVT(f,sigdigits,buffer,bufferlen) SNPRINTF_1(buffer,bufferlen,bufferlen,"%f",f)/*gcvt(val,num,buffer)*/
#define _CVTBUFSIZE 309+40
#endif

class ResipFastOStream : public ResipBasicIOStream
{
public:
	ResipFastOStream(ResipStreamBuf *buf):buf_(buf){}
	virtual ~ResipFastOStream(void){}	

	virtual UInt64 tellp(void){ return rdbuf()->tellpbuf(); }

	ResipStreamBuf * rdbuf(void) const { return buf_; }
	ResipFastOStream & flush(void) { rdbuf()->flushbuf(); return *this; }

	ResipFastOStream &write(const char *s, size_t count){ if( rdbuf() ) { rdbuf()->writebuf(s,count);} return *this; }
	ResipFastOStream &put(const char ch){ if( rdbuf() ){ rdbuf()->putbuf(ch); } return *this; }
	ResipFastOStream& operator<<(bool b)
	{
		//int i = (b == true) ? (1):(0);
		*this<<(static_cast<long>(b)); 
		return *this;
	}

	ResipFastOStream& operator<<(short s)
	{
		*this<<(static_cast<long>(s));
		return *this;
	}

	ResipFastOStream& operator<<(unsigned short us)
	{
		*this<<(static_cast<unsigned long>(us));
		return *this;
	}

	ResipFastOStream& operator<<(int i)
	{
		*this<<(static_cast<long>(i));
		return *this;
	}
	
#ifdef _W64
	//for size_t
	ResipFastOStream& operator<<(_W64 unsigned int ui)
	{
		*this<<(static_cast<unsigned long>(ui));
		return *this;
	}
#else
	ResipFastOStream& operator<<(unsigned int ui)
	{
		*this<<(static_cast<unsigned long>(ui));
		return *this;
	}
#endif

	ResipFastOStream& operator<<(long l)
	{		
		char buf[33];
		LTOA(l,buf,33,10);
		size_t count = strlen(buf);
		if( buf_->writebuf(buf,count) < count )
		{
			good_ = false;
		}

		return *this;
	}
	
	ResipFastOStream& operator<<(unsigned long ul)
	{
		char buf[33];
		ULTOA(ul,buf,33,10);
		size_t count = strlen(buf);
		if( buf_->writebuf(buf,count) < count )
		{
			good_ = false;
		}

		return *this;
	}

#ifdef WIN32
	ResipFastOStream& operator<<(__int64 i64)
	{
		char buf[66];		
		_i64toa_s(i64,buf,66,10);
		size_t count = strlen(buf);
		if( buf_->writebuf(buf,count) < count )
		{
			good_ = false;
		}

		return *this;
	}

	ResipFastOStream& operator<<(unsigned __int64 ui64)
	{
		char buf[66];
		_ui64toa_s(ui64,buf,66,10);
		size_t count = strlen(buf);
		if( buf_->writebuf(buf,count) < count )
		{
			good_ = false;
		}

		return *this;
	}
#else
	ResipFastOStream& operator<<(UInt64 ui64)
	{
		char buf[66];
		SNPRINTF_1(buf,66,66,"%llu",ui64);

		size_t count = strlen(buf);
		if( buf_->writebuf(buf,count) < count )
		{
			good_ = false;
		}

		return *this;
	}
#endif

	ResipFastOStream& operator<<(float f)
	{
		*this<< (static_cast<double>(f));

		return *this;
	}

	ResipFastOStream& operator<<(double d)
	{
		char buf[_CVTBUFSIZE];
		GCVT(d,6,buf,_CVTBUFSIZE);//6 significant digits is the default for %f
		size_t count = strlen(buf);
		if( buf_->writebuf(buf,count) < count )
		{
			good_ = false;
		}

		return *this;
	}

	ResipFastOStream& operator<<(const void *vp)
	{
		char buf[32];
		SNPRINTF_1(buf,32,_TRUNCATE,"%p",vp);
		size_t count = strlen(buf);
		if( buf_->writebuf(buf,count) < count )
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
		if( _Pfn == static_cast<std::ostream& (*)(std::ostream&)>(std::endl) )
		{
			if( buf_->writebuf("\r\n",2) < 2 )
			{
				good_ = false;
			}
		}
		else
		{
			assert(0);
		}
		return *this;
	}


private:
	ResipStreamBuf *buf_;

};


inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr,
								const char *str)
{
	ostr.write(str,strlen(str));

	return ostr;
}

inline resip::ResipFastOStream& operator<<(resip::ResipFastOStream& ostr,
								char ch)
{
	ostr.put(ch);

	return ostr;
}

inline resip::ResipFastOStream & operator<<(resip::ResipFastOStream &ostr, const std::string &str)
{
	ostr.write(str.c_str(),str.size());

	return ostr;
}

//#define  RESIP_USE_STL_STREAMS

#ifdef  RESIP_USE_STL_STREAMS
	typedef std::ostream EncodeStream;
#else
	typedef ResipFastOStream EncodeStream;
#endif

} //namespace resip





