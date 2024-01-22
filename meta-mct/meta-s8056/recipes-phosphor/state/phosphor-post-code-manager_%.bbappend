FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Implement-the-clear-bios-post-code-feature-for-host-.patch \
            file://0002-Send-dbus-singal-with-received-post-code.patch \
            file://0003-Fix-issue-that-getting-wrong-BootNum-in-getPostCodes.patch \
            file://0004-Add-restart-property-for-post-code-manager-service.patch \
            file://0005-Support-adding-current-boot-cycle-index-feature-for.patch \
            file://0006-Add-filter-post-code-function-for-BMC-resource-issue.patch \
            "

