#include "resip/stack/ValidationHelper.hxx"

#include "resip/stack/Symbols.hxx"

#include "rutil/DnsUtil.hxx"
#include "rutil/ParseBuffer.hxx"

namespace resip
{

bool 
ValidationHelper::checkIsUser(const Data& user)
{
   if(!user.containsOnly(Symbols::UserC,true))
   {
      return false;
   }
   return true;
}

bool 
ValidationHelper::checkIsUserInfo(const Data& userInfo)
{
   ParseBuffer pb(userInfo.data(), userInfo.size());
   try
   {
      // We parse from back because the stuff before the password can include
      // a telephone-subscriber, which can contain quoted strings, and therefore
      // ':'.
      pb.skipToEnd();
      pb.skipBackToChar(':');
      const char* passStart=pb.position();
      pb.skipToEnd();
      Data pass;
      pb.data(pass, passStart);
      if(pass.containsOnly(Symbols::Password,true))
      {
         // Looks like a password to me.
         pb.reset(passStart);
         pb.skipBackChar(':');
      } // else, this might be part of a quoted string in a user param

      Data beforePassword;
      pb.data(beforePassword, pb.start());
      if(!checkIsUser(beforePassword) && 
         !checkIsTelSubscriber(beforePassword,true))
      {
         return false;
      }
   }
   catch(ParseException&)
   {
      return false;
   }
   return true;
}

bool 
ValidationHelper::checkIsUserAtHost(const Data& userAtHost)
{
   ParseBuffer pb(userAtHost.data(), userAtHost.size());
   try
   {
      const char* userInfoStart=pb.position();
      pb.skipToEnd();
      pb.skipBackToChar('@');
      if(pb.position()!=userInfoStart)
      {
         const char* atSym=pb.position();
         if(*pb.position()=='[')
         {
            // ipv6 reference
            pb.skipChar('[');
            const char* v6Start=pb.position();
            pb.skipToChar(']');
            Data addr;
            pb.data(addr, v6Start);
            if(!DnsUtil::isIpV6Address(addr))
            {
               return false;
            }
            pb.skipChar(']');
         }
         else
         {
            const char* hostStart=pb.position();
            pb.skipToChar(':');
            Data host;
            pb.data(host,hostStart);
            if(!checkIsHost(host))
            {
               return false;
            }
         }

         if(!pb.eof())
         {
            pb.skipChar(':');
            UInt32 port=pb.uInt32();
            if(port > 65535)
            {
               return false;
            }
         }
         pb.assertEof();

         pb.reset(atSym);
         pb.skipBackChar('@'); // end of user part
      }
      else
      {
         pb.skipToEnd(); // end of user part
      }

      Data userInfo;
      pb.data(userInfo, userInfoStart);
      if(!checkIsUserInfo(userInfo))
      {
         return false;
      }
   }
   catch(ParseException&)
   {
      return false;
   }
   return true;
}


bool 
ValidationHelper::checkIsTelSubscriber(const Data& telSub, bool allowParams)
{
   ParseBuffer pb(telSub.data(),telSub.size());
   try
   {
      if(*pb.position()=='+')
      {
         pb.skipChar('+');
         const char* start = pb.position();
         pb.skipToChar(';');
         Data digits;
         pb.data(digits,start);
         if(digits.empty() || !digits.containsOnly(Symbols::PhoneDigit, false))
         {
            return false;
         }
         // Either eof or at a ';'
      }
      else
      {
         const char* start = pb.position();
         pb.skipToChar(';');
         Data digits;
         pb.data(digits,start);
         if(digits.empty() || !digits.containsOnly(Symbols::TelC, false))
         {
            return false;
         }
         // Either eof or at a ';'
      }

      while(!pb.eof())
      {
         pb.skipChar(';');
         const char* start=pb.position();
         pb.skipToOneOf("=;");
         Data paramName;
         pb.data(paramName, start);
         if(paramName.empty() || 
            !paramName.containsOnly(Symbols::TelToken, false))
         {
            return false;
         }

         if(!pb.eof() && *pb.position()=='=')
         {
            pb.skipChar('=');
            if(*pb.position()=='\"')
            {
               pb.skipChar('\"');
               start=pb.position();
               pb.skipToEndQuote('\"');
               Data string;
               pb.data(string, start);
               if(!string.containsOnly(Symbols::QDText,true,true))
               {
                  return false;
               }
               pb.skipChar('\"');
               pb.skipToChar(';'); // Either eof or at a ';'
            }
            else
            {
               start=pb.position();
               pb.skipToOneOf(";?");
               Data token1;
               pb.data(token1, start);
               if(token1.empty() || 
                  !token1.containsOnly(Symbols::TelToken,false))
               {
                  return false;
               }

               if(!pb.eof() && *pb.position()=='?')
               {
                  pb.skipChar('?');
                  start=pb.position();
                  pb.skipToChar(';');
                  Data token2;
                  pb.data(token2, start);
                  if(token2.empty() || 
                     !token2.containsOnly(Symbols::TelToken,false))
                  {
                     return false;
                  }
               }
            } // Either eof or at a ';'
         } // Either eof or at a ';'
      }
   }
   catch(ParseException&)
   {
      return false;
   }
   return true;
}

bool 
ValidationHelper::checkIsHost(const Data& host)
{
   if(!DnsUtil::isIpV4Address(host) && !DnsUtil::isIpV6Address(host))
   {
      ParseBuffer pb(host.data(), host.size());
      try
      {
         while(!pb.eof())
         {
            const char* start=pb.position();
            pb.skipToChar('.');
            Data label;
            pb.data(label,start);
      
            if(label.empty())
            {
               return false;
            }
      
            if(!label.containsOnly(Symbols::DomainPartChars,false))
            {
               return false;
            }
      
            if(label[0]=='-' || label[label.size()-1]=='-')
            {
               // Segment can't begin or end with a '-'
               return false;
            }
      
            if(pb.eof())
            {
               // Last label needs to start with an ALPHA
               if(!Symbols::Alpha[label[0]])
               {
                  return false;
               }
            }
            else
            {
               pb.skipChar('.');
            }
         }
      }
      catch(ParseException&)
      {
         return false;
      }
   }
   return true;
}


}
