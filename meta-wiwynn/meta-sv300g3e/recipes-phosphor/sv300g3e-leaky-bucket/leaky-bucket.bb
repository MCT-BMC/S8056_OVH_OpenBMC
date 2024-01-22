SUMMARY = "Leaky Bucket Algorithm"
DESCRIPTION = "Twitter Leaky Bucket Service for DIMM Correctable ECC"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

SRC_URI = "file://cmake \
           file://include \
           file://service \
           file://src \
           file://leaky-bucket-threshold.json \
           file://leaky-bucket-dimm.json \
           file://cmake-format.json \
           file://CMakeLists.txt \
           file://LICENSE \
          "

SYSTEMD_SERVICE_${PN} = "xyz.openbmc_project.leaky-bucket.service"

DEPENDS = "boost \
           nlohmann-json \
           sdbusplus \
          "
inherit cmake systemd

S = "${WORKDIR}"

EXTRA_OECMAKE = "-DYOCTO=1"

do_install_append() {
    install -d ${D}/etc
    install -m 0644 ${S}/leaky-bucket-dimm.json ${D}/etc/leaky-bucket-dimm.json
    install -m 0644 ${S}/leaky-bucket-threshold.json ${D}/etc/leaky-bucket-threshold.json
}
