
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
     kurento/kurento-media-server:latest

Monitoring logs from Kurento Docker image:

  docker logs --follow kms

Using SSH to tunnel from dev workstation to a Kurento server:

  ssh -L 8888:127.0.0.1:8888 ${KURENTO_HOST}

Monitoring connections from reSIProcate to Kurento:

  sudo -i tcpdump -i any -n host 127.0.0.1 and port 8888

Check if Kurento has opened ports for the peer:

  sudo netstat -nlp | grep kurento

Run reConServer in gdb:

  libtool --mode=execute gdb apps/reConServer/reConServer
  set args apps/reConServer/reConServer.config.test-local
  run

