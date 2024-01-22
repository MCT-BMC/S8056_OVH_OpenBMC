FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Support-two-byte-POST-code-for-extend-snoop-device.patch"

SNOOP_DEVICE_EXTEND = "aspeed-lpc-snoop1"
EXTRA_OEMESON += "-Dsnoop-device-extend=${SNOOP_DEVICE_EXTEND}"