FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

KCS_DEVICE = "ipmi-kcs2"
KCS_DEVICE2 = "ipmi-kcs3"

SYSTEMD_SERVICE:${PN} += "${PN}@${KCS_DEVICE2}.service"

SRC_URI += "file://0001-Change-KCS-service-wantedby-from-multi-user-target-t.patch \
            "
