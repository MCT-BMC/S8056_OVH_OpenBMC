SUMMARY = "ipmbbridged"
DESCRIPTION = "ipmbbridged from Intel"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit autotools pkgconfig
inherit obmc-phosphor-ipmiprovider-symlink
inherit pythonnative
inherit systemd
inherit obmc-phosphor-systemd
#inherit obmc-phosphor-dbus-service

S = "${WORKDIR}/source_code"

SRC_URI = "file://source_code/bootstrap.sh \
           file://source_code/configure.ac \
           file://source_code/cpudimm-temp-sensor.cpp \
           file://source_code/LICENSE \
           file://source_code/Makefile.am \
           file://xyz.openbmc_project.Ipmi.Ipmb.cpudimm.temp.service \
           file://xyz.openbmc_project.Hwmon-cpudimmtemp.service \
          "

DEPENDS += "autoconf-archive-native"
DEPENDS += "sdbusplus sdbusplus-native"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces phosphor-dbus-interfaces-native"
DEPENDS += "sdbus++-native"

RDEPENDS_${PN} += " \
        sdbusplus \
        phosphor-logging \
        phosphor-dbus-interfaces \
        "

#FILES_{PN} = "${sbindir}/cpudimm_temp_sensor"
#DBUS_SERVICE_${PN} = "xyz.openbmc_project.Ipmi.Ipmb.cpudimm.temp.service"

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.Ipmi.Ipmb.cpudimm.temp.service"
SYSTEMD_SERVICE_${PN} += "xyz.openbmc_project.Hwmon-cpudimmtemp.service"
