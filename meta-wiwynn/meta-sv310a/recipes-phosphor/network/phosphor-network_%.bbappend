FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"
SRC_URI += " \
             file://0001-Add-function-to-return-default-gateway.patch \
             file://0002-Init-fixed-static-IP-address-for-usb0-ifconfig.patch \
           "

