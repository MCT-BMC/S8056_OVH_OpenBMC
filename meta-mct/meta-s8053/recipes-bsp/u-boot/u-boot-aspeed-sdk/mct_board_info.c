// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C)  MiTAC Computing Technology Corp.
 * John Chung <john.chung@mic.com.tw>
 */

#include <common.h>
#include <errno.h>
#include <asm/io.h>
#include <asm/arch/mct_board_info.h>

#define SCU_BASE    0x1e6e2000
#define FMC_BASE    0x1e620000

void unlock_scu(void)
{
    printf("Unlocking SCU...\n");
    writel(0x1688A8A8, SCU_BASE);
}

void enable_p2a_bridge(void)
{
    u32 reg;
    printf("Enabling P2A bridge...\n");
    reg = (readl(SCU_BASE + 0xC8) & (~0x00000001));
    writel(reg, (SCU_BASE + 0xC8));
}

void enable_button_passthrough(void)
{
    u32 reg;

    printf("Enable power/reset button passthrough...\n");
    reg = readl(SCU_BASE + 0x4BC);
    reg |= 0x0F000000;
    writel(reg, (SCU_BASE + 0x4BC));
}

void disable_hwstrap_button_passthrough(void)
{
    u32 reg;
    printf("Disabling HW strap button passthrough...\n");
    reg = (readl(SCU_BASE + 0x51C) & (~0x00000200));
    writel(reg, (SCU_BASE + 0x51C));
}

void enable_Heartbeat_led_hw_mode(void)
{
    u32 reg;
    printf("Enable heart beat LED hardware mode\n");
    reg = readl(SCU_BASE + 0x69C);
    reg |= 0x80000000;
    writel(reg, (SCU_BASE + 0x69C));
}

void enable_boot_SPI_ABR(void)
{
    u32 reg;
    printf("Enable boot SPI ABR...\n");
    reg = readl(SCU_BASE + 0x510);
    reg |= 0x00000020;
    writel(reg, (SCU_BASE + 0x510));

    printf("Enable FMC_WDT2 and set timer to 360s ...\n");
    writel(0xE10, (FMC_BASE + 0x68));
    writel(0x4755, (FMC_BASE + 0x6C));
    reg = readl(FMC_BASE + 0x64);
    reg |= 0x00000001;
    writel(reg, (FMC_BASE + 0x64));

}

void mct_print_board_info(void)
{
    unlock_scu();
    enable_p2a_bridge();
    enable_button_passthrough();
    disable_hwstrap_button_passthrough();
    enable_Heartbeat_led_hw_mode();
    enable_boot_SPI_ABR();
}