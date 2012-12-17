/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

#include <iostream>

#ifdef USE_POSIX_LOCKING
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <pthread.h>

#include "Types.h"
#include "SigcompMessage.h"
#include "DeflateCompressor.h"
#include "StateHandler.h"
#include "StateChanges.h"
#include "Stack.h"
#include "Compartment.h"

#define MAXTHREADS 500

static bool running;

static osc::StateHandler *sh1;
static osc::StateHandler *sh2;

void fillBuffer(osc::byte_t *buffer, size_t size)
{
  size_t i;
  osc::u32 *b = reinterpret_cast<osc::u32 *>(buffer);
  
  for (i=0; i < (size/4); i++)
  {
    b[i] = rand();
  }
}

////////////////////////////////////////////////////////////////////////
struct cid_t
{
  int threadNum;
  long long sequence;
};


static void *threadBody(void *arg)
{
  osc::Stack s1(*sh1);
  s1.addCompressor(new osc::DeflateCompressor(*sh1));

  osc::Stack s2(*sh2);
  s2.addCompressor(new osc::DeflateCompressor(*sh2));

  osc::SigcompMessage *sm;
  osc::StateChanges *sc;

  osc::byte_t buffer[2048];

  cid_t compartmentId;

  int i;

  compartmentId.threadNum = ((int)arg);
  compartmentId.sequence = 0;

  while (running)
  {
    compartmentId.sequence++;
    for (i = 0; i < 4; i++)
    {
      fillBuffer(buffer, sizeof(buffer));
      sm = s1.compressMessage(buffer, sizeof(buffer), compartmentId);
      s2.uncompressMessage(sm->getDatagramMessage(), sm->getDatagramLength(),
                           buffer, sizeof(buffer), sc);
      s2.provideCompartmentId(sc, compartmentId);
      delete sm;

      fillBuffer(buffer, sizeof(buffer));
      sm = s2.compressMessage(buffer, sizeof(buffer), compartmentId);
      s1.uncompressMessage(sm->getDatagramMessage(), sm->getDatagramLength(),
                           buffer, sizeof(buffer), sc);
      s1.provideCompartmentId(sc, compartmentId);
      delete sm;
      usleep(10);
    }
  }
  return 0;
}

////////////////////////////////////////////////////////////////////////

int
main(int argc, char **argv)
{
  pthread_t threads[MAXTHREADS];
  int i;
  int status;
  int duration = 20;
  int pruneTime = 2;
  int numThreads = 3;

  std::cout << "Usage: " << argv[0] 
            << " [test_length] [purge_time] [num_threads]" << std::endl;

  if (argc > 1)
  {
    duration = atoi(argv[1]);
  }

  if (argc > 2)
  {
    pruneTime = atoi(argv[2]);
  }

  if (argc > 3)
  {
    numThreads = atoi(argv[3]);
    if (numThreads > MAXTHREADS)
    {
      numThreads = MAXTHREADS;
    }
  }

  running = true;
  //----------------------------------------------------------------------
  sh1 = new osc::StateHandler(8192, 64, 8192, 2);
  sh2 = new osc::StateHandler(8192, 64, 8192, 2);
  //----------------------------------------------------------------------

  std::cout << "Spinning up " << numThreads << " threads..." << std::endl;
  for (i = 0; i < numThreads; i++)
  {
    status = pthread_create(&threads[i], 0, threadBody, (void *)i);
    if (status != 0)
    {
      std::cout << "Thread " << i << " failed to start. Aborting." << std::endl;
      exit (-1);
    }
  }
  std::cout << "Done." << std::endl;
  //----------------------------------------------------------------------

  std::cout << "Running for " << duration << " seconds..." << std::endl;

  while (duration > 0)
  {
    if (duration > pruneTime)
    {
      sleep (pruneTime);
      duration -= pruneTime;
      std::cout << std::endl;
      std::cout << "sh1 has " << sh1->numberOfCompartments()
                << " compartments" << std::endl;
      std::cout << "sh1 has " << sh1->numberOfNacks()
                << " nacks" << std::endl;
      std::cout << "sh2 has " << sh2->numberOfCompartments()
                << " compartments" << std::endl;
      std::cout << "sh2 has " << sh2->numberOfNacks()
                << " nacks" << std::endl;
      std::cout << "Removing stale compartments (" << duration 
                << " seconds left)..." << std::flush;
      sh1->removeStaleCompartments();
      sh2->removeStaleCompartments();
      std::cout << " done" << std::endl;
      std::cout << "sh1 has " << sh1->numberOfCompartments()
                << " compartments" << std::endl;
      std::cout << "sh2 has " << sh2->numberOfCompartments()
                << " compartments" << std::endl;
#ifdef LEAK_DEBUG
  std::cout << "Compartments left in memory: " 
            << osc::Compartment::dInstances << std::endl;
#endif
      std::cout << std::endl;
    }
    else
    {
      sleep (duration);
      duration = 0;
    }
  }

  //----------------------------------------------------------------------
  std::cout << "Shutting down " << numThreads << " threads..." << std::endl;
  running = false;
  void *result;
  for (i = 0; i < numThreads; i++)
  {
    status = pthread_join(threads[i], &result);
    if (status != 0)
    {
      std::cout << "Thread " << i << " failed to join. Aborting." << std::endl;
      exit (-1);
    }
  }
  std::cout << "Done." << std::endl;

  std::cout << "Clearing out all compartments (double stale)..." << std::endl;
  sh1->removeStaleCompartments();
  sh2->removeStaleCompartments();
  sh1->removeStaleCompartments();
  sh2->removeStaleCompartments();
  std::cout << "sh1 has " << sh1->numberOfCompartments()
            << " compartments" << std::endl;
  std::cout << "sh1 has " << sh1->numberOfNacks()
            << " nacks" << std::endl;
  std::cout << "sh2 has " << sh2->numberOfCompartments()
            << " compartments" << std::endl;
  std::cout << "sh2 has " << sh2->numberOfNacks()
            << " nacks" << std::endl;

#ifdef LEAK_DEBUG
  std::cout << "Compartments left in memory: " 
            << osc::Compartment::dInstances << std::endl;
#endif

  std::cout << "Destroying state handlers..." << std::endl;
  delete sh1;
  delete sh2;
  std::cout << "Done." << std::endl;

#ifdef LEAK_DEBUG
  std::cout << "Compartments left in memory: " 
            << osc::Compartment::dInstances << std::endl;
#endif

  exit (0);
}
#else // no locking

int
main(int argc, char **argv)
{
  std::cout << "This test requires locking constructs." << std::endl;
  return -1;
}
#endif
