#include <iostream>
#ifndef WIN32
#include <unistd.h>
#endif

#include <limits.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <util/Logger.hxx>
#include <sipstack/HeaderTypes.hxx>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Vocal2::Subsystem::SIP

volatile bool signalled = false;

void tick(int sig)
{
   signalled = true;
}


static 
struct {
      char * keyword;
      Headers::Type type;
      int len;
} headerInfo[(int)(Headers::MAX_HEADERS-Headers::CSeq+1)];

unsigned int  InitHeaderInfo()
{
   int i = 0;
   
   for(Headers::Type t = Headers::CSeq;
       t < Headers::UNKNOWN;
       ++t,++i)
   {
      char* p = strdup(Headers::HeaderNames[t].c_str());
      
      headerInfo[i].len = Headers::HeaderNames[t].length();
      headerInfo[i].keyword = p;
      headerInfo[i].type = t;
      
      DebugLog(<< headerInfo[i].keyword << " ["
               << headerInfo[i].type <<"]");
   }

   return i;
   
}


unsigned short randomUShort()
{
   static int fd = 0;
   static bool init = false;
   if (!init)
   {
      fd = open("/dev/urandom",O_RDONLY);
      if (fd < 0)
      {
 cerr << "randomShort(): unable to open /dev/urandom -- degraded mode"
      << endl;
      }
      init = true;
   }
   if (init && fd >= 0)
   {
      unsigned short r;
      int n = 0;
      if ((n=read(fd,&r,sizeof(r))) == sizeof(r))
      {
 return r;
      }
   }
   // degraded mode
   return (unsigned short)((USHRT_MAX + 1.0) * rand() / (RAND_MAX+1.0));
}


int
main()
{
   int randomList[100*1024];


   unsigned int nRandom = sizeof(randomList)/sizeof(*randomList);
   
   register unsigned int i = 0;

   // Load up the main table
   unsigned short nKeywords =    InitHeaderInfo();
   

   // Verify that the hash function works.
   InfoLog(<<"Checking that hash function works for all known headers");
   
   for(i=0;i<nKeywords;i++)
   {
#if defined(GPERF)
      siphdrhash_s* hash = 
         Perfect_Hash::in_word_set(headerInfo[i].keyword,
                                   headerInfo[i].len);
      bool ok = headerInfo[i].type == hash->type;
#else
      Headers::Type t = Headers::getHeaderType(headerInfo[i].keyword,
                                               headerInfo[i].len);
      bool ok = headerInfo[i].type == t;
      
#endif
      InfoLog(<< headerInfo[i].keyword << " " << (ok?"OK":"FAIL"));
      if (!ok)
      {
         ErrLog(<<headerInfo[i].keyword << "["
                <<headerInfo[i].type << "] hashed to "
                << t << " ["
                << Headers::HeaderNames[t] << "]");
      }
      
   }

   
   // Make a large random list so we don't take a hit with 
   // random() calcs during the hash.

   InfoLog(<< "Pre-loading random list of " << nRandom << " entries");
   
   for(i = 0; i < nRandom ;  i++)
   {
      short r = randomUShort()%nKeywords;
      randomList[i] = r;
   }
   
   i=0;
   InfoLog(<<"Starting timing loop");

   register int totalTime=30;
   register int interval=5;
   register int timer=totalTime;
   register int elapsed=0;
   
   signal(SIGALRM, tick);
   register unsigned long long counter = 0;

   assert(totalTime/interval*interval == totalTime);
   
   while (timer>0)
   {
      alarm(interval);
      while (!signalled)
      {
#if defined(GPERF_HASH)
         volatile register siphdrhash_s* hash = 
            Perfect_Hash::in_word_set(headerInfo[randomList[i]].keyword,
                                      headerInfo[randomList[i]].len);
#else
         volatile register Headers::Type hdr = 
            Headers::getHeaderType(headerInfo[randomList[i]].keyword,
                                   headerInfo[randomList[i]].len);
#endif         
         counter++;
         i++;
         if (i >= nRandom) i = 0;
      }
      timer-=interval;
      elapsed+=interval;
      InfoLog(<< timer <<" sec rem: " 
              << counter << " hashes "
              << counter/elapsed << " h/sec");
      signalled=false;
      
   }
}
/*

         volatile register Headers::Type hdr = 
            Headers::getHeaderType(randomList[i].word,
                                   randomList[i].len);

*/
