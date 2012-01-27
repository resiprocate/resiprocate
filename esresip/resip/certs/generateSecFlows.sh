#!/bin/sh

mkdir ../stack/test/testCerts 2> /dev/null > /dev/null;
rm ../stack/test/testCerts/* 2> /dev/null > /dev/null;

# Generate CA cert
makeCA 2> /dev/null > /dev/null;
cp root_cert_fluffyCA.pem ../stack/test/testCerts;

# Generate host cert/key for example.com
makeCert example.com 2> /dev/null > /dev/null;
cp domain_cert_example.com.pem ../stack/test/testCerts;
cp domain_key_example.com.pem ../stack/test/testCerts;

# Generate host cert/key for example.net
makeCert example.net 2> /dev/null > /dev/null;
cp domain_cert_example.net.pem ../stack/test/testCerts;
cp domain_key_example.net.pem ../stack/test/testCerts;

# Generate user cert/key for fluffy@example.com
makeCert fluffy@example.com 2> /dev/null > /dev/null;
cp user_cert_fluffy@example.com.pem ../stack/test/testCerts;
cp user_key_fluffy@example.com.pem ../stack/test/testCerts;

# Generate user cert for kumiko@example.net
makeCert kumiko@example.net 2> /dev/null > /dev/null;
cp user_cert_kumiko@example.net.pem ../stack/test/testCerts;
cp user_key_kumiko@example.net.pem ../stack/test/testCerts;

cd ../stack/test 2> /dev/null > /dev/null;

# Show root cert
openssl x509 -in testCerts/root_cert_fluffyCA.pem -text -noout -certopt no_header

# Generate ASN.1 parse of CA cert
openssl asn1parse -in testCerts/root_cert_fluffyCA.pem

# Show host cert for example.com
openssl x509 -in testCerts/domain_cert_example.com.pem -text -noout -certopt no_header

# Show user cert for fluffy@example.com
openssl x509 -in testCerts/user_cert_fluffy@example.com.pem -text -noout -certopt no_header

# Run TLS test program, capture details with ssldump
# Show details of handshake
# Show unencrypted app data

# Generate example S/MIME stuff
./testSMIME testCerts fluffy@example.com kumiko@example.net

# Generate dummy message headers (3)

# Show signed example with dummy message header
# Show ASN.1 parse of signature
openssl asn1parse -inform DER -in binaryBlob1.out

# Show encrypted example with dummy message header
# Show ASN.1 parse of encrypted message
openssl asn1parse -inform DER -in binaryBlob2.out

# Show signed+encrypted example with dummy message header
# Show ASN.1 parse of encrypted message
openssl asn1parse -inform DER -in binaryBlob3.out

# Show ASN.1 parse of signature
openssl asn1parse -inform DER -in binaryBlob4.out
