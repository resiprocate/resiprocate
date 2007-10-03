#include <iostream>
#include "tfm/Box.hxx"

using namespace std;

Box::Box() :
   mX(0),
   mY(0),
   mWidth(0),
   mHeight(0)
{
}

Box::Box(unsigned int x,
         unsigned int y,
         unsigned int width,
         unsigned int height)
   : mX(x),
     mY(y),
     mWidth(width),
     mHeight(height)
{
}

Box&
Box::operator=(const Box& box)
{
   mX = box.mX;
   mY = box.mY;
   mWidth = box.mWidth;
   mHeight = box.mHeight;
   
   return *this;
}

Box& 
Box::boxUnion(const Box& box)
{
   unsigned int maxx = std::max(mX + mWidth,
                                box.mX + box.mWidth);
   unsigned int maxy = std::max(mY + mHeight,
                           box.mY + box.mHeight);

   mX = std::min(mX, box.mX);
   mY = std::min(mY, box.mY);

   mWidth = maxx - mX;
   mHeight = maxy - mY;

   return *this;
}

Box&
Box::displaceDown(const Box& box)
{
   mY += box.mY + box.mHeight;
   return *this;
}

Box& 
Box::displaceRight(const Box& box)
{
   mX += box.mX + box.mWidth;
   return *this;
}

ostream&
operator<<(ostream& str, const Box& box)
{
   str << "Box(" << box.mX << ", " << box.mY << ", "
       << box.mWidth << ", " << box.mHeight << ")";
   return str;
}
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
