#!/bin/bash
# SV305 BMC environment initialization

# Create a directory to store non-volatile SEL records
if [ ! -d /usr/share/sel ]; then
    mkdir -p /usr/share/sel
else
    echo "Directory /usr/share/sel exists."
fi

# Create ipmi_sel file at the beginning to avoid duplicate SEL record ID.
if [ ! -f "/usr/share/sel/ipmi_sel" ]; then
    touch /usr/share/sel/ipmi_sel
fi

# Create symbolic links to the non-volatile SEL records
if [ ! -L /var/log/ipmi_sel ]; then
    ln -s /usr/share/sel/ipmi_sel /var/log/ipmi_sel
fi

if [ ! -L /var/log/ipmi_sel.1 ]; then
    ln -s /usr/share/sel/ipmi_sel.1 /var/log/ipmi_sel.1
fi

if [ ! -L /var/log/ipmi_sel.2 ]; then
    ln -s /usr/share/sel/ipmi_sel.2 /var/log/ipmi_sel.2
fi

if [ ! -L /var/log/ipmi_sel.3 ]; then
    ln -s /usr/share/sel/ipmi_sel.3 /var/log/ipmi_sel.3
fi

# Set the System Clock from the Hardware Clock.
/sbin/hwclock -s
if [ $? -ne 0 ]; then
    echo "Failed to set system clock" | systemd-cat -t sv305-bmcenv-init.sh -p 3
fi

# Check BMC in primary or secondary
SCU10=0x1e785010
SCU30=0x1e785030
SCU50=0x1e785050
VALWDT1=$(devmem ${SCU10} 32)
VALWDT2=$(devmem ${SCU30} 32)
VALWDT3=$(devmem ${SCU50} 32)
if [ $((${VALWDT1} & 0x02)) -eq 2 ] || [ $((${VALWDT2} & 0x02)) -eq 2 ] || [ $((${VALWDT3} & 0x02)) -eq 2 ]; then
    mkdir -p /run/openbmc
    touch /run/openbmc/boot_from_backup
fi

# Check if SRAM records FAILOVER message
SRAMFS1=0x1e723008
SRAMFS2=0x1e72300c
FAILS1=$(devmem ${SRAMFS1} 32)
FAILS2=$(devmem ${SRAMFS2} 32)
FAILEDS1=0x53544152
FAILEDS21=0x545F32E1
FAILEDS22=0x545F46CD
if [ ${FAILS1} == ${FAILEDS1} ]; then
    if [ ${FAILS2} == ${FAILEDS21} ] || [ ${FAILS2} == ${FAILEDS22} ] ; then
        mkdir -p /run/openbmc
        touch /run/openbmc/fail_over_wdt_timeout
    fi
fi
/sbin/devmem 0x1e723008 32 0x0
/sbin/devmem 0x1e72300c 32 0x0
