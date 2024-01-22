SUMMARY = "OpenBMC for Wiwynn - Applications"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit packagegroup

PROVIDES = "${PACKAGES}"
PACKAGES = " \
        ${PN}-chassis \
        ${PN}-fans \
        ${PN}-flash \
        ${PN}-system \
        ${PN}-host \
        "

PROVIDES += "virtual/obmc-chassis-mgmt"
PROVIDES += "virtual/obmc-fan-mgmt"
PROVIDES += "virtual/obmc-flash-mgmt"
PROVIDES += "virtual/obmc-system-mgmt"
PROVIDES += "virtual/obmc-host-ctl"

RPROVIDES_${PN}-chassis += "virtual-obmc-chassis-mgmt"
RPROVIDES_${PN}-fans += "virtual-obmc-fan-mgmt"
RPROVIDES_${PN}-flash += "virtual-obmc-flash-mgmt"
RPROVIDES_${PN}-system += "virtual-obmc-system-mgmt"
RPROVIDES_${PN}-host += "virtual-obmc-host-ctl"


SUMMARY_${PN}-chassis = "Wiwynn Chassis"
RDEPENDS_${PN}-chassis = " \
        "

SUMMARY_${PN}-fans = "Wiwynn Fans"
RDEPENDS_${PN}-fans = " \
        phosphor-pid-control \
        "

SUMMARY_${PN}-flash = "Wiwynn Flash"
RDEPENDS_${PN}-flash = " \
        obmc-control-bmc \
        "

SUMMARY_${PN}-system = "Wiwynn System"
RDEPENDS_${PN}-system = " \
        systemd-analyze \
        bmcweb \
        phosphor-webui \
        "
