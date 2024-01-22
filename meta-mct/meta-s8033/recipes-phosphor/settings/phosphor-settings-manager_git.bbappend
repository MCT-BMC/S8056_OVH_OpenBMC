FILESEXTRAPATHS:append := ":${THISDIR}/${PN}"
SRC_URI:append = " file://power-restore-policy.override.yml \
                   file://service-status.override.yml \
                   file://power-cap.override.yml \
                   file://ac-boot.override.yml \
                   file://power-restore-delay.override.yml \
                   file://sensor-status.override.yml \
                   file://nmi-source.override.yml \
                   file://sol-default.override.yml \
                   file://boot-default.override.yml \
                   file://system-info.override.yml \
                   file://host-status.override.yml \
                   "
