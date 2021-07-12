
The Kurento integration is currently a work in progress

Kurento can be installed using packages or using Docker.
We have currently tested this using Docker.

To install Docker on RHEL8 or derivative OS such as
Rocky Linux or Alma Linux (successors to CentOS):

  Download your preferred OS installation ISO:
    Rocky Linux:   https://rockylinux.org/download
    AlmaLinux:     https://almalinux.org/

  Install using default options.

Ensure you have sudo permissions

  On RHEL and similar hosts, login as root on the console
  and run this command with your username:

    usermod -aG wheel daniel

Enable powertools (for *-devel packages)

  sudo dnf -y install dnf-plugins-core
  sudo dnf config-manager --set-enabled powertools

Enable EPEL

  sudo dnf install epel-release

Install reSIProcate dependencies from RHEL8 and EPEL

  sudo dnf install libtool automake autoconf \
                   python3-devel python3-pycxx-devel \
                   libdb-cxx libdb-cxx-devel \
                   cppunit cppunit-devel \
                   gperf \
                   radcli-devel \
                   c-ares-devel \
                   boost-devel \
                   openssl-devel \
                   mariadb-connector-c-devel \
                   pcre-devel \
                   popt-devel \
                   postgresql-devel \
                   xerces-c-devel \
                   net-snmp-devel \
                   qpid-proton-cpp-devel \
                   soci-devel soci-postgresql-devel

Install dependencies built manually for RHEL8/EPEL

  cd ~/rpmbuild
  sudo rpm -i x86_64/asio-devel-1.16.1-3.el8.x86_64.rpm
  sudo rpm -i noarch/cajun-jsonapi-devel-2.0.3-13.el8.noarch.rpm

You can install Kurento using a Docker image or you can setup
another host running Ubuntu for the Kurento native packages.  Some
people have had problems with UDP packets and Docker and using
the packages instead of Docker may resolve this.  We tested
Ubuntu bionic 18.04 LTS downloaded from here:

  https://releases.ubuntu.com/18.04/

  ISO download: ubuntu-18.04.5-live-server-amd64.iso

and then we used these instructions to install the packages in
the Ubuntu host:

  https://doc-kurento.readthedocs.io/en/stable/user/installation.html#local-installation

In some situations it is recommended to tell Kurento to listen
to a specific network interface, this makes the discovery of
valid IP addresses faster and more reliable:

  vi /etc/kurento/modules/kurento/WebRtcEndpoint.conf.ini

    networkInterfaces=ens3
    externalIPv4=1.2.3.4

Install Docker (for Podman, alternative to Docker, see below)

  sudo dnf config-manager --add-repo=https://download.docker.com/linux/centos/docker-ce.repo

  sudo dnf install docker-ce
  systemctl is-active docker
  sudo systemctl enable --now docker
  systemctl is-active docker
  systemctl is-enabled docker
  sudo usermod -aG docker $USER

Installing Podman (RHEL alternative to Docker):

  sudo dnf install podman
  alias docker=podman

  (and use podman instead of docker in subsequent commands,
   select the docker.io images when prompted...)

Install the Kurento Docker image:

  docker pull kurento/kurento-media-server:latest

Running the Kurento Docker image:

  docker run -d --name kms --network host \
     -e KMS_STUN_IP=${STUN_SERVER} -e KMS_STUN_PORT=${STUN_PORT} \
     -e KMS_TURN_URL=user:password@host:port \
     kurento/kurento-media-server:latest

You may also want to consider adding this argument:

     -e KMS_EXTERNAL_IPV4=${YOUR KMS SERVER PUBLIC IP}

to avoid relying on STUN and TURN servers.

Monitoring logs from Kurento on Ubuntu:

  See the logging parameters in the file:
    /etc/default/kurento-media-server

  The default logging levels are selected for production use and
  don't provide sufficient information for problems encountered
  in development.

  For example, to log absolutely everything, comment out other GST_DEBUG
  lines and use the following but beware that a single media stream
  generates so much logging that it can create latency for the media
  stream.  WebRTC ICE negotiation may fail or produce unpredictable
  results due to packets delayed by latency.

    export GST_DEBUG="7"

  If you do require the most intense level of logging then please consider
  logging to a ramdisk (tmpfs) and other strategies.

  A more practical GST_DEBUG setting for logging in a development environment:

    export GST_DEBUG="4,Kurento*:4,kms*:4,sdp*:4,webrtc*:4,*rtpendpoint:4,rtp*handler:4,rtpsynchronizer:4,agnosticbin:4,kmsiceniceagent:6,KurentoWebSocketTransport:5,kmssdpsession:5"

  This helps us to see the communications between Kurento and reConServer,
  the key stages of call setup and ICE negotiation.

  To find the log files:

    cd /var/log/kurento-media-server
    ls

  To search for the JSON messages between reSIProcate and Kurento:

    egrep 'processMessage|sendEvent()' kurento-pidxxx.log
    egrep 'jsonrpc"' kurento-pidxxx.log

  To search for WebRTC ICE activity:

    egrep -i 'kmsicenice' kurento-pidxxx.log

Monitoring logs from Kurento Docker image:

  docker logs --follow kms

Using SSH to tunnel from dev workstation to a Kurento server:

  ssh -L 8888:127.0.0.1:8888 ${KURENTO_HOST}

Monitoring connections from reSIProcate to Kurento:

  sudo -i tcpdump -i any -n host 127.0.0.1 and port 8888

Check if Kurento has opened ports for the peer:

  sudo netstat -nlp | grep kurento

Run reConServer in gdb:

  libtool --mode=execute gdb --args \
      apps/reConServer/reConServer \
      apps/reConServer/reConServer.config.test-local
  run
