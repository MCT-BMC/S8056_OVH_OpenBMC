#!/bin/bash
# SV300G3-E Switch to multi-user.target
tmpFile=/run/initramfs/WarmReset

if [ ! -f "${tmpFile}" ];then
    echo "Stay in rescue.target"
    exit 0;
fi

echo "Switch to multi-user.target"
# rm -f ${tmpFile}
systemctl isolate multi-user.target

exit 0
