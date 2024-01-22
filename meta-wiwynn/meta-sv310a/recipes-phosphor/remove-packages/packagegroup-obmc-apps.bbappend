REPENDS_${PN}-ikvm_remove += " \
    obmc-ikvm \
    "

RDEPENDS_${PN}-fan-control_remove += " \
    ${VIRTUAL-RUNTIME_obmc-fan-control} \
    phosphor-fan-monitor \
    "

RDEPENDS_${PN}-inventory_remove += " \
    ${VIRTUAL-RUNTIME_obmc-fan-presence} \
    "
