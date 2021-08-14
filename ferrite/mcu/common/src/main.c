#include <common/main.h>

#include <stdint.h>
#include <stdbool.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include <ipp.h>

#include <hal/assert.h>
#include <hal/io.h>
#include <hal/rpmsg.h>
#include <hal/spi.h>


#define TASK_STACK_SIZE 256

static void task_rpmsg(void *param) {
    hal_rpmsg_init();

    hal_rpmsg_channel channel;
    hal_assert(hal_rpmsg_create_channel(&channel, 0) == HAL_SUCCESS);
#ifdef HAL_PRINT_RPMSG
    hal_io_rpmsg_init(&channel);
#endif

    // Receive message

    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_rpmsg_recv_nocopy(&channel, &buffer, &len, HAL_WAIT_FOREVER);

    IppMsgAppAny app_msg;
    IppLoadStatus st = ipp_msg_app_load(&app_msg, buffer, len);
    hal_assert(IPP_LOAD_OK == st);
    if (IPP_APP_START == app_msg.type) {
        hal_log_info("Start message received");
    } else {
        hal_log_error("Message error: type mismatch: %d", (int)app_msg.type);
        hal_panic();
    }
    hal_rpmsg_free_rx_buffer(&channel, buffer);
    buffer = NULL;
    len = 0;

    hal_log_info("SPI init");
    hal_spi_init();
    hal_log_info("SPI enable");
    hal_assert(hal_spi_enable(0, 25000000) == HAL_SUCCESS);

    uint32_t spi_rx = 0x00000000;
    uint32_t spi_tx = 0xC0000000;
    hal_log_info("SPI Send: %x", spi_tx);
    hal_assert(hal_spi_xfer(0, (uint8_t*)&spi_tx, (uint8_t*)spi_rx, 4, HAL_WAIT_FOREVER) == HAL_SUCCESS);
    spi_tx = 0x00000000;
    hal_assert(hal_spi_xfer(0, (uint8_t*)&spi_tx, (uint8_t*)spi_rx, 4, HAL_WAIT_FOREVER) == HAL_SUCCESS);
    hal_log_info("SPI Received: %x", spi_rx);

    // Send message back
    hal_assert(HAL_SUCCESS == hal_rpmsg_alloc_tx_buffer(&channel, &buffer, &len, HAL_WAIT_FOREVER));
    IppMsgMcuAny mcu_msg = {
        .type = IPP_MCU_DEBUG,
        .debug = {
            .message = "Response message",
        },
    };
    ipp_msg_mcu_store(&mcu_msg, buffer);
    hal_assert(hal_rpmsg_send_nocopy(&channel, buffer, ipp_msg_mcu_len(&mcu_msg)) == HAL_SUCCESS);

    /* FIXME: Should never reach this point - otherwise virtio hangs */
    hal_log_error("End of task_rpmsg()");
    hal_panic();

    hal_assert(hal_rpmsg_destroy_channel(&channel) == HAL_SUCCESS);
    hal_assert(hal_spi_disable(0) == HAL_SUCCESS);
    
    hal_rpmsg_deinit();
    hal_spi_deinit();
}

int common_main() {
    /* Create tasks. */
    xTaskCreate(
        task_rpmsg, "RPMSG task",
        TASK_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL
    );

    /* Start FreeRTOS scheduler. */
    vTaskStartScheduler();

    /* Should never reach this point. */
    hal_log_error("End of main()");
    hal_panic();

    return 0;
}
