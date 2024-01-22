LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS_append := "${THISDIR}/files:"

inherit obmc-phosphor-systemd

DEPENDS += "systemd"
RDEPENDS_${PN} += "bash"
# For revising json files with command lines
RDEPENDS_${PN} += "jq"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} += "sv310a-bmcenv-init.service \
                          sv310a-checkaclost.service \
                          sv310a-nodebmc-init.service \
                          sv310a-bmc-ready.service \
                          "

S = "${WORKDIR}"
SRC_URI = "file://sv310a-bmcenv-init.sh \
           file://sv310a-checkaclost.sh \
           file://sv310a-nodebmc-init.sh \
          "

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${S}/sv310a-bmcenv-init.sh ${D}${sbindir}/
    install -m 0755 ${S}/sv310a-checkaclost.sh ${D}${sbindir}/
    install -m 0755 ${S}/sv310a-nodebmc-init.sh ${D}${sbindir}/
}
