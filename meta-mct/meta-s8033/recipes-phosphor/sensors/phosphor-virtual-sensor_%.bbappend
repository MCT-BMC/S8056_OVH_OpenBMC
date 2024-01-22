FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}/:"

SRC_URI:append = " file://${MACHINE}_sensor_config.json\
                   file://phosphor-virtual-sensor.service.replace \
                 "

do_install:append() {

    install -d ${D}/usr/share/phosphor-virtual-sensor
    install -m 0644 -D ${WORKDIR}/${MACHINE}_sensor_config.json \
                   ${D}/usr/share/phosphor-virtual-sensor/virtual_sensor_config.json

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/${PN}.service.replace \
        ${D}${systemd_system_unitdir}/${PN}.service
}