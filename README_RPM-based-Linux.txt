
We try to make packages for users of Fedora, RHEL, CentOS, Rocky Linux,
derivatives and similar systems that use the RPM packaging system.

To support this effort, we also try to ensure all dependencies are
available in Fedora, EPEL or a reliable third-party repository.

This document will help you if you wish to do one of these things:

a) building the source code in a local workspace for development, OR

b) building the RPMs yourself, the same way that we build official RPMs


To get started, download your preferred distribution
----------------------------------------------------

  Download the installation ISO, Fedora:
    Fedora:        https://getfedora.org/

  Or a RHEL-compatible derivative like the old CentOS:
    Rocky Linux:   https://rockylinux.org/download
    AlmaLinux:     https://almalinux.org/

  Install using default options.

Ensure you have sudo permissions
--------------------------------

  On RHEL and similar hosts, login as root on the console
  and run this command with your username:

    usermod -aG wheel your-user-name

  Logout and login again to verify you are in the correct group:

    $ id

  The output of the id command should include the wheel group.

RHEL, CentOS, Rocky Linux: enable repositories
----------------------------------------------

In Fedora, all the necessary packages and repositories are available.
Fedora users can skip this step.  Users of RHEL and derivaties need
to manually enable some repositories before you can install
dependencies.

Enable powertools (for common *-devel packages)

  sudo dnf -y install dnf-plugins-core
  sudo dnf config-manager --set-enabled powertools

Enable EPEL

  sudo dnf install epel-release

Setup the rpmbuild tool, configuration and directory structure
--------------------------------------------------------------

Based on the notes here:
https://wiki.centos.org/HowTos/SetupRpmBuildEnvironment

Install the rpmbuild tool and some related packages:

  sudo dnf install rpm-build
  sudo dnf install redhat-rpm-config
  sudo dnf install rpmdevtools

In your home directory, run the following command to create
the RPM directory structure:

  rpmdev-setuptree

Now you can verify the tree and config has been created correctly:

  find ~/rpmbuild -type d
  cat ~/.rpmmacros

Install reSIProcate build environment dependencies
--------------------------------------------------

  sudo dnf install git \
                   gcc-c++ \
                   libtool automake autoconf \
                   python3-devel python3-pycxx-devel \
                   libdb-cxx libdb-cxx-devel \
                   cppunit cppunit-devel \
                   gperf \
                   radcli-devel \
                   c-ares-devel \
                   libsrtp-devel \
                   boost-devel \
                   openssl-devel \
                   mariadb-connector-c-devel \
                   pcre-devel \
                   popt-devel \
                   postgresql-devel \
                   xerces-c-devel \
                   net-snmp-devel \
                   qpid-proton-cpp-devel \
                   soci-devel soci-postgresql-devel soci-mysql-devel \
                   vim-common \
                   sox \
                   fmt-devel \
                   websocketpp-devel \
                   gstreamer1-devel \
                   gstreamer1-plugins-base-devel \
                   gstreamer1-plugins-bad-free-devel \
                   gstreamermm-devel

Manually create some dependencies
---------------------------------

Some dependencies are not available in EPEL right now.  It is
necessary to copy them from Fedora and build them locally.  You
can check the availability of some common dependencies by
looking in the Fedora package catalog:

https://src.fedoraproject.org/rpms/asio
https://src.fedoraproject.org/rpms/cajun-jsonapi

Try to install them with dnf, if the command fails, you need to
add an extra repository or build them manually:

  sudo dnf install asio-devel
  sudo dnf install cajun-jsonapi-devel

Here is an example to build asio-devel from source, use exactly the
same sequence of commands for cajun-jsonapi-devel or any other
missing dependency.  Notice the repository name is not exactly the
same as the package name.

  mkdir -p ~/ws/fedora-rpms
  cd ~/ws/fedora-rpms
  git clone https://src.fedoraproject.org/rpms/asio.git
  cd asio
  spectool -g -R asio.spec
  rpmbuild -bb asio.spec
    (it will complain about missing dependencies, so we install them ...)
  sudo dnf install boost-devel openssl-devel perl-generators
  rpmbuild -bb asio.spec

Now you can install the asio-devel package, the exact filename may
vary depending on the version you built and your CPU architecture:

  sudo rpm -i ~/rpmbuild/RPMS/x86_64/asio-devel-1.16.1-3.el8.x86_64.rpm

You may need to repeat this procedure for the cajun-jsonapi package.

Install Docker (for Podman, alternative to Docker, see below)
-------------------------------------------------------------

If you are planning to use one of the related products that is
distributed as a Docker image then you need to install either Docker
or Podman.  Typical Docker images include the HOMER system for
SIP message capture and the Kurento Media Server.

These are the steps to install Docker:

  sudo dnf config-manager --add-repo=https://download.docker.com/linux/centos/docker-ce.repo

  sudo dnf install docker-ce
  systemctl is-active docker
  sudo systemctl enable --now docker
  systemctl is-active docker
  systemctl is-enabled docker
  sudo usermod -aG docker $USER

and these are the steps to install Podman:

  sudo dnf install podman

If you like, you can create an alias for the docker command to run
Podman, save this in ~/.profile or a similar location:

  alias docker=podman

Preparing to build the reSIProcate stack
----------------------------------------

You can clone the reSIProcate Git repository or download a copy
of the reSIProcate tarball release.  Here are the steps to clone
the repository:

  mkdir -p ~/ws
  cd ~/ws
  git clone https://github.com/resiprocate/resiprocate
  cd resiprocate

Configuring the source tree
---------------------------

The project is currently built using GNU Autotools.  We provide a
wrapper script for autotools on each major GNU/Linux distribution.

The wrapper script for Fedora and RHEL-based systems is:

  build/fedora.sh

Run the script from the top level of the reSIProcate tree.

We recommend that you look inside the script and tweak any settings
you require.  For example, if you are building on RHEL8 or an older
system, you need to add the configure switch:

     --with-srtp1

Compiling the code in the source tree
-------------------------------------

After running the autotools configure command or using the wrapper
script described above, it is possible to start compiling the code.
To compile with 65 threads, on a system with 64 CPU cores, you could
use the following command:

  make -j65

Running the unit tests
----------------------

The autotools "make check" target is used.

  make check

Running the binaries or unit tests in the GDB debugger
------------------------------------------------------

To use the debugger, we need help from libtool to match
the binary and libraries in our source tree.  gdb will not
work if you try to run it directly.  Here is an example:

  libtool --mode=execute gdb --args \
      apps/reConServer/reConServer \
      apps/reConServer/reConServer.config

Building reSIProcate RPM packages
---------------------------------

To build packages, it is necessary to configure the tree:

  build/fedora.sh

Create a reSIProcate release tarball:

  make dist

Finally, tell rpmbuild to compile the tarball to RPMs.  rpmbuild
uses the spec file resiprocate.spec inside the tarball:

  rpmbuild -tb resiprocate-1.13.0~alpha1.tar.gz


