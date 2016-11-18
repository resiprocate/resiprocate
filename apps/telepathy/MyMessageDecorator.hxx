/*
 * Copyright (C) 2014-2015 Daniel Pocock http://danielpocock.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MYMESSAGEDECORATOR_HXX
#define MYMESSAGEDECORATOR_HXX

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Data.hxx"
#include "resip/stack/MessageDecorator.hxx"

namespace tr
{

class MyMessageDecorator : public resip::MessageDecorator
{
   public:
      MyMessageDecorator();
      virtual ~MyMessageDecorator() {}

      virtual void decorateMessage(resip::SipMessage &msg,
                                  const resip::Tuple &source,
                                  const resip::Tuple &destination,
                                  const resip::Data& sigcompId);
      virtual void rollbackMessage(resip::SipMessage& msg) {};
      virtual resip::MessageDecorator* clone() const { return new MyMessageDecorator(); };

   private:
      bool isConnectionInvalid(const resip::SdpContents::Session::Connection& connection);

};

}

#endif


