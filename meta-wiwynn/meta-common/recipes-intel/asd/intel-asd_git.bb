SUMMARY = "Intel ASD Utility"
DESCRIPTION = "Intel At-Scale Debug utility"

LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE;md5=0d1c657b2ba1e8877940a8d1614ec560"

SRC_URI = "git://github.com/Intel-BMC/asd"
SRCREV = "451759eec5891fc2b5b19a16475df6c20e3b24cb"

S = "${WORKDIR}/git"
PV = "0.1+git${SRCPV}"

DEPENDS = "openssl libpam libgpiod systemd"

inherit cmake systemd
