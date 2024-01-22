SUMMARY = "SV305 event register application for gpio monitor"
PR = "r1"

inherit obmc-phosphor-systemd

LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${WIWYNNBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

DEPENDS += "virtual/obmc-gpio-monitor"
RDEPENDS_${PN} += "virtual/obmc-gpio-monitor bash"

SRC_URI += "file://SetPowerGoodPropertyOff.service \
            file://SetPowerGoodPropertyOn.service \
            file://SetPostCompleteAssert.service \
            file://SetPostCompleteDeassert.service \
            file://SetIdentifyBtnAssert.service \
            file://SetCPUThermalTripAssert.service \
            file://SetCoreRunFaultAssert.service \
            file://SetCoreRunVRHOTAssert.service \
            file://SetMemABCDSusFaultAssert.service \
            file://SetMemABCDVRHOTAssert.service \
            file://SetMemEFGHSusFaultAssert.service \
            file://SetMemEFGHVRHOTAssert.service \
            file://SetPsuAlert.service \
            file://SetSocRunFaultAssert.service \
            file://SetSocRunVRHOTAssert.service \
            file://SetVoltageAlert.service \
            file://SetPlatformResetAssert.service \
            file://SetPlatformResetDeassert.service \
            file://bind-mux-switch.sh \
            file://toggle-identify-led.sh \
           "

do_install() {
        install -d ${D}${sbindir}
        install -m 0755 ${WORKDIR}/bind-mux-switch.sh ${D}${sbindir}/bind-mux-switch.sh
        install -m 0755 ${WORKDIR}/toggle-identify-led.sh ${D}${sbindir}/toggle-identify-led.sh
}

SYSTEMD_SERVICE_${PN} += "SetPowerGoodPropertyOff.service"
SYSTEMD_SERVICE_${PN} += "SetPowerGoodPropertyOn.service"
SYSTEMD_SERVICE_${PN} += "SetPostCompleteAssert.service"
SYSTEMD_SERVICE_${PN} += "SetPostCompleteDeassert.service"
SYSTEMD_SERVICE_${PN} += "SetIdentifyBtnAssert.service"
SYSTEMD_SERVICE_${PN} += "SetCPUThermalTripAssert.service"
SYSTEMD_SERVICE_${PN} += "SetCoreRunFaultAssert.service"
SYSTEMD_SERVICE_${PN} += "SetCoreRunVRHOTAssert.service"
SYSTEMD_SERVICE_${PN} += "SetMemABCDSusFaultAssert.service"
SYSTEMD_SERVICE_${PN} += "SetMemABCDVRHOTAssert.service"
SYSTEMD_SERVICE_${PN} += "SetMemEFGHSusFaultAssert.service"
SYSTEMD_SERVICE_${PN} += "SetMemEFGHVRHOTAssert.service"
SYSTEMD_SERVICE_${PN} += "SetPsuAlert.service"
SYSTEMD_SERVICE_${PN} += "SetSocRunFaultAssert.service"
SYSTEMD_SERVICE_${PN} += "SetSocRunVRHOTAssert.service"
SYSTEMD_SERVICE_${PN} += "SetVoltageAlert.service"
SYSTEMD_SERVICE_${PN} += "SetPlatformResetAssert.service"
SYSTEMD_SERVICE_${PN} += "SetPlatformResetDeassert.service"
