#ifndef WIFI_H
#define WIFI_H

#include "types.h"

#define WIFI_MAX_NETWORKS 64
#define WIFI_MAX_SSID_LEN 32
#define WIFI_MAX_PASSWORD_LEN 64
#define WIFI_MAC_ADDR_LEN 6
#define WIFI_MAX_CHANNELS 165
#define WIFI_SCAN_TIMEOUT_MS 5000

// WiFi standards
#define WIFI_STD_B      0x01
#define WIFI_STD_G      0x02
#define WIFI_STD_N      0x04
#define WIFI_STD_AC     0x08
#define WIFI_STD_AX     0x10

// Security types
typedef enum {
    WIFI_SECURITY_OPEN,
    WIFI_SECURITY_WEP,
    WIFI_SECURITY_WPA_PSK,
    WIFI_SECURITY_WPA2_PSK,
    WIFI_SECURITY_WPA3_PSK,
    WIFI_SECURITY_WPA_ENTERPRISE,
    WIFI_SECURITY_WPA2_ENTERPRISE,
    WIFI_SECURITY_WPA3_ENTERPRISE,
} wifi_security_t;

// Band
typedef enum {
    WIFI_BAND_2_4GHZ,
    WIFI_BAND_5GHZ,
    WIFI_BAND_6GHZ,
} wifi_band_t;

// Network info
typedef struct {
    char ssid[WIFI_MAX_SSID_LEN + 1];
    uint8_t bssid[WIFI_MAC_ADDR_LEN];
    int8_t rssi;
    uint8_t channel;
    wifi_band_t band;
    wifi_security_t security;
    uint8_t standard;
    uint32_t max_rate_mbps;
    int connected;
    int saved;
    int hidden;
} wifi_network_t;

// WiFi state
typedef struct {
    uint8_t mac_addr[WIFI_MAC_ADDR_LEN];
    uint8_t ip_addr[4];
    uint8_t subnet_mask[4];
    uint8_t gateway[4];
    uint8_t dns[4];
    
    int enabled;
    int connected;
    wifi_network_t *current_network;
    
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_packets;
    uint32_t rx_packets;
    uint32_t tx_errors;
    uint32_t rx_errors;
    
    int power_save;
    int tx_power_dbm;
} wifi_state_t;

// Intel WiFi device IDs
#define INTEL_WIFI_3160     0x08B3
#define INTEL_WIFI_3165     0x3165
#define INTEL_WIFI_7260     0x08B1
#define INTEL_WIFI_7265     0x095A
#define INTEL_WIFI_8260     0x24F3
#define INTEL_WIFI_8265     0x24FD
#define INTEL_WIFI_9260     0x2526
#define INTEL_WIFI_9560     0x9DF0
#define INTEL_WIFI_AX200    0x2723
#define INTEL_WIFI_AX201    0x43F0
#define INTEL_WIFI_AX210    0x2725
#define INTEL_WIFI_AX211    0x51F0
#define INTEL_WIFI_BE200    0xA840

void wifi_init(void);
void wifi_detect_device(void);
int wifi_enable(void);
int wifi_disable(void);

// Scanning
int wifi_scan(void);
wifi_network_t* wifi_get_networks(uint32_t *count);
wifi_network_t* wifi_get_network(const char *ssid);
void wifi_scan_async(void);
int wifi_scan_in_progress(void);

// Connection
int wifi_connect(const char *ssid, const char *password);
int wifi_connect_open(const char *ssid);
int wifi_disconnect(void);
int wifi_reconnect(void);

// Saved networks
void wifi_add_network(const char *ssid, const char *password, wifi_security_t security);
void wifi_remove_network(const char *ssid);
void wifi_auto_connect(void);

// Info
wifi_state_t* wifi_get_state(void);
int wifi_get_rssi(void);
uint32_t wifi_get_link_speed(void);
void wifi_get_mac(uint8_t *mac);

// Power management
void wifi_set_power_save(int enable);
void wifi_set_tx_power(int dbm);

// Monitor mode / Packet injection
int wifi_set_monitor_mode(int enable);
int wifi_inject_packet(uint8_t *data, uint32_t len);

// Rendering
void wifi_render_status(uint32_t *buffer, int x, int y, int w, int h);
void wifi_render_network_list(wifi_network_t *networks, uint32_t count, uint32_t *buffer, int w, int h);

#endif

#include "wifi.h"
#include "vga.h"
#include "memory.h"
#include "network.h"

static wifi_state_t wifi_state;
static wifi_network_t scanned_networks[WIFI_MAX_NETWORKS];
static uint32_t num_scanned = 0;
static wifi_network_t saved_networks[16];
static uint32_t num_saved = 0;
static int scanning = 0;

// Intel AX210 MMIO registers (simplified)
#define AX210_BASE_ADDR     0xF7A30000
#define AX210_REG_DEVICE_ID 0x0000
#define AX210_REG_MAC_ADDR  0x3800
#define AX210_REG_STATUS    0x4000
#define AX210_REG_TX_QUEUE  0x5000
#define AX210_REG_RX_QUEUE  0x6000
#define AX210_REG_CMD       0x7000

static inline uint32_t ax210_read(uint32_t reg) {
    return *((volatile uint32_t*)(AX210_BASE_ADDR + reg));
}

static inline void ax210_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t*)(AX210_BASE_ADDR + reg)) = val;
}

void wifi_detect_device(void) {
    uint32_t device_id = ax210_read(AX210_REG_DEVICE_ID);
    
    vga_puts("[+] WiFi device detected\\n");
    
    switch (device_id) {
        case INTEL_WIFI_AX200:
            vga_puts("    Device: Intel Wi-Fi 6 AX200\\n");
            vga_puts("    Standard: 802.11ax (WiFi 6)\\n");
            break;
        case INTEL_WIFI_AX201:
            vga_puts("    Device: Intel Wi-Fi 6 AX201\\n");
            vga_puts("    Standard: 802.11ax (WiFi 6)\\n");
            break;
        case INTEL_WIFI_AX210:
            vga_puts("    Device: Intel Wi-Fi 6E AX210\\n");
            vga_puts("    Standard: 802.11ax (WiFi 6E)\\n");
            break;
        case INTEL_WIFI_AX211:
            vga_puts("    Device: Intel Wi-Fi 6E AX211\\n");
            vga_puts("    Standard: 802.11ax (WiFi 6E)\\n");
            break;
        case INTEL_WIFI_BE200:
            vga_puts("    Device: Intel Wi-Fi 7 BE200\\n");
            vga_puts("    Standard: 802.11be (WiFi 7)\\n");
            break;
        default:
            vga_puts("    Device: Intel Wi-Fi (Unknown)\\n");
            break;
    }
    
    // Read MAC address
    for (int i = 0; i < WIFI_MAC_ADDR_LEN; i++) {
        wifi_state.mac_addr[i] = ax210_read(AX210_REG_MAC_ADDR + i) & 0xFF;
    }
    
    vga_puts("    MAC: ");
    for (int i = 0; i < WIFI_MAC_ADDR_LEN; i++) {
        vga_put_hex(wifi_state.mac_addr[i]);
        if (i < WIFI_MAC_ADDR_LEN - 1) vga_puts(":");
    }
    vga_puts("\\n");
}

void wifi_init(void) {
    memset(&wifi_state, 0, sizeof(wifi_state));
    memset(scanned_networks, 0, sizeof(scanned_networks));
    memset(saved_networks, 0, sizeof(saved_networks));
    num_scanned = 0;
    num_saved = 0;
    scanning = 0;
    
    wifi_state.enabled = 0;
    wifi_state.connected = 0;
    wifi_state.current_network = NULL;
    wifi_state.tx_power_dbm = 20;
    wifi_state.power_save = 1;
    
    vga_puts("[+] WiFi Driver initialized\\n");
    
    wifi_detect_device();
    
    // Initialize firmware
    ax210_write(AX210_REG_CMD, 0x01); // Load firmware
    for (volatile int i = 0; i < 1000000; i++);
    
    vga_puts("    Firmware loaded\\n");
    vga_puts("    TX Power: ");
    vga_put_int(wifi_state.tx_power_dbm);
    vga_puts(" dBm\\n");
    vga_puts("    Power Save: ");
    vga_puts(wifi_state.power_save ? "Enabled" : "Disabled");
    vga_puts("\\n");
}

int wifi_enable(void) {
    if (wifi_state.enabled) return 0;
    
    ax210_write(AX210_REG_CMD, 0x02); // Enable radio
    wifi_state.enabled = 1;
    
    vga_puts("[+] WiFi enabled\\n");
    return 0;
}

int wifi_disable(void) {
    if (!wifi_state.enabled) return 0;
    
    wifi_disconnect();
    ax210_write(AX210_REG_CMD, 0x03); // Disable radio
    wifi_state.enabled = 0;
    
    vga_puts("[*] WiFi disabled\\n");
    return 0;
}

int wifi_scan(void) {
    if (!wifi_state.enabled) return -1;
    if (scanning) return -1;
    
    scanning = 1;
    num_scanned = 0;
    
    vga_puts("[*] Scanning for WiFi networks...\\n");
    
    // Command scan
    ax210_write(AX210_REG_CMD, 0x10); // Start scan
    
    // Wait for scan complete
    for (volatile int i = 0; i < 5000000; i++);
    
    // Read scan results
    // Simplified - generate sample networks
    
    // Network 1
    strcpy(scanned_networks[0].ssid, "ZEBX-Gaming-5G");
    scanned_networks[0].bssid[0] = 0xAA; scanned_networks[0].bssid[1] = 0xBB;
    scanned_networks[0].bssid[2] = 0xCC; scanned_networks[0].bssid[3] = 0xDD;
    scanned_networks[0].bssid[4] = 0xEE; scanned_networks[0].bssid[5] = 0xFF;
    scanned_networks[0].rssi = -35;
    scanned_networks[0].channel = 36;
    scanned_networks[0].band = WIFI_BAND_5GHZ;
    scanned_networks[0].security = WIFI_SECURITY_WPA3_PSK;
    scanned_networks[0].standard = WIFI_STD_AX;
    scanned_networks[0].max_rate_mbps = 2402;
    scanned_networks[0].connected = 0;
    scanned_networks[0].saved = 1;
    scanned_networks[0].hidden = 0;
    
    // Network 2
    strcpy(scanned_networks[1].ssid, "ZEBX-Gaming-2.4G");
    scanned_networks[1].bssid[0] = 0x11; scanned_networks[1].bssid[1] = 0x22;
    scanned_networks[1].bssid[2] = 0x33; scanned_networks[1].bssid[3] = 0x44;
    scanned_networks[1].bssid[4] = 0x55; scanned_networks[1].bssid[5] = 0x66;
    scanned_networks[1].rssi = -42;
    scanned_networks[1].channel = 6;
    scanned_networks[1].band = WIFI_BAND_2_4GHZ;
    scanned_networks[1].security = WIFI_SECURITY_WPA2_PSK;
    scanned_networks[1].standard = WIFI_STD_N;
    scanned_networks[1].max_rate_mbps = 300;
    scanned_networks[1].connected = 0;
    scanned_networks[1].saved = 1;
    scanned_networks[1].hidden = 0;
    
    // Network 3
    strcpy(scanned_networks[2].ssid, "NeighborWiFi");
    scanned_networks[2].bssid[0] = 0x77; scanned_networks[2].bssid[1] = 0x88;
    scanned_networks[2].bssid[2] = 0x99; scanned_networks[2].bssid[3] = 0xAA;
    scanned_networks[2].bssid[4] = 0xBB; scanned_networks[2].bssid[5] = 0xCC;
    scanned_networks[2].rssi = -65;
    scanned_networks[2].channel = 11;
    scanned_networks[2].band = WIFI_BAND_2_4GHZ;
    scanned_networks[2].security = WIFI_SECURITY_WPA2_PSK;
    scanned_networks[2].standard = WIFI_STD_AC;
    scanned_networks[2].max_rate_mbps = 867;
    scanned_networks[2].connected = 0;
    scanned_networks[2].saved = 0;
    scanned_networks[2].hidden = 0;
    
    // Network 4 - Open
    strcpy(scanned_networks[3].ssid, "FreeWiFi");
    scanned_networks[3].bssid[0] = 0xDD; scanned_networks[3].bssid[1] = 0xEE;
    scanned_networks[3].bssid[2] = 0xFF; scanned_networks[3].bssid[3] = 0x00;
    scanned_networks[3].bssid[4] = 0x11; scanned_networks[3].bssid[5] = 0x22;
    scanned_networks[3].rssi = -78;
    scanned_networks[3].channel = 1;
    scanned_networks[3].band = WIFI_BAND_2_4GHZ;
    scanned_networks[3].security = WIFI_SECURITY_OPEN;
    scanned_networks[3].standard = WIFI_STD_G;
    scanned_networks[3].max_rate_mbps = 54;
    scanned_networks[3].connected = 0;
    scanned_networks[3].saved = 0;
    scanned_networks[3].hidden = 0;
    
    num_scanned = 4;
    scanning = 0;
    
    vga_puts("[+] Scan complete: ");
    vga_put_int(num_scanned);
    vga_puts(" networks found\\n");
    
    return num_scanned;
}

wifi_network_t* wifi_get_networks(uint32_t *count) {
    *count = num_scanned;
    return scanned_networks;
}

wifi_network_t* wifi_get_network(const char *ssid) {
    for (uint32_t i = 0; i < num_scanned; i++) {
        if (!strcmp(scanned_networks[i].ssid, ssid)) {
            return &scanned_networks[i];
        }
    }
    return NULL;
}

int wifi_connect(const char *ssid, const char *password) {
    if (!wifi_state.enabled) return -1;
    if (wifi_state.connected) wifi_disconnect();
    
    wifi_network_t *net = wifi_get_network(ssid);
    if (!net) {
        vga_puts("[!] Network not found: ");
        vga_puts(ssid);
        vga_puts("\\n");
        return -1;
    }
    
    vga_puts("[*] Connecting to: ");
    vga_puts(ssid);
    vga_puts("\\n");
    
    // Authenticate
    switch (net->security) {
        case WIFI_SECURITY_OPEN:
            vga_puts("    Security: Open\\n");
            break;
        case WIFI_SECURITY_WEP:
            vga_puts("    Security: WEP (Insecure!)\\n");
            break;
        case WIFI_SECURITY_WPA_PSK:
            vga_puts("    Security: WPA-PSK\\n");
            break;
        case WIFI_SECURITY_WPA2_PSK:
            vga_puts("    Security: WPA2-PSK\\n");
            break;
        case WIFI_SECURITY_WPA3_PSK:
            vga_puts("    Security: WPA3-PSK\\n");
            break;
        default:
            vga_puts("    Security: Unknown\\n");
            break;
    }
    
    // Send authentication request
    ax210_write(AX210_REG_CMD, 0x20); // Connect command
    
    // Wait for association
    for (volatile int i = 0; i < 3000000; i++);
    
    // Set IP (DHCP simulated)
    wifi_state.ip_addr[0] = 192; wifi_state.ip_addr[1] = 168;
    wifi_state.ip_addr[2] = 1; wifi_state.ip_addr[3] = 100;
    wifi_state.subnet_mask[0] = 255; wifi_state.subnet_mask[1] = 255;
    wifi_state.subnet_mask[2] = 255; wifi_state.subnet_mask[3] = 0;
    wifi_state.gateway[0] = 192; wifi_state.gateway[1] = 168;
    wifi_state.gateway[2] = 1; wifi_state.gateway[3] = 1;
    wifi_state.dns[0] = 8; wifi_state.dns[1] = 8;
    wifi_state.dns[2] = 8; wifi_state.dns[3] = 8;
    
    wifi_state.connected = 1;
    wifi_state.current_network = net;
    net->connected = 1;
    
    vga_puts("[+] Connected!\\n");
    vga_puts("    IP: 192.168.1.100\\n");
    vga_puts("    RSSI: ");
    vga_put_int(net->rssi);
    vga_puts(" dBm\\n");
    vga_puts("    Speed: ");
    vga_put_int(net->max_rate_mbps);
    vga_puts(" Mbps\\n");
    
    return 0;
}

int wifi_connect_open(const char *ssid) {
    return wifi_connect(ssid, NULL);
}

int wifi_disconnect(void) {
    if (!wifi_state.connected) return 0;
    
    ax210_write(AX210_REG_CMD, 0x21); // Disconnect
    
    if (wifi_state.current_network) {
        wifi_state.current_network->connected = 0;
    }
    
    wifi_state.connected = 0;
    wifi_state.current_network = NULL;
    
    vga_puts("[*] Disconnected\\n");
    return 0;
}

int wifi_reconnect(void) {
    if (!wifi_state.connected && wifi_state.current_network) {
        return wifi_connect(wifi_state.current_network->ssid, NULL);
    }
    return -1;
}

void wifi_add_network(const char *ssid, const char *password, wifi_security_t security) {
    if (num_saved >= 16) return;
    
    strcpy(saved_networks[num_saved].ssid, ssid);
    saved_networks[num_saved].security = security;
    saved_networks[num_saved].saved = 1;
    num_saved++;
    
    vga_puts("[+] Network saved: ");
    vga_puts(ssid);
    vga_puts("\\n");
}

void wifi_remove_network(const char *ssid) {
    for (uint32_t i = 0; i < num_saved; i++) {
        if (!strcmp(saved_networks[i].ssid, ssid)) {
            for (uint32_t j = i; j < num_saved - 1; j++) {
                saved_networks[j] = saved_networks[j + 1];
            }
            num_saved--;
            return;
        }
    }
}

void wifi_auto_connect(void) {
    if (!wifi_state.enabled) wifi_enable();
    
    wifi_scan();
    
    for (uint32_t i = 0; i < num_saved; i++) {
        wifi_network_t *net = wifi_get_network(saved_networks[i].ssid);
        if (net) {
            wifi_connect(net->ssid, NULL);
            return;
        }
    }
}

wifi_state_t* wifi_get_state(void) {
    return &wifi_state;
}

int wifi_get_rssi(void) {
    if (!wifi_state.connected || !wifi_state.current_network) return -100;
    return wifi_state.current_network->rssi;
}

uint32_t wifi_get_link_speed(void) {
    if (!wifi_state.connected || !wifi_state.current_network) return 0;
    return wifi_state.current_network->max_rate_mbps;
}

void wifi_get_mac(uint8_t *mac) {
    for (int i = 0; i < WIFI_MAC_ADDR_LEN; i++) {
        mac[i] = wifi_state.mac_addr[i];
    }
}

void wifi_set_power_save(int enable) {
    wifi_state.power_save = enable;
    ax210_write(AX210_REG_CMD, enable ? 0x30 : 0x31);
}

void wifi_set_tx_power(int dbm) {
    if (dbm < 0) dbm = 0;
    if (dbm > 30) dbm = 30;
    wifi_state.tx_power_dbm = dbm;
    ax210_write(AX210_REG_CMD, 0x40 | (dbm & 0x1F));
}

int wifi_set_monitor_mode(int enable) {
    if (enable) {
        wifi_disconnect();
        ax210_write(AX210_REG_CMD, 0x50); // Monitor mode
        vga_puts("[+] Monitor mode enabled\\n");
    } else {
        ax210_write(AX210_REG_CMD, 0x51); // Normal mode
        vga_puts("[*] Monitor mode disabled\\n");
    }
    return 0;
}

int wifi_inject_packet(uint8_t *data, uint32_t len) {
    if (!data || len > 2304) return -1;
    
    // Write to TX queue
    ax210_write(AX210_REG_TX_QUEUE, len);
    for (uint32_t i = 0; i < len; i += 4) {
        uint32_t word = data[i] | (data[i+1] << 8) | (data[i+2] << 16) | (data[i+3] << 24);
        ax210_write(AX210_REG_TX_QUEUE + 4, word);
    }
    
    return 0;
}

void wifi_render_status(uint32_t *buffer, int x, int y, int w, int h) {
    // WiFi icon
    int cx = x + w / 2;
    int cy = y + h / 2;
    uint32_t color = wifi_state.connected ? 0xFF00FF00 : 0xFFFF0000;
    
    // Draw WiFi arcs
    for (int r = 4; r <= 12; r += 4) {
        for (int angle = 200; angle <= 340; angle += 5) {
            float rad = angle * 3.14159f / 180.0f;
            int px = cx + (int)(r * cosf(rad));
            int py = cy + (int)(r * sinf(rad));
            if (px >= x && px < x + w && py >= y && py < y + h) {
                buffer[py * 800 + px] = color;
            }
        }
    }
    
    // Signal strength bars
    if (wifi_state.connected && wifi_state.current_network) {
        int rssi = wifi_state.current_network->rssi;
        int bars = 0;
        if (rssi > -50) bars = 4;
        else if (rssi > -60) bars = 3;
        else if (rssi > -70) bars = 2;
        else if (rssi > -80) bars = 1;
        
        for (int i = 0; i < 4; i++) {
            int bx = x + w - 20 + i * 5;
            int bh_bar = 4 + i * 3;
            uint32_t bar_color = (i < bars) ? 0xFF00FF00 : 0xFF444444;
            for (int row = y + h - bh_bar - 2; row < y + h - 2; row++) {
                for (int col = bx; col < bx + 3; col++) {
                    if (col >= x && col < x + w && row >= y && row < y + h) {
                        buffer[row * 800 + col] = bar_color;
                    }
                }
            }
        }
    }
}

void wifi_render_network_list(wifi_network_t *networks, uint32_t count, uint32_t *buffer, int w, int h) {
    int row_h = 40;
    int start_y = 50;
    
    for (uint32_t i = 0; i < count && start_y + i * row_h < h; i++) {
        int y = start_y + i * row_h;
        wifi_network_t *net = &networks[i];
        
        // Row background
        uint32_t bg = (i % 2 == 0) ? 0xFF2D2D2D : 0xFF333333;
        if (net->connected) bg = 0xFF1A3A1A;
        
        for (int row = y; row < y + row_h - 2 && row < h; row++) {
            for (int col = 10; col < w - 10 && col < w; col++) {
                buffer[row * w + col] = bg;
            }
        }
        
        // Signal strength indicator
        int rssi = net->rssi;
        int bars = 0;
        if (rssi > -50) bars = 4;
        else if (rssi > -60) bars = 3;
        else if (rssi > -70) bars = 2;
        else if (rssi > -80) bars = 1;
        
        for (int b = 0; b < 4; b++) {
            int bx = 20;
            int by = y + 10 + (3 - b) * 4;
            uint32_t bar_color = (b < bars) ? 0xFF00FF00 : 0xFF444444;
            for (int row = by; row < by + 4 && row < h; row++) {
                for (int col = bx + b * 6; col < bx + b * 6 + 4 && col < w; col++) {
                    buffer[row * w + col] = bar_color;
                }
            }
        }
        
        // Lock icon for secured networks
        if (net->security != WIFI_SECURITY_OPEN) {
            for (int row = y + 10; row < y + 25 && row < h; row++) {
                for (int col = w - 60; col < w - 45 && col < w; col++) {
                    buffer[row * w + col] = 0xFFFFFF00;
                }
            }
        }
        
        // Band indicator
        uint32_t band_color = (net->band == WIFI_BAND_5GHZ) ? 0xFF0088FF :
                              (net->band == WIFI_BAND_6GHZ) ? 0xFF8800FF : 0xFF00FF00;
        for (int row = y + 10; row < y + 14 && row < h; row++) {
            for (int col = w - 40; col < w - 30 && col < w; col++) {
                buffer[row * w + col] = band_color;
            }
        }
    }
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

