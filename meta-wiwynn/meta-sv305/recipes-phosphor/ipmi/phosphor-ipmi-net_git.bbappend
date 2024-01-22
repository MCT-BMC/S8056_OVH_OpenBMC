FILESEXTRAPATHS_prepend_sv305 := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-netipmid-Skip-SOL-payload-activation-checking.patch \
            file://0002-Support-HMAC-SHA1-to-Authentication-and-Integrity-Algorithm.patch \
            file://0003-Limit-the-host-console-buffer-size-to-1M.patch \
           "
