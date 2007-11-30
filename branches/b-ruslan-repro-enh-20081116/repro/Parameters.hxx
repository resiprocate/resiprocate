#if !defined(RESIP_PARAMETERS_HXX)
#define RESIP_PARAMETERS_HXX

#include "ReproConfiguration.hxx"
#include "AbstractDb.hxx"

namespace repro
{

using namespace resip;

class Parameters
{
public:
   enum Param {  prmLogType = 1,  prmLogLevel, prmLogPath, prmRecordRoute, prmUdp, prmTcp,
            prmTlsDomain, prmTls, prmDtls, prmEnableCertServer, prmEnableV6, prmDisableV4, 
            prmDisableAuth, prmDisableAuthInt, prmDisableWebAuth, prmDisableReg, 
            prmDisableIdentity, prmIinterfaces, prmDomains, prmRoute, prmReqChainName, 
            prmHttp, prmRecursiveRedirect, prmQValue, prmQValueBehavior, 
            prmQValueCancelBtwForkGroups, prmQValueWaitForTerminateBtwForkGroups, 
            prmQValueMsBetweenForkGroups, prmQValueMsBeforeCancel, prmEnumSuffix, 
            prmAllowBadReg, prmParallelForkStaticRoutes, prmTimerC, prmAdminPassword, prmMax };
   enum ParamType { ptInt, ptBool, ptString, ptStrings, ptRecordRoute };
   static Parameters::ParamType TypeOfParam[Parameters::prmMax];
   
   static void disableParam(Param prm);
   static void saveParam(Param prm, Data value);
   static Data getParam(Param prm);
   static void storeParametersInArgs(ReproConfiguration *to);
   static void setDb(AbstractDb *database);

private:
   static void  initParameters();
   friend class InitParams;
   static void setParamBool(Parameters::Param prm, bool& val);
   static void setParamInt(Parameters::Param prm, int& val);
   static void setParamString(Parameters::Param prm, Data& val);
   static void setParamStrings(Parameters::Param prm, std::vector<Data>& val, const char *description);

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

