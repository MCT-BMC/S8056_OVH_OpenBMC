FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Determine-the-host-initial-state-with-pgood.patch \
            file://0002-Support-random-power-on-feature-to-always-on-policy.patch \
           "
