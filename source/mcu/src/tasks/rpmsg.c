#include "rpmsg.h"

#include <hal/assert.h>


void rpmsg_init(Rpmsg *self, Control *control, Statistics *stats) {
    self->alive = false;

    self->send_sem = xSemaphoreCreateBinary();
    hal_assert(self->send_sem != NULL);

    control_sync_init(&self->control_sync, &self->send_sem, DAC_MSG_MAX_POINTS, ADC_MSG_MAX_POINTS);
    control_set_sync(control, &self->control_sync);
    self->control = control;

    self->stats = stats;
}

void rpmsg_deinit(Rpmsg *self) {
    vSemaphoreDelete(self->send_sem);
}

static hal_retcode rpmsg_recv_message(
    Rpmsg *self,
    void (*read_message)(Rpmsg *, void *, const IppAppMsg *),
    void *user_data,
    bool check,
    uint32_t timeout //
) {
    uint8_t *buffer = NULL;
    size_t len = 0;

    // Wait for message
    {
        hal_retcode ret;
        ret = hal_rpmsg_recv_nocopy(&self->channel, &buffer, &len, timeout);
        if (ret == HAL_TIMED_OUT) {
            return HAL_TIMED_OUT;
        }
        hal_assert_retcode(ret);
    }

    // Handle message
    const IppAppMsg *message = (const IppAppMsg *)buffer;
    if (check) {
        size_t msg_size = ipp_app_msg_size(message);
        hal_assert(msg_size == len);
    }
    read_message(self, user_data, message);

    // Free received buffer
    hal_assert_retcode(hal_rpmsg_free_rx_buffer(&self->channel, buffer));

    return HAL_SUCCESS;
}

static void rpmsg_send_message(Rpmsg *self, void (*write_message)(Rpmsg *, void *, IppMcuMsg *), void *user_data) {
    uint8_t *buffer = NULL;
    size_t len = 0;
    hal_assert_retcode(hal_rpmsg_alloc_tx_buffer(&self->channel, &buffer, &len, HAL_WAIT_FOREVER));

    IppMcuMsg *message = (IppMcuMsg *)buffer;
    write_message(self, user_data, message);
    size_t msg_size = ipp_mcu_msg_size(message);
    hal_assert(msg_size <= len);

    hal_assert_retcode(hal_rpmsg_send_nocopy(&self->channel, buffer, msg_size));
}

static void write_adc_message(Rpmsg *self, void *user_data, IppMcuMsg *basic_message) {
    size_t index = *(const size_t *)user_data;
    static const size_t SIZE = ADC_MSG_MAX_POINTS;

    basic_message->type = IPP_MCU_MSG_ADC_DATA;
    IppMcuMsgAdcData *message = &basic_message->adc_data;
    message->index = (uint8_t)index;
    message->points.len = (uint16_t)SIZE;
    // It must be guaranteed that ADC buffer contains at least `ADC_MSG_MAX_POINTS` points.
    hal_assert(rb_read(&self->control->adc.buffers[index], message->points.data, SIZE) == SIZE);
}

static void rpmsg_send_adcs(Rpmsg *self) {
    for (size_t i = 0; i < ADC_COUNT; ++i) {
        RingBuffer *rb = &self->control->adc.buffers[i];
        while (rb_occupied(rb) >= ADC_MSG_MAX_POINTS) {
            rpmsg_send_message(self, write_adc_message, (void *)&i);
        }
    }
}

static void rpmsg_discard_adcs(Rpmsg *self) {
    static const size_t SIZE = ADC_MSG_MAX_POINTS;
    for (size_t i = 0; i < ADC_COUNT; ++i) {
        RingBuffer *rb = &self->control->adc.buffers[i];
        while (rb_occupied(rb) >= SIZE) {
            hal_assert(rb_skip(rb, SIZE) == SIZE);
        }
    }
}

static void write_dac_req_message(Rpmsg *self, void *user_data, IppMcuMsg *message) {
    size_t count = *(const size_t *)user_data;
    message->type = IPP_MCU_MSG_DAC_REQUEST;
    message->dac_request.count = count;
}

static void rpmsg_send_dac_request(Rpmsg *self) {
    static const size_t SIZE = DAC_MSG_MAX_POINTS;

    size_t vacant = rb_vacant(&self->control->dac.buffer);
    hal_assert(self->dac_requested <= vacant);

    size_t req_count_raw = vacant - self->dac_requested;

    if (req_count_raw >= SIZE) {
        // Request number of points that is multiple of `DAC_MSG_MAX_POINTS`.
        size_t req_count = (req_count_raw / SIZE) * SIZE;
        rpmsg_send_message(self, write_dac_req_message, (void *)&req_count);
        /// FIXME: Use atomic.
        self->dac_requested += req_count;
    }
}

static void write_din_message(Rpmsg *self, void *user_data, IppMcuMsg *basic_message) {
    SkifioDin din = *(const SkifioDin *)user_data;
    basic_message->type = IPP_MCU_MSG_DIN_UPDATE;
    basic_message->din_update.value = din;
}

static void rpmsg_send_din(Rpmsg *self) {
    rpmsg_send_message(self, write_din_message, (void *)&self->control->dio.in);
}

static void rpmsg_send_task(void *param) {
    Rpmsg *self = (Rpmsg *)param;

    for (;;) {
        if (xSemaphoreTake(self->send_sem, 10000) != pdTRUE) {
            hal_log_warn("RPMSG send task timed out");
            continue;
        }

        if (self->alive) {
            rpmsg_send_din(self);
            rpmsg_send_adcs(self);
            rpmsg_send_dac_request(self);
        } else {
            rpmsg_discard_adcs(self);
        }
    }
}

static void read_hello_world(Rpmsg *self, void *user_data, const IppAppMsg *str) {
    hal_assert(strncmp((const char *)str, "hello world!", 13) == 0);
}

static void connect(Rpmsg *self) {
    self->dac_requested = 0;
    control_dac_start(self->control);
    self->alive = true;
    xSemaphoreGive(self->send_sem);
    hal_log_info("IOC connected");
}

static void disconnect(Rpmsg *self) {
    self->alive = false;
    control_dac_stop(self->control);
    hal_log_info("IOC disconnected");
}

static void set_dout(Rpmsg *self, SkifioDout value) {
    SkifioDout mask = (SkifioDout)((1 << SKIFIO_DOUT_SIZE) - 1);
    if ((~mask & value) != 0) {
        hal_log_warn("Dout is out of bounds: %lx", (uint32_t)value);
    }
    self->control->dio.out = value & mask;
    self->control_sync.dout_changed = true;
}

static void write_dac(Rpmsg *self, const point_t *data, size_t len) {
    /// FIXME: Use atomic.
    self->dac_requested -= len;
    size_t wlen = rb_write(&self->control->dac.buffer, data, len);
    self->stats->dac.lost_full += len - wlen;
}

static void check_alive(Rpmsg *self) {
    if (!self->alive) {
        hal_log_warn("RPMSG connection is not alive");
    }
}

static void read_any_message(Rpmsg *self, void *user_data, const IppAppMsg *message) {
    switch (message->type) {
    case IPP_APP_MSG_CONNECT: {
        connect(self);
        break;
    }
    case IPP_APP_MSG_KEEP_ALIVE: {
        check_alive(self);
        break;
    }
    case IPP_APP_MSG_DOUT_UPDATE: {
        check_alive(self);
        set_dout(self, message->dout_update.value);
        break;
    }
    case IPP_APP_MSG_DAC_DATA: {
        check_alive(self);
        const IppAppMsgDacData *dac_msg = &message->dac_data;
        write_dac(self, dac_msg->points.data, (size_t)dac_msg->points.len);
        break;
    }
    default:
        hal_log_error("Wrong message type: %ld", (uint32_t)message->type);
        break;
    }
}

static void rpmsg_recv_task(void *param) {
    Rpmsg *self = (Rpmsg *)param;

    hal_rpmsg_init();

    hal_assert_retcode(hal_rpmsg_create_channel(&self->channel, 0));
#ifdef HAL_PRINT_RPMSG
    hal_io_rpmsg_init(&RPMSG_CHANNEL);
#endif
    hal_log_info("RPMSG channel created");

    // Receive `hello world!` message
    rpmsg_recv_message(self, read_hello_world, NULL, false, HAL_WAIT_FOREVER);
    hal_log_info("`hello world!` received");

    for (;;) {
        // Receive message
        hal_retcode ret = rpmsg_recv_message(self, read_any_message, NULL, true, KEEP_ALIVE_MAX_DELAY_MS);
        if (ret == HAL_TIMED_OUT) {
            if (self->alive) {
                hal_log_error("Keep-alive timeout reached. RPMSG connection is considered to be dead.");
                disconnect(self);
            }
        } else {
            hal_assert_retcode(ret);
        }
    }

    hal_unreachable();

    // FIXME: Virtio hangs on channel destroy.
    hal_assert_retcode(hal_rpmsg_destroy_channel(&self->channel));
    hal_rpmsg_deinit();
}

void rpmsg_run(Rpmsg *self) {
    hal_assert(
        xTaskCreate(rpmsg_send_task, "rpmsg_send", TASK_STACK_SIZE, (void *)self, RPMSG_SEND_TASK_PRIORITY, NULL) == pdPASS);

    hal_assert(
        xTaskCreate(rpmsg_recv_task, "rpmsg_recv", TASK_STACK_SIZE, (void *)self, RPMSG_RECV_TASK_PRIORITY, NULL) == pdPASS);
}
