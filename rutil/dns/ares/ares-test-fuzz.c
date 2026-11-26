/* Code ported from c-ares test/ares-test-fuzz.c */
#include <stddef.h>

#include "ares.h"

#include "FuzzStrict.h"

// Entrypoint for Clang's libfuzzer
int LLVMFuzzerTestOneInput(const unsigned char *data,
                           size_t size) {
  // Feed the data into each of the ares_parse_*_reply functions.
  struct hostent *host = NULL;
  // ares takes an int length; fuzz inputs are far smaller than INT_MAX.
  int len = (int)size;
  int a = ares_parse_a_reply(data, len, &host);
  if (host) ares_free_hostent(host);

  host = NULL;
  unsigned char addrv4[4] = {0x10, 0x20, 0x30, 0x40};
  int ptr = ares_parse_ptr_reply(data, len, addrv4, sizeof(addrv4), AF_INET, &host);
  if (host) ares_free_hostent(host);

  // A seed is an A or a PTR reply, so strict mode only fails if neither parsed.
  if (a != ARES_SUCCESS && ptr != ARES_SUCCESS)
  {
    fuzzRejected("neither an A nor a PTR reply");
  }

  return 0;
}
