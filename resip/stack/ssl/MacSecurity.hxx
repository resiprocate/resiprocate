#if !defined(RESIP_MACSECURITY_HXX)
#define RESIP_MACSECURITY_HXX

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "resip/stack/ssl/Security.hxx"

namespace resip
{

// Instead of using Mac specific data types
// we pass around handles. This keeps us from
// having to include Mac OS headers.
typedef void * KeychainHandle;

/*
 * Manages certificates in the Mac Keychain.
 */
class MacSecurity : public Security
{
   public:

      MacSecurity(){};

      // load root certificates into memory
      virtual void preload();

      // we need to stub out these functions since we don't
      // dynamically add or remove certificates on the mac
      virtual void onReadPEM(const Data&, PEMType, Data&) const
      { }
      virtual void onWritePEM(const Data&, PEMType, const Data&) const
      { }
      virtual void onRemovePEM(const Data&, PEMType) const
      { }

   protected:

      // Opens a search handle to certificates store in
      // the X509Anchors keychain
      KeychainHandle openSystemCertStore();
      
      // loads root certificates into memory
      void getCerts();
      
      void closeCertifStore(KeychainHandle searchReference);
};

} // namespace resip

#endif // ndef RESIP_MACSECURITY_HXX

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
