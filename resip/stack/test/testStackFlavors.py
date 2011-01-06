#!/usr/bin/python

import os
import subprocess
import re
import platform
import datetime

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
    usingEpoll = pset['intepoll'] or pset['thread']=='event'
    if pset['intepoll'] and pset['thread']=='event':
	return False	# redundant
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

    resfile.close()


TheParamTbl = [
  RunParam('runs', 'num-runs', [50000]),
  RunParam('protocol', 'protocol', ['tcp','udp']),
  RunParam('intepoll', 'intepoll', [0,1]),
  RunParam('ports', 'numports', [1, 100, 10000]),
  RunParam('listen', 'listen', [1,0]),
  RunParam('thread', 'thread-type', ['common','intr','event']),
]

RunParamTbl(TheParamTbl)
