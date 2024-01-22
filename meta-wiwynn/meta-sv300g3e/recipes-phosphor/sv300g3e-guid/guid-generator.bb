SUMMARY = "SV300G3-E GUID Generator"
DESCRIPTION = "SV300G3-E GUID Creation and Backup"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit cmake systemd
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

DEPENDS += "boost systemd sdbusplus obmc-libmisc obmc-libi2c"
RDEPENDS_${PN} += "libsystemd sdbusplus obmc-libmisc obmc-libi2c"

SRC_URI = "file://CMakeLists.txt \
           file://src \
           file://LICENSE \
           file://service \
           file://include \
          "
EXTRA_OECMAKE = "-DYOCTO=1"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.guid-generator.service"

