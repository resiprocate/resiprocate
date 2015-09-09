
Working with public keys:

You can download a public certificate from verisign in .p7c format or export it from outlook or netscape. This can be converted to a public cert with:

openssl pkcs7 -in fluffy.p7c -inform DER -print_certs -outform PEM -out fluffy.pem



Working with private keys:

You can export a pkcs12 (.p12 or .pfx) file from outlook. This can be convered to a private key with. 

openssl pkcs12 -in fluffy.pfx -passin pass:password -passout pass:password -out id_key.pem -nocerts

The public cert can also be extracted with 
openssl pkcs12 -in fluffy.pfx -passin pass:password -out id.pem -nokeys

The root certifcates can be extracted with 
openssl pkcs12 -in fluffy.pfx -passin pass:password -out root.pem -cacerts -nokeys


Notes: 

To encrypt for someone ...
echo hello | openssl smime -encrypt -out foo.msg -des fluffy.pem

To decrypt
openssl smime -decrypt -in foo.msg -inkey fluffy_key.pem -recip fluffy.pem -passin pass:password

Sign a message 
echo hello | openssl smime -sign -text -signer fluffy.pem -passin pass:password -inkey fluffy_key.pem -certfile fluffy.pem -nodetach > bar.msg

Verify a message 
openssl smime -verify -in bar.msg -signer fluffy.pem -CAfile root.pem


-- Generating a self signed cert and key -- 
openssl genrsa -out id_key.pem 2048
openssl req -x509 -new -config extn.cnf -sha256 -key id_key.pem -days 500 -out id.pem 


--- Generating a cert for TLS use --- 
openssl req -new -out server.csr
- when asked for common name - enter domainname 
- don't use abreviation for state - ie use California not CA
openssl rsa -in privkey.pem -out server.key

This server.csr file can be used to get a free test certificate from Thawte or verisign. Put
result back form the CA in server.crt

Can read the cert with 
openssl x509 -in server.crt -text -noout

Can read the request with 
openssl req -in server.csr -text -noout

Convert certificate in DER to PRM 
openssl x509 -in root_der.crt -inform DER -out root.pem

----

Dumping asn 

openssl asn1parse -inform der -i -in resip-asn-decrypt 

or

resiprocate/test/dumpasn1 resip-encrpt-out
