#!/bin/sh
rsync -avz ~/c/ICE 10.0.0.107:src ; rsync -avz ~/c/ICE 10.0.0.109:src
ssh 10.0.0.107 "cd /home/ekr/src/ICE/make/linux-fedora-test/; make"
ssh 10.0.0.109 "cd /home/ekr/src/ICE/make/linux-fedora-test/; make"
