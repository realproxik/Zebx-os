// ============================================================================
// ZEBX OS - REAL HARDWARE INPUT SYSTEM
// USB HID, Direct GPIO, Raw Polling, Interrupt-driven
// Supports: Gamepads, Keyboards, Mice, Joysticks, Wheels, Flight Sticks
// ============================================================================

#ifndef INPUT_H
#define INPUT_H

#include "types.h"
#include "usb.h"

// ============================================================================
// INPUT DEVICE TYPES
// ============================================================================

typedef enum {
    INPUT_TYPE_KEYBOARD,
    INPUT_TYPE_MOUSE,
    INPUT_TYPE_GAMEPAD,
    INPUT_TYPE_JOYSTICK,
    INPUT_TYPE_RACING_WHEEL,
    INPUT_TYPE_FLIGHT_STICK,
    INPUT_TYPE_DANCE_PAD,
    INPUT_TYPE_GUN,
    INPUT_TYPE_VR_CONTROLLER,
    INPUT_TYPE_TOUCHPAD,
    INPUT_TYPE_TABLET,
    INPUT_TYPE_ARCADE_STICK,
    INPUT_TYPE_GUITAR,
    INPUT_TYPE_DRUMS,
    INPUT_TYPE_DJ_TURNTABLE,
    INPUT_TYPE_STEERING_WHEEL_FORCE_FEEDBACK,
    INPUT_TYPE_CUSTOM_HID,
} input_device_type_t;

// ============================================================================
// USB VENDOR IDs (Gaming Controllers)
// ============================================================================

#define USB_VID_SONY          0x054C  // PlayStation controllers
#define USB_VID_MICROSOFT     0x045E  // Xbox controllers
#define USB_VID_NINTENDO      0x057E  // Switch Pro, Wii U
#define USB_VID_LOGITECH      0x046D  // Logitech wheels, gamepads
#define USB_VID_RAZER         0x1532  // Razer controllers
#define USB_VID_STEELSERIES   0x1038  // SteelSeries
#define USB_VID_THRUSTMASTER  0x044F  // Thrustmaster wheels
#define USB_VID_FANATEC       0x0EB7  // Fanatec racing
#define USB_VID_8BITDO        0x2DC8  // 8BitDo retro
#define USB_VID_BROOK         0x0C12  // Brook adapters
#define USB_VID_MAYFLASH      0x0079  // Mayflash adapters
#define USB_VID_HID_GENERIC   0x0000  // Generic HID

// Specific Product IDs
#define USB_PID_DUALSHOCK4    0x05C4
#define USB_PID_DUALSHOCK4_V2 0x09CC
#define USB_PID_DUALSENSE     0x0CE6  // PS5
#define USB_PID_XBOX360_WIRED 0x028E
#define USB_PID_XBOX360_WIRELESS 0x0719
#define USB_PID_XBOXONE       0x02D1
#define USB_PID_XBOXONE_S     0x02DD
#define USB_PID_XBOX_SERIES   0x0B12
#define USB_PID_SWITCH_PRO    0x2009
#define USB_PID_WII_U_GC      0x0337  // GameCube adapter

// ============================================================================
// INPUT CAPABILITIES
// ============================================================================

typedef struct {
    // Digital buttons
    uint32_t num_buttons;
    uint32_t button_map[32];  // HID usage to logical button
    
    // Analog axes
    uint32_t num_axes;
    uint32_t axis_map[16];    // HID usage to logical axis
    
    // Hats/D-pads
    uint32_t num_hats;
    
    // Force feedback
    int has_rumble;
    int has_force_feedback;
    uint32_t rumble_motors;
    float max_rumble_force;
    
    // LEDs
    int has_leds;
    uint32_t num_leds;
    
    // Audio
    int has_speaker;
    int has_headset;
    
    // Touch/Sensors
    int has_touchpad;
    int has_gyro;
    int has_accelerometer;
    int has_barometer;  // PS5 DualSense
    
    // Battery
    int has_battery;
    
    // Special features
    int has_paddles;      // Elite controller back buttons
    int has_analog_triggers;  // L2/R2 with pressure
    int has_haptic;       // Advanced haptics (DualSense)
    
} input_capabilities_t;

// ============================================================================
// INPUT STATE
// ============================================================================

// Button state with timestamps for debouncing
typedef struct {
    int pressed;           // Currently down
    int was_pressed;       // Previous frame
    int just_pressed;      // Rising edge
    int just_released;     // Falling edge
    uint64_t press_time;   // When pressed (ms)
    uint64_t release_time; // When released (ms)
    float pressure;        // Analog pressure (0-1)
} input_button_t;

// Axis with smoothing
typedef struct {
    float value;           // Current value (-1 to 1)
    float raw_value;       // Unfiltered
    float deadzone;        // Deadzone radius
    float sensitivity;     // Multiplier
    float smoothing;       // EMA factor (0-1)
    float min_val;         // Calibrated min
    float max_val;         // Calibrated max
    int inverted;          // Invert axis
} input_axis_t;

// Hat/D-pad
typedef struct {
    int angle;             // -1 = center, 0-360 degrees
    int x;                 // -1, 0, 1
    int y;                 // -1, 0, 1
} input_hat_t;

// Touch point
typedef struct {
    int active;
    float x, y;            // Normalized 0-1
    float pressure;
    uint32_t id;
} input_touch_t;

// Gyroscope/IMU
typedef struct {
    float pitch;           // X rotation
    float yaw;             // Y rotation
    float roll;            // Z rotation
    float accel_x, accel_y, accel_z;
} input_imu_t;

// Force feedback state
typedef struct {
    float left_motor;      // 0-1 intensity
    float right_motor;
    float left_trigger;    // DualSense adaptive trigger
    float right_trigger;
    uint32_t duration_ms;
    uint32_t waveform;     // Sine, square, sawtooth, etc.
} input_rumble_t;

// LED state
typedef struct {
    uint8_t r, g, b;
    uint32_t brightness;   // 0-255
    uint32_t pattern;      // Solid, blink, breathe, etc.
    uint32_t speed;        // Blink/breathe speed
} input_led_t;

// Battery
typedef struct {
    int present;
    int charging;
    int level;             // 0-100%
    uint32_t voltage;      // mV
    uint32_t time_remaining; // minutes
} input_battery_t;

// ============================================================================
// INPUT DEVICE
// ============================================================================

typedef struct {
    uint32_t id;
    char name[64];
    input_device_type_t type;
    
    // USB info
    uint32_t usb_vendor;
    uint32_t usb_product;
    uint8_t usb_interface;
    uint8_t usb_endpoint_in;
    uint8_t usb_endpoint_out;
    uint32_t usb_packet_size;
    
    // HID report
    uint8_t hid_report_desc[512];
    uint32_t hid_report_size;
    uint8_t hid_input_report[64];
    uint8_t hid_output_report[64];
    uint32_t hid_input_size;
    uint32_t hid_output_size;
    
    // State
    input_capabilities_t caps;
    
    input_button_t buttons[32];
    input_axis_t axes[16];
    input_hat_t hats[4];
    input_touch_t touches[10];      // Multi-touch
    input_imu_t imu;
    input_rumble_t rumble;
    input_led_t led;
    input_battery_t battery;
    
    // Raw HID data
    uint8_t raw_data[64];
    uint32_t raw_size;
    
    // Callbacks
    void (*on_button)(struct input_device *dev, uint32_t button, int pressed);
    void (*on_axis)(struct input_device *dev, uint32_t axis, float value);
    void (*on_connect)(struct input_device *dev);
    void (*on_disconnect)(struct input_device *dev);
    
    // Polling
    uint32_t poll_rate_hz;       // 125, 250, 500, 1000 Hz
    uint64_t last_poll_time;
    uint64_t last_packet_time;
    
    // Connected?
    int connected;
    int initialized;
    int claimed;                 // USB interface claimed
    
    // User data
    void *user_data;
    
} input_device_t;

// ============================================================================
// INPUT SYSTEM STATE
// ============================================================================

#define INPUT_MAX_DEVICES 32
#define INPUT_MAX_MAPPINGS 64

// Action mapping (game-level abstraction)
typedef struct {
    char action_name[32];      // "jump", "fire", "move_forward"
    input_device_type_t device_type;
    uint32_t device_id;
    uint32_t input_type;       // 0=button, 1=axis, 2=hat
    uint32_t input_index;
    float threshold;           // For axis-as-button
    int inverted;
} input_mapping_t;

// Input system
typedef struct {
    input_device_t devices[INPUT_MAX_DEVICES];
    uint32_t num_devices;
    
    input_mapping_t mappings[INPUT_MAX_MAPPINGS];
    uint32_t num_mappings;
    
    // Global state
    int initialized;
    uint32_t master_poll_rate;
    uint64_t last_update;
    
    // Raw keyboard/mouse fallback
    int keyboard_fd;
    int mouse_fd;
    
    // Callback for any input
    void (*on_any_input)(input_device_t *dev, uint32_t type, uint32_t index, float value);
    
} input_system_t;

// ============================================================================
// FUNCTIONS
// ============================================================================

// Init
void input_init(void);
void input_shutdown(void);
input_system_t* input_get_system(void);

// USB device detection
int input_scan_usb(void);
int input_probe_device(usb_device_t *usb_dev);
int input_claim_interface(input_device_t *dev);

// Device management
input_device_t* input_device_add(input_device_type_t type, const char *name);
void input_device_remove(input_device_t *dev);
input_device_t* input_device_get(uint32_t id);
input_device_t* input_device_by_type(input_device_type_t type);
uint32_t input_device_count(void);

// HID parsing
int input_parse_hid_descriptor(input_device_t *dev, uint8_t *desc, uint32_t size);
int input_parse_hid_report(input_device_t *dev, uint8_t *report, uint32_t size);
int input_send_hid_output(input_device_t *dev, uint8_t *report, uint32_t size);

// Specific device init
int input_init_dualshock4(input_device_t *dev);
int input_init_dualsense(input_device_t *dev);
int input_init_xbox360(input_device_t *dev);
int input_init_xboxone(input_device_t *dev);
int input_init_switch_pro(input_device_t *dev);
int input_init_generic_gamepad(input_device_t *dev);

// Polling
void input_poll_all(void);
void input_poll_device(input_device_t *dev);
void input_set_poll_rate(input_device_t *dev, uint32_t hz);

// State queries
int input_button_pressed(input_device_t *dev, uint32_t button);
int input_button_just_pressed(input_device_t *dev, uint32_t button);
int input_button_just_released(input_device_t *dev, uint32_t button);
float input_button_pressure(input_device_t *dev, uint32_t button);
float input_axis_value(input_device_t *dev, uint32_t axis);
float input_axis_delta(input_device_t *dev, uint32_t axis);
int input_hat_direction(input_device_t *dev, uint32_t hat);

// Force feedback
void input_rumble(input_device_t *dev, float left, float right, uint32_t duration_ms);
void input_rumble_triggers(input_device_t *dev, float left, float right);
void input_stop_rumble(input_device_t *dev);

// LEDs
void input_set_led(input_device_t *dev, uint8_t r, uint8_t g, uint8_t b);
void input_set_led_pattern(input_device_t *dev, uint32_t pattern, uint32_t speed);

// Mapping system
void input_map_button(const char *action, input_device_type_t type, uint32_t button);
void input_map_axis(const char *action, input_device_type_t type, uint32_t axis, int invert);
float input_get_action(const char *action);
int input_action_pressed(const char *action);
int input_action_just_pressed(const char *action);

// Calibration
void input_calibrate_axis(input_device_t *dev, uint32_t axis);
void input_set_deadzone(input_device_t *dev, uint32_t axis, float deadzone);
void input_set_sensitivity(input_device_t *dev, uint32_t axis, float sensitivity);

// Hotplug
void input_enable_hotplug(void);
void input_disable_hotplug(void);
void input_on_usb_connect(usb_device_t *usb_dev);
void input_on_usb_disconnect(usb_device_t *usb_dev);

// Debug
void input_print_device_info(input_device_t *dev);
void input_print_all_devices(void);
void input_test_device(input_device_t *dev);
void input_dump_raw_report(input_device_t *dev);

// Specific gamepad button names
#define GP_BUTTON_A             0
#define GP_BUTTON_B             1
#define GP_BUTTON_X             2
#define GP_BUTTON_Y             3
#define GP_BUTTON_LB            4
#define GP_BUTTON_RB            5
#define GP_BUTTON_BACK          6
#define GP_BUTTON_START         7
#define GP_BUTTON_GUIDE         8
#define GP_BUTTON_LS            9   // Left stick click
#define GP_BUTTON_RS            10  // Right stick click
#define GP_BUTTON_DPAD_UP       11
#define GP_BUTTON_DPAD_DOWN     12
#define GP_BUTTON_DPAD_LEFT     13
#define GP_BUTTON_DPAD_RIGHT    14
#define GP_BUTTON_TOUCHPAD      15  // PS4/PS5
#define GP_BUTTON_PADDLE_1      16  // Elite
#define GP_BUTTON_PADDLE_2      17
#define GP_BUTTON_PADDLE_3      18
#define GP_BUTTON_PADDLE_4      19

// Axis names
#define GP_AXIS_LX              0
#define GP_AXIS_LY              1
#define GP_AXIS_RX              2
#define GP_AXIS_RY              3
#define GP_AXIS_LT              4   // Left trigger
#define GP_AXIS_RT              5   // Right trigger

#endif