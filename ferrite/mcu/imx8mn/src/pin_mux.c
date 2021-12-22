/*
 * Copyright 2019-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
!!GlobalInfo
product: Pins v6.0
processor: MIMX8MN6xxxJZ
package_id: MIMX8MN6DVTJZ
mcu_data: ksdk2_0
processor_version: 0.0.0
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */

#include "fsl_common.h"
#include "fsl_iomuxc.h"
#include "pin_mux.h"

/* FUNCTION ************************************************************************************************************
 *
 * Function Name : BOARD_InitBootPins
 * Description   : Calls initialization functions.
 *
 * END ****************************************************************************************************************/
void BOARD_InitBootPins(void)
{
    BOARD_InitPins();
}

/*
 * TEXT BELOW IS USED AS SETTING FOR TOOLS *************************************
BOARD_InitPins:
- options: {callFromInitBoot: 'true', coreID: m7}
- pin_list:
  - {pin_num: E18, peripheral: UART3, signal: uart_rx, pin_signal: UART3_RXD, PE: Disabled, FSEL: FAST0, DSE: X6_0}
  - {pin_num: D18, peripheral: UART3, signal: uart_tx, pin_signal: UART3_TXD, PE: Disabled, FSEL: FAST0, DSE: X6_0}
  - {pin_num: A7, peripheral: ECSPI1, signal: ecspi_miso, pin_signal: ECSPI1_MISO, PE: Disabled, HYS: Enabled, FSEL: SLOW0, DSE: X6_0}
  - {pin_num: B7, peripheral: ECSPI1, signal: ecspi_mosi, pin_signal: ECSPI1_MOSI, PE: Disabled, HYS: Enabled, FSEL: SLOW0, DSE: X6_0}
  - {pin_num: D6, peripheral: ECSPI1, signal: ecspi_sclk, pin_signal: ECSPI1_SCLK, PE: Enabled, HYS: Enabled, FSEL: SLOW0, DSE: X6_0}
  - {pin_num: B6, peripheral: ECSPI1, signal: 'ecspi_ss, 0', pin_signal: ECSPI1_SS0, PE: Enabled, PUE: Enabled, SION: DISABLED, HYS: Enabled, ODE: Disabled, FSEL: SLOW0,
    DSE: X6_0}
 * BE CAREFUL MODIFYING THIS COMMENT - IT IS YAML SETTINGS FOR TOOLS ***********
 */

/* FUNCTION ************************************************************************************************************
 *
 * Function Name : BOARD_InitPins
 * Description   : Configures pin routing and optionally pin electrical features.
 *
 * END ****************************************************************************************************************/
void BOARD_InitPins(void) {                                /*!< Function assigned for the core: Cortex-M7F[m7] */
    // Debug UART pins
    IOMUXC_SetPinMux(IOMUXC_UART3_RXD_UART3_RX, 0U);
    IOMUXC_SetPinConfig(IOMUXC_UART3_RXD_UART3_RX,
                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
                        IOMUXC_SW_PAD_CTL_PAD_FSEL(2U));
    IOMUXC_SetPinMux(IOMUXC_UART3_TXD_UART3_TX, 0U);
    IOMUXC_SetPinConfig(IOMUXC_UART3_TXD_UART3_TX,
                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
                        IOMUXC_SW_PAD_CTL_PAD_FSEL(2U));

    // ECSPI pins
    IOMUXC_SetPinMux(IOMUXC_ECSPI1_MISO_ECSPI1_MISO, 0U);
    IOMUXC_SetPinConfig(IOMUXC_ECSPI1_MISO_ECSPI1_MISO,
                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
                        IOMUXC_SW_PAD_CTL_PAD_HYS_MASK);
    IOMUXC_SetPinMux(IOMUXC_ECSPI1_MOSI_ECSPI1_MOSI, 0U);
    IOMUXC_SetPinConfig(IOMUXC_ECSPI1_MOSI_ECSPI1_MOSI,
                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
                        IOMUXC_SW_PAD_CTL_PAD_HYS_MASK);
    IOMUXC_SetPinMux(IOMUXC_ECSPI1_SCLK_ECSPI1_SCLK, 0U);
    IOMUXC_SetPinConfig(IOMUXC_ECSPI1_SCLK_ECSPI1_SCLK,
                        IOMUXC_SW_PAD_CTL_PAD_DSE(6U) |
                        IOMUXC_SW_PAD_CTL_PAD_HYS_MASK |
                        IOMUXC_SW_PAD_CTL_PAD_PE_MASK);

}

void BOARD_InitGpioPins(void) {                                /*!< Function assigned for the core: Cortex-M7F[m7] */
    // GPIO pins
    IOMUXC_SetPinMux(IOMUXC_UART1_TXD_GPIO5_IO23, 0U); // SMP_RDY
    IOMUXC_SetPinMux(IOMUXC_ECSPI1_SS0_GPIO5_IO09, 0U); // Read_RDY

    // DAC keys
    IOMUXC_SetPinMux(IOMUXC_SAI3_MCLK_GPIO5_IO02, 0U);
    IOMUXC_SetPinMux(IOMUXC_SPDIF_TX_GPIO5_IO03, 0U);
}

void BOARD_InitGptPins(void) {
    // Internal Sync
    IOMUXC_SetPinMux(IOMUXC_SAI3_RXD_GPT1_COMPARE1, 0u); // IOMUXC_SAI3_RXD_GPIO4_IO30 // used by linux
    IOMUXC_SetPinMux(IOMUXC_SAI3_TXC_GPIO5_IO00, 0u); // IOMUXC_SAI3_TXC_GPT1_COMPARE2

    // IOMUXC_UART4_RXD_GPT1_COMPARE1 // IOMUXC_UART4_RXD_GPIO5_IO28
    // IOMUXC_UART2_TXD_GPT1_COMPARE2 // IOMUXC_UART2_TXD_GPIO5_IO25
    // IOMUXC_SAI3_RXD_GPT1_COMPARE1 // IOMUXC_SAI3_RXD_GPIO4_IO30
    // IOMUXC_SAI3_TXC_GPT1_COMPARE2 // IOMUXC_SAI3_TXC_GPIO5_IO00
}

/***********************************************************************************************************************
 * EOF
 **********************************************************************************************************************/
