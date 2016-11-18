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

#ifndef TelepathyMasterProfile_hxx
#define TelepathyMasterProfile_hxx

#include "resip/recon/UserAgentMasterProfile.hxx"

#include "TelepathyParameters.hxx"

namespace tr
{

class TelepathyMasterProfile : public recon::UserAgentMasterProfile, public TelepathyParameters
{
public:
   // Initialize the profile using parameters from the account settings
   // panel.
   TelepathyMasterProfile(const QVariantMap& parameters);
};


}


#endif



