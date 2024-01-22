SUMMARY = "AMD EPYC System Management Interface Library"
DESCRIPTION = "AMD EPYC System Management Interface Library for user space APML implementation"

FILESEXTRAPATHS:prepend := "${THISDIR}:"

LICENSE = "CLOSED"

DEPENDS += "i2c-tools"

SRC_URI = "git://github.com/amd/esmi_oob_library.git;protocol=ssh \
           file://0001-Using-local-amd-apml-header-file-to-repacle-header-f.patch \
           "
SRCREV = "00cc0fb0265af1d240a0aff5ed96f90a73ff8c51"

PV = "1.0+git${SRCPV}"
S = "${WORKDIR}/git"

inherit cmake

do_install () {
        install -d ${D}${libdir}
        cp --preserve=mode,timestamps -R ${B}/libapml64* ${D}${libdir}/

        install -d ${D}${bindir}
        install -m 0755 ${B}/apml_tool ${D}${bindir}/
        install -m 0755 ${B}/apml_cpuid_tool ${D}${bindir}/

        install -d ${D}${includedir}
        install -m 0644 ${S}/include/esmi_oob/* ${D}${includedir}/
}