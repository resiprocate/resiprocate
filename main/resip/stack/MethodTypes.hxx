#if !defined(RESIP_METHODTYPES_HXX)
#define RESIP_METHODTYPES_HXX 

#define defineMethod(_enum, _name, _rfc) _enum
namespace resip
{

class Data;

#ifdef  __BORLANDC__
   #undef MESSAGE
#endif

typedef enum
{
   defineMethod(UNKNOWN, "UNKNOWN", "NA"),
   defineMethod(ACK, "ACK", " RFC 3261"),
   defineMethod(BYE, "BYE", "RFC 3261"),
   defineMethod(CANCEL, "CANCEL", "RFC 3261"),
   defineMethod(INVITE, "INVITE", "RFC 3261"),
   defineMethod(NOTIFY, "NOTIFY", "RFC 3265"),
   defineMethod(OPTIONS, "OPTIONS", "RFC 3261"),
   defineMethod(REFER, "REFER", "RFC 3515"),
   defineMethod(REGISTER, "REGISTER", "RFC 3261"),
   defineMethod(SUBSCRIBE, "SUBSCRIBE", "RFC 3265"),
   defineMethod(RESPONSE, "RESPONSE", "RFC ????"),
   defineMethod(MESSAGE, "MESSAGE", "RFC ????"),
   //_MESSAGE,
   defineMethod(INFO, "INFO", "RFC 2976"),
   defineMethod(PRACK, "PRACK", "RFC 3262"),
   defineMethod(PUBLISH, "PUBLISH", "RFC draft"),
   defineMethod(SERVICE, "SERVICE", "!RFC"),
   defineMethod(UPDATE, "UPDATE", "RFC 3311"),
   MAX_METHODS
} MethodTypes;

// extern Data MethodNames[]; // !ah! Do not touch. want a name, call getMethodName()

MethodTypes
getMethodType(const Data& name);

MethodTypes
getMethodType(const char* name, int len);

// TODO -- !dcm! -- fix to return unknown method as a string
const Data&
getMethodName(MethodTypes t);

}

#undef defineMethod

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
