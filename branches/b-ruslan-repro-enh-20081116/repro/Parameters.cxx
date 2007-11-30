#include "Parameters.hxx"
#include "rutil/MD5Stream.hxx"

namespace repro{

static bool EnabledParam[Parameters::prmMax]; 
static Data NameOfParam[Parameters::prmMax]; 
static AbstractDb *Db;
Parameters::ParamType Parameters::TypeOfParam[Parameters::prmMax];

static class InitParams
{
public:
   InitParams() { Parameters::initParameters(); };
} Init;

void 
Parameters::disableParam( Param prm )
{
   EnabledParam[prm] = false;
}

void 
Parameters::setDb( AbstractDb *database )
{
   Db = database;
}

void 
Parameters::saveParam(Param prm, Data value)
{
   assert(Db);
   assert(prm>0 && prm <prmMax);
   Data str;
   switch (TypeOfParam[prm])
   {
   case ptBool:
      if (!value.empty() && value != " ")
      {
         if (value=="0" || value.lowercase() == "false" || value.lowercase() == "no")
         {
            str = Data( "No" );
         }
         else
         {
            str = Data( "Yes" );
         }
      }
      break;
   case ptInt:
      if (!value.empty() && value != " ")
      {
         str = Data(value.convertInt());
      }
      break;
   case ptStrings:
      str = value;
      str.replace("\n","");
      str.replace("\t","");
      str.replace("\r","");
      str.replace(" ","");
      str.replace("\"","");
      str.replace("\'","");
      break;
   default:
      str = value;
      break;
   };
   if((str.empty() || str == " ") && prm != prmAdminPassword)
   {
      Db->dbEraseRecord(AbstractDb::ParametersTable, NameOfParam[prm]);
   }
   else
   {
      if(prm == prmAdminPassword)
      {
         if(!str.empty())
         {
            MD5Stream a1;
            a1 << Data( "admin" ) // username
               << Symbols::COLON
               << Data::Empty // realm
               << Symbols::COLON
               << str;
            Data str2 = a1.getHex();
            Db->dbWriteRecord(AbstractDb::ParametersTable, NameOfParam[prm], str2);
         }
      }
      else
      {
         Db->dbWriteRecord(AbstractDb::ParametersTable, NameOfParam[prm], str);
      }
   }
}

Data 
Parameters::getParam(Param prm)
{
   assert(Db);
   Data ret;
   Db->dbReadRecord(AbstractDb::ParametersTable, NameOfParam[prm], ret);
   return ret;
}

void 
Parameters::setParamBool(Parameters::Param prm, bool& val)
{
   Data str;
   static Data True( "Yes" );
   if ( EnabledParam[prm] && Db->dbReadRecord( AbstractDb::ParametersTable, NameOfParam[prm], str ) )
   {
      val = str == True;
   }
}

void 
Parameters::setParamInt(Parameters::Param prm, int& val)
{
   Data str;
   if (EnabledParam[prm] && Db->dbReadRecord(AbstractDb::ParametersTable, NameOfParam[prm], str))
   {
      val = str.convertInt();
   }
}

void 
Parameters::setParamString(Parameters::Param prm, Data& val)
{
   Data str;
   if (EnabledParam[prm] && Db->dbReadRecord(AbstractDb::ParametersTable, NameOfParam[prm], str ))
   {
      val = str;
   }
}

void 
Parameters::setParamStrings( Parameters::Param prm, std::vector<Data>& val, const char *description )
{
   Data str;
   if (EnabledParam[prm] && Db->dbReadRecord(AbstractDb::ParametersTable, NameOfParam[prm], str))
   {
      val = ReproConfiguration::toVector(str.c_str(), description);
   }
}

void 
Parameters::storeParametersInArgs(ReproConfiguration *to)
{
   assert( Db );
   setParamString(prmLogType, to->mLogType);
   setParamString(prmLogLevel, to->mLogLevel);
   setParamString(prmLogPath, to->mLogFilePath);
   Data str;
   if ( Db->dbReadRecord( AbstractDb::ParametersTable, NameOfParam[prmRecordRoute], str ) )
   {
      to->mShouldRecordRoute = true;
      to->mRecordRoute = ReproConfiguration::toUri( str.c_str(), "Record-Route");
   }
   setParamInt( prmUdp, to->mUdpPort );
   setParamInt( prmTcp, to->mTcpPort );
   setParamString( prmTlsDomain, to->mTlsDomain );
   setParamInt( prmTls, to->mTlsPort );
   setParamInt( prmDtls, to->mDtlsPort );
   setParamBool( prmEnableCertServer, to->mCertServer );
   setParamBool( prmEnableV6, to->mUseV6 );
   bool Bool = !to->mUseV4 ;
   setParamBool( prmDisableV4, Bool );
   to->mUseV4 = !Bool;
   setParamBool( prmDisableAuth, to->mNoChallenge );
   setParamBool( prmDisableAuthInt, to->mNoAuthIntChallenge );
   setParamBool( prmDisableWebAuth, to->mNoWebChallenge );
   setParamBool( prmDisableReg, to->mNoRegistrar );
   setParamBool( prmDisableIdentity, to->mNoIdentityHeaders );
   setParamStrings( prmIinterfaces, to->mInterfaces, "interfaces" );
   setParamStrings( prmDomains, to->mDomains, "domains" );
   setParamStrings( prmRoute, to->mRouteSet, "routeSet" );
   setParamString( prmReqChainName, to->mRequestProcessorChainName );
   setParamInt( prmHttp, to->mHttpPort );
   setParamBool( prmRecursiveRedirect, to->mRecursiveRedirect );
   setParamBool( prmQValue, to->mDoQValue );
   setParamString( prmQValueBehavior, to->mForkBehavior );
   setParamBool( prmQValueCancelBtwForkGroups, to->mCancelBetweenForkGroups );
   setParamBool( prmQValueWaitForTerminateBtwForkGroups, to->mWaitForTerminate );
   setParamInt( prmQValueMsBetweenForkGroups, to->mMsBetweenForkGroups );
   setParamInt( prmQValueMsBeforeCancel, to->mMsBeforeCancel );
   setParamString( prmEnumSuffix, to->mEnumSuffix );
   setParamBool( prmAllowBadReg, to->mAllowBadReg );
   setParamBool( prmParallelForkStaticRoutes, to->mParallelForkStaticRoutes );
   setParamInt( prmTimerC, to->mTimerC );
//   setParamString( prmAdminPassword, To->mAdminPassword );
}

void 
Parameters::initParameters()
{
   for (int i=0; i<prmMax; i++)
   {
      EnabledParam[i] = true;
   }

   NameOfParam[prmLogType] = "prmLogType";
   TypeOfParam[prmLogType] = ptString;
   NameOfParam[prmLogLevel] = "prmLogLevel";
   TypeOfParam[prmLogLevel] = ptString;
   NameOfParam[prmLogPath] = "prmLogPath";
   TypeOfParam[prmLogPath] = ptString;
   NameOfParam[prmRecordRoute] = "prmRecordRoute";
   TypeOfParam[prmRecordRoute] = ptRecordRoute;
   NameOfParam[prmUdp] = "prmUdp";
   TypeOfParam[prmUdp] = ptInt;
   NameOfParam[prmTcp] = "prmTcp";
   TypeOfParam[prmTcp] = ptInt;
   NameOfParam[prmTlsDomain] = "prmTlsDomain";
   TypeOfParam[prmTlsDomain] = ptString;
   NameOfParam[prmTls] = "prmTls";
   TypeOfParam[prmTls] = ptInt;
   NameOfParam[prmDtls] = "prmDtls";
   TypeOfParam[prmDtls] = ptInt;
   NameOfParam[prmEnableCertServer] = "prmEnableCertServer";
   TypeOfParam[prmEnableCertServer] = ptBool;
   NameOfParam[prmEnableV6] = "prmEnableV6";
   TypeOfParam[prmEnableV6] = ptBool;
   NameOfParam[prmDisableV4] = "prmDisableV4";
   TypeOfParam[prmDisableV4] = ptBool;
   NameOfParam[prmDisableAuth] = "prmDisableAuth";
   TypeOfParam[prmDisableAuth] = ptBool;
   NameOfParam[prmDisableAuthInt] = "prmDisableAuthInt";
   TypeOfParam[prmDisableAuthInt] = ptBool;
   NameOfParam[prmDisableWebAuth] = "prmDisableWebAuth";
   TypeOfParam[prmDisableWebAuth] = ptBool;
   NameOfParam[prmDisableReg] = "prmDisableReg";
   TypeOfParam[prmDisableReg] = ptBool;
   NameOfParam[prmDisableIdentity] = "prmDisableIdentity";
   TypeOfParam[prmDisableIdentity] = ptBool;
   NameOfParam[prmIinterfaces] = "prmIinterfaces";
   TypeOfParam[prmIinterfaces] = ptStrings;
   NameOfParam[prmDomains] = "prmDomains";
   TypeOfParam[prmDomains] = ptStrings;
   NameOfParam[prmRoute] = "prmRoute";
   TypeOfParam[prmRoute] = ptStrings;
   NameOfParam[prmReqChainName] = "prmReqChainName";
   TypeOfParam[prmReqChainName] = ptString;
   NameOfParam[prmHttp] = "prmHttp";
   TypeOfParam[prmHttp] = ptInt;
   NameOfParam[prmRecursiveRedirect] = "prmRecursiveRedirect";
   TypeOfParam[prmRecursiveRedirect] = ptBool;
   NameOfParam[prmQValue] = "prmQValue";
   TypeOfParam[prmQValue] = ptBool;
   NameOfParam[prmQValueBehavior] = "prmQValueBehavior";
   TypeOfParam[prmQValueBehavior] = ptString;
   NameOfParam[prmQValueCancelBtwForkGroups] = "prmQValueCancelBtwForkGroups";
   TypeOfParam[prmQValueCancelBtwForkGroups] = ptBool;
   NameOfParam[prmQValueWaitForTerminateBtwForkGroups] = "prmQValueWaitForTerminateBtwForkGroups";
   TypeOfParam[prmQValueWaitForTerminateBtwForkGroups] = ptBool;
   NameOfParam[prmQValueMsBetweenForkGroups] = "prmQValueMsBetweenForkGroups";
   TypeOfParam[prmQValueMsBetweenForkGroups] = ptInt;
   NameOfParam[prmQValueMsBeforeCancel] = "prmQValueMsBeforeCancel";
   TypeOfParam[prmQValueMsBeforeCancel] = ptInt;
   NameOfParam[prmEnumSuffix] = "prmEnumSuffix";
   TypeOfParam[prmEnumSuffix] = ptString;
   NameOfParam[prmAllowBadReg] = "prmAllowBadReg";
   TypeOfParam[prmAllowBadReg] = ptBool;
   NameOfParam[prmParallelForkStaticRoutes] = "prmParallelForkStaticRoutes";
   TypeOfParam[prmParallelForkStaticRoutes] = ptBool;
   NameOfParam[prmTimerC] = "prmTimerC";
   TypeOfParam[prmTimerC] = ptInt;
   NameOfParam[prmAdminPassword] = "prmAdminPassword";
   TypeOfParam[prmAdminPassword] = ptString;
}

}

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
