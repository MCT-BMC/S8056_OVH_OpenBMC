FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " file://0001-Revise-the-algorithm-for-initializing-BMC-health-mon.patch \
             file://0002-Support-SEL-for-CPU-and-memory-utilization-in-phosph.patch \
             file://bmc_health_config.json \
"

do_install_append(){
    install -m 0644 -D ${WORKDIR}/bmc_health_config.json \
                   ${D}/etc/healthMon/bmc_health_config.json
}

