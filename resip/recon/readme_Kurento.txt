
The Kurento integration is currently a work in progress

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

