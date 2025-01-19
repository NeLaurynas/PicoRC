// Example file - Public Domain
// Need help? https://tinyurl.com/bluepad32-help

#include <stddef.h>
#include <string.h>

#include <pico/cyw43_arch.h>
#include <pico/time.h>
#include <uni.h>

#include "sdkconfig.h"

// Sanity check
#ifndef CONFIG_BLUEPAD32_PLATFORM_CUSTOM
#error "Pico W must use BLUEPAD32_PLATFORM_CUSTOM"
#endif

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t* d);

//
// Platform Overrides
//
static void rc_platform_init(int argc, const char** argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    logi("rc_platform: init()\n");

    uni_gamepad_mappings_t mappings = GAMEPAD_DEFAULT_MAPPINGS;

    // Inverted axis with inverted Y in RY.
    // mappings.axis_x = UNI_GAMEPAD_MAPPINGS_AXIS_RX;
    // mappings.axis_y = UNI_GAMEPAD_MAPPINGS_AXIS_RY;
    mappings.axis_ry_inverted = true;
	mappings.axis_y_inverted = true;
    // mappings.axis_rx = UNI_GAMEPAD_MAPPINGS_AXIS_X;
    // mappings.axis_ry = UNI_GAMEPAD_MAPPINGS_AXIS_Y;

    // Invert A & B
    // mappings.button_a = UNI_GAMEPAD_MAPPINGS_BUTTON_B;
    // mappings.button_b = UNI_GAMEPAD_MAPPINGS_BUTTON_A;

    uni_gamepad_set_mappings(&mappings);
}

static void rc_platform_on_init_complete(void) {
    logi("rc_platform: on_init_complete()\n");

    // Start scanning and autoconnect to supported controllers.
    uni_bt_start_scanning_and_autoconnect_unsafe();

    // Based on runtime condition, you can delete or list the stored BT keys.
    if (0)
        uni_bt_del_keys_unsafe();
    else
        uni_bt_list_keys_unsafe();

    // Turn off LED once init is done.
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // uni_property_dump_all();
}

static uni_error_t rc_platform_on_device_discovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    // You can filter discovered devices here. Return any value different from UNI_ERROR_SUCCESS;
    // @param addr: the Bluetooth address
    // @param name: could be NULL, could be zero-length, or might contain the name.
    // @param cod: Class of Device. See "uni_bt_defines.h" for possible values.
    // @param rssi: Received Signal Strength Indicator (RSSI) measured in dBms. The higher (255) the better.

    // As an example, if you want to filter out keyboards, do:
    if (((cod & UNI_BT_COD_MINOR_MASK) & UNI_BT_COD_MINOR_KEYBOARD) == UNI_BT_COD_MINOR_KEYBOARD) {
        logi("Ignoring keyboard\n");
        return UNI_ERROR_IGNORE_DEVICE;
    }

    return UNI_ERROR_SUCCESS;
}

static void rc_platform_on_device_connected(uni_hid_device_t* d) {
    logi("STOP!!!! rc_platform: device connected: %p\n", d);
    uni_bt_stop_scanning_unsafe();
}

static void rc_platform_on_device_disconnected(uni_hid_device_t* d) {
    logi("START!!!! rc_platform: device disconnected: %p\n", d);
    uni_bt_start_scanning_and_autoconnect_unsafe();
}

static uni_error_t rc_platform_on_device_ready(uni_hid_device_t* d) {
    logi("rc_platform: device ready: %p\n", d);

    // You can reject the connection by returning an error.
    return UNI_ERROR_SUCCESS;
}

static void rc_platform_on_controller_data(uni_hid_device_t* d, uni_controller_t* ctl) {
    static uint8_t leds = 0;
    static uint8_t enabled = true;
    static uni_controller_t prev = {0};
    uni_gamepad_t* gp;

    prev = *ctl;
    // logi("(%p) id=%d ", d, uni_hid_device_get_idx_for_instance(d));
    // uni_controller_dump(ctl);

    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            gp = &ctl->gamepad;

            // Debugging
            // Axis ry: control rumble
            if ((gp->buttons & BUTTON_A) && d->report_parser.play_dual_rumble != NULL) {
				logi("A");
            	d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 250 /* duration ms */,
                                                  128 /* weak magnitude */, 0 /* strong magnitude */);
            }

            if ((gp->buttons & BUTTON_B) && d->report_parser.play_dual_rumble != NULL) {
            	logi("B");
            	d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 250 /* duration ms */,
                                                  0 /* weak magnitude */, 128 /* strong magnitude */);
            }
            // Buttons: Control LEDs On/Off
            if ((gp->buttons & BUTTON_X)) {
            	logi("X");
            }
            // Axis: control RGB color
            if ((gp->buttons & BUTTON_Y)) {
                logi("Y");
            }

            // Toggle Bluetooth connections
            if ((gp->buttons & BUTTON_SHOULDER_L)) {
                logi("BUTTON_SHOULDER_L");
            }
            if ((gp->buttons & BUTTON_SHOULDER_R)) {
            	logi("BUTTON_SHOULDER_R");
            }
    		logi("\nX: %d, Y: %d\n", gp->axis_x, gp->axis_y);
    		logi("R - X: %d, Y: %d\n", gp->axis_rx, gp->axis_ry);
            break;
        default:
            loge("Unsupported controller class: %d\n", ctl->klass);
            break;
    }
}

static const uni_property_t* rc_platform_get_property(uni_property_idx_t idx) {
    ARG_UNUSED(idx);
    return NULL;
}

static void rc_platform_on_oob_event(uni_platform_oob_event_t event, void* data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON:
            // Optional: do something when "system" button gets pressed.
            trigger_event_on_gamepad((uni_hid_device_t*)data);
            break;

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            // When the "bt scanning" is on / off. Could be triggered by different events
            // Useful to notify the user
            logi("rc_platform_on_oob_event: Bluetooth enabled: %d\n", (bool)(data));
            break;

        default:
            logi("rc_platform_on_oob_event: unsupported event: 0x%04x\n", event);
    }
}

//
// Helpers
//
static void trigger_event_on_gamepad(uni_hid_device_t* d) {
    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 50 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        static uint8_t led = 0;
        led += 1;
        led &= 0xf;
        d->report_parser.set_player_leds(d, led);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        static uint8_t red = 0x10;
        static uint8_t green = 0x20;
        static uint8_t blue = 0x40;

        red += 0x10;
        green -= 0x20;
        blue += 0x40;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

struct uni_platform* get_rc_platform(void) {
    static struct uni_platform plat = {
        .name = "My Platform",
        .init = rc_platform_init,
        .on_init_complete = rc_platform_on_init_complete,
        .on_device_discovered = rc_platform_on_device_discovered,
        .on_device_connected = rc_platform_on_device_connected,
        .on_device_disconnected = rc_platform_on_device_disconnected,
        .on_device_ready = rc_platform_on_device_ready,
        .on_oob_event = rc_platform_on_oob_event,
        .on_controller_data = rc_platform_on_controller_data,
        .get_property = rc_platform_get_property,
    };

    return &plat;
}
