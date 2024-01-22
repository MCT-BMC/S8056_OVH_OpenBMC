FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-MCT-YAML-file-for-warm-reset.patch \
            file://0002-Add-interface-for-caterr-logging.patch \
            file://0003-Add-SOL-PATTERN-Interface-for-SOL-Processing.patch"

