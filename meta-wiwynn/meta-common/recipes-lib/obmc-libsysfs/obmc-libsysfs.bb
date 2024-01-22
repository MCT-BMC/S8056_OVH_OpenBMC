SUMMARY = "Phosphor OpenBMC Project-specific Misc. Library"
DESCRIPTION = "Phosphor OpenBMC Misc. Library for Project-specific."
PR = "r1"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

inherit cmake

DEPENDS += "nlohmann-json"

S = "${WORKDIR}"

SRC_URI = "file://libsysfs.hpp \
           file://libsysfs.cpp \
           file://CMakeLists.txt \
           file://COPYING.MIT \
          "

