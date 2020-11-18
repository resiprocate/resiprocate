#if !defined(RESIP_DialogInfoContents_HXX)
#define RESIP_DialogInfoContents_HXX 

#include <map>
#include <list>

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/HeapInstanceCounter.hxx"

namespace resip
{

class XMLCursor;

/**
   SIP body type for holding DialogInfo contents (MIME content-type application/dialog-info+xml).
*/
class DialogInfoContents : public Contents
{
public:

   enum DialogInfoState
   {
      Full,
      Partial,
      MaxDialogInfoState
   };
   // Warning if you modify the above list, make sure you modify DialogInfoStateStrings in the .cxx file to match

   enum DialogState
   {
      Trying = 0,
      Proceeding,
      Early,
      Confirmed,
      Terminated,
      MaxDialogState
   };
   // Warning if you modify the above list, make sure you modify DialogStateStrings in the .cxx file to match

   enum DialogStateEvent
   {
      Cancelled = 0,
      Rejected,
      Replaced,
      LocalBye,
      RemoteBye,
      Error,
      Timeout,
      MaxOrUnsetDialogStateEvent
   };
   // Warning if you modify the above list, make sure you modify DialogStateEventStrings in the .cxx file to match

   enum Direction
   {
      Initiator = 0,
      Recipient,
      MaxOrUnsetDirection
   };
   // Warning if you modify the above list, make sure you modify DialogStateEventStrings in the .cxx file to match

   static const DialogInfoContents Empty;

   RESIP_HeapCount(DialogInfoContents);
   DialogInfoContents(const Mime& contentType);
   DialogInfoContents();
   DialogInfoContents(const HeaderFieldValue& hfv, const Mime& contentType);
   virtual ~DialogInfoContents();

   /** @brief duplicate an DialogInfoContents object
       @return pointer to a new DialogInfoContents object
       **/
   virtual Contents* clone() const;
   static const Mime& getStaticType();
   virtual EncodeStream& encodeParsed(EncodeStream& str) const;
   virtual void parse(ParseBuffer& pb);

   // Default is 3 spaces "   "
   void setEncodeIndent(const Data& indent) { mIndent = indent; }

   // Accesors for data
   void setEntity(const Uri& entity) { checkParsed(); mEntity = entity; }
   const Uri& getEntity() const { checkParsed(); return mEntity; }

   void setVersion(UInt32 version) { checkParsed(); mVersion = version; }
   UInt32 getVersion() const { checkParsed(); return mVersion; }

   void setDialogInfoState(DialogInfoState dialogInfoState) { checkParsed(); mDialogInfoState = dialogInfoState; }
   DialogInfoState getDialogInfoState() const { checkParsed(); return mDialogInfoState; }

   // A DialogInfoContents can have multiple Dialog sections in it
   class Dialog
   {
   public:
      Dialog() : mDirection(MaxOrUnsetDirection), mState(Trying), mStateEvent(MaxOrUnsetDialogStateEvent), mStateCode(0), mDuration(0),
         mHasDuration(false) {}

      // Accesors for data
      void setId(const Data& id) { mId = id; }
      const Data& getId() const { return mId; }

      void setCallId(const Data& callId) { mCallId = callId; }
      const Data& getCallId() const { return mCallId; }

      void setLocalTag(const Data& localTag) { mLocalTag = localTag; }
      const Data& getLocalTag() const { return mLocalTag; }

      void setRemoteTag(const Data& remoteTag) { mRemoteTag = remoteTag; }
      const Data& getRemoteTag() const { return mRemoteTag; }

      void setDirection(const Direction& direction) { mDirection = direction; }
      const Direction& getDirection() const { return mDirection; }

      void setState(const DialogState& state) { mState = state; }
      const DialogState& getState() const { return mState; }

      void setStateEvent(const DialogStateEvent& stateEvent) { mStateEvent = stateEvent; }
      const DialogStateEvent& getStateEvent() const { return mStateEvent; }

      void setStateCode(const int& stateCode) { mStateCode = stateCode; }
      const int& getStateCode() const { return mStateCode; }

      void setDuration(const UInt32& duration) { mDuration = duration; mHasDuration = true; }
      void clearDuration() { mDuration = 0; mHasDuration = false; }
      bool hasDuration() const { return mHasDuration; }
      const UInt32& getDuration() const { return mDuration; }

      void setReplacesInfo(const Data& callId, const Data& localTag, const Data& remoteTag) 
         { mReplacesCallId = callId; mReplacesLocalTag = localTag; mReplacesRemoteTag = remoteTag; }
      const Data& getReplacesCallId() const { return mReplacesCallId; }
      const Data& getReplacesLocalTag() const { return mReplacesLocalTag; }
      const Data& getReplacesRemoteTag() const { return mReplacesRemoteTag; }

      void setReferredBy(const NameAddr& referredBy) { mReferredBy = referredBy; }
      const NameAddr& getReferredBy() const { return mReferredBy; }

      void addRouteToRouteSet(const NameAddr& route) { mRouteSet.push_back(route); }
      void setRouteSet(const NameAddrs& routes) { mRouteSet = routes; }
      void clearRouteSet() { mRouteSet.clear(); }
      const NameAddrs& getRouteSet() const { return mRouteSet; }

      class Participant
      {
      public:
         Participant() : mCSeq(0), mHasCSeq(false) {}

         void setIdentity(const NameAddr& identity) { mIdentity = identity; }
         const NameAddr& getIdentity() const { return mIdentity; }

         void setTarget(const Uri& target) { mTarget = target; }
         void setTarget(const NameAddr& targetWithContactParams);
         const Uri& getTarget() const { return mTarget; }

         typedef std::map<Data, Data> TargetParams;
         void addTargetParam(const Data& name, const Data& value) { mTargetParams[name] = value; }
         void clearTargetParams() { mTargetParams.clear(); }
         const TargetParams& getTargetParams() const { return mTargetParams; }
         bool getTargetParam(const Data& name, Data& value) const;

         void setSessionDescription(const Data& sessionDescription, const Data& sessionDescriptionType)
            {  mSessionDescription = sessionDescription; mSessionDescriptionType = sessionDescriptionType; }
         const Data& getSessionDescription() const { return mSessionDescription; }
         const Data& getSessionDescriptionType() const { return mSessionDescriptionType; }

         void setCSeq(const UInt32& cseq) { mCSeq = cseq; mHasCSeq = true; }
         void clearCSeq() { mCSeq = 0; mHasCSeq = false; }
         bool hasCSeq() const { return mHasCSeq; }
         const UInt32& getCSeq() const { return mCSeq; }

      private:
         NameAddr mIdentity;
         Uri mTarget;
         TargetParams mTargetParams;
         Data mSessionDescription;
         Data mSessionDescriptionType;
         UInt32 mCSeq;
         bool mHasCSeq;

         friend class Dialog;
         friend class DialogInfoContents;
         EncodeStream& encode(EncodeStream& str, const char* baseElementName, const Data& indent) const;
         void parse(XMLCursor& xml);
         void parseParam(XMLCursor& xml);
      };

      Participant& localParticipant() { return mLocalParticipant; }
      const Participant& localParticipant() const { return mLocalParticipant; }

      Participant& remoteParticipant() { return mRemoteParticipant; }
      const Participant& remoteParticipant() const { return mRemoteParticipant; }

      // add element name and value to the extra Dialog child elements map
      // These are local/non-standard child elements to the Dialog element
      void addDialogElement(const Data& childElementName, const Data& elementValue) { mExtraDialogElements.insert( std::pair<Data,Data>(childElementName, elementValue));}

      // get the instance'th occurance of the named Dialog child element.
      // The first instance is index 0.
      bool getDialogElement(const Data& childElementName, Data& elementValue, int instance=0) const;

   private:
      Data mId;
      Data mCallId;
      Data mLocalTag;
      Data mRemoteTag;
      Direction mDirection;

      DialogState mState;
      DialogStateEvent mStateEvent;
      int mStateCode;

      UInt32 mDuration;
      bool mHasDuration;

      Data mReplacesCallId;
      Data mReplacesLocalTag;
      Data mReplacesRemoteTag;

      NameAddr mReferredBy;

      NameAddrs mRouteSet;

      Participant mLocalParticipant;
      Participant mRemoteParticipant;

      std::multimap<Data, Data> mExtraDialogElements;

      friend class DialogInfoContents;
      EncodeStream& encodeParsed(EncodeStream& str, const Data& indent) const;
   };

   typedef std::list<Dialog> DialogList;
   const DialogList& getDialogs() const { checkParsed(); return mDialogs; }
   void addDialog(const Dialog& dialog);
   bool removeDialog(const Data& id);
   void clearDialogs() { checkParsed();  mDialogs.clear(); }

   static bool init();
   static const char* dialogInfoStateToString(const DialogInfoState& dialogInfoState);
   static DialogInfoState dialogInfoStateStringToEnum(const Data& dialogInfoStateString);
   static const char* dialogStateToString(const DialogState& dialogState);
   static DialogState dialogStateStringToEnum(const Data& dialogStateString);
   static const char* dialogStateEventToString(const DialogStateEvent& dialogStateEvent);
   static DialogStateEvent dialogStateEventStringToEnum(const Data& dialogStateEventString);
   static const char* directionToString(const Direction& direction);
   static Direction directionStringToEnum(const Data& directionString);

private:

   static EncodeStream& encodeNameAddrElement(EncodeStream& str, const char* elementName, const NameAddr& nameAddr);
   void parseDialog(XMLCursor& xml);
   static bool parseUriValue(XMLCursor& xml, Uri& uri);
   static bool parseNameAddrElement(XMLCursor& xml, NameAddr& nameAddr);

   // Base DialogInfoContents data members
   Data mIndent;
   Uri mEntity;
   UInt32 mVersion;
   DialogInfoState mDialogInfoState;

   DialogList mDialogs;
};

static bool invokeDialogInfoContentsInit = DialogInfoContents::init();

}

#endif

/* ====================================================================
*
* Copyright (c) 2016 SIP Spectrum, Inc.  All rights reserved.
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
/*
* vi: set shiftwidth=3 expandtab:
*/
