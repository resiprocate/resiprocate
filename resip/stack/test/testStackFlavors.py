#!/usr/bin/python

# Linux-configuration
# Some of the test flavors require many open ports, and your system must be
# configured to enabled this. In particular:
# * This script opens up more than 1024 ports which means it needs to run
#   as root. Thus do:
#	sudo ./testStackFlavors.py
# * Ports 11000 thru 31000 will be bound. Ports in this range must not be
#   bound (kill any servers you have running in this range). Further,
#   client TCP connections must not be configured to run in this range.
#   This is configured via
#	/proc/sys/net/ipv4/ip_local_port_range
#   The default is "32678 61000" which is good. The left number must be
#   larger than 31000.
# * TCP connections will enter TIME_WAIT when each test completes,
#   which may not leave enough ports for the next test.
#   The script has a 5sec delay between tests that might help with this.
#   You might need to tweak one of the following to work:
#	/proc/sys/net/ipv4/tcp_tw_recycle
#	/proc/sys/net/ipv4/tcp_tw_reuse
#	/proc/sys/net/ipv4/tcp_fin_timeout
#   I don't claim to fully understand this.

import os
import subprocess
import re
import platform
import datetime
import time

def TryUnlink(path):
    try:
	os.unlink(path)
    except OSError:
        pass
    return


def RunTest(tag, testArgs):
    cmdExec = "./testStack"
    cmdFull = cmdExec + " " + testArgs
    TryUnlink("./gmon.out")
    # XXX: perhaps should close stdin
    p = subprocess.Popen(cmdFull,shell=True,close_fds=True,stdout=subprocess.PIPE)
    result = None
    while p.stdout:
	line = p.stdout.readline()
	if len(line)==0:
	    break
        print "%s: %s" % (tag, line.rstrip())
	mo = re.match(r'(\d+).* performed in (\d+).* ms.*of (\d+).* per second',line)
	if mo:
	    result = {}
	    result['runs'] = int(mo.group(1))
	    result['dur'] = int(mo.group(2))
	    result['tps'] = int(mo.group(3))
	    # XXX: for calls, need to convert cps into tps!

    p.wait()

    if p.returncode!=0:
	print "%s: test failed (sts %d)!" % (tag, p.returncode)
	print "%s: command was:\n---------\n%s\n---------" % (tag, cmdFull)
	raise ValueError # fix this

    if result is None:
	print "%s: failed to find transaction rate in test output!"
	raise ValueError # fix this

    print "%s: runs=%d dur=%dms rate=%dtps" % (tag,
	     result['runs'], result['dur'], result['tps'])
    return result

class RunParam:
    def __init__(self, paramName, optionName, optionValues):
	self.paramName = paramName
	self.optionName = optionName
	# assert optionValue is list
	self.optionValues = optionValues

# Compute and return a runSet
# A runSet is list of paramSets
# A paramSet is a hash. The hashKey is the paramName, and the hash value
# is the specific value for this paramSet.

def CalcRunSet(ptbl):
    restRunSet = []
    if len(ptbl)==0:
	return restRunSet
    if len(ptbl)>1:
        restRunSet = CalcRunSet(ptbl[1:])
    if len(restRunSet)==0:
	restRunSet = [{}]
    doParam = ptbl[0]
    #print "CalcRunSet: expanding param %s" % (doParam.paramName)
    runSet = []
    for optVal in doParam.optionValues:
        #print "CalcRunSet: add param %s = %s" % (doParam.paramName, optVal)
	for restParamSet in restRunSet:
	    paramSet = restParamSet.copy()
	    paramSet[doParam.paramName] = optVal
	    runSet.append(paramSet)
    return runSet

def CalcParamStr(ptbl, pset):
   popts = ["--%s=%s" % (pdef.optionName, pset[pdef.paramName]) for pdef in ptbl]
   return ' '.join(popts)

def CalcParamCsv(ptbl, pset):
   # TBD: excape and commas
   popts = ["%s=,%s" % (pdef.paramName, pset[pdef.paramName]) for pdef in ptbl]
   return ','.join(popts)

def IsParamSetValid(pset):
    usingEpoll = pset['intepoll'] or pset['thread']=='epoll'
    if pset['intepoll'] and (pset['thread'] in ('epoll','fdset')):
	return False	# redundant/silly
    if pset['ports'] > 100 and not usingEpoll:
	return False
    if pset['listen']==0 and pset['protocol']=='udp':
        return False
    if usingEpoll and platform.system()=='Windows':
	return False
    return True

def RunParamTbl(ptbl):
    runSet = CalcRunSet(ptbl)
    #print runSet

    resfile = open('testStack.results.csv', 'a')

    platdesc = '-'.join(
	    [platform.system(),platform.release(),platform.machine()])
    platnode = platform.node()

    runCnt = 0
    for pset in runSet:
	runCnt += 1
	pstr = CalcParamStr(ptbl, pset)
	if not IsParamSetValid(pset):
	    print "skipping pstr: %s" % pstr
	    continue
	print "running pstr: %s" % pstr
	result = RunTest(runCnt, "--bind=127.0.0.1 " + pstr)
	assert( result['runs'] == pset['runs'] )
	now = datetime.datetime.now()
	nowfmt = now.strftime("%Y%m%d.%H%M%S")
	resstr = "tag=,%s,date=,%s,plat=,%s,node=,%s," % (runCnt,nowfmt,platdesc,platnode)
	resstr += CalcParamCsv(ptbl, pset)
	resstr += ",dur=,%d,tps=,%d" % (result['dur'], result['tps'])
	resstr += "\n"
	resfile.write(resstr)
	# TCP ports just entered TIME_WAIT. That may create problems
	# for next test if it tries to bind one of those ports
	# and/or needs more client ports. Try waiting short time
	# See comments at top re system configuration.
	time.sleep(5)

    resfile.close()


TheParamTbl = [
  RunParam('runs', 'num-runs', [50000]),
  RunParam('protocol', 'protocol', ['tcp','udp']),
  RunParam('intepoll', 'intepoll', [0,1]),
  RunParam('ports', 'numports', [1, 100, 10000]),
  RunParam('listen', 'listen', [1,0]),
  RunParam('thread', 'thread-type', ['common','intr','epoll','fdset']),
]

RunParamTbl(TheParamTbl)
