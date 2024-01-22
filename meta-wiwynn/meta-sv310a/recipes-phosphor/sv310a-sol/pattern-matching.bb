SUMMARY = "SOL Pattern Matching"
DESCRIPTION = "Twitter SOL Pattern Matching Service"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

SRC_URI = "file://cmake \
           file://include \
           file://service \
           file://src \
           file://sol-pattern.json \
           file://cmake-format.json \
           file://CMakeLists.txt \
           file://LICENSE \
          "

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.sol-pattern-matching.service"

DEPENDS = "boost \
           nlohmann-json \
           sdbusplus \
          "
inherit cmake systemd

S = "${WORKDIR}"

EXTRA_OECMAKE = "-DYOCTO=1"

do_install_append() {
    install -d ${D}/etc
    install -m 0644 ${S}/sol-pattern.json ${D}/etc/sol-pattern.json
}
