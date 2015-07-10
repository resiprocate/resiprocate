#include "DataBuffer.hxx"
#include <memory.h>
#include "rutil/ResipAssert.h"
#include <rutil/WinLeakCheck.hxx>

namespace reTurn {

void ArrayDeallocator(char* data)
{
   delete [] data;
}

DataBuffer::DataBuffer(const char* data, unsigned int size, deallocator dealloc)
   : mDealloc(dealloc)
{
   mBuffer = 0;
   mSize   = size;
   if ( mSize > 0 )
   {
      mBuffer = new char[mSize];
      memcpy(mBuffer, data, mSize);
   }
   mStart = mBuffer;
}

DataBuffer::DataBuffer(unsigned int size, deallocator dealloc)
   : mDealloc(dealloc)
{
   mBuffer = 0;
   mSize   = size;
   if ( mSize > 0 )
   {
      mBuffer = new char[mSize];
      memset(mBuffer, 0, mSize);
   }

   mStart  = mBuffer;
}

DataBuffer::~DataBuffer() 
{ 
   mDealloc(mBuffer);
}

DataBuffer* DataBuffer::own(char* data, unsigned int size, deallocator dealloc)
{
   DataBuffer* buff = new reTurn::DataBuffer(0, dealloc);
   buff->mBuffer = data;
   buff->mSize = size;
   buff->mStart = buff->mBuffer;
   return buff;
}

const char* 
DataBuffer::data() 
{ 
   return mStart; 
}

unsigned int 
DataBuffer::size() 
{ 
   return mSize; 
}

char* 
DataBuffer::mutableData()
{
   return mStart;
}

unsigned int&
DataBuffer::mutableSize()
{
   return mSize;
}

char& 
DataBuffer::operator[](unsigned int p) 
{ 
   resip_assert(p < mSize); 
   return mBuffer[p]; 
}

char 
DataBuffer::operator[](unsigned int p) const 
{ 
   resip_assert(p < mSize); 
   return mBuffer[p]; 
}

unsigned int 
DataBuffer::truncate(unsigned int newSize) 
{ 
   resip_assert(newSize <= mSize); 
   mSize = newSize; 
   return mSize; 
}

unsigned int 
DataBuffer::offset(unsigned int bytes) 
{ 
   resip_assert(bytes < mSize); 
   mStart = mStart+bytes; 
   mSize = mSize-bytes; 
   return mSize;
}

} // namespace


/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
