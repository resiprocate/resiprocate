#ifndef SdpContents_hxx
#define SdpContents_hxx

#include <list>
#include <map>

#include "sip2/sipstack/Contents.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/util/Data.hxx"

namespace Vocal2
{

class SdpContents;

class AttributeHelper
{
   public:
      bool exists(const Data& key) const;
      const list<Data>& getValue(const Data& key) const;
      ostream& encode(ostream& s) const;
      void parse(ParseBuffer& pb);
      void addAttribute(const Data& key, const Data& value = Data::Empty);
   private:
      std::map< Data, std::list<Data> > mAttributes;
};

class SdpContents : public Contents
{
   public:
      typedef enum {IP4=1, IP6} AddrType;

      class Session;

      class Session 
      {
         public:
            class Medium;
            class Origin
            {
               public:
                  Origin(const Data& user,
                         const Data& sesionId,
                         const Data& version,
                         AddrType addr,
                         const Data& address);

                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  const Data& getSessionId() const {return mSessionId;}
                  const Data& getVersion() const {return mVersion;}
                  const Data& getUser() const {return mUser;}
                  AddrType getAddressType() const {return mAddrType;}
                  const Data& getAddress() const {return mAddress;}

               private:
                  Origin();

                  Data mUser;
                  Data mSessionId;
                  Data mVersion;
                  AddrType mAddrType;
                  Data mAddress;

                  friend class Session;
            };

            class Email
            {
               public:
                  Email(const Data& address,
                        const Data& freeText);

                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  const Data& getAddress() const {return mAddress;}
                  const Data& getFreeText() const {return mFreeText;}

               private:
                  Email() {}

                  Data mAddress;
                  Data mFreeText;

                  friend class Session;
            };

            class Phone
            {
               public:
                  Phone(const Data& number,
                        const Data& freeText);

                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  const Data& getNumber() const {return mNumber;}
                  const Data& getFreeText() const {return mFreeText;}

               private:
                  Phone() {}

                  Data mNumber;
                  Data mFreeText;

                  friend class Session;
            };

            class Connection
            {
               public:
                  Connection(AddrType addType,
                             const Data& address,
                             unsigned int ttl = 0);
                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  AddrType getAddressType() const {return mAddrType;}
                  const Data& getAddress() const {return mAddress;}
                  unsigned int getTTL() const {return mTTL;}

               private:
                  Connection();

                  AddrType mAddrType;
                  Data mAddress;
                  unsigned int mTTL;

                  friend class Session;
                  friend class Medium;
            };

            class Bandwidth
            {
               public:
                  Bandwidth(const Data& modifier,
                            unsigned int kbPerSecond);
                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  const Data& getModifier() const {return mModifier;}
                  unsigned int getKbPerSecond() const {return mKbPerSecond;}

               private:
                  Bandwidth() {}
                  Data mModifier;
                  unsigned int mKbPerSecond;

                  friend class Session;
                  friend class Medium;
            };

            class Time
            {
               public:
                  Time(unsigned int start,
                       unsigned int stop);
                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  class Repeat
                  {
                     public:
                        Repeat(unsigned int interval,
                               unsigned int duration,
                               std::list<int> offsets);
                        void parse(ParseBuffer& pb);
                        std::ostream& encode(std::ostream&) const;

                        unsigned int getInterval() const {return mInterval;}
                        unsigned int getDuration() const {return mDuration;}
                        const std::list<int> getOffsets() const {return mOffsets;}
                        
                     private:
                        Repeat() {}
                        unsigned int mInterval;
                        unsigned int mDuration;
                        std::list<int> mOffsets;

                        friend class Time;
                  };

                  void addRepeat(const Repeat& repeat);

                  unsigned int getStart() const {return mStart;}
                  unsigned int getStop() const {return mStop;}
                  const std::list<Repeat>& getRepeats() const {return mRepeats;}

               private:
                  Time() {}
                  unsigned int mStart;
                  unsigned int mStop;
                  std::list<Repeat> mRepeats;

                  friend class Session;
            };

            class Timezones
            {
               public:
                  class Adjustment
                  {
                     public:
                        Adjustment(unsigned int time,
                                   int offset);
                        
                        unsigned int time;
                        int offset;
                  };

                  Timezones();
                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  void addAdjustment(const Adjustment& adjusment);
                  const std::list<Adjustment>& getAdjustments() const {return mAdjustments; }
               private:
                  std::list<Adjustment> mAdjustments;
            };

            class Encryption
            {
               public:
                  typedef enum {NoEncryption = 0, Prompt, Clear, Base64, UriKey} KeyType;
                  Encryption(const KeyType& method,
                             const Data& key);
                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;

                  const KeyType& getMethod() const {return mMethod;}
                  void setMethod( const KeyType& k) { mMethod=k; }
                  const Data& getKey() const {return mKey;}

                  Encryption();
               private:
             
                  KeyType mMethod;
                  Data mKey;

				  //friend class Vocal2::SdpContents::Session;
				  //friend class Vocal2::SdpContents::Session::Medium;
            };

            class Medium
            {
               public:
                  Medium(const Data& name,
                         unsigned int port,
                         unsigned int multicast,
                         const Data& protocol);
                  void parse(ParseBuffer& pb);
                  std::ostream& encode(std::ostream&) const;
                  
                  void addFormat(const Data& format);
                  void addConnection(const Connection& connection);
                  void addBandwidth(const Bandwidth& bandwidth);
                  void addAttribute(const Data& key, const Data& value = Data::Empty);

                  const Data& getName() const {return mName;}
                  int getPort() const {return mPort;}
                  int getMulticast() const {return mMulticast;}
                  const Data& getProtocol() const {return mProtocol;}
                  const std::list<Data>& getFormats() const {return mFormats;}
                  const Data& getInformation() const {return mInformation;}
                  const std::list<Bandwidth>& getBandwidths() const {return mBandwidths;}

                  // from session if empty
                  const std::list<Connection>& getConnections() const;
                  const Encryption& getEncryption() const;
                  bool exists(const Data& key) const;
                  const list<Data>& getValue(const Data& key) const;

               private:
                  Medium();
                  void setSession(Session* session);
                  Session* mSession;

                  Data mName;
                  unsigned int mPort;
                  unsigned int mMulticast;
                  Data mProtocol;
                  std::list<Data> mFormats;
                  
                  Data mTransport;

                  Data mInformation;
                  std::list<Connection> mConnections;
                  std::list<Bandwidth> mBandwidths;
                  Encryption mEncryption;
                  AttributeHelper mAttributeHelper;

                  friend class Session;
            };

            Session(int version,
                    const Origin& origin,
                    const Data& name);

            void parse(ParseBuffer& pb);
            std::ostream& encode(std::ostream&) const;

            int getVersion() const {return mVersion;}
            const Origin& getOrigin() const {return mOrigin;}
            const Data& getName() const {return mName;}
            const Data& getInformation() const {return mInformation;}
            const Uri& getUri() const {return mUri;}
            const std::list<Email>& getEmails() const {return mEmails;}
            const std::list<Phone>& getPhones() const {return mPhones;}
            Connection& connection() {return mConnection;}
            const Connection& connection() const {return mConnection;}
            const std::list<Bandwidth>& getBandwidths() const {return mBandwidths;}
            const std::list<Time>& getTimes() const {return mTimes;}
            const Timezones& getTimezones() const {return mTimezones;}
            const Encryption& getEncryption() const {return mEncryption;}
            const std::list<Medium> getMedia() const {return mMedia;}
            
            void addEmail(const Email& email);
            void addPhone(const Phone& phone);
            void addBandwidth(const Bandwidth& bandwidth);
            void addTime(const Time& t);
            void addMedium(const Medium& medium);
            void addAttribute(const Data& key, const Data& value = Data::Empty);
            bool exists(const Data& key) const;
            const list<Data>& getValue(const Data& key) const;

         private:
            Session() {}

            int mVersion;
            Origin mOrigin;
            Data mName;
            std::list<Medium> mMedia;

            // applies to all Media where unspecified
            Data mInformation;
            Uri mUri;            
            std::list<Email> mEmails;
            std::list<Phone> mPhones;
            Connection mConnection;
            std::list<Bandwidth> mBandwidths;
            std::list<Time> mTimes;
            Timezones mTimezones;
            Encryption mEncryption;
            AttributeHelper mAttributeHelper;

            friend class SdpContents;
      };

      SdpContents();
      SdpContents(const Data& data, const Mime& contentTypes);
      SdpContents(HeaderFieldValue* hfv, const Mime& contentTypes);
      virtual Contents* clone() const;

      Session& session() {checkParsed(); return mSession;}
      const Session& session() const {checkParsed(); return mSession;}

      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual void parse(ParseBuffer& pb);
      virtual const Mime& getStaticType() const;
   private:
      Session mSession;
      static ContentsFactory<SdpContents> Factory;
};

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
 *    notice, this std::list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this std::list of conditions and the following disclaimer in
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
