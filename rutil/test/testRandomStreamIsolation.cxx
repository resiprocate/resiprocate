#include <cstdlib>
#include <iostream>

#include "rutil/Data.hxx"
#include "rutil/Random.hxx"
#include "rutil/stun/Stun.hxx"

using namespace std;
using namespace resip;

// Values that have to be unpredictable must not be drawn from the same
// process-wide random() stream that also feeds tokens which are put on the
// wire.  MultipartMixedContents::setBoundary() emits Random::getRandomHex(8)
// -- raw output of that stream -- in every multipart body, so an observer can
// follow the stream.  Anything else taken from it is then predictable too.
//
// The test pins the stream with srandom() and checks three things:
//   1. control      -- a wire-visible draw really does repeat, so the stream is
//                      observable at all (otherwise the rest proves nothing)
//   2. control      -- getCryptoRandom() does not repeat, so srandom() has no
//                      hold on the CSPRNG
//   3. under test   -- stunRandomPort() must not repeat

#ifdef WIN32
int main(int, char**)
{
   // Random::getRandom() goes through RtlGenRandom on Windows
   // (RESIP_RANDOM_WIN32_RTL), so there is no seeded stream to pin here.
   cerr << "skipped on Windows" << endl;
   return 0;
}
#else

static const unsigned int SEED = 20260810;

struct Draws
{
   Data wireVisible;
   int  stunPort;
   Data crypto;
};

static Draws
drawAll()
{
   Draws d;
   srandom(SEED);
   d.wireVisible = Random::getRandomHex(8);  // what setBoundary() does
   d.stunPort    = stunRandomPort();
   d.crypto      = Random::getCryptoRandomHex(8);
   return d;
}

int main(int, char**)
{
   Random::initialize();

   const Draws a = drawAll();
   const Draws b = drawAll();

   int failures = 0;

   if (a.wireVisible != b.wireVisible)
   {
      cerr << "CONTROL FAILED: the wire-visible draw did not repeat under a "
           << "pinned seed (" << a.wireVisible << " vs " << b.wireVisible
           << "). The stream is not observable, so this test proves nothing."
           << endl;
      ++failures;
   }

   if (a.crypto == b.crypto)
   {
      cerr << "CONTROL FAILED: getCryptoRandomHex() repeated under a pinned "
           << "seed (" << a.crypto << "). srandom() must not reach the CSPRNG."
           << endl;
      ++failures;
   }

   if (a.stunPort == b.stunPort)
   {
      cerr << "FAILED: stunRandomPort() returned " << a.stunPort
           << " twice under a pinned seed. It is drawn from the same stream "
           << "that the wire-visible token " << a.wireVisible << " discloses."
           << endl;
      ++failures;
   }

   // The CSPRNG returns a full-width int and may be negative, where the old
   // random() path never was.  stunRandomPort() masks down to 15 bits, so the
   // documented range must still hold; check it rather than assume it.
   int lowest = 0x7FFF, highest = 0x4000;
   for (int i = 0; i < 100000; ++i)
   {
      const int port = stunRandomPort();
      if (port < 0x4000 || port > 0x7FFF)
      {
         cerr << "FAILED: stunRandomPort() returned " << port
              << ", outside the documented range 0x4000..0x7FFF" << endl;
         ++failures;
         break;
      }
      if (port < lowest)  lowest = port;
      if (port > highest) highest = port;
   }

   if (failures == 0)
   {
      cerr << "ok: stunRandomPort() is independent of the seeded stream "
           << "(wire-visible draw repeated as expected: " << a.wireVisible
           << "); range over 100000 draws: 0x" << hex << lowest << "..0x"
           << highest << dec << endl;
      return 0;
   }
   return -1;
}
#endif
