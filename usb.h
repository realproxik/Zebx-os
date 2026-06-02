#ifndef USB_H
#define USB_H

#include "types.h"

#define USB_MAX_DEVICES 128
#define USB_MAX_ENDPOINTS 16
#define USB_MAX_HUBS 8
#define USB_MAX_CONTROLLERS 4

// USB speeds
#define USB_SPEED_LOW       0
#define USB_SPEED_FULL      1
#define USB_SPEED_HIGH      2
#define USB_SPEED_SUPER     3
#define USB_SPEED_SUPER_PLUS 4

// USB packet IDs
#define USB_PID_OUT         0xE1
#define USB_PID_IN          0x69
#define USB_PID_SOF         0xA5
#define USB_PID_SETUP       0x2D
#define USB_PID_DATA0       0xC3
#define USB_PID_DATA1       0x4B
#define USB_PID_ACK         0xD2
#define USB_PID_NAK         0x5A
#define USB_PID_STALL       0x1E

// Request types
#define USB_REQ_GET_STATUS          0x00
#define USB_REQ_CLEAR_FEATURE       0x01
#define USB_REQ_SET_FEATURE         0x03
#define USB_REQ_SET_ADDRESS         0x05
#define USB_REQ_GET_DESCRIPTOR      0x06
#define USB_REQ_SET_DESCRIPTOR      0x07
#define USB_REQ_GET_CONFIGURATION   0x08
#define USB_REQ_SET_CONFIGURATION   0x09
#define USB_REQ_GET_INTERFACE       0x0A
#define USB_REQ_SET_INTERFACE       0x0B

// Descriptor types
#define USB_DESC_DEVICE         0x01
#define USB_DESC_CONFIGURATION  0x02
#define USB_DESC_STRING         0x03
#define USB_DESC_INTERFACE      0x04
#define USB_DESC_ENDPOINT       0x05
#define USB_DESC_DEVICE_QUAL    0x06
#define USB_DESC_OTHER_SPEED    0x07
#define USB_DESC_INTERFACE_PWR  0x08
#define USB_DESC_OTG            0x09
#define USB_DESC_DEBUG          0x0A
#define USB_DESC_INTERFACE_ASSOC 0x0B
#define USB_DESC_BOS            0x0F
#define USB_DESC_CAPABILITY     0x10
#define USB_DESC_SS_ENDPOINT    0x30

// Class codes
#define USB_CLASS_PER_INTERFACE 0x00
#define USB_CLASS_AUDIO         0x01
#define USB_CLASS_CDC           0x02
#define USB_CLASS_HID           0x03
#define USB_CLASS_PHYSICAL      0x05
#define USB_CLASS_IMAGE         0x06
#define USB_CLASS_PRINTER       0x07
#define USB_CLASS_MASS_STORAGE  0x08
#define USB_CLASS_HUB           0x09
#define USB_CLASS_CDC_DATA      0x0A
#define USB_CLASS_SMART_CARD    0x0B
#define USB_CLASS_CONTENT_SEC   0x0D
#define USB_CLASS_VIDEO         0x0E
#define USB_CLASS_PERSONAL_HC   0x0F
#define USB_CLASS_DIAGNOSTIC    0xDC
#define USB_CLASS_WIRELESS      0xE0
#define USB_CLASS_MISC          0xEF
#define USB_CLASS_APP_SPEC      0xFE
#define USB_CLASS_VENDOR_SPEC   0xFF

// HID
#define HID_REQ_GET_REPORT      0x01
#define HID_REQ_GET_IDLE        0x02
#define HID_REQ_GET_PROTOCOL    0x03
#define HID_REQ_SET_REPORT      0x09
#define HID_REQ_SET_IDLE        0x0A
#define HID_REQ_SET_PROTOCOL    0x0B

// USB device descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} usb_device_desc_t;

// USB configuration descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
} usb_config_desc_t;

// USB interface descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} usb_interface_desc_t;

// USB endpoint descriptor
typedef struct __attribute__((packed)) {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} usb_endpoint_desc_t;

// USB setup packet
typedef struct __attribute__((packed)) {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

// USB device
typedef struct {
    uint8_t address;
    uint8_t port;
    uint8_t hub;
    uint8_t speed;
    uint8_t class;
    uint8_t subclass;
    uint8_t protocol;
    uint16_t vendor_id;
    uint16_t product_id;
    uint8_t max_packet_size;
    uint8_t num_configurations;
    uint8_t current_configuration;
    int connected;
    int configured;
    char manufacturer[64];
    char product[64];
    char serial[32];
} usb_device_t;

// USB controller
typedef struct {
    uint32_t type; // 0=UHCI, 1=OHCI, 2=EHCI, 3=xHCI
    uint32_t base_addr;
    uint8_t irq;
    int enabled;
    uint32_t num_ports;
} usb_controller_t;

// HID device
typedef struct {
    usb_device_t *usb_dev;
    uint8_t report_desc[256];
    uint32_t report_desc_len;
    uint8_t protocol;
    uint8_t idle_rate;
    uint8_t device_type; // 0=unknown, 1=mouse, 2=keyboard, 3=gamepad
    int mouse_buttons;
    int mouse_x;
    int mouse_y;
    int mouse_wheel;
    uint8_t keyboard_keys[6];
    uint8_t keyboard_modifiers;

    // Gamepad (best-effort generic HID mapping)
    // Notes:
    // - Real USB HID gamepads vary; this OS's USB stack is currently simplified.
    // - We store a normalized snapshot so games can read a consistent state.
    uint16_t gamepad_buttons;
    uint8_t gamepad_hat;     // 0-7=dpad directions, 8=neutral
    int16_t gamepad_lx;      // -32768..32767
    int16_t gamepad_ly;      // -32768..32767
    int16_t gamepad_rx;      // -32768..32767
    int16_t gamepad_ry;      // -32768..32767
    uint8_t gamepad_lt;      // 0..255
    uint8_t gamepad_rt;      // 0..255
} usb_hid_t;

typedef struct {
    uint32_t connected;
    uint16_t buttons;
    uint8_t hat;
    int16_t lx, ly, rx, ry;
    uint8_t lt, rt;
    uint16_t vendor_id;
    uint16_t product_id;
} usb_gamepad_state_t;

// Mass storage
typedef struct {
    usb_device_t *usb_dev;
    uint8_t interface;
    uint8_t bulk_in;
    uint8_t bulk_out;
    uint32_t block_size;
    uint64_t num_blocks;
    int removable;
} usb_mass_storage_t;

// Functions
void usb_init(void);
int usb_detect_controllers(void);
void usb_scan_bus(void);
int usb_enumerate_device(uint8_t port, uint8_t hub, uint8_t speed);
int usb_get_descriptor(usb_device_t *dev, uint8_t type, uint8_t index, uint8_t *buffer, uint16_t len);
int usb_set_address(usb_device_t *dev, uint8_t addr);
int usb_set_configuration(usb_device_t *dev, uint8_t config);
int usb_control_transfer(usb_device_t *dev, usb_setup_packet_t *setup, uint8_t *data, uint16_t len);
int usb_bulk_transfer(usb_device_t *dev, uint8_t endpoint, uint8_t *data, uint16_t len);
int usb_interrupt_transfer(usb_device_t *dev, uint8_t endpoint, uint8_t *data, uint16_t len);

// HID
void usb_hid_init(usb_device_t *dev);
void usb_hid_poll(usb_hid_t *hid);
void usb_hid_parse_report(usb_hid_t *hid, uint8_t *report, uint32_t len);

// Mass Storage
void usb_msc_init(usb_device_t *dev);
int usb_msc_read_sector(usb_mass_storage_t *msc, uint64_t lba, uint8_t *buffer);
int usb_msc_write_sector(usb_mass_storage_t *msc, uint64_t lba, uint8_t *buffer);

// Hub
void usb_hub_init(usb_device_t *dev);
void usb_hub_poll(usb_device_t *hub);

// xHCI specific
void xhci_init(uint32_t base_addr);
void xhci_reset(void);
void xhci_start(void);
int xhci_port_reset(uint32_t port);
int xhci_enable_slot(void);
int xhci_address_device(uint32_t slot_id, usb_device_t *dev);

// Info
usb_device_t* usb_get_device(uint8_t addr);
uint32_t usb_get_device_count(void);
void usb_list_devices(void);

#endif

#include "usb.h"
#include "vga.h"
#include "memory.h"
#include "interrupt.h"

static usb_controller_t controllers[USB_MAX_CONTROLLERS];
static uint32_t num_controllers = 0;
static usb_device_t devices[USB_MAX_DEVICES];
static uint32_t num_devices = 0;
static usb_hid_t hid_devices[16];
static uint32_t num_hid = 0;
static usb_mass_storage_t msc_devices[8];
static uint32_t num_msc = 0;
static uint8_t next_address = 1;

// xHCI registers
#define XHCI_CAP_LENGTH     0x00
#define XHCI_CAP_HCIVERSION 0x02
#define XHCI_CAP_HCSPARAMS1 0x04
#define XHCI_CAP_HCSPARAMS2 0x08
#define XHCI_CAP_HCSPARAMS3 0x0C
#define XHCI_CAP_HCCPARAMS  0x10
#define XHCI_CAP_DBOFF      0x14
#define XHCI_CAP_RTSOFF     0x18

#define XHCI_OP_USBCMD      0x00
#define XHCI_OP_USBSTS      0x04
#define XHCI_OP_PAGESIZE    0x08
#define XHCI_OP_DNCTRL      0x14
#define XHCI_OP_CRCR        0x18
#define XHCI_OP_DCBAAP      0x30
#define XHCI_OP_CONFIG      0x38

#define XHCI_PORT_STATUS(n) (0x400 + (n) * 0x10)
#define XHCI_PORT_POWER(n)  (0x400 + (n) * 0x10 + 0x04)
#define XHCI_PORT_LINK(n)   (0x400 + (n) * 0x10 + 0x08)

static inline uint32_t xhci_read_cap(uint32_t base, uint32_t reg) {
    return *((volatile uint32_t*)(base + reg));
}

static inline uint32_t xhci_read_op(uint32_t base, uint32_t reg) {
    uint32_t caplen = xhci_read_cap(base, XHCI_CAP_LENGTH) & 0xFF;
    return *((volatile uint32_t*)(base + caplen + reg));
}

static inline void xhci_write_op(uint32_t base, uint32_t reg, uint32_t val) {
    uint32_t caplen = xhci_read_cap(base, XHCI_CAP_LENGTH) & 0xFF;
    *((volatile uint32_t*)(base + caplen + reg)) = val;
}

static inline uint32_t xhci_read_port(uint32_t base, uint32_t port, uint32_t reg) {
    return *((volatile uint32_t*)(base + 0x400 + port * 0x10 + reg));
}

static inline void xhci_write_port(uint32_t base, uint32_t port, uint32_t reg, uint32_t val) {
    *((volatile uint32_t*)(base + 0x400 + port * 0x10 + reg)) = val;
}

void usb_init(void) {
    memset(controllers, 0, sizeof(controllers));
    memset(devices, 0, sizeof(devices));
    memset(hid_devices, 0, sizeof(hid_devices));
    memset(msc_devices, 0, sizeof(msc_devices));
    num_controllers = 0;
    num_devices = 0;
    num_hid = 0;
    num_msc = 0;
    next_address = 1;
    
    vga_puts("[+] USB Stack initialized\\n");
    
    usb_detect_controllers();
    
    for (uint32_t i = 0; i < num_controllers; i++) {
        if (controllers[i].type == 3) { // xHCI
            xhci_init(controllers[i].base_addr);
        }
    }
    
    usb_scan_bus();
}

int usb_detect_controllers(void) {
    // Scan PCI for USB controllers
    // Simplified - would scan PCI config space
    
    // xHCI controller (typical address)
    controllers[0].type = 3;
    controllers[0].base_addr = 0xF7A20000;
    controllers[0].irq = 16;
    controllers[0].enabled = 1;
    controllers[0].num_ports = 4;
    num_controllers = 1;
    
    vga_puts("    Controllers: ");
    vga_put_int(num_controllers);
    vga_puts("\\n");
    vga_puts("    Type: xHCI (USB 3.0)\\n");
    vga_puts("    Ports: ");
    vga_put_int(controllers[0].num_ports);
    vga_puts("\\n");
    
    return num_controllers;
}

void xhci_init(uint32_t base_addr) {
    vga_puts("[*] Initializing xHCI controller\\n");
    
    // Read capabilities
    uint32_t caplen = xhci_read_cap(base_addr, XHCI_CAP_LENGTH) & 0xFF;
    uint32_t version = xhci_read_cap(base_addr, XHCI_CAP_HCIVERSION);
    uint32_t params1 = xhci_read_cap(base_addr, XHCI_CAP_HCSPARAMS1);
    uint32_t params2 = xhci_read_cap(base_addr, XHCI_CAP_HCSPARAMS2);
    uint32_t hccparams = xhci_read_cap(base_addr, XHCI_CAP_HCCPARAMS);
    
    uint32_t max_slots = params1 & 0xFF;
    uint32_t max_ports = (params1 >> 24) & 0xFF;
    uint32_t max_intrs = (params1 >> 8) & 0x7FF;
    
    vga_puts("    Version: ");
    vga_put_int(version >> 8);
    vga_puts(".");
    vga_put_int(version & 0xFF);
    vga_puts("\\n");
    vga_puts("    Max Slots: ");
    vga_put_int(max_slots);
    vga_puts("\\n");
    vga_puts("    Max Ports: ");
    vga_put_int(max_ports);
    vga_puts("\\n");
    vga_puts("    Max Interrupters: ");
    vga_put_int(max_intrs);
    vga_puts("\\n");
    
    xhci_reset();
    xhci_start();
    
    // Check ports for connected devices
    for (uint32_t port = 1; port <= max_ports; port++) {
        uint32_t status = xhci_read_port(base_addr, port, 0);
        if (status & 0x01) { // CCS - Current Connect Status
            vga_puts("    Port ");
            vga_put_int(port);
            vga_puts(": Device connected\\n");
            
            // Determine speed
            uint8_t speed = USB_SPEED_FULL;
            uint32_t portsc = xhci_read_port(base_addr, port, 0);
            uint32_t speed_bits = (portsc >> 10) & 0x0F;
            
            switch (speed_bits) {
                case 1: speed = USB_SPEED_FULL; break;
                case 2: speed = USB_SPEED_LOW; break;
                case 3: speed = USB_SPEED_HIGH; break;
                case 4: speed = USB_SPEED_SUPER; break;
                case 5: speed = USB_SPEED_SUPER_PLUS; break;
            }
            
            xhci_port_reset(port);
            usb_enumerate_device(port, 0, speed);
        }
    }
}

void xhci_reset(void) {
    uint32_t base = controllers[0].base_addr;
    
    // Halt controller
    uint32_t usbcmd = xhci_read_op(base, XHCI_OP_USBCMD);
    usbcmd &= ~0x01; // Clear Run/Stop
    xhci_write_op(base, XHCI_OP_USBCMD, usbcmd);
    
    // Wait for halt
    while (!(xhci_read_op(base, XHCI_OP_USBSTS) & 0x01));
    
    // Reset
    usbcmd = xhci_read_op(base, XHCI_OP_USBCMD);
    usbcmd |= 0x02; // Set Reset
    xhci_write_op(base, XHCI_OP_USBCMD, usbcmd);
    
    // Wait for reset complete
    while (xhci_read_op(base, XHCI_OP_USBCMD) & 0x02);
    
    vga_puts("    Controller reset complete\\n");
}

void xhci_start(void) {
    uint32_t base = controllers[0].base_addr;
    
    // Set up device context base address array
    uint64_t *dcbaap = (uint64_t*)kmalloc_aligned(2048, 64);
    memset(dcbaap, 0, 2048);
    xhci_write_op(base, XHCI_OP_DCBAAP, (uint32_t)(uint64_t)dcbaap);
    xhci_write_op(base, XHCI_OP_DCBAAP + 4, 0);
    
    // Set up command ring
    uint64_t *cmd_ring = (uint64_t*)kmalloc_aligned(4096, 64);
    memset(cmd_ring, 0, 4096);
    xhci_write_op(base, XHCI_OP_CRCR, (uint32_t)(uint64_t)cmd_ring | 1);
    xhci_write_op(base, XHCI_OP_CRCR + 4, 0);
    
    // Set max slots
    xhci_write_op(base, XHCI_OP_CONFIG, 32);
    
    // Start controller
    uint32_t usbcmd = xhci_read_op(base, XHCI_OP_USBCMD);
    usbcmd |= 0x01; // Run/Stop
    xhci_write_op(base, XHCI_OP_USBCMD, usbcmd);
    
    vga_puts("    Controller started\\n");
}

int xhci_port_reset(uint32_t port) {
    uint32_t base = controllers[0].base_addr;
    
    // Reset port
    uint32_t portsc = xhci_read_port(base, port, 0);
    portsc |= 0x10; // Port Reset
    xhci_write_port(base, port, 0, portsc);
    
    // Wait for reset complete
    for (volatile int i = 0; i < 1000000; i++);
    
    portsc = xhci_read_port(base, port, 0);
    if (portsc & 0x02) { // Port Enabled
        return 0;
    }
    return -1;
}

int xhci_enable_slot(void) {
    // Send Enable Slot command
    // Simplified - would add TRB to command ring
    return 1; // Slot ID 1
}

void usb_scan_bus(void) {
    vga_puts("[*] Scanning USB bus\\n");
    
    for (uint32_t i = 0; i < num_devices; i++) {
        if (devices[i].connected) {
            vga_puts("    Device ");
            vga_put_int(devices[i].address);
            vga_puts(": VID=0x");
            vga_put_hex(devices[i].vendor_id);
            vga_puts(" PID=0x");
            vga_put_hex(devices[i].product_id);
            vga_puts(" Class=");
            vga_put_int(devices[i].class);
            vga_puts("\\n");
        }
    }
}

int usb_enumerate_device(uint8_t port, uint8_t hub, uint8_t speed) {
    if (num_devices >= USB_MAX_DEVICES) return -1;
    
    usb_device_t *dev = &devices[num_devices];
    memset(dev, 0, sizeof(usb_device_t));
    
    dev->port = port;
    dev->hub = hub;
    dev->speed = speed;
    dev->connected = 1;
    dev->max_packet_size = (speed == USB_SPEED_LOW) ? 8 : (speed == USB_SPEED_FULL) ? 64 : 512;
    
    // Get device descriptor
    uint8_t desc_buffer[18];
    usb_setup_packet_t setup;
    setup.bmRequestType = 0x80;
    setup.bRequest = USB_REQ_GET_DESCRIPTOR;
    setup.wValue = (USB_DESC_DEVICE << 8) | 0;
    setup.wIndex = 0;
    setup.wLength = 18;
    
    // Simplified - would send actual USB transfer
    // For demo, fill with sample data
    usb_device_desc_t *desc = (usb_device_desc_t*)desc_buffer;
    desc->bLength = 18;
    desc->bDescriptorType = USB_DESC_DEVICE;
    desc->bcdUSB = 0x0200;
    desc->bDeviceClass = USB_CLASS_HID;
    desc->bDeviceSubClass = 0;
    desc->bDeviceProtocol = 0;
    desc->bMaxPacketSize0 = dev->max_packet_size;
    desc->idVendor = 0x046D; // Logitech
    desc->idProduct = 0xC52B; // Unifying Receiver
    desc->bcdDevice = 0x0100;
    desc->iManufacturer = 1;
    desc->iProduct = 2;
    desc->iSerialNumber = 0;
    desc->bNumConfigurations = 1;
    
    dev->vendor_id = desc->idVendor;
    dev->product_id = desc->idProduct;
    dev->class = desc->bDeviceClass;
    dev->num_configurations = desc->bNumConfigurations;
    dev->max_packet_size = desc->bMaxPacketSize0;
    
    // Set address
    dev->address = next_address++;
    
    // Set configuration
    dev->current_configuration = 1;
    dev->configured = 1;
    
    // Class-specific initialization
    switch (dev->class) {
        case USB_CLASS_HID:
            usb_hid_init(dev);
            break;
        case USB_CLASS_MASS_STORAGE:
            usb_msc_init(dev);
            break;
        case USB_CLASS_HUB:
            usb_hub_init(dev);
            break;
    }
    
    num_devices++;
    
    vga_puts("[+] USB device enumerated\\n");
    vga_puts("    Address: ");
    vga_put_int(dev->address);
    vga_puts("\\n");
    vga_puts("    Vendor: 0x");
    vga_put_hex(dev->vendor_id);
    vga_puts("\\n");
    vga_puts("    Product: 0x");
    vga_put_hex(dev->product_id);
    vga_puts("\\n");
    vga_puts("    Class: ");
    switch (dev->class) {
        case USB_CLASS_HID: vga_puts("HID"); break;
        case USB_CLASS_MASS_STORAGE: vga_puts("Mass Storage"); break;
        case USB_CLASS_HUB: vga_puts("Hub"); break;
        case USB_CLASS_AUDIO: vga_puts("Audio"); break;
        case USB_CLASS_VIDEO: vga_puts("Video"); break;
        default: vga_puts("Unknown"); break;
    }
    vga_puts("\\n");
    
    return dev->address;
}

void usb_hid_init(usb_device_t *dev) {
    if (num_hid >= 16) return;
    
    usb_hid_t *hid = &hid_devices[num_hid++];
    hid->usb_dev = dev;
    hid->protocol = 1; // Report protocol
    hid->idle_rate = 0;
    hid->device_type = 0;

    hid->gamepad_buttons = 0;
    hid->gamepad_hat = 8;
    hid->gamepad_lx = hid->gamepad_ly = 0;
    hid->gamepad_rx = hid->gamepad_ry = 0;
    hid->gamepad_lt = hid->gamepad_rt = 0;
    
    // Get report descriptor
    // Simplified
    hid->report_desc_len = 64;

    // Best-effort device typing.
    // - In real HID you'd parse the report descriptor (Usage Page/Usage).
    // - Here we use protocol + known vendor heuristic to unlock controller input.
    if (dev->protocol == 2) {
        hid->device_type = 1; // mouse (boot)
    } else if (dev->protocol == 1) {
        hid->device_type = 2; // keyboard (boot)
    } else if (dev->vendor_id == 0x045E /* Microsoft */) {
        hid->device_type = 3; // gamepad (likely Xbox controller)
    }
    
    vga_puts("    HID device initialized\\n");
}

void usb_hid_poll(usb_hid_t *hid) {
    if (!hid || !hid->usb_dev) return;
    
    uint8_t report[8];
    // Read interrupt endpoint
    // Simplified
    
    usb_hid_parse_report(hid, report, 8);
}

void usb_hid_parse_report(usb_hid_t *hid, uint8_t *report, uint32_t len) {
    if (!hid || !report || len < 3) return;
    
    // Parse based on device type
    if (hid->usb_dev->class == USB_CLASS_HID) {
        // Prefer explicit device type when known.
        if (hid->device_type == 3) {
            // Generic 8-byte "gamepad-like" report mapping:
            //   b0..b1: buttons (bitfield)
            //   b2:     hat/dpad (0-7) or 8=neutral
            //   b3..b6: axes (signed 8-bit -> expanded to 16-bit)
            //   b7:     triggers packed (high nibble=LT, low nibble=RT) scaled to 0..255
            //
            // This is intentionally simple; it enables controller support without full HID parsing.
            if (len >= 8) {
                hid->gamepad_buttons = (uint16_t)report[0] | ((uint16_t)report[1] << 8);
                hid->gamepad_hat = report[2];

                hid->gamepad_lx = ((int16_t)(int8_t)report[3]) << 8;
                hid->gamepad_ly = ((int16_t)(int8_t)report[4]) << 8;
                hid->gamepad_rx = ((int16_t)(int8_t)report[5]) << 8;
                hid->gamepad_ry = ((int16_t)(int8_t)report[6]) << 8;

                uint8_t t = report[7];
                hid->gamepad_lt = (uint8_t)(((t >> 4) & 0x0F) * 17);
                hid->gamepad_rt = (uint8_t)((t & 0x0F) * 17);
            }
            return;
        }

        // Fallback heuristics: mouse vs keyboard.
        if (hid->device_type == 1 || report[0] <= 0x07) { // Mouse report
            hid->mouse_buttons = report[0];
            hid->mouse_x = (int8_t)report[1];
            hid->mouse_y = (int8_t)report[2];
            if (len > 3) hid->mouse_wheel = (int8_t)report[3];
        } else { // Keyboard report
            hid->keyboard_modifiers = report[0];
            for (int i = 0; i < 6 && i + 2 < (int)len; i++) {
                hid->keyboard_keys[i] = report[i + 2];
            }
        }
    }
}

int usb_gamepad_get_state(int index, usb_gamepad_state_t *out) {
    if (!out) return -1;
    if (index < 0) return -1;

    int found = 0;
    for (uint32_t i = 0; i < num_hid; i++) {
        usb_hid_t *hid = &hid_devices[i];
        if (!hid->usb_dev) continue;
        if (!hid->usb_dev->connected) continue;
        if (hid->device_type != 3) continue;

        if (found == index) {
            out->connected = 1;
            out->buttons = hid->gamepad_buttons;
            out->hat = hid->gamepad_hat;
            out->lx = hid->gamepad_lx;
            out->ly = hid->gamepad_ly;
            out->rx = hid->gamepad_rx;
            out->ry = hid->gamepad_ry;
            out->lt = hid->gamepad_lt;
            out->rt = hid->gamepad_rt;
            out->vendor_id = hid->usb_dev->vendor_id;
            out->product_id = hid->usb_dev->product_id;
            return 0;
        }
        found++;
    }

    memset(out, 0, sizeof(*out));
    return -1;
}

void usb_msc_init(usb_device_t *dev) {
    if (num_msc >= 8) return;
    
    usb_mass_storage_t *msc = &msc_devices[num_msc++];
    msc->usb_dev = dev;
    msc->block_size = 512;
    msc->num_blocks = 0; // Would read from device
    msc->removable = 1;
    
    vga_puts("    Mass Storage initialized\\n");
}

int usb_msc_read_sector(usb_mass_storage_t *msc, uint64_t lba, uint8_t *buffer) {
    if (!msc || !buffer) return -1;
    
    // Send CBW (Command Block Wrapper)
    // Read data
    // Receive CSW (Command Status Wrapper)
    
    // Simplified - would perform actual USB bulk transfer
    memset(buffer, 0, msc->block_size);
    
    return 0;
}

int usb_msc_write_sector(usb_mass_storage_t *msc, uint64_t lba, uint8_t *buffer) {
    if (!msc || !buffer) return -1;
    
    // Send CBW
    // Write data
    // Receive CSW
    
    return 0;
}

void usb_hub_init(usb_device_t *dev) {
    vga_puts("    USB Hub initialized\\n");
    
    // Get hub descriptor
    // Power on ports
    // Enable port status change interrupts
}

void usb_hub_poll(usb_device_t *hub) {
    if (!hub) return;
    
    // Check port status changes
    // Handle connect/disconnect events
}

usb_device_t* usb_get_device(uint8_t addr) {
    for (uint32_t i = 0; i < num_devices; i++) {
        if (devices[i].address == addr) return &devices[i];
    }
    return NULL;
}

uint32_t usb_get_device_count(void) {
    return num_devices;
}

void usb_list_devices(void) {
    vga_puts("\\n--- USB Devices ---\\n");
    vga_puts("Addr  VID     PID     Class           Speed\\n");
    vga_puts("----  ------  ------  --------------  -------\\n");
    
    for (uint32_t i = 0; i < num_devices; i++) {
        usb_device_t *dev = &devices[i];
        vga_put_int(dev->address);
        vga_puts("    0x");
        vga_put_hex(dev->vendor_id);
        vga_puts("  0x");
        vga_put_hex(dev->product_id);
        vga_puts("  ");
        
        switch (dev->class) {
            case USB_CLASS_HID: vga_puts("HID            "); break;
            case USB_CLASS_MASS_STORAGE: vga_puts("Mass Storage   "); break;
            case USB_CLASS_HUB: vga_puts("Hub            "); break;
            case USB_CLASS_AUDIO: vga_puts("Audio          "); break;
            case USB_CLASS_VIDEO: vga_puts("Video          "); break;
            case USB_CLASS_PRINTER: vga_puts("Printer        "); break;
            default: vga_puts("Unknown        "); break;
        }
        
        switch (dev->speed) {
            case USB_SPEED_LOW: vga_puts("Low"); break;
            case USB_SPEED_FULL: vga_puts("Full"); break;
            case USB_SPEED_HIGH: vga_puts("High"); break;
            case USB_SPEED_SUPER: vga_puts("Super"); break;
            case USB_SPEED_SUPER_PLUS: vga_puts("Super+"); break;
        }
        vga_puts("\\n");
    }
    
    vga_puts("\\nTotal: ");
    vga_put_int(num_devices);
    vga_puts(" devices\\n");
}

static inline void outb(uint8_t val, uint16_t port) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void outl(uint32_t val, uint16_t port) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "dN"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

void* kmalloc_aligned(uint32_t size, uint32_t align) {
    void *ptr = kmalloc(size + align);
    if (!ptr) return NULL;
    uint32_t addr = (uint32_t)ptr;
    addr = (addr + align - 1) & ~(align - 1);
    return (void*)addr;
}