
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
openssl genrsa -out id_key.pem 512
openssl req -x509 -new  -key id_key.pem -days 180 -out id.pem
