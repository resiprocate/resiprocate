#include "Sdp.hxx"
#include "SdpMediaLine.hxx"

#include <rutil/Data.hxx>

using namespace sdpcontainer;

const char* Sdp::SdpNetTypeString[] = 
{
   "NONE",
   "IN"
};

const char* Sdp::SdpAddressTypeString[] =
{
   "NONE",
   "IP4",
   "IP6"
};

const char* Sdp::SdpBandwidthTypeString[] =
{
   "NONE",
   "CT",
   "AS",
   "TIAS",
   "RS",
   "RR"
};

const char* Sdp::SdpConferenceTypeString[] = 
{
   "NONE",
   "BROADCAST",
   "MODERATED",
   "TEST",
   "H332"
};

const char* Sdp::SdpGroupSemanticsString[] =
{
   "NONE",
   "LS",
   "FID",
   "SRF",
   "ANAT"
};


// Constructor
Sdp::Sdp()
{
   mSdpVersion = 1;
   mOriginatorSessionId = 0;
   mOriginatorSessionVersion = 0;
   mOriginatorNetType = NET_TYPE_NONE;
   mOriginatorAddressType = ADDRESS_TYPE_NONE;
   mConferenceType = CONFERENCE_TYPE_NONE;
   mIcePassiveOnlyMode = false;
   mMaximumPacketRate = 0;
}

// Copy constructor
Sdp::Sdp(const Sdp& rhs)
{
   operator=(rhs); 
}

// Destructor
Sdp::~Sdp()
{
   clearMediaLines();
}

// Assignment operator
Sdp& 
Sdp::operator=(const Sdp& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   // Assign values
   mSdpVersion = rhs.mSdpVersion;
   mOriginatorUserName = rhs.mOriginatorUserName;
   mOriginatorSessionId = rhs.mOriginatorSessionId;
   mOriginatorSessionVersion = rhs.mOriginatorSessionVersion;
   mOriginatorNetType = rhs.mOriginatorNetType;
   mOriginatorAddressType = rhs.mOriginatorAddressType;
   mOriginatorUnicastAddress = rhs.mOriginatorUnicastAddress;
   mSessionName = rhs.mSessionName;
   mSessionInformation = rhs.mSessionInformation;
   mSessionUri = rhs.mSessionUri;
   mEmailAddresses = rhs.mEmailAddresses;
   mPhoneNumbers = rhs.mPhoneNumbers;
   mBandwidths = rhs.mBandwidths;
   mTimes = rhs.mTimes;
   mTimeZones = rhs.mTimeZones;
   mCategory = rhs.mCategory;
   mKeywords = rhs.mKeywords;
   mToolNameAndVersion = rhs.mToolNameAndVersion;
   mConferenceType = rhs.mConferenceType;
   mCharSet = rhs.mCharSet;
   mIcePassiveOnlyMode = rhs.mIcePassiveOnlyMode;
   mGroups = rhs.mGroups;
   mSessionLanguage = rhs.mSessionLanguage;
   mDescriptionLanguage = rhs.mDescriptionLanguage;
   mMaximumPacketRate = rhs.mMaximumPacketRate;
   mFoundationIds = rhs.mFoundationIds;

   // Copy over media lines - deep copy of pointers
   clearMediaLines();
   MediaLineList::const_iterator it = rhs.mMediaLines.begin();
   for(;it != rhs.mMediaLines.end(); it++)
   {
      SdpMediaLine* mediaLineCopy = new SdpMediaLine(*(*it));
      addMediaLine(mediaLineCopy);
   }

   return *this;
}

Sdp::SdpAddressType 
Sdp::getAddressTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("IP4", dataType))
   {
      return ADDRESS_TYPE_IP4;
   }
   else if(resip::isEqualNoCase("IP6", dataType))
   {
      return ADDRESS_TYPE_IP6;
   }
   else
   {
      return ADDRESS_TYPE_NONE;
   }
}

Sdp::SdpBandwidthType 
Sdp::SdpBandwidth::getTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("CT", dataType))
   {
      return BANDWIDTH_TYPE_CT;
   }
   else if(resip::isEqualNoCase("AS", dataType))
   {
      return BANDWIDTH_TYPE_AS;
   }
   else if(resip::isEqualNoCase("TIAS", dataType))
   {
      return BANDWIDTH_TYPE_TIAS;
   }
   else if(resip::isEqualNoCase("RS", dataType))
   {
      return BANDWIDTH_TYPE_RS;
   }
   else if(resip::isEqualNoCase("RR", dataType))
   {
      return BANDWIDTH_TYPE_RR;
   }
   else
   {
      return BANDWIDTH_TYPE_NONE;
   }
}

Sdp::SdpConferenceType 
Sdp::getConferenceTypeFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("broadcast", dataType))
   {
      return CONFERENCE_TYPE_BROADCAST;
   }
   else if(resip::isEqualNoCase("moderated", dataType))
   {
      return CONFERENCE_TYPE_MODERATED;
   }
   else if(resip::isEqualNoCase("test", dataType))
   {
      return CONFERENCE_TYPE_TEST;
   }
   else if(resip::isEqualNoCase("H332", dataType))
   {
      return CONFERENCE_TYPE_H332;
   }
   else
   {
      return CONFERENCE_TYPE_NONE;
   }
}

Sdp::SdpGroupSemantics 
Sdp::SdpGroup::getSemanticsFromString(const char * type)
{
   resip::Data dataType(type);

   if(resip::isEqualNoCase("LS", dataType))
   {
      return GROUP_SEMANTICS_LS;
   }
   else if(resip::isEqualNoCase("FID", dataType))
   {
      return GROUP_SEMANTICS_FID;
   }
   else if(resip::isEqualNoCase("SRF", dataType))
   {
      return GROUP_SEMANTICS_SRF;
   }
   else if(resip::isEqualNoCase("ANAT", dataType))
   {
      return GROUP_SEMANTICS_ANAT;
   }
   else
   {
      return GROUP_SEMANTICS_NONE;
   }
}

void 
Sdp::setOriginatorInfo(const char* userName, 
                       UInt64 sessionId, 
                       UInt64 sessionVersion, 
                       SdpNetType netType, 
                       SdpAddressType addressType, 
                       const char* unicastAddress)
{
   mOriginatorUserName = userName;
   mOriginatorSessionId = sessionId;
   mOriginatorSessionVersion = sessionVersion;
   mOriginatorNetType = netType;
   mOriginatorAddressType = addressType;
   mOriginatorUnicastAddress = unicastAddress;
}

void 
Sdp::addMediaLine(SdpMediaLine* mediaLine) 
{ 
   mMediaLines.push_back(mediaLine); 
}

void 
Sdp::clearMediaLines() 
{ 
   MediaLineList::iterator it = mMediaLines.begin();
   for(;it != mMediaLines.end(); it++)
   {
      delete *it;
   }
   mMediaLines.clear(); 
}

const Sdp::MediaLineList& 
Sdp::getMediaLines() const 
{ 
   return mMediaLines; 
}

resip::Data 
Sdp::getLocalFoundationId(SdpCandidate::SdpCandidateType candidateType, 
                          const char * baseAddress, 
                          const char * stunAddress)
{
   SdpFoundation sdpFoundation(candidateType, baseAddress, stunAddress);

   std::map<resip::Data, SdpFoundation>::iterator it;
   for(it = mFoundationIds.begin(); it != mFoundationIds.end(); it++)
   {
      if(it->second == sdpFoundation)
      {
         return it->first;
      }
   }

   // Not found - insert
   char foundationId[15];
   sprintf(foundationId, "%d", mFoundationIds.size() + 1);
   mFoundationIds[foundationId] = sdpFoundation;

   return foundationId;
}

EncodeStream& 
sdpcontainer::operator<<( EncodeStream& strm, const Sdp& sdp)
{
   strm << "Sdp:" << std::endl
        << "SdpVersion: " << sdp.mSdpVersion << std::endl
        << "OrigUserName: '" << sdp.mOriginatorUserName << "'" << std::endl
        << "OrigSessionId: " << sdp.mOriginatorSessionId << std::endl
        << "OrigSessionVersion: " << sdp.mOriginatorSessionVersion << std::endl
        << "OrigNetType: " << Sdp::SdpNetTypeString[sdp.mOriginatorNetType] << std::endl
        << "OrigAddressType: " << Sdp::SdpAddressTypeString[sdp.mOriginatorAddressType] << std::endl
        << "OrigUnicastAddr: '" << sdp.mOriginatorUnicastAddress << "'" << std::endl
        << "SessionName: '" << sdp.mSessionName << "'" << std::endl
        << "SessionInformation: '" << sdp.mSessionInformation << "'" << std::endl
        << "SessionUri: '" << sdp.mSessionUri << "'" << std::endl;

   Sdp::EmailAddressList::const_iterator itEmail = sdp.mEmailAddresses.begin();
   for(;itEmail != sdp.mEmailAddresses.end(); itEmail++)
   {
      strm << "EmailAddress: '" << *itEmail << "'" << std::endl;
   }

   Sdp::PhoneNumberList::const_iterator itPhone = sdp.mPhoneNumbers.begin();
   for(;itPhone != sdp.mPhoneNumbers.end(); itPhone++)
   {
      strm << "PhoneNumber: '" << *itPhone << "'" << std::endl;
   }

   Sdp::BandwidthList::const_iterator itBandwidth = sdp.mBandwidths.begin();
   for(;itBandwidth != sdp.mBandwidths.end(); itBandwidth++)
   {
      strm << "Bandwidth: type=" << Sdp::SdpBandwidthTypeString[itBandwidth->getType()]
           << ", bandwidth=" << itBandwidth->getBandwidth() << std::endl;
   }

   Sdp::TimeList::const_iterator itTime = sdp.mTimes.begin();
   for(;itTime != sdp.mTimes.end(); itTime++)
   {
      strm << "Time: start=" << itTime->getStartTime()
           << ", stop=" << itTime->getStopTime() << std::endl;

      Sdp::SdpTime::RepeatsList::const_iterator itRepeat = itTime->getRepeats().begin();
      for(;itRepeat!=itTime->getRepeats().end(); itRepeat++)
      {
         strm << "TimeRepeat: interval=" << itRepeat->getRepeatInterval()
              << ", duration=" << itRepeat->getActiveDuration();

         Sdp::SdpTime::SdpTimeRepeat::OffsetsList::const_iterator itOffset = itRepeat->getOffsetsFromStartTime().begin();
         for(;itOffset!=itRepeat->getOffsetsFromStartTime().end(); itOffset++)
         {
            strm << ", offset=" << *itOffset;
         }
         strm << std::endl;
      }
   }

   Sdp::TimeZoneList::const_iterator itTimeZone = sdp.mTimeZones.begin();
   for(;itTimeZone != sdp.mTimeZones.end(); itTimeZone++)
   {
      strm << "TimeZone: adjustment time=" << itTimeZone->getAdjustmentTime()
           << ", offset=" << itTimeZone->getOffset() << std::endl;
   }

   strm << "Category: '" << sdp.mCategory << "'" << std::endl
        << "Keywords: '" << sdp.mKeywords << "'" << std::endl
        << "ToolNameAndVersion: '" << sdp.mToolNameAndVersion << "'" << std::endl
        << "ConferenceType: " << Sdp::SdpConferenceTypeString[sdp.mConferenceType] << std::endl
        << "CharSet: '" << sdp.mCharSet << "'" << std::endl
        << "IcePassiveOnlyMode: " << sdp.mIcePassiveOnlyMode << std::endl;

   Sdp::GroupList::const_iterator itGroup = sdp.mGroups.begin();
   for(;itGroup != sdp.mGroups.end(); itGroup++)
   {
      strm << "Group: semantics=" << Sdp::SdpGroupSemanticsString[itGroup->getSemantics()];
      Sdp::SdpGroup::TagsList::const_iterator itTag = itGroup->getIdentificationTags().begin();
      for(;itTag!=itGroup->getIdentificationTags().end(); itTag++)
      {
         strm << ", idTag=" << *itTag;
      }
      strm << std::endl;
   }

   strm << "SessionLanguage: '" << sdp.mSessionLanguage << "'" << std::endl
        << "DescriptionLanguage: '" << sdp.mDescriptionLanguage << "'" << std::endl
        << "MaximumPacketRate: " << sdp.mMaximumPacketRate << std::endl;

   Sdp::MediaLineList::const_iterator itMediaLine = sdp.mMediaLines.begin();
   for(;itMediaLine!=sdp.mMediaLines.end();itMediaLine++)
   {
      strm << std::endl << *(*itMediaLine);
   }

   return strm;
}


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
