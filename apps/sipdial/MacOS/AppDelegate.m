/*
  Copyright (C) 2007 Marcus Mueller and Helge Hess

  This file is part of reSIProcate

*/

#include "AppDelegate.h"
#import <AppKit/NSApplication.h>

@implementation AppDelegate

- (void)_registerDefaults {
  NSDictionary *bundleInfo;
  NSString     *bundleInfoPath;
  
  bundleInfoPath = [[NSBundle mainBundle]
    pathForResource:[[NSBundle mainBundle] bundleIdentifier]
    ofType:@"plist"];
  
  bundleInfo = [NSDictionary dictionaryWithContentsOfFile:bundleInfoPath];
  
  [[NSUserDefaults standardUserDefaults] registerDefaults:bundleInfo];
}

- (void)_registerForAppleEvents {
  [[NSAppleEventManager sharedAppleEventManager]
      setEventHandler:self andSelector:@selector(handleURLEvent:withReplyEvent:)
      forEventClass:kInternetEventClass
      andEventID:kAEGetURL];
}

- (void)applicationWillFinishLaunching:(NSNotification *) notification {
  [self _registerDefaults];
  [self _registerForAppleEvents];
  
  /* quit the application now and then */
  
  [NSTimer scheduledTimerWithTimeInterval:(60.0 * 5.0) // every five minutes
	   target:self selector:@selector(quit:)
	   userInfo:nil repeats:NO];
}

- (void)quit:(NSTimer *)_timer {
  [[NSApplication sharedApplication] terminate:nil];
}

- (NSString *)toolPath {
  NSString *toolPath;
  
  toolPath = [[NSBundle bundleForClass:[self class]]
			pathForResource:@"sipdialer" ofType:nil];
  if ([toolPath length] > 3)
    return toolPath;
  
  toolPath = @"/usr/local/bin/sipdialer";
  if ([[NSFileManager defaultManager] isExecutableFileAtPath:toolPath])
    return toolPath;
    
  NSLog(@"ERROR: did not find sipdialer tool inside the bundle nor "
        @"in /usr/local/bin!");
  return nil;
}

/* AppleEvents glue code */

- (void)handleURLEvent:(NSAppleEventDescriptor *) event
  withReplyEvent:(NSAppleEventDescriptor *) replyEvent
{
  NSURL *url = [NSURL URLWithString:[[event descriptorAtIndex:1] stringValue]];
  [self performActionForURL:url];
}

/* the actual method to invoke the sipdialer */

- (void)performActionForURL:(NSURL *)_url {
  NSTask   *task;
  NSArray  *args;
  NSString *toolPath;
  
  if ((toolPath = [self toolPath]) == nil)
    return;
  
  args = [NSArray arrayWithObject:[_url absoluteString]];
  NSLog(@"invoking sipdialer with arguments: %@", args);
  
  task = [[NSTask alloc] init];
  [task setLaunchPath:toolPath];
  [task setArguments:args];
  [task launch];
  
  /*
    Somehow the sipdialer does not properly shut down, so we just wait a bit
    and then kill it. Possibly we need to reset stdio to /dev/null or something?
    (setting stdio to nil raises an exception)
    (and no, waitFor... doesn't solve it ;-)
  */
  
  sleep(2);
  
  if ([task isRunning]) {
    NSLog(@"sipdialer still running after 2s, sending terminate ...");
    [task terminate];
  }
  [task release]; task = nil;
  
  /*
    Enable this to quit the application after each request during development.
    Otherwise it continues running in the background.
  */
  //exit(0);
}

@end /* AppDelegate */

/* ====================================================================
 * 
 * Copyright 2007 Marcus Mueller and Helge Hess.  All rights reserved.
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

