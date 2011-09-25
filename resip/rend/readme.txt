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

Troop - A troop is one instance of a sketch running.  For example, if 5 users are configured, then
        the Reg1 sketch will dispatch 5 troops, one for each user account.

Wave - A wave is a complete run of a particular sketch, from start to finish.

Command Line Options:

rendIt [optional-arguments] <sketch-name>      - where sketch-name can be "reg1" or "pres1"


C:\MyProjects\resip-main\resip\rend>SSL-Debug\rend --help pres1
INFO | 20110925-152609.285 |  | RESIP:DNS | 3244 | dnsutil.cxx:146 | local hostname does not contain a domain part spectrumq6600
Usage: [OPTION...]

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


Note:  At a minimum the following command line options MUST be specified:
  --localuri=STRING                                        Local URI
                                                           (transports)
                                                           (default: null)

  --acctdomain=DOM                                         Domain and realm
                                                           for accounts
                                                           (default: null)



Sample Command Line for testing Reg1 Sketch with 1 user:
rendIt --localuri=sip:192.168.1.106:5070;transport=tcp --acctnum=1 --acctbase=1000 --acctsuflen=4 --acctdomain=testdomain.com --proxy=sip:192.168.1.106:5060;transpor=tcp reg1