/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#include "rutil/DbSerializationHelper.hxx"

#include "rutil/EsLogger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::NONE

namespace resip
{

void 
DbSerializationHelper::encodeData(const resip::Data& token,resip::oDataStream& s)
{
   short len = (short)token.size();
   s.write( (char*)(&len) , sizeof( len ) );
   s.write( token.data(), len );
}

void 
DbSerializationHelper::encodeInt(const int token,resip::oDataStream& s)
{
   s.write( (char*)(&token) , sizeof(token) );
}

void 
DbSerializationHelper::encodeShort(const short token,resip::oDataStream& s)
{
   s.write( (char*)(&token) , sizeof(token) );
}

void 
DbSerializationHelper::encodeUShort(const unsigned short token,resip::oDataStream& s)
{
   s.write( (char*)(&token) , sizeof(token) );
}

void 
DbSerializationHelper::encodeDataList(const std::list<resip::Data>& token,resip::oDataStream& s)
{
   s.write("dl",3);
   short len=token.size();
   s.write((char*)(&len),sizeof(len));
   std::list<resip::Data>::const_iterator i;
   for(i=token.begin();i!=token.end();i++)
   {
      encodeData(*i,s);
   }
   
}

void 
DbSerializationHelper::encodeIntList(const std::list<int>& token,resip::oDataStream& s)
{
   s.write("il",3);
   short len=token.size();
   s.write((char*)(&len),sizeof(len));
   std::list<int>::const_iterator i;
   for(i=token.begin();i!=token.end();i++)
   {
      encodeInt(*i,s);
   }
}

void 
DbSerializationHelper::encodeUShortList(const std::list<unsigned short>& token,resip::oDataStream& s)
{
   s.write("usl",4);
   short len=token.size();
   s.write((char*)(&len),sizeof(len));
   std::list<unsigned short>::const_iterator i;
   for(i=token.begin();i!=token.end();i++)
   {
      encodeUShort(*i,s);
   }
}

void
DbSerializationHelper::encodeBool(const bool& token,resip::oDataStream& s)
{
   s.write((char*)(&token),sizeof(token));
}

resip::Data 
DbSerializationHelper::decodeData(resip::iDataStream& s)
{
	short len;
	s.read( (char*)(&len), sizeof(len) ); 

   if(len > 0)
   {
      char buf[len+1];
      s.read( buf, len );
      
      Data data( buf, len );
      return data;      
	}
   else
   {
      return Data::Empty;
   }
}

int 
DbSerializationHelper::decodeInt(resip::iDataStream& s)
{
   int result;
   s.read((char*)(&result),sizeof(result));
   return result;
}

short 
DbSerializationHelper::decodeShort(resip::iDataStream& s)
{
   short result;
   s.read((char*)(&result),sizeof(result));
   return result;
}

unsigned short 
DbSerializationHelper::decodeUShort(resip::iDataStream& s)
{
   unsigned short result;
   s.read((char*)(&result),sizeof(result));
   return result;
}

std::list<resip::Data> 
DbSerializationHelper::decodeDataList(resip::iDataStream& s)
{
   std::list<resip::Data> result;
   char buf[3];
   s.read(buf,3);
   
   if(buf[0]=='d' && buf[1]=='l' && buf[2]=='\0')
   {
      short len;
      s.read( (char*)(&len), sizeof(len) );
      if(len < 0)
      {
         ErrLog(<<"Corrupt Db!");
      }
      else
      {
         for(short i=0;i<len;i++)
         {
            result.push_back(decodeData(s));
         }
      }
   }
   
   return result;
}

std::list<int> 
DbSerializationHelper::decodeIntList(resip::iDataStream& s)
{
   std::list<int> result;
   char buf[3];
   s.read(buf,3);
   
   if(buf[0]=='i' && buf[1]=='l' && buf[2]=='\0')
   {
      short len;
      s.read( (char*)(&len), sizeof(len) );
      if(len < 0)
      {
         ErrLog(<<"Corrupt Db!");
      }
      else
      {
         for(short i=0;i<len;i++)
         {
            result.push_back(decodeInt(s));
         }
      }
      
   }
   
   return result;

}

std::list<unsigned short> 
DbSerializationHelper::decodeUShortList(resip::iDataStream& s)
{
   std::list<unsigned short> result;
   char buf[4];
   s.read(buf,4);
   
   if(buf[0]=='u' && buf[1]=='s' && buf[2]=='l' && buf[3]=='\0')
   {
      short len;
      s.read( (char*)(&len), sizeof(len) );
      if(len < 0)
      {
         ErrLog(<<"Corrupt Db!");
      }
      else
      {
         for(short i=0;i<len;i++)
         {
            result.push_back(decodeUShort(s));
         }
      }
   }
   
   return result;

}

bool 
DbSerializationHelper::decodeBool(resip::iDataStream& s)
{
   bool result;
   s.read((char*)(&result),sizeof(result));
   return result;
}




}

/* Copyright 2007 Estacado Systems */
