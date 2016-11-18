/*
 * Copyright (C) 2015 Daniel Pocock http://danielpocock.com
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

#ifndef TelepathyParameters_hxx
#define TelepathyParameters_hxx

#include <QVariant>

#include <TelepathyQt/ProtocolParameterList>

#include <rutil/Data.hxx>
#include <resip/stack/NameAddr.hxx>

namespace tr
{

class TelepathyParameters
{
public:
   // Get the list of parameters that should appear in the account settings
   // panel.
   static Tp::ProtocolParameterList getParameterList();

   // Store the parameters from the account settings panel.
   TelepathyParameters(const QVariantMap& parameters);

protected:
   const char* getString(const char* parameter); 

   const resip::Data& account() { return mAccount; };
   const resip::NameAddr& contact() { return mContact; };
   const resip::Data& realm() { return mRealm; };
   const resip::Data& authUser() { return mAuthUser; };
   const resip::Data& password() { return mPassword; };
   

private:
   const QVariantMap& mParameters;
   resip::Data mAccount;
   resip::NameAddr mContact;
   resip::Data mRealm;
   resip::Data mAuthUser;
   resip::Data mPassword;
};

}


#endif


