MACHINE_FEATURES += "\
        obmc-phosphor-fan-mgmt \
        obmc-host-ipmi \
        "

VIRTUAL-RUNTIME_obmc-host-state-manager = "x86-power-control"
VIRTUAL-RUNTIME_obmc-chassis-state-manager = "x86-power-control"
VIRTUAL-RUNTIME_obmc-discover-system-state = "x86-power-control"
VIRTUAL-RUNTIME_obmc-inventory-manager = "entity-manager"

PREFERRED_PROVIDER_virtual/obmc-fan-mgmt = "packagegroup-mct-apps"
PREFERRED_PROVIDER_virtual/obmc-system-mgmt = "packagegroup-mct-apps"
PREFERRED_PROVIDER_virtual/obmc-host-ipmi-hw = "phosphor-ipmi-kcs"
PREFERRED_PROVIDER_virtual/obmc-gpio-monitor = "phosphor-gpio-monitor"
