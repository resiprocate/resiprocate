#if !defined(Sdp_hxx)
#define Sdp_hxx

#include <iostream>
#include <list>
#include <map>

#include "rutil/compat.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/Data.hxx"

#include "SdpCandidate.hxx"

namespace sdpcontainer
{

class SdpMediaLine;

// Container for SDP specification
// This class holds the information related to an SDP.
class Sdp
{
public:

   typedef enum 
   {
      NET_TYPE_NONE,
      NET_TYPE_IN             // "IN" - Internet - RFC4566
   } SdpNetType;
   static const char* SdpNetTypeString[];

   typedef enum 
   {
      ADDRESS_TYPE_NONE,
      ADDRESS_TYPE_IP4,       // "IP4" - RFC4566
      ADDRESS_TYPE_IP6        // "IP6" - RFC4566
   } SdpAddressType;
   static const char* SdpAddressTypeString[];

   typedef enum 
   {
      BANDWIDTH_TYPE_NONE,
      BANDWIDTH_TYPE_CT,      // "CT" - Conference Total - RFC4566
      BANDWIDTH_TYPE_AS,      // "AS" - Application Specific - RFC4566
      BANDWIDTH_TYPE_TIAS,    // "TIAS" - Transport Independent Application Specific - RFC3890,
      BANDWIDTH_TYPE_RS,      // "RS" - RTCP bandwidth on active senders - RFC3556
      BANDWIDTH_TYPE_RR       // "RR" - RTCP bandwidth allocated to other participants - RFC3556
   } SdpBandwidthType;
   static const char* SdpBandwidthTypeString[];

   class SdpBandwidth
   {
   public:
      SdpBandwidth(SdpBandwidthType type, unsigned int bandwidth) : mType(type), mBandwidth(bandwidth) {}
      SdpBandwidth(const SdpBandwidth& rhs) : mType(rhs.mType), mBandwidth(rhs.mBandwidth) {}

      // Accessors
      void setType(SdpBandwidthType type) { mType = type; }
      SdpBandwidthType getType() const { return mType; }
      static SdpBandwidthType getTypeFromString(const char * type);

      void setBandwidth(unsigned int bandwidth) { mBandwidth = bandwidth; }
      unsigned int getBandwidth() const { return mBandwidth; }

   private:
      SdpBandwidthType  mType;
      unsigned int      mBandwidth;
   };

   class SdpTime 
   {
   public:
      class SdpTimeRepeat 
      {
      public:
         SdpTimeRepeat(unsigned int repeatInterval, unsigned int activeDuration) :
            mRepeatInterval(repeatInterval), mActiveDuration(activeDuration) {}
          SdpTimeRepeat(const SdpTimeRepeat& rhs) :
            mRepeatInterval(rhs.mRepeatInterval), mActiveDuration(rhs.mActiveDuration), mOffsetsFromStartTime(rhs.mOffsetsFromStartTime) {}

         typedef std::list<unsigned int> OffsetsList;

         void setRepeatInterval(unsigned int repeatInterval) { mRepeatInterval = repeatInterval; }
         unsigned int getRepeatInterval() const { return mRepeatInterval; }

         void setActiveDuration(unsigned int activeDuration) { mActiveDuration = activeDuration; }
         unsigned int getActiveDuration() const { return mActiveDuration; }

         void addOffsetFromStartTime(unsigned int offset) { mOffsetsFromStartTime.push_back(offset); }
         void clearOffsetsFromStartTime() { mOffsetsFromStartTime.clear(); }
         const OffsetsList& getOffsetsFromStartTime() const { return mOffsetsFromStartTime; }

      private:
         unsigned int mRepeatInterval;
         unsigned int mActiveDuration;
         OffsetsList  mOffsetsFromStartTime;
      };

      SdpTime(UInt64 startTime, UInt64 stopTime) : mStartTime(startTime), mStopTime(stopTime) {}
      SdpTime(const SdpTime& rhs) : mStartTime(rhs.mStartTime), mStopTime(rhs.mStopTime), mRepeats(rhs.mRepeats) {}

      typedef std::list<SdpTimeRepeat> RepeatsList;

      void setStartTime(UInt64 startTime) { mStartTime = startTime; }
      UInt64 getStartTime() const { return mStartTime; }

      void setStopTime(UInt64 stopTime) { mStopTime = stopTime; }
      UInt64 getStopTime() const { return mStopTime; }

      void addRepeat(const SdpTimeRepeat& sdpTimeRepeat) { mRepeats.push_back(sdpTimeRepeat); }
      void clearRepeats() { mRepeats.clear(); }
      const RepeatsList& getRepeats() const { return mRepeats; }

   private:
      UInt64    mStartTime;
      UInt64    mStopTime;
      RepeatsList mRepeats;       
   };

   class SdpTimeZone 
   {
   public:
      SdpTimeZone(int adjustmentTime, int offset) : mAdjustmentTime(adjustmentTime), mOffset(offset) {}
      SdpTimeZone(const SdpTimeZone& rhs) : mAdjustmentTime(rhs.mAdjustmentTime), mOffset(rhs.mOffset) {}

      void setAdjustmentTime(int adjustmentTime) { mAdjustmentTime = adjustmentTime; }
      int getAdjustmentTime() const { return mAdjustmentTime; }

      void setOffset(int offset) { mOffset = offset; }
      int getOffset() const { return mOffset; }

   private:
      int         mAdjustmentTime;
      int         mOffset;
   };

   typedef enum 
   {
      CONFERENCE_TYPE_NONE,
      CONFERENCE_TYPE_BROADCAST, // "broadcast" - RFC4566
      CONFERENCE_TYPE_MODERATED, // "moderated" - RFC4566
      CONFERENCE_TYPE_TEST,      // "test" - RFC4566
      CONFERENCE_TYPE_H332       // "H332" - RFC4566
   } SdpConferenceType;
   static const char* SdpConferenceTypeString[];

   typedef enum 
   {
      GROUP_SEMANTICS_NONE,
      GROUP_SEMANTICS_LS,        // "LS" - Lip Sync - RFC3388
      GROUP_SEMANTICS_FID,       // "FID" - Flow Identifier - RFC3388
      GROUP_SEMANTICS_SRF,       // "SRF" - Single Reservation Flow - RFC3524
      GROUP_SEMANTICS_ANAT       // "ANAT" - Alternative Network Address Types - RFC4091
   } SdpGroupSemantics;
   static const char* SdpGroupSemanticsString[];

   class SdpGroup
   {
   public:
      SdpGroup(SdpGroupSemantics semantics) : mSemantics(semantics) {}
      SdpGroup(const SdpGroup& rhs) : mSemantics(rhs.mSemantics), mIdentificationTags(rhs.mIdentificationTags) {}

      typedef std::list<resip::Data> TagsList;

      void setSemantics(SdpGroupSemantics semantics) { mSemantics = semantics; }
      SdpGroupSemantics getSemantics() const { return mSemantics; }
      static SdpGroupSemantics getSemanticsFromString(const char * type);

      void addIdentificationTag(const char * identificationTag) { mIdentificationTags.push_back(identificationTag); }
      void clearIdentificationTags() { mIdentificationTags.clear(); }
      const TagsList& getIdentificationTags() const { return mIdentificationTags; }
      
   private:
      SdpGroupSemantics mSemantics;
      TagsList          mIdentificationTags;
   };

   class SdpFoundation
   {
   public:
      SdpFoundation() : mCandidateType(SdpCandidate::CANDIDATE_TYPE_NONE) {}
      SdpFoundation(SdpCandidate::SdpCandidateType candidateType, const char * baseAddress, const char * stunAddress) : 
         mCandidateType(candidateType), mBaseAddress(baseAddress), mStunAddress(stunAddress) {}
      SdpFoundation(const SdpFoundation& rhs) :
         mCandidateType(rhs.mCandidateType), mBaseAddress(rhs.mBaseAddress), mStunAddress(rhs.mStunAddress) {}

      bool operator==(const SdpFoundation& rhs) { return mCandidateType == rhs.mCandidateType &&
                                                         mBaseAddress == rhs.mBaseAddress &&
                                                         mStunAddress == rhs.mStunAddress; }
   private:
      SdpCandidate::SdpCandidateType mCandidateType;
      resip::Data                    mBaseAddress;
      resip::Data                    mStunAddress;
   };

   Sdp();
   Sdp(const Sdp& rSdp);
   virtual ~Sdp();

   Sdp& operator=(const Sdp& rhs);

   void setSdpVersion(unsigned int sdpVersion) { mSdpVersion = sdpVersion; }

   void setOriginatorInfo(const char* userName, UInt64 sessionId, UInt64 sessionVersion, SdpNetType netType, SdpAddressType addressType, const char* unicastAddress);
   void setOriginatorUserName(const char* originatorUserName) { mOriginatorUserName = originatorUserName; }
   void setOriginatorSessionId(UInt64 originatorSessionId) { mOriginatorSessionId = originatorSessionId; }
   void setOriginatorSessionVersion(UInt64 originatorSessionVersion) { mOriginatorSessionVersion = originatorSessionVersion; }
   void setOriginatorNetType(SdpNetType originatorNetType) { mOriginatorNetType = originatorNetType; }
   void setOriginatorAddressType(SdpAddressType originatorAddressType) { mOriginatorAddressType = originatorAddressType; }
   void setOriginatorUnicastAddress(const char* originatorUnicastAddress) { mOriginatorUnicastAddress = originatorUnicastAddress; }

   void setSessionName(const char * sessionName) { mSessionName = sessionName; }
   void setSessionInformation(const char * sessionInformation) { mSessionInformation = sessionInformation; }
   void setSessionUri(const char * sessionUri) { mSessionUri = sessionUri; }

   void addEmailAddress(const char * emailAddress) { mEmailAddresses.push_back(emailAddress); }
   void clearEmailAddresses() { mEmailAddresses.clear(); }

   void addPhoneNumber(const char * phoneNumber) { mPhoneNumbers.push_back(phoneNumber); }
   void clearPhoneNumbers() { mPhoneNumbers.clear(); }

   void addBandwidth(SdpBandwidthType type, unsigned int bandwidth) { addBandwidth(SdpBandwidth(type, bandwidth)); }
   void addBandwidth(const SdpBandwidth& sdpBandwidth) { mBandwidths.push_back(sdpBandwidth); }
   void clearBandwidths() { mBandwidths.clear(); }

   void addTime(UInt64 startTime, UInt64 stopTime) { addTime(SdpTime(startTime, stopTime)); }
   void addTime(const SdpTime& time) { mTimes.push_back(time); }
   void clearTimes() { mTimes.clear(); }

   void addTimeZone(int adjustmentTime, int offset) { addTimeZone(SdpTimeZone(adjustmentTime, offset)); }
   void addTimeZone(const SdpTimeZone& timeZone) { mTimeZones.push_back(timeZone); }
   void clearTimeZones() { mTimeZones.clear(); }

   void setCategory(const char * category) { mCategory = category; }
   void setKeywords(const char * keywords) { mKeywords = keywords; }
   void setToolNameAndVersion(const char * toolNameAndVersion) { mToolNameAndVersion = toolNameAndVersion; }
   void setConferenceType(SdpConferenceType conferenceType) { mConferenceType = conferenceType; }
   void setCharSet(const char * charSet) { mCharSet = charSet; }
   void setIcePassiveOnlyMode(bool icePassiveOnlyMode) { mIcePassiveOnlyMode = icePassiveOnlyMode; }

   void addGroup(const SdpGroup& group) { mGroups.push_back(group); }
   void clearGroups() { mGroups.clear(); }

   void setSessionLanguage(const char * sessionLanguage) { mSessionLanguage = sessionLanguage; }
   void setDescriptionLanguage(const char * descriptionLanguage) { mDescriptionLanguage = descriptionLanguage; }
   void setMaximumPacketRate(double maximumPacketRate) { mMaximumPacketRate = maximumPacketRate; }

   void addMediaLine(SdpMediaLine* mediaLine);
   void clearMediaLines();
   
   void toString(resip::Data& sdpString) const;

   static SdpAddressType getAddressTypeFromString(const char * type);

   unsigned int getSdpVersion() const { return mSdpVersion; }

   const resip::Data& getOriginatorUserName() const { return mOriginatorUserName; }
   UInt64 getOriginatorSessionId() const { return mOriginatorSessionId; }
   UInt64 getOriginatorSessionVersion() const { return mOriginatorSessionVersion; }
   SdpNetType getOriginatorNetType() const { return mOriginatorNetType; }
   SdpAddressType getOriginatorAddressType() const { return mOriginatorAddressType; }
   const resip::Data& getOriginatorUnicastAddress() const { return mOriginatorUnicastAddress; }

   const resip::Data& getSessionName() const { return mSessionName; }
   const resip::Data& getSessionInformation() const { return mSessionInformation; }
   const resip::Data& getSessionUri() const { return mSessionUri; }

   typedef std::list<resip::Data> EmailAddressList;
   typedef std::list<resip::Data> PhoneNumberList;
   typedef std::list<SdpBandwidth> BandwidthList;
   typedef std::list<SdpTime> TimeList;
   typedef std::list<SdpTimeZone> TimeZoneList;
   typedef std::list<SdpGroup> GroupList;
   typedef std::list<SdpMediaLine*> MediaLineList;

   const EmailAddressList& getEmailAddresses() const { return mEmailAddresses; }
   const PhoneNumberList& getPhoneNumbers() const { return mPhoneNumbers; }
   const BandwidthList& getBandwidths() const { return mBandwidths; }
   const TimeList& getTimes() const { return mTimes; }
   const TimeZoneList& getTimeZones() const { return mTimeZones; }

   const resip::Data& getCategory() const { return mCategory; }
   const resip::Data& getKeywords() const { return mKeywords; }
   const resip::Data& getToolNameAndVersion() const { return mToolNameAndVersion; }
   SdpConferenceType getConferenceType() const { return mConferenceType; }
   static SdpConferenceType getConferenceTypeFromString(const char * type);
   const resip::Data& getCharSet() const { return mCharSet; }
   bool isIcePassiveOnlyMode() const { return mIcePassiveOnlyMode; }

   const GroupList& getGroups() const { return mGroups; }

   const resip::Data& getSessionLanguage() const { return mSessionLanguage; }
   const resip::Data& getDescriptionLanguage() const { return mDescriptionLanguage; }
   double getMaximumPacketRate() const { return mMaximumPacketRate; }

   const MediaLineList& getMediaLines() const;

   resip::Data getLocalFoundationId(SdpCandidate::SdpCandidateType candidateType, const char * baseAddress, const char * stunAddress=0);

private:
   // v=
   unsigned int   mSdpVersion;

   // o=
   resip::Data    mOriginatorUserName;
   UInt64         mOriginatorSessionId;
   UInt64         mOriginatorSessionVersion;
   SdpNetType     mOriginatorNetType;
   SdpAddressType mOriginatorAddressType;
   resip::Data    mOriginatorUnicastAddress;

   // s=
   resip::Data    mSessionName;

   // i=
   resip::Data    mSessionInformation;

   // u=         
   resip::Data    mSessionUri;

   // e=
   EmailAddressList mEmailAddresses;

   // p=
   PhoneNumberList mPhoneNumbers;

   // c= is only stored in sdpMediaLine

   // b=
   BandwidthList   mBandwidths;

   // t=, r=
   TimeList        mTimes;

   // z=
   TimeZoneList    mTimeZones;

   // k= is only stored in sdpMediaLine

   // a= session level only attributes 
   resip::Data      mCategory;           // a=cat:<category> - RFC4566
   resip::Data      mKeywords;           // a=keywds:<keywords> - RFC4566
   resip::Data      mToolNameAndVersion; // a=tool:<name and version of tool> - RFC4566
   SdpConferenceType mConferenceType;  // a=type:<conference type> - RFC4566
   resip::Data      mCharSet;            // a=charset:<character set> - RFC4566
   bool             mIcePassiveOnlyMode; // a=ice-passive - ietf-draft-mmusic-ice-12
   GroupList        mGroups;             // a=group:<semantics> <id-tag> ... - RFC3388

   // a= attributes that have meaning when not associated to a particular media line
   resip::Data      mSessionLanguage;     // a=lang:<language tag> - RFC4566
   resip::Data      mDescriptionLanguage; // a=sdplang:<language tag> - RFC4566
   double           mMaximumPacketRate;   // a=maxprate:<packetrate> in packets/s - RFC3890

   // Media Lines
   MediaLineList    mMediaLines;

   // Foundation Id 
   std::map<resip::Data, SdpFoundation> mFoundationIds;

   friend EncodeStream& operator<<(EncodeStream& strm, const Sdp& );
};

EncodeStream& operator<<(EncodeStream& strm, const Sdp& );

} // namespace

#endif  


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
