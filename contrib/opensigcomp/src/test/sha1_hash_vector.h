#ifndef _SHA1_HASH_VECTOR_H
#define _SHA1_HASH_VECTOR_H 1

#include "Types.h"

typedef struct
{
  int length;
  char *message;
  char *hash;
}
sha1_vector_t;

extern sha1_vector_t sha1Vector[229];

#endif
