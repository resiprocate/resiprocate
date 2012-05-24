#! /bin/sh
### BEGIN INIT INFO
# Provides:          repro
# Required-Start:    $local_fs $remote_fs
# Required-Stop:     $local_fs $remote_fs
# Default-Start:     2 3 4 5
# Default-Stop:      S 0 1 6
# Short-Description: Repro initscript
### END INIT INFO
#
# Author:	Martin Hoffmann <hn@nvnc.de>
#
# Version:	@(#)repro 0.2  18-Feb-2006  hn@nvnc.de
#

set -e

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DESC="Repro SIP Proxy"
NAME=repro
DAEMON=/usr/sbin/$NAME
PIDFILE=/var/run/$NAME.pid
SCRIPTNAME=/etc/init.d/$NAME
RUNDIR=/var/lib/$NAME
RUNUSER=repro:repro

# Gracefully exit if the package has been removed.
test -x $DAEMON || exit 0

# Read config file if it is present.
#if [ -r /etc/default/$NAME ]
#then
#	. /etc/default/$NAME
#fi

#
#	Function that starts the daemon/service.
#
d_start() {
	CMDARGS=`sed s/#.*// /etc/repro.conf`
	start-stop-daemon --start --quiet --pidfile $PIDFILE --background \
		--make-pidfile --chuid $RUNUSER --chdir $RUNDIR \
		--exec $DAEMON -- $CMDARGS \
		|| echo -n " already running"
}

#
#	Function that stops the daemon/service.
#
d_stop() {
	start-stop-daemon --stop --quiet --pidfile $PIDFILE \
		--name $NAME \
		|| echo -n " not running"
}

#
#	Function that sends a SIGHUP to the daemon/service.
#
d_reload() {
	start-stop-daemon --stop --quiet --pidfile $PIDFILE \
		--name $NAME --signal 1
}

case "$1" in
  start)
	echo -n "Starting $DESC: $NAME"
	d_start
	echo "."
	;;
  stop)
	echo -n "Stopping $DESC: $NAME"
	d_stop
	echo "."
	;;
  restart|force-reload)
	echo -n "Restarting $DESC: $NAME"
	d_stop
	sleep 1
	d_start
	echo "."
	;;
  *)
	echo "Usage: $SCRIPTNAME {start|stop|restart|force-reload}" >&2
	exit 1
	;;
esac

exit 0

