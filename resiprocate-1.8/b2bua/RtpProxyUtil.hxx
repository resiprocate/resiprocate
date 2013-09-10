
#ifndef __RtpProxyUtil_h
#define __RtpProxyUtil_h

#include <map>

namespace b2bua
{

#define DEFAULT_RTPPROXY_SOCK "/var/run/rtpproxy.sock"
#define DEFAULT_RTPPROXY_TIMEOUT_SOCK "/var/run/rtpproxy.timeout.sock"
#define DEFAULT_RTPPROXY_RETR 5
#define DEFAULT_RTPPROXY_TOUT 1

#define BUF_SIZE 250

class RtpProxyUtil {

public:

  class TimeoutListener {
  public:
    virtual ~TimeoutListener() {};
    virtual void onMediaTimeout() = 0;
  };


private:


  // Static variables for system wide settings
  static int umode;
  static char *rtpproxy_sock;
  static int controlfd;
  static char *timeout_sock;
  static int timeoutfd;
  static int timeout_clientfd;
  static int rtpproxy_retr;
  static int rtpproxy_tout;
  static std::map<int, RtpProxyUtil *> proxies;

  // Instance variables for per-instance data
  pid_t mypid;
  int myseqn;


  TimeoutListener *timeoutListener;

  bool valid;

  char *callID;
  char *callerAddr;
  unsigned int callerPort;
  char *calleeAddr;
  unsigned int calleePort;
  char *fromTag;
  char *toTag; 

  unsigned int callerProxyPort;
  unsigned int calleeProxyPort;

protected:
  static char *sendCommandRetry(int retries, struct iovec *v, int vcnt, char *my_cookie);
  static char *sendCommand(struct iovec *v, int vcnt, char *my_cookie);
  char *gencookie();

  // called when a media timeout occurs
  void mediaTimeout();

public:

  static void setSocket(const char *socket);
  static void setTimeoutSocket(const char *socket);
  static void init();
  // check for timeout data from socket
  static void do_timeouts();

  RtpProxyUtil();
  ~RtpProxyUtil();

  void setTimeoutListener(TimeoutListener *timeoutListener);

  // Inform rtpproxy of the caller's details
  unsigned int setupCaller(const char *callID, const char *callerAddr, int callerPort, const char *fromTag, bool callerAsymmetric);
  // ammend the caller's details (after the caller sends re-INVITE)
  void ammendCaller(const char *callerAddr, int callerPort);
  // Inform rtpproxy of the callee's details
  unsigned int setupCallee(const char *calleeAddr, int calleePort, const char *toTag, bool calleeAsymmetric);
  // ammend the callee's details
  void ammendCallee(const char *calleeAddr, int calleePort);

  unsigned int getCallerProxyPort();
  unsigned int getCalleeProxyPort();

};

}

#endif

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

