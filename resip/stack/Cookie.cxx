#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Cookie.hxx"
#include "resip/stack/StringCategory.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/ParseBuffer.hxx"

using namespace resip;

//====================
// Cookie
//====================
Cookie::Cookie() :
   mName(),
   mValue()
{}

Cookie::Cookie(const Data& name, const Data& value) :
     mName(name.urlDecoded()),
     mValue(value.urlDecoded())
{}

Cookie&
Cookie::operator=(const Cookie& rhs)
{
   if (this != &rhs)
   {
      mName = rhs.mName;
      mValue = rhs.mValue;
   }
   return *this;
}

bool
Cookie::operator==(const Cookie& other) const
{
   return name() == other.name() && value() == other.value();
}

bool Cookie::operator<(const Cookie& rhs) const
{
   return name() + value() < rhs.name() + rhs.value();
}

EncodeStream&
resip::operator<<(EncodeStream& strm, const Cookie& c)
{
   strm << c.name() << Symbols::EQUALS[0] << c.value();
   return strm;
}

const Data&
Cookie::name() const
{
   return mName;
}

Data&
Cookie::name()
{
   return mName;
}

const Data&
Cookie::value() const
{
   return mValue;
}

Data&
Cookie::value()
{
   return mValue;
}

/* ====================================================================
 * BSD License
 *
 * Copyright (c) 2013 Catalin Constantin Usurelu  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
