inherit extrausers

DEFAULT_OPENBMC_PASSWORD = "'\$1\$KdSxTq5i\$W7g5v8sWXDsF0zvh.8Rk61'"

OBMC_IMAGE_EXTRA_INSTALL:append = " \
    libgpiod \
    gpio-initial \
    hbled-ctrl \
    mct-default-action \
    mct-gpio-monitor \
    entity-manager \
    dbus-sensors \
    obmc-ikvm \
    phosphor-sel-logger \
    phosphor-pid-control \
    system-watchdog \
    phosphor-u-boot-mgr \
    phosphor-post-code-manager \
    phosphor-host-postd \
    srvcfg-manager \
    mct-ipmi-oem \
    twitter-ipmi-oem-bin \
    mct-dcmi-power \
    mct-register-monitor \
    mct-bios-update \
    mct-leaky-bucket \
    mct-fw-update \
    ethtool \
    mct-acpi-handler \
    mct-bmc2-update \
    phosphor-pid-control \
    mct-sol-handler \
    iperf3 \
    bmc-rtc-time-sync \
    mct-power-handler \
    mct-led-manager \
    jbi \
    mct-cpld-update \
    phosphor-ipmi-flash \
    phosphor-ipmi-blobs \
    phosphor-ipmi-blobs-binarystore \
    phosphor-image-signing \
    bmc-update \
    bios-update \
    amd-apml \
    mct-dual-bios-handler \
    mct-psu-status-led-handler \
    mct-sku-manager \
    iptables \
    mct-version-handler \
    mct-psu-update \
    mct-vr-update \
"
