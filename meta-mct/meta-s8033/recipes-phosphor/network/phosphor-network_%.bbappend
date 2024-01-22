FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

DEPENDS += "nlohmann-json"

SRC_URI += "file://0001-Add-keep-Configuration-property-to-eth0-for-avoiding.patch \
            file://0002-Init-fixed-static-IP-address-for-usb0-ifconfig.patch\
            file://0003-Support-DHCP-configuration-updating.patch \
           "

