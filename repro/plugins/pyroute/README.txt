

To use a Python routing script:

- copy the example.py script and adapt it to your requirements

- your script may have an on_load method, it will be called once, it is
  optional

- your script MUST have a provide_route method.  The arguments are
  in the example script.  It must return a list of zero or more SIP
  URIs, these will be used to deliver the SIP request.

- the script will only be read at startup.  If you change the script,
  you must restart the repro proxy.  A runtime reload facility may
  be added in future.

The following is an example of how to enable PyRoute in repro.config:

# location of libpyroute.so:
PluginDirectory = /usr/local/lib/resiprocate/repro/plugins/
# load the libpyroute.so plugin:
LoadPlugins = pyroute

# path where the script(s) are located:
PyRoutePath = /usr/lib/resiprocate/repro/pyroute
# use the example_ldap.py like this:
PyRouteScript = example_ldap

# How many worker threads to create for running the Python script
# (default: 2)
# If the provide_route method is not thread-safe then set this to 1
#PyRouteNumWorkerThreads = 2

