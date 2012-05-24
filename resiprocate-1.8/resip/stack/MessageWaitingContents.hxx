#if !defined(RESIP_MESSAGEWAITINGCONTENTS_HXX)
#define RESIP_MESSAGEWAITINGCONTENTS_HXX 

#include <map>

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"

namespace resip
{

const char* skipSipLWS(ParseBuffer& pb);

/** @brief An enumeration of MessageWaiting header types for use with resip::MessageWaitingContents.

    Also see resip::mw_account to access the special Message-Account header for MessageWaiting.
 **/
typedef enum {mw_voice=0, mw_fax, mw_pager, mw_multimedia, mw_text, mw_none, MW_MAX} HeaderType;

/**
   @ingroup sip_payload
   @brief SIP body type for holding MWI contents (MIME content-type application/simple-message-summary).
   
   See resip/stack/test/testMessageWaiting.cxx for usage examples.
*/
class MessageWaitingContents : public Contents
{
   public:
      MessageWaitingContents();
      MessageWaitingContents(const HeaderFieldValue& hfv, const Mime& contentType);
      MessageWaitingContents(const Data& data, const Mime& contentType);
      MessageWaitingContents(const MessageWaitingContents& rhs);
      virtual ~MessageWaitingContents();
      MessageWaitingContents& operator=(const MessageWaitingContents& rhs);

      /** @brief duplicate an MessageWaitingContents object
          @return pointer to a new MessageWaitingContents object  
        **/
      virtual Contents* clone() const;

      static const Mime& getStaticType() ;

      virtual EncodeStream& encodeParsed(EncodeStream& str) const;
      virtual void parse(ParseBuffer& pb);

      class Header;

      /** @brief Get the header correspoding to ht
          @param ht HeaderType used to lookup MessageWaiting header
          @return Header corresponding to ht
       **/
      Header& header(HeaderType ht);
      const Header& header(HeaderType ht) const;

      /** @brief Check if HeaderType is present
          @param ht HeaderType used to lookup MessageWaiting header
          @return bool true if HeaderType exists
       **/
      bool exists(HeaderType ht) const;

      /** @brief Remove the header correspoding to ht
          @param ht HeaderType used to remove from MessageWaiting header
       **/
      void remove(HeaderType ht);

      /** @brief makes accessing the MessageAccount Uri
          operate like accessing the other MessageWaiting headers.
		  
          However, the user of these calls has to treat them differently 
          since the two header calls return different classes.
		  
	      @todo This is an awkward attempt to make accessing the MessageAccount Uri
          operate like accessing the other MessageWaiting headers even though
          the user of these calls has to treat them differently to being with
          since the two header calls return different classes.  ugh. .mjf.
       **/
      class AccountHeader {};

      /** @brief Get the Uri for Message-Account line
          @param ht AccountHeader used to lookup MessageWaiting header - not really used computationally
          @return Uri for Message-Account line
       **/
      const Uri& header(const AccountHeader& ht) const;
      Uri& header(const AccountHeader& ht);

      /** @brief Check if Message-Account line is present
          @note This call only indicates the actual existence of the Message-Account
                line in the MessageWaiting doc if header(mw_account) hasn't been called.
                After that call the exists(mw_account) call will return true (since a
                Uri now exists) whether or not the Uri was built based on the
                MessageWaiting doc.
          @param ht AccountHeader used to lookup MessageWaiting header - not really used computationally
          @return bool true if HeaderType exists
       **/
      bool exists(const AccountHeader& ht) const;

      /** @brief Remove the Uri for Message-Account line
          @param ht AccountHeader used to lookup MessageWaiting header - not really used computationally
       **/
      void remove(const AccountHeader& ht);

      /** @brief Get the value correspoding to optional message header hn in the MessageWaiting doc.
      
          Used to access optional message headers in the MessageWaiting doc
      
          @param hn HeaderType used to lookup the optional MessageWaiting header
          @return Data corresponding to hn
       **/
      const Data& header(const Data& hn) const;
      Data& header(const Data& hn);

      /** @brief Check if optional message header hn is present
          @param hn HeaderType used to lookup the optional MessageWaiting header
          @return bool true if optional message header exists
       **/
      bool exists(const Data& hn) const;

      /** @brief Remove the optional message header corresponding to hn
          @param hn HeaderType used to lookup the optional MessageWaiting header
       **/
      void remove(const Data& hn);

      /** @brief Check to see if there are messages
          @return bool true if there are messages
       **/
      bool& hasMessages() { checkParsed(); return mHasMessages; }

      /** @brief Provides an interface for reading and modifying MessageWaiting bodies.
       **/
      class Header
      {
         public:
            /** @brief Header constructor.
                Create a Header using new count and old message counts.
                @param numNew new message count
                @param numOld old message count
             **/
            Header(unsigned int numNew,
                   unsigned int numOld);

            /** @brief Header constructor with urgent counts.
                Create a Header using new/old/urgent new/urgent old message counts.
                @param numNew new message count
                @param numOld old message count
                @param numUrgentNew new urgent message count
                @param numUrgentOld old urgent message count
             **/
            Header(unsigned int numNew,
                   unsigned int numOld,
                   unsigned int numUrgentNew,
                   unsigned int numUrgentOld);

            /** @brief Return new message count
                @return int new message count
             **/
            const unsigned int& newCount() const {return mNew;}

            /** @brief Return new message count
                @return int new message count
             **/
            unsigned int& newCount() {return mNew;}

            /** @brief Return old message count
                @return int new message count
             **/
            const unsigned int& oldCount() const {return mOld;}

            /** @brief Return old message count
                @return int new message count
             **/
            unsigned int& oldCount() {return mOld;}

            /** @brief Return bool indicating that there are urgent messages
                @note Currently this is only set during construction.  Modification of the new and old urgent counts does not update this field.
                @return bool true if there are urgent messages
             **/
            const bool& urgent() const {return mHasUrgent;}

            /** @brief Return bool indicating that there are urgent messages
                @note Currently this is only set during construction.  Modification of the new and old urgent counts does not update this field.
                @return bool true if there are urgent messages
             **/
            bool& urgent() {return mHasUrgent;}

            /** @brief Return new urgent message count
                @return int new message count
             **/
            const unsigned int& urgentNewCount() const {return mUrgentNew;}

            /** @brief Return new urgent message count
                @return int new message count
             **/
            unsigned int& urgentNewCount() {return mUrgentNew;}

            /** @brief Return new message count
                @return int new message count
             **/
            const unsigned int& urgentOldCount() const {return mUrgentOld;}

            /** @brief Return new message count
                @return int new message count
             **/
            unsigned int& urgentOldCount() {return mUrgentOld;}

         private:
            unsigned int mNew;
            unsigned int mOld;
            bool mHasUrgent;
            unsigned int mUrgentNew;
            unsigned int mUrgentOld;

            friend class MessageWaitingContents;
      };

      static bool init();
   private:
      void clear();

      bool mHasMessages;
      Uri* mAccountUri;
      Header* mHeaders[MW_MAX];
      std::map<Data, Data> mExtensions;
};

/** @brief Used to access the Message-Account header in MessageWaiting docs.
 **/
extern MessageWaitingContents::AccountHeader mw_account;
static bool invokeMessageWaitingContentsInit = MessageWaitingContents::init();

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
