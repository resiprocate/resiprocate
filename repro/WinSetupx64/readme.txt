Repro Version:  Capuchin 0.2 - Feb, 2006

Main Web Site:
http://www.sipfoundry.org/repro/

For the latest documentation please see:  
http://wiki.resiprocate.org/wiki/index.php?title=Main_Page#Repro_SIP_Proxy_Server or
http://wiki.resiprocate.org/

repro --help
Usage: SSL-Debug\repro [OPTION...]
  -l, --log-type=syslog|cerr|cout                       where to send logging
                                                        messages (default:
                                                        "cout")
  -v, --log-level=STACK|DEBUG|INFO|WARNING|ALERT        specify the default
                                                        log level (default:
                                                        "INFO")
  -r, --record-route=sip:example.com                    specify uri to use as
                                                        Record-Route
  --udp=5060                                            listen on UDP port
                                                        (default: 5060)
  --tcp=5060                                            listen on TCP port
                                                        (default: 5060)
  -t, --tls-domain=example.com                          act as a TLS server
                                                        for specified domain
  --tls=5061                                            add TLS transport on
                                                        specified port
                                                        (default: 5061)
  --dtls=0                                              add DTLS transport on
                                                        specified port
                                                        (default: 0)
  --enable-cert-server                                  run a cert server
  -c, --cert-path=STRING                                path for certificates
                                                        (default: c:\sipCerts)
                                                        (default:
                                                        "C:\sipCerts")
  --enable-v6                                           enable IPV6
  --disable-v4                                          disable IPV4
  --disable-auth                                        disable DIGEST
                                                        challenges
  --disable-web-auth                                    disable HTTP challenges
  --disable-reg                                         disable registrar
  -i, --interfaces=sip:10.1.1.1:5065;transport=tls      specify interfaces to
                                                        add transports to
  -d, --domains=example.com,foo.com                     specify domains that
                                                        this proxy is
                                                        authorative
  -R, --route=sip:p1.example.com,sip:p2.example.com     specify where to route
                                                        requests that are in
                                                        this proxy's domain
  --reqChainName=STRING                                 name of request chain
                                                        (default: default)
  --http=5080                                           run HTTP server on
                                                        specified port
                                                        (default: 5080)
  --recursive-redirect                                  Handle 3xx responses
                                                        in the proxy
  --q-value                                             Enable sequential
                                                        q-value processing
  --q-value-behavior=STRING                             Specify forking
                                                        behavior for q-value
                                                        targets:
                                                        FULL_SEQUENTIAL,
                                                        EQUAL_Q_PARALLEL, or
                                                        FULL_PARALLEL
  --q-value-cancel-btw-fork-groups                      Whether to cancel
                                                        groups of parallel
                                                        forks after the period
                                                        specified by the
                                                        --q-value-ms-before-cancel 
                                                        parameter.
  --q-value-wait-for-terminate-btw-fork-groups          Whether to wait for
                                                        parallel fork groups
                                                        to terminate before
                                                        starting new
                                                        fork-groups.
  --q-value-ms-between-fork-groups=INT                  msec to wait before
                                                        starting new groups of
                                                        parallel forks
  --q-value-ms-before-cancel=INT                        msec to wait before
                                                        cancelling parallel
                                                        fork groups
  -e, --enum-suffix=e164.arpa                           specify enum suffix to
                                                        search
  -b, --allow-bad-reg                                   allow To tag in
                                                        registrations
  --timer-C=180                                         specify length of
                                                        timer C in sec (0 or
                                                        negative will disable
                                                        timer C)
  -a, --admin-password=                                 set web administrator
                                                        password
  -V, --version                                         show the version number

Help options:
  -?, --help                                            Show this help message
  --usage                                               Display brief usage
                                                        message
