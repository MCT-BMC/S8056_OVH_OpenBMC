inherit extrausers

DEFAULT_OPENBMC_PASSWORD = "'\$1\$KdSxTq5i\$W7g5v8sWXDsF0zvh.8Rk61'"

# OBMC
OBMC_IMAGE_EXTRA_INSTALL:append = " \
    obmc-ikvm \
    phosphor-u-boot-mgr \
    phosphor-post-code-manager \
    phosphor-host-postd \
    phosphor-ipmi-flash \
    phosphor-ipmi-blobs \
    phosphor-ipmi-blobs-binarystore \
    phosphor-image-signing \
    phosphor-inventory-manager \
    phosphor-ipmi-ipmb \
"
# MCT
OBMC_IMAGE_EXTRA_INSTALL:append = " \
    gpio-initial \
    mct-default-action \
    mct-gpio-monitor \
    mct-ipmi-oem \
    mct-power-handler \
    mct-node-manager \
    mct-led-manager \
    mct-fan-status-led-handler \
    mct-sol-handler \
    mct-fw-update \
    mct-bios-update \
    mct-cpld-update \
    mct-dcmi-power \
    mct-acpi-handler \
    mct-version-handler \
    mct-special-mode-manager \
    mct-leaky-bucket \
    mct-cpld-manager \
    mct-register-monitor \
    twitter-ipmi-oem-bin \
    mct-vr-update \
    mct-env-manager \
    system-fmc-watchdog \
    mct-bios-update-inband \
    mct-bmc-update-in-band \
    hbled-ctrl \
    mct-alt-bmc-update \
    mct-dual-bios-handler \
    mct-aspeed-otp-tool-bin \
    mct-post-code-manager \
"
# TOOL
OBMC_IMAGE_EXTRA_INSTALL:append = " \
    jq \
    iperf3 \
    ethtool \
    iptables \
"
# AMD
OBMC_IMAGE_EXTRA_INSTALL:append = " \
    amd-apml \
"
