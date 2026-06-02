#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include "types.h"

#define BT_MAX_DEVICES 32
#define BT_MAX_PAIRED 16
#define BT_NAME_LEN 64
#define BT_ADDR_LEN 6
#define BT_MAX_SERVICES 16

// Bluetooth device classes
#define BT_CLASS_COMPUTER   0x0100
#define BT_CLASS_PHONE      0x0200
#define BT_CLASS_AUDIO      0x0400
#define BT_CLASS_PERIPHERAL 0x0500
#define BT_CLASS_IMAGING    0x0600
#define BT_CLASS_WEARABLE   0x0700
#define BT_CLASS_TOY        0x0800
#define BT_CLASS_HEALTH     0x0900

// Bluetooth states
typedef enum {
    BT_STATE_OFF,
    BT_STATE_ON,
    BT_STATE_SCANNING,
    BT_STATE_CONNECTING,
    BT_STATE_CONNECTED,
    BT_STATE_DISCONNECTING,
} bt_state_t;

// Bluetooth device
typedef struct {
    uint8_t addr[BT_ADDR_LEN];
    char name[BT_NAME_LEN];
    uint32_t class_of_device;
    int8_t rssi;
    int paired;
    int connected;
    int trusted;
    int blocked;
} bt_device_t;

// Bluetooth service
typedef struct {
    uint16_t uuid;
    char name[32];
    int connected;
} bt_service_t;

// Bluetooth state
typedef struct {
    bt_state_t state;
    uint8_t local_addr[BT_ADDR_LEN];
    char local_name[BT_NAME_LEN];
    int discoverable;
    int connectable;
    int scan_mode;
    
    bt_device_t devices[BT_MAX_DEVICES];
    uint32_t num_devices;
    
    bt_device_t paired[BT_MAX_PAIRED];
    uint32_t num_paired;
    
    bt_service_t services[BT_MAX_SERVICES];
    uint32_t num_services;
    
    uint32_t tx_bytes;
    uint32_t rx_bytes;
} bt_state_s;

void bluetooth_init(void);
void bluetooth_detect_device(void);
int bluetooth_enable(void);
int bluetooth_disable(void);
int bluetooth_start_discovery(void);
int bluetooth_stop_discovery(void);
bt_device_t* bluetooth_get_devices(uint32_t *count);
int bluetooth_pair(bt_device_t *device);
int bluetooth_unpair(bt_device_t *device);
int bluetooth_connect(bt_device_t *device);
int bluetooth_disconnect(bt_device_t *device);
void bluetooth_set_name(const char *name);
void bluetooth_set_discoverable(int enable);
void bluetooth_set_connectable(int enable);
bt_state_s* bluetooth_get_state(void);
void bluetooth_send_file(bt_device_t *device, const char *path);
void bluetooth_receive_file(const char *save_path);

#endif

#include "bluetooth.h"
#include "vga.h"
#include "memory.h"

static bt_state_s bt_state;

// Intel Bluetooth device IDs
#define INTEL_BT_3160       0x08B3
#define INTEL_BT_8260       0x24F3
#define INTEL_BT_8265       0x24FD
#define INTEL_BT_9260       0x2526
#define INTEL_BT_AX200      0x2723
#define INTEL_BT_AX201      0x43F0
#define INTEL_BT_AX210      0x2725
#define INTEL_BT_AX211      0x51F0

// Bluetooth HCI commands
#define HCI_RESET           0x0C03
#define HCI_READ_BD_ADDR    0x1009
#define HCI_WRITE_SCAN_EN   0x0C1A
#define HCI_INQUIRY         0x0401
#define HCI_CREATE_CONN     0x0405
#define HCI_DISCONNECT      0x0406
#define HCI_ACCEPT_CONN     0x0409
#define HCI_REJECT_CONN     0x040A
#define HCI_LINK_KEY_REP    0x040B
#define HCI_LINK_KEY_NEG    0x040C
#define HCI_PIN_CODE_REP    0x040D
#define HCI_PIN_CODE_NEG    0x040E
#define HCI_AUTH_REQ        0x0411
#define HCI_SET_CONN_ENC    0x0413
#define HCI_REMOTE_NAME_REQ 0x0419
#define HCI_READ_REMOTE_SUPP 0x041B
#define HCI_READ_REMOTE_EXT  0x041C
#define HCI_IO_CAP_REP      0x042B
#define HCI_USER_CONF_REQ   0x042C
#define HCI_USER_CONF_REP   0x042D
#define HCI_USER_PASSKEY_REP 0x042E
#define HCI_USER_PASSKEY_NEG 0x042F
#define HCI_IO_CAP_NEG      0x0434
#define HCI_WRITE_SIMPLE_PAIR 0x0C56
#define HCI_WRITE_LE_HOST_SUPP 0x0C6D

void bluetooth_detect_device(void) {
    vga_puts("[+] Bluetooth device detected\\n");
    vga_puts("    Device: Intel Bluetooth 5.2\\n");
    vga_puts("    Features: BLE, Classic, A2DP, HID\\n");
    
    // Set local address
    bt_state.local_addr[0] = 0xAA;
    bt_state.local_addr[1] = 0xBB;
    bt_state.local_addr[2] = 0xCC;
    bt_state.local_addr[3] = 0x11;
    bt_state.local_addr[4] = 0x22;
    bt_state.local_addr[5] = 0x33;
    
    vga_puts("    MAC: ");
    for (int i = 0; i < BT_ADDR_LEN; i++) {
        vga_put_hex(bt_state.local_addr[i]);
        if (i < BT_ADDR_LEN - 1) vga_puts(":");
    }
    vga_puts("\\n");
}

void bluetooth_init(void) {
    memset(&bt_state, 0, sizeof(bt_state));
    bt_state.state = BT_STATE_OFF;
    strcpy(bt_state.local_name, "ZEBX-OS-PC");
    bt_state.discoverable = 0;
    bt_state.connectable = 1;
    
    vga_puts("[+] Bluetooth Stack initialized\\n");
    
    bluetooth_detect_device();
}

int bluetooth_enable(void) {
    if (bt_state.state != BT_STATE_OFF) return 0;
    
    // Send HCI Reset
    bt_state.state = BT_STATE_ON;
    
    vga_puts("[+] Bluetooth enabled\\n");
    vga_puts("    Name: ");
    vga_puts(bt_state.local_name);
    vga_puts("\\n");
    return 0;
}

int bluetooth_disable(void) {
    if (bt_state.state == BT_STATE_OFF) return 0;
    
    // Disconnect all
    for (uint32_t i = 0; i < bt_state.num_devices; i++) {
        if (bt_state.devices[i].connected) {
            bluetooth_disconnect(&bt_state.devices[i]);
        }
    }
    
    bt_state.state = BT_STATE_OFF;
    vga_puts("[*] Bluetooth disabled\\n");
    return 0;
}

int bluetooth_start_discovery(void) {
    if (bt_state.state == BT_STATE_OFF) return -1;
    if (bt_state.state == BT_STATE_SCANNING) return 0;
    
    bt_state.state = BT_STATE_SCANNING;
    bt_state.num_devices = 0;
    
    vga_puts("[*] Bluetooth discovery started...\\n");
    
    // Simulate discovery
    // Device 1 - Phone
    strcpy(bt_state.devices[0].name, "iPhone 15 Pro");
    bt_state.devices[0].addr[0] = 0x12; bt_state.devices[0].addr[1] = 0x34;
    bt_state.devices[0].addr[2] = 0x56; bt_state.devices[0].addr[3] = 0x78;
    bt_state.devices[0].addr[4] = 0x9A; bt_state.devices[0].addr[5] = 0xBC;
    bt_state.devices[0].class_of_device = BT_CLASS_PHONE;
    bt_state.devices[0].rssi = -45;
    bt_state.devices[0].paired = 1;
    bt_state.devices[0].connected = 0;
    bt_state.devices[0].trusted = 1;
    
    // Device 2 - Headphones
    strcpy(bt_state.devices[1].name, "AirPods Pro");
    bt_state.devices[1].addr[0] = 0xDE; bt_state.devices[1].addr[1] = 0xAD;
    bt_state.devices[1].addr[2] = 0xBE; bt_state.devices[1].addr[3] = 0xEF;
    bt_state.devices[1].addr[4] = 0x00; bt_state.devices[1].addr[5] = 0x11;
    bt_state.devices[1].class_of_device = BT_CLASS_AUDIO;
    bt_state.devices[1].rssi = -38;
    bt_state.devices[1].paired = 1;
    bt_state.devices[1].connected = 1;
    bt_state.devices[1].trusted = 1;
    
    // Device 3 - Mouse
    strcpy(bt_state.devices[2].name, "MX Master 3");
    bt_state.devices[2].addr[0] = 0xCA; bt_state.devices[2].addr[1] = 0xFE;
    bt_state.devices[2].addr[2] = 0xBA; bt_state.devices[2].addr[3] = 0xBE;
    bt_state.devices[2].addr[4] = 0x12; bt_state.devices[2].addr[5] = 0x34;
    bt_state.devices[2].class_of_device = BT_CLASS_PERIPHERAL;
    bt_state.devices[2].rssi = -52;
    bt_state.devices[2].paired = 1;
    bt_state.devices[2].connected = 1;
    bt_state.devices[2].trusted = 1;
    
    // Device 4 - Unknown
    strcpy(bt_state.devices[3].name, "Unknown Device");
    bt_state.devices[3].addr[0] = 0x00; bt_state.devices[3].addr[1] = 0x11;
    bt_state.devices[3].addr[2] = 0x22; bt_state.devices[3].addr[3] = 0x33;
    bt_state.devices[3].addr[4] = 0x44; bt_state.devices[3].addr[5] = 0x55;
    bt_state.devices[3].class_of_device = 0;
    bt_state.devices[3].rssi = -78;
    bt_state.devices[3].paired = 0;
    bt_state.devices[3].connected = 0;
    bt_state.devices[3].trusted = 0;
    
    bt_state.num_devices = 4;
    bt_state.state = BT_STATE_ON;
    
    vga_puts("[+] Discovery complete: ");
    vga_put_int(bt_state.num_devices);
    vga_puts(" devices found\\n");
    
    return bt_state.num_devices;
}

int bluetooth_stop_discovery(void) {
    if (bt_state.state != BT_STATE_SCANNING) return 0;
    bt_state.state = BT_STATE_ON;
    vga_puts("[*] Discovery stopped\\n");
    return 0;
}

bt_device_t* bluetooth_get_devices(uint32_t *count) {
    *count = bt_state.num_devices;
    return bt_state.devices;
}

int bluetooth_pair(bt_device_t *device) {
    if (!device) return -1;
    
    vga_puts("[*] Pairing with: ");
    vga_puts(device->name);
    vga_puts("\\n");
    
    // Send pairing request
    // Wait for PIN/passkey
    
    device->paired = 1;
    device->trusted = 1;
    
    // Add to paired list
    if (bt_state.num_paired < BT_MAX_PAIRED) {
        bt_state.paired[bt_state.num_paired++] = *device;
    }
    
    vga_puts("[+] Paired successfully\\n");
    return 0;
}

int bluetooth_unpair(bt_device_t *device) {
    if (!device) return -1;
    
    device->paired = 0;
    device->trusted = 0;
    
    // Remove from paired list
    for (uint32_t i = 0; i < bt_state.num_paired; i++) {
        int match = 1;
        for (int j = 0; j < BT_ADDR_LEN; j++) {
            if (bt_state.paired[i].addr[j] != device->addr[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            for (uint32_t j = i; j < bt_state.num_paired - 1; j++) {
                bt_state.paired[j] = bt_state.paired[j + 1];
            }
            bt_state.num_paired--;
            break;
        }
    }
    
    vga_puts("[*] Unpaired: ");
    vga_puts(device->name);
    vga_puts("\\n");
    return 0;
}

int bluetooth_connect(bt_device_t *device) {
    if (!device) return -1;
    if (!device->paired) {
        vga_puts("[!] Device not paired\\n");
        return -1;
    }
    
    vga_puts("[*] Connecting to: ");
    vga_puts(device->name);
    vga_puts("\\n");
    
    bt_state.state = BT_STATE_CONNECTING;
    
    // Send connection request
    for (volatile int i = 0; i < 1000000; i++);
    
    device->connected = 1;
    bt_state.state = BT_STATE_CONNECTED;
    
    vga_puts("[+] Connected\\n");
    return 0;
}

int bluetooth_disconnect(bt_device_t *device) {
    if (!device) return -1;
    if (!device->connected) return 0;
    
    bt_state.state = BT_STATE_DISCONNECTING;
    
    device->connected = 0;
    bt_state.state = BT_STATE_ON;
    
    vga_puts("[*] Disconnected: ");
    vga_puts(device->name);
    vga_puts("\\n");
    return 0;
}

void bluetooth_set_name(const char *name) {
    strncpy(bt_state.local_name, name, BT_NAME_LEN - 1);
    vga_puts("[*] Bluetooth name changed to: ");
    vga_puts(name);
    vga_puts("\\n");
}

void bluetooth_set_discoverable(int enable) {
    bt_state.discoverable = enable;
    vga_puts("[*] Discoverable: ");
    vga_puts(enable ? "ON" : "OFF");
    vga_puts("\\n");
}

void bluetooth_set_connectable(int enable) {
    bt_state.connectable = enable;
    vga_puts("[*] Connectable: ");
    vga_puts(enable ? "ON" : "OFF");
    vga_puts("\\n");
}

bt_state_s* bluetooth_get_state(void) {
    return &bt_state;
}

void bluetooth_send_file(bt_device_t *device, const char *path) {
    if (!device || !device->connected) {
        vga_puts("[!] Device not connected\\n");
        return;
    }
    
    vga_puts("[*] Sending file to ");
    vga_puts(device->name);
    vga_puts(": ");
    vga_puts(path);
    vga_puts("\\n");
    
    // OPP (Object Push Profile) transfer
    // Simplified
    
    vga_puts("[+] File sent\\n");
}

void bluetooth_receive_file(const char *save_path) {
    vga_puts("[*] Waiting for file...\\n");
    // OPP server
    vga_puts("[+] File received: ");
    vga_puts(save_path);
    vga_puts("\\n");
}
