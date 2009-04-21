#ifndef ARES__SOCKETFUNC_H
#define ARES__SOCKETFUNC_H


#ifdef WIN32
#include <winsock2.h>
#endif

#ifndef WIN32
typedef int Socket;
#else
typedef SOCKET Socket;
#endif

typedef void(*socket_function_ptr)(Socket s, int transportType, const char* file, int line);

#endif

