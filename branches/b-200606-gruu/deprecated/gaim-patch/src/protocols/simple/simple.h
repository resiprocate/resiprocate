#ifndef _GAIM_SIMPLE_H_
#define _GAIM_SIMPLE_H_

#include <glib.h>
#include "connection.h"
#include "sslconn.h"

struct simple_connection_cache {
  GHashTable *friends;
};

struct simple_friend_cache {
  char *status_string; 
  int status_int;
};

static struct simple_friend_cache *simple_friend_cache_new();

static void simple_friend_cache_free(gpointer);
	
#endif /* _GAIM_SIMPLE_H_ */
