SUMMARY = "Event policy for Mingder"
PR = "r1"

inherit native
inherit phosphor-dbus-monitor

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

SRC_URI += "file://caterr_event.yaml"

do_install() {
        install -D ${WORKDIR}/caterr_event.yaml ${D}${config_dir}/caterr_event.yaml
}
