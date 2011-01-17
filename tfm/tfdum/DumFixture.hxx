#ifndef _DumFixture_hxx_
#define _DumFixture_hxx_

#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestSuite.h"

#include "DumUserAgent.hxx"
#include "tfm/StunEndPoint.hxx"

#include "rutil/Data.hxx"
#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/Pidf.hxx"
#include "tfm/SequenceSet.hxx"
#include "tfm/TestUser.hxx"
#include "tfm/repro/TestReproUser.hxx"
#include "tfm/Source.hxx"
#include "tfm/TestSipEndPoint.hxx"
#include "tfm/tfdum/TfdTestSipEndPoint.hxx"

class DumFixture : public CppUnit::TestFixture
{
   public:

      DumFixture();
      
      virtual ~DumFixture() ;
      virtual void setUp();
      virtual void tearDown();
      static void initialize(int argc, char** argv);
      static void destroyStatic();

      static DumUserAgent* createUserAgentForFailoverTest(const resip::Data& user, const resip::Data&);
      
      static std::auto_ptr<resip::Pidf> makePidf(const DumUserAgent* dua);

      static resip::ExternalDns* createExternalDns();

      static resip::Security* security;
      static TestProxy* proxy;

      static DumUserAgent* derek;
      static DumUserAgent* jason;
      static DumUserAgent* scott;
      static DumUserAgent* duane;
      static DumUserAgent* hiss;
      static TestReproUser* david;

      //static DumUserAgent* jozsef;
      static StunEndPoint* stunEndPoint;
      static TestSipEndPoint* sipEndPoint;
      static TestSipEndPoint* serviceEndPoint;
      
      static resip::SdpContents* standardOffer;
      static resip::SdpContents* standardAnswer;

      static resip::SdpContents* standardOfferAudio;
      static resip::SdpContents* standardAnswerAudio;

      static TfdTestSipEndPoint* sipEndPointTcp;


      static resip::Data ipProxy1;
      static resip::Data domainProxy1;
      static int portProxy1;
      static resip::Data ipProxy2;
      static resip::Data domainProxy2;
      static int portProxy2;
      static resip::Data ipProxy3;
      static resip::Data domainProxy3;
      static int portProxy3;

      static int portLocalhost;

      static resip::Data domain;
      static TestSipEndPoint* registrar;;

};

#endif

