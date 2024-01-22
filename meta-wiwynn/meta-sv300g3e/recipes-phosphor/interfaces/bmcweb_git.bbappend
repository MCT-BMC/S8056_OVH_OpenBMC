FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-download-method-for-host-console-log.patch \
            file://0002-Enlarge-time-frame-for-TFTP-uploading-BMC-image.patch \
           "

EXTRA_OECMAKE_append += " -DBMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE=ON"
EXTRA_OECMAKE_append += " -DBMCWEB_HTTP_REQ_BODY_LIMIT_MB=128"
