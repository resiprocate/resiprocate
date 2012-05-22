
/* Todo:
    - handle re-INVITE
    - share a socket?
    - test socket when app starts
    - release ports when object destroyed
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <exception>
#include <iostream>

#include "Logging.hxx"
#include "RtpProxyUtil.hxx"

#define STR2IOVEC(sx, ix)       {(ix).iov_base = (sx); (ix).iov_len = strlen((sx));}

#define RTPPROXY_RETRY_COUNT 3

using namespace b2bua;
using namespace std;

int RtpProxyUtil::umode = 0;
char *RtpProxyUtil::rtpproxy_sock = (char *)DEFAULT_RTPPROXY_SOCK;
int RtpProxyUtil::controlfd = 0;
char *RtpProxyUtil::timeout_sock = (char *)DEFAULT_RTPPROXY_TIMEOUT_SOCK;
int RtpProxyUtil::timeoutfd = 0;
int RtpProxyUtil::timeout_clientfd = -1;
int RtpProxyUtil::rtpproxy_retr = DEFAULT_RTPPROXY_RETR;
int RtpProxyUtil::rtpproxy_tout = DEFAULT_RTPPROXY_TOUT;

map<int, RtpProxyUtil *> RtpProxyUtil::proxies;

void RtpProxyUtil::setSocket(const char *socket) {
  if((rtpproxy_sock = (char *)malloc(strlen(socket) + 1)) == NULL) {
    B2BUA_LOG_ERR("setSocket: malloc failed");
    throw;
  }
  strcpy(rtpproxy_sock, socket);
}

void RtpProxyUtil::setTimeoutSocket(const char *socket) {
  if((timeout_sock = (char *)malloc(strlen(socket) + 1)) == NULL) {
    B2BUA_LOG_ERR("setSocket: malloc failed");
    throw;
  }
  strcpy(timeout_sock, socket);
}

void RtpProxyUtil::init() {
  umode = 0;
//  rtpproxy_sock = DEFAULT_RTPPROXY_SOCK;
//  timeout_sock = DEFAULT_RTPPROXY_TIMEOUT_SOCK;
  rtpproxy_retr = DEFAULT_RTPPROXY_RETR;
  rtpproxy_tout = DEFAULT_RTPPROXY_TOUT;

  int len;
  struct sockaddr_un local;

  int flags;

  if ((timeoutfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    B2BUA_LOG_ERR("socket: %m");
    exit(1); // FIXME
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, timeout_sock);
  unlink(local.sun_path);
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(timeoutfd, (struct sockaddr *)&local, len) == -1) {
    B2BUA_LOG_ERR("bind: %m");
    exit(1); // FIXME
  }

  if (listen(timeoutfd, 5) == -1) {
    B2BUA_LOG_ERR("listen: %m");
    exit(1); // FIXME
  } 

  flags = fcntl(timeoutfd, F_GETFL);
  flags |= O_NONBLOCK;
  fcntl(timeoutfd, F_SETFL, flags);

  timeout_clientfd = -1;

  B2BUA_LOG_NOTICE("telling rtpproxy to flush calls");
  // Check the version
  struct iovec v[2] = {{NULL, 0}, {(char *)"x", 1}};
  char *cp = sendCommandRetry(RTPPROXY_RETRY_COUNT, v, 2, "");
  if(cp == NULL)
    throw new exception;
}

void RtpProxyUtil::do_timeouts() {
  // check for new connections with accept
  // read any data with read
  socklen_t t;
  struct sockaddr_un remote;
  int flags;
  int n;
  char buf[100];
  int p1, p2;

  if(timeout_clientfd == -1) {
    t = sizeof(remote);
    if((timeout_clientfd = accept(timeoutfd, (struct sockaddr *)&remote, &t)) == -1) {
      if(errno == EAGAIN) {
        // no connections coming in from rtpproxy
        return;
      }
      B2BUA_LOG_ERR("accept: %m");
      exit(1);
    }
    B2BUA_LOG_DEBUG("accepted a new connection from rtpproxy");
    flags = fcntl(timeout_clientfd, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(timeout_clientfd, F_SETFL, flags);
  }
  
  n = recv(timeout_clientfd, buf, 100, 0);
  if (n == -1) {
    if (errno != EAGAIN) {
      // FIXME
      B2BUA_LOG_ERR("recv: %m");
      close(timeout_clientfd);
      timeout_clientfd = -1;
    } 
    return;
  }
  if(n== 0) {
    // n = 0 means that socket closed remotely
    timeout_clientfd = -1;
    return;
  }

  buf[n] = 0;
  if((n = sscanf(buf, "%d %d\n", &p1, &p2)) != 2) {
    B2BUA_LOG_WARNING("invalid number of arguments from rtpproxy_timeout client [%s]", buf);
  } else {
    B2BUA_LOG_DEBUG("timeout on ports %d %d", p1, p2);
    if(proxies.count(p1) == 1) {
      RtpProxyUtil *proxy = proxies.find(p1)->second;
      proxy->mediaTimeout();
    }
  }
}


RtpProxyUtil::RtpProxyUtil() {
  timeoutListener = NULL;
  valid = true;
  mypid = getpid();
  myseqn = 0;
  callID = NULL;
  callerAddr = NULL;
  callerPort = 0;
  calleeAddr = NULL;
  calleePort = 0;
  fromTag = NULL;
  toTag = NULL;

  callerProxyPort = 0;
  calleeProxyPort = 0;

  // Check the version
  struct iovec v[2] = {{NULL, 0}, {(char *)"V", 1}};
  char *cp = sendCommandRetry(RTPPROXY_RETRY_COUNT, v, 2, gencookie());
  if(cp == NULL)
    throw new exception;

}

RtpProxyUtil::~RtpProxyUtil() {

  if(callerProxyPort != 0)
    proxies.erase(callerProxyPort);
  if(calleeProxyPort != 0)
    proxies.erase(calleeProxyPort);

  struct iovec v[1 + 4 + 3] = {{NULL, 0}, {(char *)"D", 1}, {(char *)" ", 1}, {NULL, 0}, {(char *)" ", 1}, {NULL, 0}, {(char *)" ", 1}, {NULL, 0}};
  STR2IOVEC(callID, v[3]);
  STR2IOVEC(fromTag, v[5]);
  if(toTag != NULL)
    STR2IOVEC(toTag, v[7]);
  // ignore return value
  sendCommandRetry(RTPPROXY_RETRY_COUNT, v, (toTag != NULL) ? 8 : 6, gencookie());

  if(callID != NULL)
    free(callID);
  if(callerAddr != NULL)
    free(callerAddr);
  if(calleeAddr != NULL)
    free(calleeAddr);
  if(fromTag != NULL)
    free(fromTag);
  if(toTag != NULL)
    free(toTag);

}

void RtpProxyUtil::setTimeoutListener(TimeoutListener *timeoutListener) {
  this->timeoutListener = timeoutListener; 
}

void RtpProxyUtil::mediaTimeout() {
  valid = false;
  if(timeoutListener != NULL)
    timeoutListener->onMediaTimeout();
}

unsigned int RtpProxyUtil::setupCaller(const char *callID, const char *callerAddr, int callerPort, const char *fromTag, bool callerAsymmetric) {
  if(this->callID != NULL)
    free(this->callID);
  if((this->callID=(char *)malloc(strlen(callID) + 1))==NULL) {
    return 0;
  }
  if(this->callerAddr != NULL)
    free(this->callerAddr);
  if((this->callerAddr=(char *)malloc(strlen(callerAddr) + 1))==NULL) {
    return 0;
  }
  if(this->fromTag != NULL)
    free(this->fromTag);
  if((this->fromTag=(char *)malloc(strlen(fromTag) + 1))==NULL) {
    return 0;
  }
  strcpy(this->callID, callID);
  strcpy(this->callerAddr, callerAddr);
  this->callerPort = callerPort;
  strcpy(this->fromTag, fromTag);

  char buf[BUF_SIZE];
  struct iovec v[1 + 6 + 5] = {{NULL, 0}, {NULL, 0}, {(char *)" ", 1}, {NULL, 0},
    {(char *)" ", 1}, {NULL, 7}, {(char *)" ", 1}, {NULL, 1}, {(char *)" ", 1}, {NULL, 0},
    {(char *)" ", 1}, {NULL, 0}};
  if(callerAsymmetric == true)
    v[1].iov_base = (char *)"Ua";
  else
    v[1].iov_base = (char *)"Us";
  v[1].iov_len = 2;
  STR2IOVEC((char *)callID, v[3]);
  STR2IOVEC((char *)callerAddr, v[5]);
  sprintf(buf, "%d", callerPort);
  STR2IOVEC(buf, v[7]);
  STR2IOVEC((char *)fromTag, v[9]);
  // STR2IOVEC(toTag, v[11]);
  char *cp = sendCommandRetry(RTPPROXY_RETRY_COUNT, v, 10, gencookie());
  if(cp == NULL)
    throw new exception;
  callerProxyPort = atoi(cp);
  proxies[callerProxyPort] = this;
  return callerProxyPort;
}

void RtpProxyUtil::ammendCaller(const char *callerAddr, int callerPort) {
}

unsigned int RtpProxyUtil::setupCallee(const char *calleeAddr, int calleePort, const char *toTag, bool calleeAsymmetric) {
  if(this->calleeAddr != NULL)
    free(this->calleeAddr);
  if((this->calleeAddr=(char *)malloc(strlen(calleeAddr) + 1))==NULL) {
    return 0;
  }
  if(this->toTag != NULL)
    free(this->toTag);
  if((this->toTag=(char *)malloc(strlen(toTag) + 1))==NULL) {
    return 0;
  }  
  strcpy(this->calleeAddr, calleeAddr);
  this->calleePort = calleePort;
  strcpy(this->toTag, toTag);

  char buf[BUF_SIZE];
  struct iovec v[1 + 6 + 5] = {{NULL, 0}, {NULL, 0}, {(char *)" ", 1}, {NULL, 0},
    {(char *)" ", 1}, {NULL, 7}, {(char *)" ", 1}, {NULL, 1}, {(char *)" ", 1},
{NULL, 0},
    {(char *)" ", 1}, {NULL, 0}};
  if(calleeAsymmetric == true)
    v[1].iov_base = (char *)"La";
  else 
    v[1].iov_base = (char *)"Ls";
  v[1].iov_len = 2;
  STR2IOVEC(callID, v[3]);
  STR2IOVEC((char *)calleeAddr, v[5]);
  sprintf(buf, "%d", calleePort);
  STR2IOVEC(buf, v[7]);
  STR2IOVEC(fromTag, v[9]);
  STR2IOVEC((char *)toTag, v[11]);
  char *cp = sendCommandRetry(RTPPROXY_RETRY_COUNT, v, 12, gencookie());
  if(cp == NULL)
    throw new exception;
  calleeProxyPort = atoi(cp);
  proxies[calleeProxyPort] = this;
  return calleeProxyPort;
}

void RtpProxyUtil::ammendCallee(const char *calleeAddr, int calleePort) {
}

unsigned int RtpProxyUtil::getCallerProxyPort() {
  return callerProxyPort;
}

unsigned int RtpProxyUtil::getCalleeProxyPort() {
  return calleeProxyPort;
}

char *RtpProxyUtil::sendCommandRetry(int retries, struct iovec *v, int vcnt, char *my_cookie) {
  int c = 0;
  char *result;
  while(c++ < retries) {
    result = sendCommand(v, vcnt, my_cookie);
    if(result != NULL)
      return result; 
  }
  return NULL;
}

char *RtpProxyUtil::sendCommand(struct iovec *v, int vcnt, char *my_cookie) {

        struct sockaddr_un addr;
        int fd, i, len;
        char *cp;
        static char buf[256];
        struct pollfd fds[1];

        len = 0;
        cp = buf;
        if (umode == 0) {
                memset(&addr, 0, sizeof(addr));
                addr.sun_family = AF_LOCAL;
                strncpy(addr.sun_path, rtpproxy_sock,
                    sizeof(addr.sun_path) - 1);
#ifdef HAVE_SOCKADDR_SA_LEN
                addr.sun_len = strlen(addr.sun_path);
#endif

                fd = socket(AF_LOCAL, SOCK_STREAM, 0);
                if (fd < 0) {
			B2BUA_LOG_ERR("send_rtpp_command: can't create socket");
                        return NULL;
                }
                if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
                        close(fd);
			B2BUA_LOG_ERR("ERROR: send_rtpp_command: can't connect to RTP proxy: %s", addr.sun_path);
			//perror("RtpProxyUtil");
                        return NULL;
                }

                do {
                        len = writev(fd, v + 1, vcnt - 1);
                } while (len == -1 && errno == EINTR);
                if (len <= 0) {
                        close(fd);
			B2BUA_LOG_ERR("ERROR: send_rtpp_command: can't send command to a RTP proxy");
                        return NULL;
                }
                do {
                        len = read(fd, buf, sizeof(buf) - 1);
                } while (len == -1 && errno == EINTR);
                close(fd);
                if (len <= 0) {
			B2BUA_LOG_ERR("ERROR: send_rtpp_command: can't read reply from the RTP proxy, errno = %d", errno);
                        return NULL;
                }
        } else {
                fds[0].fd = controlfd;
                fds[0].events = POLLIN;
                fds[0].revents = 0;
                /* Drain input buffer */
                while ((poll(fds, 1, 0) == 1) &&
                    ((fds[0].revents & POLLIN) != 0)) {
                        recv(controlfd, buf, sizeof(buf) - 1, 0);
                        fds[0].revents = 0;
                }
                v[0].iov_base = my_cookie;
                v[0].iov_len = strlen((const char *)v[0].iov_base);
                for (i = 0; i < rtpproxy_retr; i++) {
                        do {
                                len = writev(controlfd, v, vcnt);
                        } while (len == -1 && (errno == EINTR || errno == ENOBUFS));
                        if (len <= 0) {
                                /* LOG(L_ERR, "ERROR: send_rtpp_command: "
                                    "can't send command to a RTP proxy\n"); */
				B2BUA_LOG_ERR("ERROR: send_rtpp_command: can't send command to a RTP proxy");
                                return NULL;
                        }
                        while ((poll(fds, 1, rtpproxy_tout * 1000) == 1) &&
                            (fds[0].revents & POLLIN) != 0) {
                                do {
                                        len = recv(controlfd, buf, sizeof(buf) - 1, 0);
                                } while (len == -1 && errno == EINTR);
                                if (len <= 0) {
                                        /* LOG(L_ERR, "ERROR: send_rtpp_command: "
                                            "can't read reply from a RTP proxy\n"); */
					B2BUA_LOG_ERR("ERROR: send_rtpp_command:can't read reply from a RTP proxy");
                                        return NULL;
                                }
                                if (len >= (v[0].iov_len - 1) &&
                                    memcmp(buf, v[0].iov_base, (v[0].iov_len - 1)) == 0) {
                                        len -= (v[0].iov_len - 1);
                                        cp += (v[0].iov_len - 1);
                                        if (len != 0) {
                                                len--;
                                                cp++;
                                        }
                                        goto out;
                                }
                                fds[0].revents = 0;
                        }
                }
                if (i == rtpproxy_retr) {
                        /* LOG(L_ERR, "ERROR: send_rtpp_command: "
                            "timeout waiting reply from a RTP proxy\n"); */
			B2BUA_LOG_ERR("ERROR: send_rtpp_command: timeout waiting reply from a RTP proxy");
                        return NULL;
                }
        }

out:
        cp[len] = '\0';
        return cp;  

}


char *RtpProxyUtil::gencookie() {
        static char cook[34];

        sprintf(cook, "%d_%u ", (int)mypid, myseqn);
        myseqn++;
        return cook;
}

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

