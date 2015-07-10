#include "rutil/ResipAssert.h"
#include <algorithm>
#include <iostream>

#include "AsciiGraphic.hxx"

using namespace std;

AsciiGraphic::AsciiGraphic() : mBox(0, 0, 0, 0)
{
}

void
AsciiGraphic::renderToStream(EncodeStream& str) const
{
   // determine the layout of the complete ascii art
   Box box = layout();
   // allocate a matrix of spaces
   CharRaster out;
   for (unsigned int i = 0; i < box.mHeight+1; i++)
   {
      char* spaces = new char[box.mWidth+2];
      spaces[box.mWidth+1] = 0;
      for (unsigned int j = 0; j < box.mWidth+1; j++)
      {
         spaces[j] = ' ';
      }
      out.push_back(spaces);
   }
   resip_assert(out.size() == box.mHeight+1);

   // transfer the Sequence to matrix
   render(out);

   // null terminate pad spaces on right and ouptut
   for (unsigned int i = 0; i < box.mHeight; i++)
   {
      for (unsigned int j = box.mWidth-1; j > 0; j--)
      {
         if (out[i][j-1] != ' ')
         {
            out[i][j] = 0;
            break;
         }
      }
      str << out[i] << endl;
   }

   str.flush();
   // cleanup
   for (CharRaster::iterator i = out.begin(); i != out.end(); i++)
   {
      delete[] *i;
   }
}

Box&
AsciiGraphic::displaceDown(const Box& box)
{
   mBox.mY += box.mY + box.mHeight;
   return mBox;
}

Box&
AsciiGraphic::displaceRight(const Box& box)
{
   mBox.mX += box.mX + box.mWidth;
   return mBox;
}

resip::Data 
indent(int count)
{
   resip::Data spaces; //(count, true);
   for(int i=0; i < count; i++)
   {
      spaces += ' ';
   }
   return spaces;
}


// Copyright 2005 Purplecomm, Inc.
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
