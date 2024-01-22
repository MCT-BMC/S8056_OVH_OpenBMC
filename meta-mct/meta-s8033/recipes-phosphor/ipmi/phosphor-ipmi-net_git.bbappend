FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Revert-Remove-HMAC-SHA1-from-Authentication-Integrit.patch \
            file://0002-Add-the-max-size-limit-feature-to-SOL-Console-data.patch \
            file://0003-PATCH-Remove-Get-SOL-Config-Command-from-Netipmid.patch \
            file://0004-Change-net-service-wantedby-from-multi-user-target-t.patch \
            "
