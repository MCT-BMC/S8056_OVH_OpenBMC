LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS_append := "${THISDIR}/files:"

inherit obmc-phosphor-systemd

DEPENDS += "systemd"
RDEPENDS_${PN} += "bash"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} += "sv305-bmcenv-init.service \
                          sv305-checkaclost.service \
                          sv305-checkproductversion.service \
                          sv305-defaultdisablessh.service \
                          sv305-setdefaultbootoption.service \
                          "

S = "${WORKDIR}"
SRC_URI = "file://sv305-bmcenv-init.sh \
           file://sv305-checkaclost.sh \
           file://sv305-checkproductversion.sh \
          "

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${S}/sv305-bmcenv-init.sh ${D}${sbindir}/
    install -m 0755 ${S}/sv305-checkaclost.sh ${D}${sbindir}/
    install -m 0755 ${S}/sv305-checkproductversion.sh ${D}${sbindir}/
}
