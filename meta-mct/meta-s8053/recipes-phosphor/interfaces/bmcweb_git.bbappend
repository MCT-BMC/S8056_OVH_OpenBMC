FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

LIC_FILES_CHKSUM = "file://LICENSE;md5=175792518e4ac015ab6696d16c4f607e"

SRC_URI += "file://mct_oem_rest.hpp;subdir=git/include \
            file://0001-Support-rest-API-for-MCT-OEM.patch \
            file://0002-Fix-the-issue-for-update-bmcweb-recipe.patch \
            file://0003-Implement-the-SEL-feature-for-redfish-log-service.patch \
            file://0004-Add-dynamic-virtual-media-setting-supproted.patch \
            file://0005-Chanage-upload-image-function-timeout-to-600-second.patch \
            file://0006-PATCH-Inventory-properties-via-Assembly-schema-on-bm.patch \
            file://0007-Support-FRU-list-in-chassis-assembly-node.patch \
            file://0008-Add-sol-pattern-sensor-type-supported-for-chassis-se.patch \
            file://0009-Support-setting-fan-contol-mode-in-manager-BMC-OEM-n.patch \
            file://0010-Support-reading-all-and-specified-offset-for-Redfish.patch \
            file://0011-Bind-bmcweb-socket-connection-to-interface-eth0.patch \
            "
# add a user called bmcweb for the server to assume
# bmcweb is part of group shadow for non-root pam authentication
#USERADD_PARAM_${PN} = "-r -s /usr/sbin/nologin -d /home/bmcweb -m -G shadow bmcweb"

#GROUPADD_PARAM_${PN} = "web; redfish "

# Enable CPU log service transactions through Redfish
EXTRA_OEMESON += "-Dredfish-cpu-log=enabled"
# Enable BMC journal access through Redfish
EXTRA_OEMESON += "-Dredfish-bmc-journal=enabled"
# Specifies the http request body length limit
EXTRA_OEMESON += "-Dhttp-body-limit=128"
# Enable TFTP based firmware update transactions through Redfish UpdateService.SimpleUpdate.
EXTRA_OEMESON += "-Dinsecure-tftp-update=enabled"
# Enable/disable the new PowerSubsystem, ThermalSubsystem, and all children schemas
EXTRA_OEMESON += "-Dredfish-new-powersubsystem-thermalsubsystem=enabled"
# Enable Phosphor REST (D-Bus) APIs
EXTRA_OEMESON += "-Drest=enabled"
# Enable the Virtual Media WebSocket.
#EXTRA_OEMESON += "-Dvm-nbdproxy=enabled"
# Enable MCT OEM Rest support
EXTRA_OEMESON += "-Dmct_oem_rest=enabled"
