class Acl
{


   private:
      class V6AddrMask
      {
         public:
            V6AddrMask(in_addr6 ip6Addr = v6Loopback, int mask = 128)
         friend:
            in_addr6 mIp6Addr;  // ?? check these types
            int mMask;
      };
      
      class V4AddrMask
      {
         public:
            V4AddrMask(in_addr ip4Addr = v4Loopback, int mask = 32)
         friend:
            in_addr mIp4Addr;  // ?? check these types
            int mMask;
      };
      
      typedef list<V6AddrMask> v6addrMaskList;
      typedef list<V4AddrMask> v4addrMaskList;

      v6addrMaskList mV6AclList;
      v4addrMaskList mV4AclList;
};



bool Acl::parseAcl(Data& rawInput)
{
   // Input can be in any of these formats
   // localhost         localhost  (becomes 127.0.0.1/8, ::1/128 and fe80::1/64)
   // bare hostname     server1
   // FQDN              server1.example.com
   // IPv4 address      192.168.1.100
   // IPv4 + mask       192.168.1.0/24
   // IPv6 address      :341:0:23:4bb:0011:2435:abcd
   // IPv6 + mask       :341:0:23:4bb:0011:2435:abcd/80
   // IPv6 reference    [:341:0:23:4bb:0011:2435:abcd]
   // IPv6 ref + mask   [:341:0:23:4bb:0011:2435:abcd]/64
   
   ParseBuffer pb(rawInput);
   const char* anchor = pb.start();
   
   bool ipv4 = false;
   bool ipv6 = false;
   bool fqdn = false;
   Data hostOrIp;
   u_char in6[20];
   u_char in4[4];
   int mask;
   
   if (*pb.position() == '[')   // encountered beginning of IPv6 reference
   {
      anchor = pb.skipChar();
      skipToEndQuote(']');
      // TODO check for end of stream here
      
      pb.data(hostOrIp, anchor);  // copy the presentation form of the IPv6 address
      anchor = pb.skipChar();
      
      // try to convert into IPv6 network form
      if (!inet_pton6(hostOrIp.c_str(), in6))  // is this correct?
      {
         return INVALID;
      }
      ipv6 = true;
   }
   else
   {
      pb.skipToOneOf(".:");
      if (pb.position() == pb.end())   // We probably have a bare hostname
      {
         pb.data(hostOrIp, anchor);
         if (hostOrIp.lowercase() == "localhost")
         {
            // add special localhost addresses for v4 and v6 to list and return
            return SUCCESS;
         }
         // hostOrIp += default domain name
      }
      else if (*pb.position() == ':')     // Must be an IPv6 address
      {
         pb.skipToChar('/');

         pb.data(hostOrIp, anchor);  // copy the presentation form of the IPv6 address
         anchor = pb.skipChar();

         // try to convert into IPv6 network form
         if (!inet_pton6(hostOrIp.c_str(), in6))  // is this correct?
         {
            return INVALID;
         }
         ipv6 = true;
      }
      else // *pb.position() == '.'
      {
         resip_assert( *pb.position() == '.');
         
         // Could be either an IPv4 address or an FQDN
         pb.skipToChar('/');
         pb.data(hostOrIp, anchor);  // copy the presentation form of the address

         // try to interpret as an IPv4 address, if that fails look it up in DNS
         if (inet_pton4(hostOrIp.c_str(), in4)) // is this correct?
         {
            // it was an IPv4 address
            ipv4 = true;
         }
         else
         {
            // hopefully it is a legal FQDN, try it.
            fqdn = true;
         }
      }   
   }

   if (fqdn)
   {
      // do DNS A and AAAA lookups and store the results with the default (host) mask
      return;
   }
      
   if (*pb.position() == '/')    // grab the mask as well
   {
      anchor = pb.skipChar();
      mask = pb.integer();
      
      if (ipv4)
      {
         if (mask < 8 || mask > 32)
         {
            return INVALID;
         }
      }
      else if (ipv6)
      {
         if (mask < 64 || mask > 128)
         {
            return INVALID;
         }
      }
   }
   else
   {
      if (ipv4)
      {
         mask = 32;
      }
      else // ipv6
      {
         mask = 128;
      }
   }
   
   if pb.position() == pb.end())
   {
      if (ipv6)
      {
         mV6AclList.pushback(V6AddrMask(in6,mask));
      }
      
      if (ipv4)
      {
         mV4AclList.pushback(V4AddrMask(in4,mask));
      }
      return SUCCESS;
   }      
   return INVALID;
};
