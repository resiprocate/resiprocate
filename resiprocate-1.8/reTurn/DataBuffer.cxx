#include "DataBuffer.hxx"
#include <memory.h>
#include <assert.h>
#include <rutil/WinLeakCheck.hxx>

namespace reTurn {

DataBuffer::DataBuffer(const char* data, unsigned int size) : 
mBuffer(size != 0 ? new char[size] : 0), mSize(size), mStart(mBuffer) 
{
   memcpy(mBuffer, data, size);
}

DataBuffer::DataBuffer(unsigned int size) : 
mBuffer(size != 0 ? new char[size] : 0), mSize(size), mStart(mBuffer) 
{
}

DataBuffer::~DataBuffer() 
{ 
   delete[] mBuffer; 
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

char& 
DataBuffer::operator[](unsigned int p) 
{ 
   assert(p < mSize); 
   return mBuffer[p]; 
}

char 
DataBuffer::operator[](unsigned int p) const 
{ 
   assert(p < mSize); 
   return mBuffer[p]; 
}

unsigned int 
DataBuffer::truncate(unsigned int newSize) 
{ 
   assert(newSize <= mSize); 
   mSize = newSize; 
   return mSize; 
}

unsigned int 
DataBuffer::offset(unsigned int bytes) 
{ 
   assert(bytes < mSize); 
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
