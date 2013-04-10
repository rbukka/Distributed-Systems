#!/bin/bash

#Make all the directories needed
mkdir ca
mkdir certs
mkdir crl
mkdir newcerts
touch index.txt
echo 01 > serial

#Take the hostfile.txt as input
exec<$1
config="./openssl.cnf"
# Generate private key for CA
openssl genrsa -out ./ca/ca_key.pem 2048

# Generate certificate (public key) for CA
openssl req -batch -new -x509 -extensions v3_ca -key ./ca/ca_key.pem -out ./ca/ca_cert.pem -days 365

#generate for each file
while read line
do
	touch index.txt
	echo 01 > serial

	# Generate private key for host with id = $line
	openssl genrsa -out ./key_"$line".pem 2048

	# Generate certificate (public key) for host with id = $line
	openssl req -batch -new -extensions v3_ca -key ./key_"$line".pem -out ./host_"$line"_unsigned_cert.pem -days 365

	# Sign the certificate by the CA
	openssl ca -batch -out ./cert_"$line".pem -keyfile ./ca/ca_key.pem -cert ./ca/ca_cert.pem -config $config -infiles ./host_"$line"_unsigned_cert.pem

	rm -rf index*
	rm -rf serial*
done

