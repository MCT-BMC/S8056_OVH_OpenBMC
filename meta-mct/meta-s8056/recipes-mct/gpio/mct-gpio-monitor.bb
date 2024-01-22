SUMMARY = "MCT GPIO monitor application"
PR = "r1"

inherit obmc-phosphor-systemd
inherit obmc-phosphor-ipmiprovider-symlink
inherit systemd
inherit obmc-phosphor-systemd
inherit cmake

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

S = "${WORKDIR}"

SRC_URI += " \
            file://CMakeLists.txt\
            file://LICENSE \
            file://prochotThermtripHandler.cpp \
            file://gpio-prochot-thermtrip-assert@.service \
            file://gpio-prochot-thermtrip-deassert@.service \
            file://vrHotHandler.cpp \
            file://gpio-vr-hot-assert@.service \
            file://gpio-vr-hot-deassert@.service \
            file://prochot-manager.cpp \
            file://gpio-id-btn@.service \
            file://rotDetectHandler.cpp \
            file://gpio-rot-detect-assert@.service \
            "

DEPENDS += "virtual/obmc-gpio-monitor"
DEPENDS += "sdbusplus"
DEPENDS += "phosphor-logging"
DEPENDS += "phosphor-dbus-interfaces"
DEPENDS += "libgpiod"
DEPENDS += "obmc-libmisc"

RDEPENDS:${PN} += "virtual/obmc-gpio-monitor"
RDEPENDS:${PN} += "obmc-libmisc"

EXTRA_OECMAKE += "-DTHERMTRIP_ENABLE_POWER_FILTER=OFF"
EXTRA_OECMAKE += "-DPROCHOT_ENABLE_POWER_FILTER=OFF"
EXTRA_OECMAKE += "-DENABLE_VR_POWER_FILTER=OFF"
EXTRA_OECMAKE += "-DENABLE_PROCHOT_ASSET=OFF"

SYSTEMD_SERVICE:${PN} += "gpio-prochot-thermtrip-assert@.service"
SYSTEMD_SERVICE:${PN} += "gpio-prochot-thermtrip-deassert@.service"
SYSTEMD_SERVICE:${PN} += "gpio-vr-hot-assert@.service"
SYSTEMD_SERVICE:${PN} += "gpio-vr-hot-deassert@.service"
SYSTEMD_SERVICE:${PN} += "prochot-manager.service"
SYSTEMD_SERVICE:${PN} += "gpio-id-btn@.service"
SYSTEMD_SERVICE:${PN} += "gpio-rot-detect-assert@.service"
# linux-libc-headers guides this way to include custom uapi headers
CFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include"
CXXFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CXXFLAGS:append = " -I ${STAGING_KERNEL_DIR}/include"
do_configure[depends] += "virtual/kernel:do_shared_workdir"

do_install() {
    install -d ${D}/usr/bin
    install -m 0755 prochotThermtripHandler ${D}/${bindir}/
    install -m 0755 vrHotHandler ${D}/${bindir}/
    install -m 0755 prochot-manager ${D}/${bindir}/
    install -m 0755 rotDetectHandler ${D}/${bindir}/
}
