SUMMARY = "OpenBMC for MCT - Applications"
PR = "r1"

inherit packagegroup

PROVIDES = "${PACKAGES}"
PACKAGES = " \
        ${PN}-fans \
        ${PN}-system \
        "

PROVIDES += "virtual/obmc-fan-mgmt"
PROVIDES += "virtual/obmc-system-mgmt"

RPROVIDES:${PN}-fans += "virtual-obmc-fan-mgmt"
RPROVIDES:${PN}-system += "virtual-obmc-system-mgmt"

RDEPENDS:${PN}-chassis:remove = " obmc-op-control-power"

SUMMARY:${PN}-fans = "MCT Fans"
RDEPENDS:${PN}-fans = " \
        phosphor-pid-control \
        "

SUMMARY:${PN}-system = "MCT System"
RDEPENDS:${PN}-system = " \
        ipmitool \
        systemd-analyze \
        bmcweb \
        phosphor-sel-logger  \
        dbus-sensors \
        phosphor-settings-manager \
        srvcfg-manager \
        "
