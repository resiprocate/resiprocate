Rend - Registration and Presence Load Test Tool

Rend Defined:  To tear or split apart or into pieces violently.

Overview:
Rend is tool that is was created specifically to load test SIP based presence servers.  
Existing tools such as SIPP are not suitable for such testing, since the dialog state tracking 
and SIP messaging required to effectively load test presence is too complex.  Rend is
architected in a manner where is it realively straight forward for a developer to program in 
new test scenarios (sketches), and still take advantage of the SIP account management and
rate control features that Rend implements.

Rend has support for simulating "Outbound" RFC5626 enabled clients, and is capable of connecting
to a proxy / presence server over UDP, TCP, or TLS and sending keep alive messaging on these
transports.

Terms Used in Rend:
Sketch - a particular test pattern or message flow.  There are currently two test sketches built
         into rend:
         Reg1 - a simple registration and refresh load test scenario.
         Pres1 - a presence load test scenario

Troop - A collection of users all doing the same thing. All users in a troop may not be doing things in 
        exact lock step, but close. For example, in the simple register schetch, you might have 100 users 
		in the troop. The entire troop will enter the "REGISTER" phase, but they cannot all send REGISTERs
		at the same time. They will all send REGISTERs following each other subject to the rate limit 
		constraints. The idea is that all users of a troop share a bunch of common state.  

Wave - The concept of a wave evolved over time. The basic idea is that a sketch does a for(;;) { wave() }. 
       That ignores the boundary cases and such, but the idea is that a wave is the work within the loop 
	   of the sketch. In the case of presence, there are multiple (sequential) waves in the loop. The wave 
	   is the current working set of stuff to do. So for example, for presence, we select the set of users 
	   we want to "test" on a give wave, where test means send a PUB and expect to get a NOTIFY on 
	   subscribed accounts. Those users selected into the test are part of the wave.  Statistics are 
	   aggregated over a wave. E.g., how many lost messsages in a wave, what was the average response time 
	   in a wave, etc.

		
Developer Notes:
		
Troop - With the existing sketches, there is 1-to-1 relationship between troop type and troop instance. So 
        there is a troop type (C++ class) for registration. And "reg1" has single instance of it.  But one 
		could image having two instances, each with different pools of users, each with different "rates". 
		E.g., to simulate one pool of users that hold live registrations for long periods, and another pool 
		that pops in and out of the network frequently. Or one pool of "normal" users and then another pool 
		"bad" users that don't disconnect cleanly.

Sketch - A sketch can have multiple troop instances. An example would be the above case of having two distinct 
         registration pools. The pres1 sketch has two troop instances: one for PUBs and one for SUBs. In that 
		 case, it is tied to needing two different troop types, because the troop type defines the behavior 
		 of the troop (sending PUBs vs sending SUBs).

Wave - The wave serves two purposes: from programming perspective, it is the current working set: when the 
       wave is empty, we can move on. From user perspective, it counts how many iterations we've completed. 

	   
Command Line Options:

rendIt [optional-arguments] <sketch-name>      - where sketch-name can be "reg1" or "pres1"

Runner options
  --log-type=cout|cerr|file|syslog                         Where to send log
                                                           message (default:
                                                           "file")
  --log-level=DEBUG|INFO|NOTICE|WARNING|ERR|ALERT          Log level (default:
                                                           "WARNING")
  --stacklog-level=DEBUG|INFO|NOTICE|WARNING|ERR|ALERT     SIP Stack Log level
                                                           (default: "WARNING")
  --log-file=STRING                                        Name of file for
                                                           log-type=file
                                                           (default:
                                                           "rend.log")
  --numfds=INT                                             Number of fds to
                                                           ask for (setrlimit)
                                                           (default: 0)
  --anyaddrport=INT                                        Create transports
                                                           on this port
                                                           (0.0.0.0) (default:
                                                           0)
  --sketchthread=INT                                       Run sketch thread
                                                           in own process
                                                           (default: 0)
  --sketchtick=INT                                         Polling interval
                                                           (us) for sketch
                                                           (default: 100000)
  --tlsrootcert=STRING                                     Filename of root
                                                           certificate for TLS
                                                           (default: null)
  --stackevent=STRING                                      Type of event loop
                                                           for SipStack thread
                                                           (default: "event")

Account options
  --acctnum=NUM                                            Number of
                                                           sequential accounts
                                                           (default: 10)
  --acctbase=BASE                                          Starting account
                                                           index (default: 0)
  --acctuserpre=PRE                                        Prefix for account
                                                           user names
                                                           (default: null)
  --acctusersuf=SUF                                        Suffix for account
                                                           user names
                                                           (default: null)
  --acctpass=PASS                                          Prefix for account
                                                           passwords (default:
                                                           null)
  --acctsuflen=LEN                                         Length of numeric
                                                           middle portion
                                                           (default: 6)
  --acctdomain=DOM                                         Domain and realm
                                                           for accounts
                                                           (default: null)

Transaction user options
  --proxy=STRING                                           Outbound proxy URI
                                                           (default: null)
  --localuri=STRING                                        Local URI
                                                           (transports)
                                                           (default: null)
  --localports=INT                                         Number of local
                                                           ports (transports)
                                                           (default: 1)
  --localbind=INT                                          Bind local ports
                                                           (default: 1)
  --kasecs=INT                                             Keep alive interval
                                                           (secs) (default: 20)

Help options:
  -?, --help                                               Show this help
                                                           message
  --usage                                                  Display brief usage
                                                           message



Sketch Specific Command Line Options:

Reg1Options
  --regacctbase=INT                                        First account index
                                                           (default: 0)
  --regacctlen=INT                                         Number of accounts
                                                           (default: 0)
  --regrepeatbase=INT                                      Repeat base
                                                           (default: 1)
  --regrepeatlen=INT                                       Repeat factor
                                                           (default: 1)
  --regexpire=INT                                          Expires (default:
                                                           600)
  --level=INT                                              Target number of
                                                           open REGISTER
                                                           dialogs (default: 1)
  -r, --minrate=FLOAT                                      Target minimum
                                                           change REGISTER
                                                           rate (default: 0)
  -R, --maxrate=FLOAT                                      Target maximum
                                                           change REGISTER
                                                           rate (default: 1)
  --maxpend=INT                                            Maximum count of
                                                           pending work
                                                           (default: 5)


Pres1Options
  --pubrepeat=INT                                          PUBLISH repeat
                                                           factor (default: 1)
  --subrepeat=INT                                          SUBSCRIBE repeat
                                                           factor (default: 1)
  --level=INT                                              Target number of
                                                           open PUBLISH
                                                           dialogs (default: 1)
  --pubexpire=INT                                          PUBLISH Expires
                                                           (default: 600)
  --subexpire=INT                                          SUBSCRIBE Expires
                                                           (default: 1200)
  -r, --minrate=FLOAT                                      Target minimum
                                                           change rate
                                                           (default: 0)
  -R, --maxrate=FLOAT                                      Target maximum
                                                           change rate
                                                           (default: 1)
  --maxpend=INT                                            Maximum count of
                                                           pending work
                                                           (default: 10)
  --maxage=INT                                             Maximum age (secs)
                                                           of pending work
                                                           (default: 8)
  --failage=INT                                            Age (secs) at which
                                                           to give up on
                                                           request/notify
                                                           (default: 35)
  --maxsubperpub=INT                                       Maximum SUB per
                                                           presentity
                                                           (default: 0)


WARNING:  At a minimum the following command line options MUST be specified:
  --localuri=STRING                                        Local URI
                                                           (transports)
                                                           (default: null)

  --acctdomain=DOM                                         Domain and realm
                                                           for accounts
                                                           (default: null)



Sample Command Line for testing Reg1 Sketch with 1 user:
rendIt --localuri=sip:192.168.1.106:5070;transport=tcp --acctnum=1 --acctbase=1000 --acctsuflen=4 --acctdomain=testdomain.com --proxy=sip:192.168.1.106:5060;transpor=tcp reg1