FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
            file://0001-Reference-platform-reset-GPIO-for-clearing-buffer.patch \
            file://0002-Add-an-1-second-timer-to-save-the-POST-codes-to-file.patch \
           "

EXTRA_OECMAKE="-DMAX_POST_CODE_CYCLES=5"
EXTRA_OECMAKE="-DMAX_POST_CODE_SIZE_PER_CYCLE=512"
