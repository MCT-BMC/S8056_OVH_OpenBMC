SUMMARY = "Intel PECI interface library"
DESCRIPTION = "Intel PECI interface library"

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=7becf906c8f8d03c237bad13bc3dac53"

SRC_URI = "git://github.com/Intel-BMC/provingground.git"
SRCREV = "bee56d62b209088454d166d1efae4825a2b175df"

S = "${WORKDIR}/git/libpeci"
PV = "0.1+git${SRCPV}"

inherit cmake

# linux-libc-headers guides this way to include custom uapi headers
CFLAGS_append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CFLAGS_append = " -I ${STAGING_KERNEL_DIR}/include"
CXXFLAGS_append = " -I ${STAGING_KERNEL_DIR}/include/uapi"
CXXFLAGS_append = " -I ${STAGING_KERNEL_DIR}/include"
do_configure[depends] += "virtual/kernel:do_shared_workdir"
