#!/bin/sh
# SV305 GPIO initialization

# GROUP A
GPIO_A0=0   # O(H) - BMC_HDR_LVL_XLTR_OE_N
GPIO_A1=1   # I    - FP_ID_BTN_N
GPIO_A2=2   # O(H) - JTAG_BMC_OE_N
GPIO_A3=3   # O(H) - FM_BMC_BATTERY_SENSE_EN_N

# GROUP B
GPIO_B0=8   # O(H) - FM_PWRBRK_N
GPIO_B3=11  # O(L) - FM_USB_EN_BOOT_N_BMC
GPIO_B5=13  # I    - P0_LPC_PD_N
GPIO_B6=14  # I    - P0_LPC_PME_N

# GROUP C

# GROUP D
GPIO_D0=24  # I    - VR_VDD_MEM_ABCD_SUS_VRHOT_N
GPIO_D1=25  # I    - VR_VDD_MEM_EFGH_SUS_VRHOT_N
GPIO_D2=26  # O(L) - FM_BIOS_SPI_BMC_CTRL
GPIO_D3=27  # I    - NMI_BTN_N
GPIO_D5=29  # O(H) - RST_OCP_CARD_SMB_N

# GROUP E
GPIO_E0=32  # I    - FP_RST_BTN_BMC_IN_N
GPIO_E1=33  # O(H) - RST_BMC_RSTBTN_OUT_R_N
GPIO_E2=34  # I    - FM_BMC_PWRBTN_IN_N
GPIO_E3=35  # O(H) - FM_BMC_PWRBTN_OUT_R_N
GPIO_E5=37  # I    - BMC_SYS_MON_PWROK

# GROUP F
GPIO_F0=40  # O(H) - IRQ_BMC_PCH_NMI_N
GPIO_F2=42  # I    - P0_THERMTRIP_LVT3_PLD_N
GPIO_F3=43  # I    - P0_RSMRST_N

# GROUP G
GPIO_G1=49  # I    - FM_UART_SWITCH_N
GPIO_G3=51  # O(L) - MGMT_FPGA_RSVD_R
GPIO_G4=52  # I    - P0_APML_ALERT_N

# GROUP H
GPIO_H0=56  # I    - VDD_MEM_ABCD_SUS_FAULT
GPIO_H1=57  # I    - VDD_MEM_EFGH_SUS_FAULT
GPIO_H2=58  # I    - VDD_SOC_RUN_VRHOT_N
GPIO_H7=63  # O(L) - BMC_LED_PWR_GRN

# GROUP I
GPIO_I0=64  # O(H) - FM_BIOS_MRC_DEBUG_MSG_DIS_N
GPIO_I1=65  # O(H) - I2C_SWITCH1_RESET_N
GPIO_I3=67  # O(H) - I2C_SWITCH2_RESET_N

# GROUP J
GPIO_J0=72  # O(H) - FM_ME_RECOVERY_BMC_N_R
GPIO_J3=75  # O(H) - EN_ADC_P1V5_RTC

# GROUP L
GPIO_L1=89  # I    - VDD_SOC_RUN_FAULT
GPIO_L3=91  # I    - RST_PLTRST_PLD_B_N
GPIO_L4=92  # O(L) - LPC_ENABLE_ESPI_N

# GROUP M
GPIO_M0=96  # I    - FM_BIOS_POST_CMPLT_N
GPIO_M2=98  # I    - P0_CPLD_PRESENT_N
GPIO_M3=99  # O(L) - MGMT_ASSERT_LOCAL_LOCK
GPIO_M4=100 # O(L) - MGMT_ASSERT_P0_PROCHOT
GPIO_M5=101 # O(L) - BMC_LED_PWR_AMBER

# GROUP N
GPIO_N6=110 # I    - VDD_CORE_RUN_VRHOT_N
GPIO_N7=111 # I    - P0_PROCHOT_BMC_N

# GROUP P
GPIO_P7=127 # O(H) - FP_PWR_ID_LED_N

# GROUP Q
GPIO_Q7=135 # O(H) - SP3_PCIE_WAKE_N

# GROUP R
GPIO_R0=136 # O(H) - MGMT_ROM_SPI_CS1_N_R
GPIO_R1=137 # I    - VDD_CORE_RUN_FAULT
GPIO_R2=138 # I    - VOLTAGE_ALERT_R_N
GPIO_R3=139 # O(H) - MGMT_SMB_MUX1_SEL
GPIO_R4=140 # O(H) - MGMT_ROM_SPI_WP_N
GPIO_R5=141 # I    - FP_PRSNT_N

# GROUP S
GPIO_S1=145 # O(L) - BMC_HDT_PWROK
GPIO_S3=147 # I    - I2C_VR_ALERT_N
GPIO_S4=148 # O(L) - BMC_JTAG_SEL
GPIO_S7=151 # O(H) - BMC_SPD_SMBUS_SEL

# GROUP T
GPIO_T6=158 # O(H) - MGMT_SLI_SMB_MUX5_SEL

# GROUP U
GPIO_U2=162 # I
GPIO_U3=163 # I

# GROUP X

# GROUP Y
GPIO_Y1=193 # I    - P0_SLP_S5_PLD_N

# GROUP Z
GPIO_Z0=200 # I    - MGMT_P0_PWROK
GPIO_Z2=202 # O(L) - MGMT_ASSERT_CLR_CMOS
GPIO_Z3=203 # O(H) - FM_BMC_LPC_SCI_N
GPIO_Z4=204 # I
GPIO_Z5=205 # O(H) - BMC_HDT_DBREQ_N
GPIO_Z6=206 # O(H) - MGMT_ASSERT_BMC_READY
GPIO_Z7=207 # O(H) - IRQ_SMI_ACTIVE_BMC_N

# GROUP AA
GPIO_AA0=208 # I   - P0_SMB_M2_1_ALERT_N
GPIO_AA3=211 # I   - SENSOR1_THERM_N
GPIO_AA4=212 # I   - SENSOR2_THERM_N
GPIO_AA5=213 # I   - IRQ_TPM_SPI_N
GPIO_AA6=214 # I   - ATX_PWR_OK
GPIO_AA7=215 # I   - IRQ_SML1_PMBUS_ALERT_N

# GROUP AB
GPIO_AB0=216 # I   - USB_DB_PRSNT_N
GPIO_AB1=217 # I   - P0_NVDIMM_EVENT_N
GPIO_AB2=218 # I   - P3V3_NIC_FAULT_N
GPIO_AB3=219 # I   - P12V_NIC_FAULT_N

# GROUP AC
GPIO_AC7=227 # I   - MGMT_LPC_RST_N

# Init GPIO to output high
gpio_out_high="\
    ${GPIO_A0} \
    ${GPIO_A2} \
    ${GPIO_A3} \
    ${GPIO_B0} \
    ${GPIO_D5} \
    ${GPIO_E1} \
    ${GPIO_E3} \
    ${GPIO_F0} \
    ${GPIO_I0} \
    ${GPIO_I1} \
    ${GPIO_I3} \
    ${GPIO_J0} \
    ${GPIO_Q7} \
    ${GPIO_R3} \
    ${GPIO_R4} \
    ${GPIO_S7} \
    ${GPIO_T6} \
    ${GPIO_Z3} \
    ${GPIO_Z5} \
    ${GPIO_Z6} \
    ${GPIO_Z7} \
"
for i in ${gpio_out_high}
do
    gpioset gpiochip0 ${i}=1
done

# Init GPIO to output low
gpio_out_low="\
    ${GPIO_B3} \
    ${GPIO_D2} \
    ${GPIO_G3} \
    ${GPIO_J3} \
    ${GPIO_L4} \
    ${GPIO_M3} \
    ${GPIO_M4} \
    ${GPIO_S1} \
    ${GPIO_S4} \
    ${GPIO_Z2} \
"
for i in ${gpio_out_low}
do
    gpioset gpiochip0 ${i}=0
done

POSTComplete=`busctl get-property org.openbmc.control.Power /org/openbmc/control/power0 org.openbmc.control.PostComplete postcomplete`
if [[ $POSTComplete == "i 0" ]]; then
    /usr/bin/gpioset gpiochip0 $GPIO_S7=0
    for retry in {1..4};
    do
        if [ -d "/sys/bus/i2c/drivers/pca954x/9-0070" ]; then
            break
        else
            /bin/sh -c '/bin/echo "9-0070" > /sys/bus/i2c/drivers/pca954x/bind'
        fi
    done
fi

# Setting Status Input Mask Alert to block using SMBALERT_MASK command(0x1B)
/usr/sbin/i2ctransfer -f -y 7 w7@0x58 0x5 0x4 0x1 0x1b 0x7c 0xff 0xf

exit 0
