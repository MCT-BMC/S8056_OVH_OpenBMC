FILESEXTRAPATHS_append := ":${THISDIR}/${PN}"
SRC_URI_append = " \
    file://apply-time.override.yml \
    file://power-restore-policy.override.yml \
    file://time-sync-method.override.yml \
    file://service-status.override.yml \
    file://boot-option.override.yml \
    file://sol-settings.override.yml \
"
