SYSTEMD_LINK_${PN}-ledmanager_remove += "${@compose_list(d, 'FMT', 'STATES')}"
SYSTEMD_LINK_${PN}-ledmanager_remove += "../obmc-led-group-start@.service:multi-user.target.wants/obmc-led-group-start@bmc_booted.service"

