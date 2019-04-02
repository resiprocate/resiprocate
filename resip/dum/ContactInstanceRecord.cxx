#include <sstream>

#include "resip/dum/ContactInstanceRecord.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/XMLCursor.hxx"
#include "rutil/Timer.hxx"

using namespace resip;

ContactInstanceRecord::ContactInstanceRecord() : 
   mRegExpires(0),
   mLastUpdated(Timer::getTimeSecs()),
   mRegId(0),
   mSyncContact(false),
   mUseFlowRouting(false),
   mUserInfo(0),
   mUserData(0)
{
}

ContactInstanceRecord::ContactInstanceRecord(const ContactInstanceRecord& rhs) :
   mUserData(0)
{
   *this = rhs;
}

ContactInstanceRecord& ContactInstanceRecord::operator=(const ContactInstanceRecord& rhs)
{
   mContact = rhs.mContact;
   mRegExpires = rhs.mRegExpires;
   mLastUpdated = rhs.mLastUpdated;
   mReceivedFrom = rhs.mReceivedFrom;
   mPublicAddress = rhs.mPublicAddress;
   mSipPath = rhs.mSipPath;
   mInstance = rhs.mInstance;
   mUserAgent = rhs.mUserAgent;
   mRegId = rhs.mRegId;
   mSyncContact = rhs.mSyncContact;
   mUseFlowRouting = rhs.mUseFlowRouting;
   mUserInfo = rhs.mUserInfo;
   if(mUserData && rhs.mUserData == 0)
   {
      delete mUserData;
      mUserData = 0;
   }
   else if(mUserData == 0 && rhs.mUserData)
   {
      mUserData = new Data(*rhs.mUserData);
   }
   else if(rhs.mUserData)
   {
      *mUserData = *rhs.mUserData;
   }
   // else both are already NULL

   return(*this);
}

ContactInstanceRecord::~ContactInstanceRecord()
{
    if(mUserData)
    {
        delete mUserData;
        mUserData = 0;
    }
}

bool
ContactInstanceRecord::operator==(const ContactInstanceRecord& rhs) const
{
   if((mRegId != 0 && !mInstance.empty()) || 
      (rhs.mRegId != 0 && !rhs.mInstance.empty()))
   {
      // If regId and instanceId is specified on either, then outbound RFC5626 is 
      // in use - match only if both instance id and reg-id match - ignore contact URI
      return mInstance == rhs.mInstance && 
             mRegId == rhs.mRegId;
   }
   else if (mRegId == 0 && rhs.mRegId == 0 &&
           !mInstance.empty() && !rhs.mInstance.empty())
   {
       // If RegId is not specified on either but instance Id is, then look for instanceId
       // match only - RFC5627 matching (even though we don't fully support GRUU yet)
       return mInstance == rhs.mInstance;
   }
   else
   {
      // otherwise both instance (if specified) and contact must match
      return mInstance == rhs.mInstance &&
             mContact.uri() == rhs.mContact.uri();
   }
}

void ContactInstanceRecord::stream(std::iostream& ss) const
{
    UInt64 now = Timer::getTimeSecs();

    ss << "   <contactinfo>" << Symbols::CRLF;
    ss << "      <contacturi>" << Data::from(mContact).xmlCharDataEncode() << "</contacturi>" << Symbols::CRLF;
    // If contact is expired or removed, then pass expires time as 0, otherwise send number of seconds until expirey
    ss << "      <expires>" << (((mRegExpires == 0) || (mRegExpires <= now)) ? 0 : (mRegExpires-now)) << "</expires>" << Symbols::CRLF;
    ss << "      <lastupdate>" << now-mLastUpdated << "</lastupdate>" << Symbols::CRLF;
    if(mReceivedFrom.getPort() != 0)
    {
        resip::Data binaryFlowToken;
        Tuple::writeBinaryToken(mReceivedFrom,binaryFlowToken);            
        ss << "      <receivedfrom>" << binaryFlowToken.base64encode() << "</receivedfrom>" << Symbols::CRLF;
    }
    if(mPublicAddress.getType() != UNKNOWN_TRANSPORT)
    {
        resip::Data binaryFlowToken;
        Tuple::writeBinaryToken(mPublicAddress,binaryFlowToken);            
        ss << "      <publicaddress>" << binaryFlowToken.base64encode() << "</publicaddress>" << Symbols::CRLF;
    }
    NameAddrs::const_iterator naIt = mSipPath.begin();
    for(; naIt != mSipPath.end(); naIt++)
    {
        ss << "      <sippath>" << Data::from(naIt->uri()).xmlCharDataEncode() << "</sippath>" << Symbols::CRLF;
    }
    if(!mInstance.empty())
    {
        ss << "      <instance>" << mInstance.xmlCharDataEncode() << "</instance>" << Symbols::CRLF;
    }
    if(mRegId != 0)
    {
        ss << "      <regid>" << mRegId << "</regid>" << Symbols::CRLF;
    }
    if (!mUserAgent.empty())
    {
       ss << "      <useragent>" << mUserAgent.xmlCharDataEncode() << "</useragent>" << Symbols::CRLF;
    }

    if(mUserData != 0 && mUserData->size())
    {
        ss << "      <userdata>" << *mUserData << "</userdata>" << Symbols::CRLF;
    }
    ss << "   </contactinfo>" << Symbols::CRLF;
}

bool ContactInstanceRecord::deserialize(resip::XMLCursor& xml, UInt64 now)
{
   bool success = false;
   // Reset this members
   *this = ContactInstanceRecord();
   if(now <= 0)
   {
       now = Timer::getTimeSecs();
   }
   
   if(isEqualNoCase(xml.getTag(), "contactinfo"))
   {
      if(xml.firstChild())
      {
         do
         {
            if(isEqualNoCase(xml.getTag(), "contacturi"))
            {
               if(xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: contacturi=" << xml.getValue());
                  mContact = NameAddr(xml.getValue().xmlCharDataDecode());
                  xml.parent();
                  success = true;
               }
            }
            else if(isEqualNoCase(xml.getTag(), "expires"))
            {
               if(xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: expires=" << xml.getValue());
                  UInt64 expires = xml.getValue().convertUInt64();
                  mRegExpires = (expires == 0 ? 0 : now+expires);
                  xml.parent();
               }
            }
            else if(isEqualNoCase(xml.getTag(), "lastupdate"))
            {
               if(xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: lastupdate=" << xml.getValue());
                  mLastUpdated = now-xml.getValue().convertUInt64();
                  xml.parent();
               }
            }
            else if(isEqualNoCase(xml.getTag(), "receivedfrom"))
            {
               if(xml.firstChild())
               {
                  mReceivedFrom = Tuple::makeTupleFromBinaryToken(xml.getValue().base64decode());
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: receivedfrom=" << xml.getValue() << " tuple=" << mReceivedFrom);
                  xml.parent();
               }
            }
            else if(isEqualNoCase(xml.getTag(), "publicaddress"))
            {
               if(xml.firstChild())
               {
                  mPublicAddress = Tuple::makeTupleFromBinaryToken(xml.getValue().base64decode());
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: publicaddress=" << xml.getValue() << " tuple=" << mPublicAddress);
                  xml.parent();
               }
            }
            else if(isEqualNoCase(xml.getTag(), "sippath"))
            {
               if(xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: sippath=" << xml.getValue());
                  mSipPath.push_back(NameAddr(xml.getValue().xmlCharDataDecode()));
                  xml.parent();
               }
            }
            else if(isEqualNoCase(xml.getTag(), "instance"))
            {
               if(xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: instance=" << xml.getValue());
                  mInstance = xml.getValue().xmlCharDataDecode();
                  xml.parent();
               }
            }
            else if(isEqualNoCase(xml.getTag(), "regid"))
            {
               if(xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: regid=" << xml.getValue());
                  mRegId = xml.getValue().convertUnsignedLong();
                  xml.parent();
               }
            }
            else if (isEqualNoCase(xml.getTag(), "useragent"))
            {
               if (xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: useragent=" << xml.getValue());
                  mUserAgent = xml.getValue().xmlCharDataDecode();
                  xml.parent();
               }
            }
            else if(isEqualNoCase(xml.getTag(), "userdata"))
            {
               if(xml.firstChild())
               {
                  //InfoLog(<< "RegSyncClient::handleRegInfoEvent: userdata=" << xml.getValue());
                  if(mUserData == 0)
                  {
                      mUserData = new Data("");
                  }
                  *mUserData = xml.getValue().xmlCharDataDecode();
                  xml.parent();
               }
            }
         } while(xml.nextSibling());
         xml.parent();
      }
   }

   return(success);
}


ContactInstanceRecord 
ContactInstanceRecord::makeRemoveDelta(const NameAddr& contact)
{
   ContactInstanceRecord c;
   c.mContact = contact;
   return c;
}

ContactInstanceRecord 
ContactInstanceRecord::makeUpdateDelta(const NameAddr& contact, 
                                       UInt64 expires,  // absolute time in secs
                                       const SipMessage& msg)
{
   ContactInstanceRecord c;
   c.mContact = contact;
   c.mRegExpires = expires;
   c.mReceivedFrom = msg.getSource();
   c.mPublicAddress = Helper::getClientPublicAddress(msg);
   if (msg.exists(h_Paths))
   {
      c.mSipPath = msg.header(h_Paths);
   }
   if (contact.exists(p_Instance))
   {
      c.mInstance = contact.param(p_Instance);
   }
   if (contact.exists(p_regid))
   {
      c.mRegId = contact.param(p_regid);
   }
   // !jf! need to fill in mServerSessionId here
   return c;
  
}

      
