FILESEXTRAPATHS_prepend_sv310a := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-netipmid-Skip-SOL-payload-activation-checking.patch \
            file://0002-Support-HMAC-SHA1-to-Authentication-and-Integrity-Algorithm.patch \
            file://0003-Limit-the-host-console-buffer-size-to-1M.patch \
            file://0004-Fix-ipmitool-sol-coredump-issue.patch \
            file://phosphor-ipmi-net@.service;subdir=git/ \
           "
