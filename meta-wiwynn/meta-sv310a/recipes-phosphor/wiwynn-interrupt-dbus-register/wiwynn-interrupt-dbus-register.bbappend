FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://wiwynn-interrupt-dbus-register.json \
           "

do_install_append(){
        install -d ${D}/usr/share/wiwynn-interrupt-dbus-register
        install -m 644 -D ${WORKDIR}/wiwynn-interrupt-dbus-register.json ${D}/usr/share/wiwynn-interrupt-dbus-register/wiwynn-interrupt-dbus-register.json
}

SYSTEMD_SERVICE_${PN} = "interrupt-dbus-register.service"

EXTRA_OECONF += "GPIO_INTERRUPT_DBUS_PATH=/run/initramfs/ro/usr/share/wiwynn-interrupt-dbus-register/wiwynn-interrupt-dbus-register.json"

