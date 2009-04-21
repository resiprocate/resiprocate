#if !defined(SdpCandidate_hxx)
#define SdpCandidate_hxx

#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include <list>

namespace sdpcontainer
{

class SdpCandidate 
{
public:

   typedef enum
   {
      CANDIDATE_TRANSPORT_TYPE_NONE,
      CANDIDATE_TRANSPORT_TYPE_UDP,      // "udp"      - draft-ietf-mmusic-ice-12
      CANDIDATE_TRANSPORT_TYPE_TCP_SO,   // "tcp-so"   - TCP simultaneous-open - draft-ietf-mmusic-ice-tcp-02
      CANDIDATE_TRANSPORT_TYPE_TCP_ACT,  // "tcp-act"  - TCP active - draft-ietf-mmusic-ice-tcp-02
      CANDIDATE_TRANSPORT_TYPE_TCP_PASS, // "tcp-pass" - TCP passive - draft-ietf-mmusic-ice-tcp-02
      CANDIDATE_TRANSPORT_TYPE_TLS_SO,   // "tls-so"   - TCP simultaneous-open - draft-ietf-mmusic-ice-tcp-02
      CANDIDATE_TRANSPORT_TYPE_TLS_ACT,  // "tls-act"  - TCP active - draft-ietf-mmusic-ice-tcp-02
      CANDIDATE_TRANSPORT_TYPE_TLS_PASS  // "tls-pass" - TCP passive - draft-ietf-mmusic-ice-tcp-02
   } SdpCandidateTransportType;
   static const char* SdpCandidateTransportTypeString[];

   typedef enum
   {
      CANDIDATE_TYPE_NONE,
      CANDIDATE_TYPE_HOST,        // "host" - draft-ietf-mmusic-ice-12
      CANDIDATE_TYPE_SRFLX,       // "srflx" - server reflexive - draft-ietf-mmusic-ice-12
      CANDIDATE_TYPE_PRFLX,       // "prflx" - peer reflexive - draft-ietf-mmusic-ice-12
      CANDIDATE_TYPE_RELAY,       // "relay" - draft-ietf-mmusic-ice-12
   } SdpCandidateType;
   static const char* SdpCandidateTypeString[];

   class SdpCandidateExtensionAttribute 
   {
   public:
      SdpCandidateExtensionAttribute(const char * name, const char * value) : mName(name), mValue(value) {}
      SdpCandidateExtensionAttribute(const SdpCandidateExtensionAttribute& rhs) : mName(rhs.mName), mValue(rhs.mValue) {}

      bool operator==(const SdpCandidateExtensionAttribute& rhs) const { return mName == rhs.mName && mValue == rhs.mValue; }

      void setName(const char * name) { mName = name; }
      const resip::Data& getName() const { return mName; }

      void setValue(const char * value) { mValue = value; }
      const resip::Data& getValue() const { return mValue; }

   private:
      resip::Data mName;
      resip::Data mValue;
   };

   SdpCandidate(const char * foundation = 0,
                unsigned int id = 0,
                SdpCandidateTransportType transport = CANDIDATE_TRANSPORT_TYPE_NONE,
                UInt64 priority = 0,
                const char * connectionAddress = 0,
                unsigned int port = 0,
                SdpCandidateType candidateType = CANDIDATE_TYPE_NONE,
                const char * relatedAddress = 0,
                unsigned int relatedPort = 0,
                bool inUse = false);

   SdpCandidate(const SdpCandidate& rSdpCandidate);

   virtual ~SdpCandidate();

   typedef std::list<SdpCandidateExtensionAttribute> SdpCandidateExtensionAttributeList;

   SdpCandidate& operator=(const SdpCandidate& rhs);
   bool operator<(const SdpCandidate& rhs) const;
   bool operator==(const SdpCandidate& rhs) const;
   bool operator!=(const SdpCandidate& rhs) const;

   void setFoundation(const char * foundation) { mFoundation = foundation; }
   void setId(unsigned int id) { mId = id; }
   void setTransport(SdpCandidateTransportType transport) { mTransport = transport; }
   void setPriority(UInt64 priority) { mPriority = priority; }
   void setConnectionAddress(const char * connectionAddress) { mConnectionAddress = connectionAddress; }
   void setPort(unsigned int port) { mPort = port; }
   void setCandidateType(SdpCandidateType candidateType) { mCandidateType = candidateType; }
   void setRelatedAddress(const char * relatedAddress) { mRelatedAddress = relatedAddress; }
   void setRelatedPort(unsigned int relatedPort) { mRelatedPort = relatedPort; }

   void addExtensionAttribute(const char * name, const char * value) { addExtensionAttribute(SdpCandidateExtensionAttribute(name, value)); }
   void addExtensionAttribute(const SdpCandidateExtensionAttribute& sdpCandidateExtensionAttribute) { mExtensionAttributes.push_back(sdpCandidateExtensionAttribute); }
   void clearExtensionAttributes() { mExtensionAttributes.clear(); }

   void setInUse(bool inUse) { mInUse = inUse; }

   void toString(resip::Data& sdpCandidateString) const;

   const resip::Data& getFoundation() const { return mFoundation; }
   unsigned int getId() const { return mId; }
   SdpCandidateTransportType getTransport() const { return mTransport; }
   static SdpCandidateTransportType getCandidateTransportTypeFromString(const char * type);
   UInt64 getPriority() const { return mPriority; }
   const resip::Data& getConnectionAddress() const { return mConnectionAddress; }
   unsigned int getPort() const { return mPort; }
   SdpCandidateType getCandidateType() const { return mCandidateType; }
   static SdpCandidateType getCandidateTypeFromString(const char * type);
   const resip::Data& getRelatedAddress() const { return mRelatedAddress; }
   unsigned int getRelatedPort() const { return mRelatedPort; }
   const SdpCandidateExtensionAttributeList& getExtensionAttributes() const { return mExtensionAttributes; }
   bool isInUse() const { return mInUse; }

private:
   resip::Data                mFoundation;
   unsigned int               mId;
   SdpCandidateTransportType  mTransport;
   UInt64                     mPriority;
   resip::Data                mConnectionAddress;
   unsigned int               mPort;
   SdpCandidateType           mCandidateType;
   resip::Data                mRelatedAddress;
   unsigned int               mRelatedPort;
   SdpCandidateExtensionAttributeList mExtensionAttributes;

   bool                       mInUse;

   friend EncodeStream& operator<<(EncodeStream& strm, const SdpCandidate& );
};

EncodeStream& operator<< ( EncodeStream& strm, const SdpCandidate& );

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
