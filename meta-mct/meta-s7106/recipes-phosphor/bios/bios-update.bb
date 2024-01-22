SUMMARY = "MCT BIOS update utility"
DESCRIPTION = "MCT BIOS update utility."

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

FILESEXTRAPATHS_append := "${THISDIR}/files:"

#inherit skeleton-python
#inherit native
#inherit obmc-phosphor-license
#inherit obmc-phosphor-dbus-service
inherit obmc-phosphor-systemd

S = "${WORKDIR}/"

SRC_URI = "file://bios_update.py \
           file://update_bios.sh \
           file://org.openbmc.control.BiosFlash.conf \
           file://setup.py"

DEPENDS = "systemd"

RDEPENDS_${PN} += "\
        python-dbus \
        python-compression \
        python-shell \
        python-pygobject \
        python-subprocess \
        python-io \
        pyphosphor-dbus \
        "

#DBUS_SERVICE_${PN} += "org.openbmc.control.BiosFlash.service"
SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "org.openbmc.control.BiosFlash.service"

do_install() {
    install -d ${D}/usr/sbin
    install -m 0755 ${S}bios_update.py ${D}/${sbindir}/
    install -m 0755 ${S}update_bios.sh ${D}/${sbindir}/
    install -d ${D}/etc/dbus-1/system.d/
    install ${s}org.openbmc.control.BiosFlash.conf ${D}/etc/dbus-1/system.d/
}
