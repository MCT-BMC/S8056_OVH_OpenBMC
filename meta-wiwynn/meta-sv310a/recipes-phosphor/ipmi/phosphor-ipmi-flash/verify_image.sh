#!/bin/bash

STATUS_FILE=/tmp/bmc.verify
RESULT_FILE=/tmp/result.verify
PUBLIC_KEY=/etc/activationdata/OpenBMC/publickey
SIG_FILE=/tmp/bmc.sig
STAGED_FILE=/run/initramfs/bmc-image
TARGET_FILE=/run/initramfs/image-bmc

echo "running" > $STATUS_FILE
result=$(openssl dgst -verify $PUBLIC_KEY -signature $SIG_FILE $STAGED_FILE)
echo $result > $RESULT_FILE

if [[ "${result}" =~ "OK" ]] ;then
    mv $STAGED_FILE $TARGET_FILE
    echo "success" > $STATUS_FILE
else
    echo "failed" > $STATUS_FILE
fi

