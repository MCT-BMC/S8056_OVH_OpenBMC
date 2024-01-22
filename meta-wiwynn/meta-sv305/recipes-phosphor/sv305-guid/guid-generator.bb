SUMMARY = "SV305 GUID Generator"
DESCRIPTION = "SV305 GUID Creation and Backup"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit cmake
inherit obmc-phosphor-systemd

S = "${WORKDIR}"

DEPENDS += "obmc-libmisc obmc-libsysfs"
RDEPENDS_${PN} += "obmc-libmisc obmc-libsysfs"

SRC_URI = "file://LICENSE \
           file://CMakeLists.txt \
           file://main.cpp \
           file://guid.hpp \
          "

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.guid-generator.service"
