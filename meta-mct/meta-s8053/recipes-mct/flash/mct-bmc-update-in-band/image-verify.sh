#!/bin/sh

sigfile="/tmp/bmc.sig"
publickey="/etc/activationdata/OpenBMC/publickey"
bmclog="/tmp/update-bmc.log"
image=$1

echo "Verifying new image"
if [ -f $publickey ];then
    r="$(openssl dgst -verify $publickey -sha256 -signature $sigfile $image)"
    echo "$r" > $bmclog
    if [[ "Verified OK" == "$r" ]]; then
        rm -f $sigfile
        exit 0
    else
        exit 1
    fi
else
    echo "No $publickey file" > $bmclog
    exit 1
fi
