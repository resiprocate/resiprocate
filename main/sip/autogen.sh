(cd contrib/ares ; ./configure && make) 
rm -rf build configure.ac 
autoreconf
autoreconf
export PATH=/usr/local/bin:$PATH
mkdir build
cd build
../configure -C --enable-scanner --enable-ipv6 --with-ares --with-openssl
