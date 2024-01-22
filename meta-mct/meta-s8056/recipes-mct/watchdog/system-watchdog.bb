SUMMARY = "System watchdog"
DESCRIPTION = "BMC hardware watchdog service that is used to reset BMC \
               when unrecoverable events occurs"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${MCTBASE}/COPYING.apache-2.0;md5=34400b68072d710fecd0a2940a0d1658"

inherit allarch
inherit obmc-phosphor-systemd

SYSTEMD_SERVICE:${PN} += "system-watchdog.service"
SYSTEMD_ENVIRONMENT_FILE:${PN} += "obmc/system-watchdog/system-watchdog.conf"
