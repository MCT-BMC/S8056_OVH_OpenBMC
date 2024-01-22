FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://0001-Disable-NTP-time-sync-by-default.patch \
             file://0002-Disable-to-release-IP-from-DHCP.patch \
             file://0003-Attach-BMC-MAC-to-published-hostname.patch \
           "
