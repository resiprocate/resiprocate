
The Kurento integration is currently a work in progress

Kurento can be installed using packages or using Docker.
We have currently tested this using Docker.

To install Docker on RHEL8 or derivative OS such as
Rocky Linux or Alma Linux (successors to CentOS):

  Download your preferred OS installation ISO:
    Rocky Linux:   https://rockylinux.org/download
    AlmaLinux:     https://almalinux.org/

  Install using default options.

  sudo dnf config-manager --add-repo=https://download.docker.com/linux/centos/docker-ce.repo

  sudo dnf install docker-ce
  systemctl is-active docker
  sudo systemctl enable --now docker
  systemctl is-active docker
  systemctl is-enabled docker
  sudo usermod -aG docker $USER

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

