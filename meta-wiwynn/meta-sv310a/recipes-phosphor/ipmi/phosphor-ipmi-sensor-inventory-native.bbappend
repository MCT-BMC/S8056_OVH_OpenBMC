FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://config.yaml"

S = "${WORKDIR}"

do_install() {
        DEST=${D}${sensor_datadir}
        install -d ${DEST}
        install config.yaml ${DEST}/sensor.yaml
}
