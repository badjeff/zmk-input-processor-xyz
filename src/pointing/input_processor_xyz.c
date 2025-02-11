/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_xyz

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/keymap.h>

struct zip_xyz_config {
    uint8_t type;
    bool decompress;
    uint16_t as_code;
    size_t codes_len;
    uint16_t codes[];
};

struct zip_xyz_data {
    int16_t rmds[CONFIG_ZMK_INPUT_PROCESSOR_XYZ_CODES_MAX_LEN];
};

static int unpack_val(const struct device *dev, struct input_event *event) {

    const struct zip_xyz_config *cfg = dev->config;
    // struct zip_xyz_data *data = dev->data;

    uint32_t val = event->value;
    // LOG_HEXDUMP_DBG(&val, 4, "unpack");

    for (int i = cfg->codes_len - 1; i >= 0; i--) {
        // LOG_DBG("(%d) %d", i, val & 0xFFFF);
        input_report(event->dev, cfg->type, cfg->codes[i], val & 0xFFFF, i == 0, K_NO_WAIT);
        val = val >> 16;
    }

    return ZMK_INPUT_PROC_STOP;
}

static int pack_val(const struct device *dev, struct input_event *event, int code_idx) {

    const struct zip_xyz_config *cfg = dev->config;
    struct zip_xyz_data *data = dev->data;

    // deposite delta, stop processing
    data->rmds[code_idx] = (int16_t)event->value;
    if (!event->sync) {
        event->value = 0;
        return ZMK_INPUT_PROC_STOP;
    }

    uint32_t val = 0;
    for (int i = 0; i < cfg->codes_len; i++) {
        // LOG_DBG("[%d] %d", i, data->rmds[i]);
        val = (val << 16) | (data->rmds[i] & 0xFFFF);
        data->rmds[i] = 0;
    }

    event->code = cfg->as_code;
    event->value = val;
    // LOG_HEXDUMP_DBG(&val, 4, "pack");

    return ZMK_INPUT_PROC_CONTINUE;
}

static int zip_xyz_handle_event(const struct device *dev, struct input_event *event, 
                                uint32_t param1, uint32_t param2, 
                                struct zmk_input_processor_state *state) {

    const struct zip_xyz_config *cfg = dev->config;
    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    if (cfg->decompress) {
        if (event->code == cfg->as_code) {
            return unpack_val(dev, event);
        }
        return ZMK_INPUT_PROC_CONTINUE;
    }

    for (int i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] == event->code) {
            return pack_val(dev, event, i);
        }
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api sy_driver_api = {
    .handle_event = zip_xyz_handle_event,
};

static int zip_xyz_init(const struct device *dev) {
    // const struct zip_xyz_config *cfg = dev->config;
    struct zip_xyz_data *data = dev->data;

    for (int i = 0; i < CONFIG_ZMK_INPUT_PROCESSOR_XYZ_CODES_MAX_LEN; i++) {
        data->rmds[i] = 0;
    }

    return 0;
}

#define ZIP_XYZ_INST(n)                                                                        \
    BUILD_ASSERT(DT_INST_PROP_LEN(n, xy_codes)                                                 \
                 <= CONFIG_ZMK_INPUT_PROCESSOR_XYZ_CODES_MAX_LEN,                              \
                 "Codes length > CONFIG_ZMK_INPUT_PROCESSOR_XYZ_CODES_MAX_LEN");               \
    static struct zip_xyz_data data_##n = {};                                                  \
    static struct zip_xyz_config config_##n = {                                                \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                        \
        .decompress = DT_INST_PROP_OR(n, decompress, false),                                   \
        .as_code = DT_INST_PROP(n, z_code),                                                    \
        .codes_len = DT_INST_PROP_LEN(n, xy_codes),                                            \
        .codes = DT_INST_PROP(n, xy_codes),                                                    \
    };                                                                                         \
    DEVICE_DT_INST_DEFINE(n, &zip_xyz_init, NULL, &data_##n, &config_##n, POST_KERNEL,         \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &sy_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ZIP_XYZ_INST)
