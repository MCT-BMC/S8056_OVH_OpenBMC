FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-Add-http-end-point-for-SOL-buffer.patch \
            file://0002-Disable-version-compare-judgement-in-image-upload-AP.patch \
           "

# Enable Redfish BMC Journal support
EXTRA_OECMAKE += "-DBMCWEB_ENABLE_REDFISH_BMC_JOURNAL=ON"
EXTRA_OECMAKE += "-DBMCWEB_HTTP_REQ_BODY_LIMIT_MB=512"
EXTRA_OECMAKE_append += " -DBMCWEB_INSECURE_ENABLE_REDFISH_FW_TFTP_UPDATE=ON"

