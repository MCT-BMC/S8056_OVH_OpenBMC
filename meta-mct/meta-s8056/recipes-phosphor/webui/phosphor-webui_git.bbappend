FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://platform-config.json;subdir=git/ \
            file://0002-Implement-the-feature-for-setting-web-status-to-enab.patch \
            file://0003-Fix-the-page-for-sensors-reading-feature.patch \
            file://0004-Fix-the-page-for-manage-power-usage.patch \
            file://0005-Disable-unfinished-feature-temporarily.patch \
            file://0006-Fix-the-page-for-server-information-and-hardware-sta.patch \
            file://0007-Fix-the-page-for-system-logs-feature.patch \
            file://0008-Fix-the-page-for-firmware-feature.patch \
            file://0009-Fix-the-page-for-local-user-management-feature.patch \
            file://0011-Fix-the-system-log-feature-issue-for-getting-too-muc.patch \
            file://0012-Chnage-the-setting-IP-address-feature-to-read-only-i.patch \
            file://0013-Add-dynamic-virtual-media-setting-supproted.patch \
"

