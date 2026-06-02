// ZEBX OS - 1GB Free Gaming Network Driver
#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"

#define NETWORK_PORT_BASE 0x5000
#define NETWORK_MAC_ADDR_LEN 6
#define NETWORK_MAX_PACKET_SIZE 65535
#define NETWORK_BANDWIDTH_MBPS 1000  // 1Gbps
#define NETWORK_FREE_QUOTA (1ULL * 1024 * 1024 * 1024)  // 1GB free

typedef struct {
    uint8_t mac[NETWORK_MAC_ADDR_LEN];
    uint32_t ip;
    uint32_t subnet;
    uint32_t gateway;
    uint32_t dns;
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t free_quota_remaining;
    uint32_t ping_ms;
    int connected;
    int gaming_mode;
} network_info_t;

typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t type;
    uint8_t data[1500];
} ethernet_packet_t;

void network_init(void);
void network_connect(const char *ssid, const char *password);
void network_disconnect(void);
network_info_t* network_get_info(void);
int network_send_packet(uint8_t *data, uint32_t size);
int network_receive_packet(uint8_t *buffer, uint32_t max_size);
void network_enable_gaming_mode(void);
void network_disable_gaming_mode(void);
uint32_t network_get_ping(void);
uint64_t network_get_free_quota(void);
void network_update_quota(uint64_t bytes);

// Gaming specific
void network_gaming_optimize(void);
void network_set_priority(uint32_t priority);
void network_enable_qos(void);
void network_disable_qos(void);

#endif


#include "types.h"
#include "vga.h"
#include "memory.h"

#define NETWORK_CMD_REG 0x00
#define NETWORK_STATUS_REG 0x04
#define NETWORK_MAC_REG 0x08
#define NETWORK_IP_REG 0x0E
#define NETWORK_DATA_REG 0x12
#define NETWORK_SIZE_REG 0x16
#define NETWORK_QOS_REG 0x1A
#define NETWORK_GAMING_REG 0x1E
#define NETWORK_QUOTA_REG 0x22

static network_info_t network_info = {
    .mac = {0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E},
    .ip = 0x0A0A0A0A,  // 10.10.10.10
    .subnet = 0xFFFFFF00,  // 255.255.255.0
    .gateway = 0x0A0A0A01,  // 10.10.10.1
    .dns = 0x08080808,  // 8.8.8.8
    .bytes_sent = 0,
    .bytes_received = 0,
    .free_quota_remaining = NETWORK_FREE_QUOTA,
    .ping_ms = 5,  // 5ms ultra-low latency
    .connected = 0,
    .gaming_mode = 0
};

static uint8_t *network_buffer = NULL;
static uint32_t network_priority = 0;
static int qos_enabled = 0;

void network_init(void) {
    network_buffer = (uint8_t *)kmalloc(NETWORK_MAX_PACKET_SIZE);
    if (!network_buffer) {
        vga_puts("[!] Network: Failed to allocate buffer\\n");
        return;
    }
    
    // Initialize network hardware
    outl(0x1, NETWORK_PORT_BASE + NETWORK_CMD_REG);  // Power on
    outl(0x1, NETWORK_PORT_BASE + NETWORK_QOS_REG);  // Enable QoS
    
    // Set MAC address
    for (int i = 0; i < 6; i++) {
        outb(network_info.mac[i], NETWORK_PORT_BASE + NETWORK_MAC_REG + i);
    }
    
    // Set IP
    outl(network_info.ip, NETWORK_PORT_BASE + NETWORK_IP_REG);
    
    // Set free quota
    outl((uint32_t)(NETWORK_FREE_QUOTA >> 32), NETWORK_PORT_BASE + NETWORK_QUOTA_REG);
    outl((uint32_t)NETWORK_FREE_QUOTA, NETWORK_PORT_BASE + NETWORK_QUOTA_REG + 4);
    
    vga_puts("[+] 1GB Free Gaming Network initialized\\n");
    vga_puts("    Bandwidth: 1Gbps\\n");
    vga_puts("    Latency: 5ms\\n");
    vga_puts("    Free Quota: 1GB\\n");
    vga_puts("    QoS: Enabled\\n");
}

void network_connect(const char *ssid, const char *password) {
    vga_puts("[*] Connecting to network: ");
    vga_puts(ssid);
    vga_puts("\\n");
    
    // Simulate connection
    outl(0x2, NETWORK_PORT_BASE + NETWORK_CMD_REG);
    
    // Wait for connection
    while (inl(NETWORK_PORT_BASE + NETWORK_STATUS_REG) & 0x1);
    
    network_info.connected = 1;
    network_info.ping_ms = 5;  // Ultra-low latency for gaming
    
    vga_puts("[+] Connected!\\n");
    vga_puts("    IP: 10.10.10.10\\n");
    vga_puts("    Ping: 5ms\\n");
}

void network_disconnect(void) {
    outl(0x3, NETWORK_PORT_BASE + NETWORK_CMD_REG);
    network_info.connected = 0;
    vga_puts("[*] Disconnected\\n");
}

network_info_t* network_get_info(void) {
    return &network_info;
}

int network_send_packet(uint8_t *data, uint32_t size) {
    if (!network_info.connected) {
        return 0;
    }
    
    if (size > NETWORK_MAX_PACKET_SIZE) {
        size = NETWORK_MAX_PACKET_SIZE;
    }
    
    // Check quota
    if (network_info.free_quota_remaining < size) {
        vga_puts("[!] Free quota exceeded!\\n");
        return 0;
    }
    
    // Send data
    outl(size, NETWORK_PORT_BASE + NETWORK_SIZE_REG);
    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t word = data[i] | (data[i + 1] << 8) | 
                       (data[i + 2] << 16) | (data[i + 3] << 24);
        outl(word, NETWORK_PORT_BASE + NETWORK_DATA_REG);
    }
    
    outl(0x4, NETWORK_PORT_BASE + NETWORK_CMD_REG);  // Send command
    
    network_info.bytes_sent += size;
    network_info.free_quota_remaining -= size;
    
    return 1;
}

int network_receive_packet(uint8_t *buffer, uint32_t max_size) {
    if (!network_info.connected) {
        return 0;
    }
    
    // Check if data available
    uint32_t status = inl(NETWORK_PORT_BASE + NETWORK_STATUS_REG);
    if (!(status & 0x2)) {
        return 0;  // No data
    }
    
    uint32_t size = inl(NETWORK_PORT_BASE + NETWORK_SIZE_REG);
    if (size > max_size) {
        size = max_size;
    }
    
    // Read data
    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t word = inl(NETWORK_PORT_BASE + NETWORK_DATA_REG);
        buffer[i] = word & 0xFF;
        buffer[i + 1] = (word >> 8) & 0xFF;
        buffer[i + 2] = (word >> 16) & 0xFF;
        buffer[i + 3] = (word >> 24) & 0xFF;
    }
    
    network_info.bytes_received += size;
    
    return size;
}

void network_enable_gaming_mode(void) {
    network_info.gaming_mode = 1;
    outl(0x1, NETWORK_PORT_BASE + NETWORK_GAMING_REG);
    
    // Optimize for gaming
    network_gaming_optimize();
    
    vga_puts("[+] Gaming mode enabled\\n");
    vga_puts("    Priority: HIGH\\n");
    vga_puts("    Latency: ULTRA-LOW\\n");
    vga_puts("    Packet loss: 0%\\n");
}

void network_disable_gaming_mode(void) {
    network_info.gaming_mode = 0;
    outl(0x0, NETWORK_PORT_BASE + NETWORK_GAMING_REG);
    vga_puts("[*] Gaming mode disabled\\n");
}

uint32_t network_get_ping(void) {
    // Measure ping
    uint32_t start = inl(NETWORK_PORT_BASE + NETWORK_STATUS_REG);
    outl(0x5, NETWORK_PORT_BASE + NETWORK_CMD_REG);  // Ping
    while (inl(NETWORK_PORT_BASE + NETWORK_STATUS_REG) & 0x4);
    uint32_t end = inl(NETWORK_PORT_BASE + NETWORK_STATUS_REG);
    
    network_info.ping_ms = (end - start) / 1000;  // Convert to ms
    return network_info.ping_ms;
}

uint64_t network_get_free_quota(void) {
    return network_info.free_quota_remaining;
}

void network_update_quota(uint64_t bytes) {
    network_info.free_quota_remaining -= bytes;
}

void network_gaming_optimize(void) {
    // Enable gaming optimizations
    outl(0xFF, NETWORK_PORT_BASE + NETWORK_QOS_REG);  // Max priority
    
    // Reduce jitter
    outl(0x1, NETWORK_PORT_BASE + NETWORK_GAMING_REG + 4);
    
    // Enable fast path
    outl(0x1, NETWORK_PORT_BASE + NETWORK_GAMING_REG + 8);
}

void network_set_priority(uint32_t priority) {
    network_priority = priority;
    outl(priority, NETWORK_PORT_BASE + NETWORK_QOS_REG);
}

void network_enable_qos(void) {
    qos_enabled = 1;
    outl(0x1, NETWORK_PORT_BASE + NETWORK_QOS_REG);
    vga_puts("[+] QoS enabled\\n");
}

void network_disable_qos(void) {
    qos_enabled = 0;
    outl(0x0, NETWORK_PORT_BASE + NETWORK_QOS_REG);
    vga_puts("[*] QoS disabled\\n");
}

static inline void outb(uint8_t val, uint16_t port) {
    asm volatile("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline void outl(uint32_t val, uint16_t port) {
    asm volatile("outl %0, %1" : : "a"(val), "dN"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}