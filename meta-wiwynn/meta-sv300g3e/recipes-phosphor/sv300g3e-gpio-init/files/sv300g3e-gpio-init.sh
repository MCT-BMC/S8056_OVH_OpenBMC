#!/bin/sh
# SV300G3-E GPIO initialization

# GROUP A
GPIO_A0=0   # O(H)
GPIO_A2=2   # I
GPIO_A3=3   # I
GPIO_A6=6   # O(L)
GPIO_A7=7   # I

# GROUP B
GPIO_B0=8   # O(L) - BMC_READY_N
GPIO_B1=9   # I
GPIO_B2=10  # I
GPIO_B3=11  # O(L)
GPIO_B4=12  # I
GPIO_B5=13  # I
GPIO_B6=14  # I
GPIO_B7=15  # I

# GROUP C
GPIO_C0=16  # O(L)
GPIO_C1=17  # I
GPIO_C2=18  # I
GPIO_C3=19  # O(H)
GPIO_C4=20  # I
GPIO_C5=21  # I
GPIO_C6=22  # I
GPIO_C7=23  # I

# GROUP D
GPIO_D0=24  # O(H)
GPIO_D1=25  # I
GPIO_D2=26  # I - BMC_PWRBTN_IN_N
GPIO_D3=27  # O(H) - BMC_PWRBTN_OUT_N
GPIO_D4=28  # I - BMC_RSTBTN_IN_N
GPIO_D5=29  # O(H) - BMC_RSTBTN_OUT_N
GPIO_D6=30  # I
GPIO_D7=31  # I

# GROUP E
GPIO_E0=32  # I
GPIO_E1=33  # I - PWRGD_PCH_BMC_PWROK
GPIO_E2=34  # I - PWRGD_SYS_BMC_PWROK
GPIO_E4=36  # I
GPIO_E5=37  # I
GPIO_E6=38  # I
GPIO_E7=39  # I

# GROUP F
GPIO_F0=40  # I
GPIO_F1=41  # I
GPIO_F2=42  # I
GPIO_F3=43  # I
GPIO_F4=44  # I
GPIO_F5=45  # O(H) 
GPIO_F6=46  # O(H)
GPIO_F7=47  # I

# GROUP G
GPIO_G0=48  # I
GPIO_G1=49  # I 
GPIO_G2=50  # I
GPIO_G3=51  # I
GPIO_G4=52  # O(L)
GPIO_G6=54  # I
GPIO_G7=55  # I

# GROUP H
GPIO_H0=56  # I
GPIO_H1=57  # I
GPIO_H2=58  # I
GPIO_H3=59  # I
GPIO_H4=60  # I
GPIO_H5=61  # I
GPIO_H6=62  # I
GPIO_H7=63  # I

# GROUP I
GPIO_I0=64  # I
GPIO_I1=65  # I

# GROUP J
GPIO_J1=73  # O(H)
GPIO_J2=74  # O(H)

# GROUP L
GPIO_L0=88  # I
GPIO_L2=90  # O(H)
GPIO_L3=91  # I
GPIO_L4=92  # I
GPIO_L5=93  # O(L)

# GROUP M
GPIO_M0=96  # O(H)
GPIO_M1=97  # I
GPIO_M2=98  # I
GPIO_M3=99  # I
GPIO_M4=100 # I
GPIO_M5=101 # I - PWRGD_PS_PWROK 
GPIO_M6=102 # I
GPIO_M7=103 # O(L)

# GROUP N
GPIO_N7=111 # I

# GROUP Q
GPIO_Q4=132 # I
GPIO_Q5=133 # I
GPIO_Q6=134 # O(H) - FP_PWR_ID_LED_N
GPIO_Q7=135 # I

# GROUP R
GPIO_R0=136 # I
GPIO_R1=137 # I
GPIO_R2=138 # I
GPIO_R3=139 # I
GPIO_R4=140 # I
GPIO_R5=141 # I

# GROUP S
GPIO_S2=146 # O(L)
GPIO_S3=147 # I
GPIO_S4=148 # I
GPIO_S6=150 # I
GPIO_S7=151 # I

# GROUP T
GPIO_T6=158 # I

# GROUP U
GPIO_U2=162 # I
GPIO_U3=163 # I

# GROUP X
GPIO_X1=185 # I
GPIO_X2=186 # I
GPIO_X3=187 # I
GPIO_X4=188 # I - ID_BTN_N

# GROUP Y
GPIO_Y0=192 # O(H)
GPIO_Y1=193 # O(H)
GPIO_Y2=194 # O(L)
GPIO_Y3=195 # O(H)

# GROUP Z
GPIO_Z4=204 # I
GPIO_Z5=205 # I
GPIO_Z6=206 # I
GPIO_Z7=207 # I

# GROUP AA
GPIO_AA0=208 # O(H) - BMC_LED_PWR_GRN
GPIO_AA1=209 # O(L) - BMC_LED_PWR_AMBER
GPIO_AA2=210 # I
GPIO_AA3=211 # I
GPIO_AA4=212 # I
GPIO_AA5=213 # O(L)
GPIO_AA6=214 # I
GPIO_AA7=215 # I

GPIO_SYSFS=/sys/class/gpio
GPIO_EXPORT=${GPIO_SYSFS}/export
GPIO_BASE=$(cat ${GPIO_SYSFS}/gpio*/base)

gpio_export() {
    local gpio=$(($1 + ${GPIO_BASE}))
    local gpio_path=${GPIO_SYSFS}/gpio${gpio}

    if [ ! -d "${gpio_path}" ]; then
        echo ${gpio} > ${GPIO_EXPORT}
    fi
}

gpio_set_direction() {
    local gpio=$(($1 + ${GPIO_BASE}))
    local dir_path=${GPIO_SYSFS}/gpio${gpio}/direction
    local dir=$2

    if [ -f "${dir_path}" ]; then
        echo ${dir} > ${dir_path}
    fi
}

gpio_set_value() {
    local gpio=$(($1 + ${GPIO_BASE}))
    local val_path=${GPIO_SYSFS}/gpio${gpio}/value
    local val=$2

    if [ -f "${val_path}" ]; then
        echo ${val} > ${val_path}
    fi
}

# direction: in/out
# value: 0/1

# Init GPIO to output high
gpio_out_high="\
    ${GPIO_A0} \
    ${GPIO_C3} \
    ${GPIO_D0} \
    ${GPIO_D3} \
    ${GPIO_D5} \
    ${GPIO_F5} \
    ${GPIO_F6} \
    ${GPIO_J1} \
    ${GPIO_J2} \
    ${GPIO_L2} \
    ${GPIO_M0} \
    ${GPIO_Q6} \
    ${GPIO_Y0} \
    ${GPIO_Y1} \
    ${GPIO_Y3} \
"
for i in ${gpio_out_high}
do
    gpioset gpiochip0 ${i}=1
    # gpio_export ${i}
    # gpio_set_direction ${i} out
    # gpio_set_value ${i} 1
done

# Init GPIO to output low
gpio_out_low="\
    ${GPIO_A6} \
    ${GPIO_B0} \
    ${GPIO_B3} \
    ${GPIO_C0} \
    ${GPIO_G4} \
    ${GPIO_L5} \
    ${GPIO_M7} \
    ${GPIO_S2} \
    ${GPIO_Y2} \
    ${GPIO_AA5} \
"
for i in ${gpio_out_low}
do
    gpioset gpiochip0 ${i}=0
    # gpio_export ${i}
    # gpio_set_direction ${i} out
    # gpio_set_value ${i} 0
done

# Init GPIO to input
gpio_in="\
    ${GPIO_E1} \
    ${GPIO_E2} \
    ${GPIO_M5} \
"
# for i in ${gpio_in}
# do
    # gpio_export ${i}
    # gpio_set_direction ${i} in
# done

exit 0
