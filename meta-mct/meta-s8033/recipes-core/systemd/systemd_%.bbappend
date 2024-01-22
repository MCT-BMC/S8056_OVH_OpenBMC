FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://0001-Disable-IP-released-from-DHCP.patch \
             file://0002-Disable-NTP-time-sync-by-default.patch \
             file://0003-Attach-BMC-MAC-to-published-hostname.patch \
           "
pkg_postinst_ontarget:${PN} () {
  systemctl stop start-ipkvm.service
  systemctl disable start-ipkvm.service
}
