#ifndef TIMER_H
#define TIMER_H

#include "types.h"

#define TIMER_FREQ_HZ       1000
#define PIT_BASE_FREQ       1193182
#define HPET_BASE_ADDR      0xFED00000

// RTC registers
#define RTC_INDEX_PORT      0x70
#define RTC_DATA_PORT       0x71
#define RTC_REG_SECONDS     0x00
#define RTC_REG_MINUTES     0x02
#define RTC_REG_HOURS       0x04
#define RTC_REG_DAY         0x07
#define RTC_REG_MONTH       0x08
#define RTC_REG_YEAR        0x09
#define RTC_REG_CENTURY     0x32
#define RTC_REG_STATUS_A    0x0A
#define RTC_REG_STATUS_B    0x0B

// HPET registers
#define HPET_GENERAL_CAP    0x000
#define HPET_GENERAL_CONFIG 0x010
#define HPET_GENERAL_STATUS 0x020
#define HPET_MAIN_COUNTER   0x0F0
#define HPET_TIMER_CONFIG   0x100
#define HPET_TIMER_COMPARATOR 0x108
#define HPET_TIMER_FSB      0x110

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    uint8_t century;
} datetime_t;

typedef struct {
    uint64_t ticks;
    uint64_t uptime_ms;
    uint64_t boot_time;
    uint32_t freq_hz;
    int hpet_available;
    int tsc_available;
    uint64_t tsc_freq;
} timer_state_t;

void timer_init(void);
void pit_init(uint32_t freq);
void hpet_init(void);
uint64_t hpet_read_counter(void);
void hpet_wait_ms(uint32_t ms);
void pit_wait_ms(uint32_t ms);

// RTC
datetime_t rtc_read(void);
void rtc_get_time(datetime_t *dt);
void rtc_get_date(datetime_t *dt);
uint64_t rtc_to_timestamp(datetime_t *dt);
void rtc_from_timestamp(uint64_t ts, datetime_t *dt);

// System time
uint64_t timer_get_ticks(void);
uint64_t timer_get_uptime_ms(void);
uint64_t timer_get_uptime_seconds(void);
void timer_delay_ms(uint32_t ms);
void timer_delay_us(uint32_t us);
void timer_sleep(uint32_t ms);

// High precision
uint64_t timer_read_tsc(void);
uint64_t timer_tsc_to_ns(uint64_t tsc);
void timer_calibrate_tsc(void);

// Alarm / callbacks
typedef void (*timer_callback_t)(void);
void timer_register_callback(timer_callback_t cb, uint32_t interval_ms);
void timer_unregister_callback(timer_callback_t cb);
void timer_handle_irq(void);

// Formatting
void timer_format_time(char *buf, uint32_t max_len, datetime_t *dt);
void timer_format_uptime(char *buf, uint32_t max_len);

#endif

#include "timer.h"
#include "vga.h"
#include "interrupt.h"

static timer_state_t timer_state;
static timer_callback_t callbacks[16];
static uint32_t callback_intervals[16];
static uint32_t callback_counters[16];
static uint32_t num_callbacks = 0;

static inline uint8_t rtc_read_reg(uint8_t reg) {
    outb(reg, RTC_INDEX_PORT);
    return inb(RTC_DATA_PORT);
}

static inline void rtc_write_reg(uint8_t reg, uint8_t val) {
    outb(reg, RTC_INDEX_PORT);
    outb(val, RTC_DATA_PORT);
}

static inline uint8_t bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline uint8_t bin_to_bcd(uint8_t bin) {
    return ((bin / 10) << 4) | (bin % 10);
}

void pit_init(uint32_t freq) {
    uint32_t divisor = PIT_BASE_FREQ / freq;
    if (divisor > 65535) divisor = 65535;
    
    outb(0x36, 0x43); // Channel 0, lobyte/hibyte, mode 3
    outb(divisor & 0xFF, 0x40);
    outb((divisor >> 8) & 0xFF, 0x40);
    
    timer_state.freq_hz = freq;
    
    vga_puts("[+] PIT initialized\\n");
    vga_puts("    Frequency: ");
    vga_put_int(freq);
    vga_puts(" Hz\\n");
}

void hpet_init(void) {
    // Check if HPET exists via ACPI
    // Simplified - assume available on modern systems
    timer_state.hpet_available = 1;
    
    // Read capabilities
    uint64_t cap = *((volatile uint64_t*)(HPET_BASE_ADDR + HPET_GENERAL_CAP));
    uint32_t period_fs = (uint32_t)(cap >> 32);
    uint32_t num_timers = ((uint32_t)cap >> 8) & 0x1F;
    
    // Enable HPET
    *((volatile uint64_t*)(HPET_BASE_ADDR + HPET_GENERAL_CONFIG)) = 1;
    
    vga_puts("[+] HPET initialized\\n");
    vga_puts("    Period: ");
    vga_put_int(period_fs);
    vga_puts(" fs\\n");
    vga_puts("    Timers: ");
    vga_put_int(num_timers);
    vga_puts("\\n");
}

uint64_t hpet_read_counter(void) {
    return *((volatile uint64_t*)(HPET_BASE_ADDR + HPET_MAIN_COUNTER));
}

void hpet_wait_ms(uint32_t ms) {
    if (!timer_state.hpet_available) {
        pit_wait_ms(ms);
        return;
    }
    
    uint64_t start = hpet_read_counter();
    uint64_t ticks = (uint64_t)ms * 1000000000000ULL / 14318180; // Approximate
    
    while ((hpet_read_counter() - start) < ticks);
}

void pit_wait_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        // Wait for PIT tick
        uint8_t status;
        do {
            outb(0xE2, 0x43); // Read back command
            status = inb(0x40);
        } while (status & 0x80);
    }
}

void timer_init(void) {
    memset(&timer_state, 0, sizeof(timer_state));
    timer_state.ticks = 0;
    timer_state.uptime_ms = 0;
    timer_state.boot_time = 0;
    timer_state.tsc_available = 1;
    
    pit_init(TIMER_FREQ_HZ);
    
    // Try HPET
    hpet_init();
    
    // Calibrate TSC
    timer_calibrate_tsc();
    
    // Read initial RTC time
    datetime_t dt = rtc_read();
    timer_state.boot_time = rtc_to_timestamp(&dt);
    
    vga_puts("[+] Timer system initialized\\n");
    vga_puts("    TSC: ");
    vga_puts(timer_state.tsc_available ? "Available" : "Not Available");
    vga_puts("\\n");
    vga_puts("    TSC Freq: ");
    vga_put_int((uint32_t)(timer_state.tsc_freq / 1000000));
    vga_puts(" MHz\\n");
}

datetime_t rtc_read(void) {
    datetime_t dt;
    
    // Wait for update to complete
    while (rtc_read_reg(RTC_REG_STATUS_A) & 0x80);
    
    uint8_t status_b = rtc_read_reg(RTC_REG_STATUS_B);
    int bcd_mode = !(status_b & 0x04);
    int hour_24 = (status_b & 0x02) != 0;
    
    dt.second = rtc_read_reg(RTC_REG_SECONDS);
    dt.minute = rtc_read_reg(RTC_REG_MINUTES);
    dt.hour = rtc_read_reg(RTC_REG_HOURS);
    dt.day = rtc_read_reg(RTC_REG_DAY);
    dt.month = rtc_read_reg(RTC_REG_MONTH);
    dt.year = rtc_read_reg(RTC_REG_YEAR);
    dt.century = rtc_read_reg(RTC_REG_CENTURY);
    
    if (bcd_mode) {
        dt.second = bcd_to_bin(dt.second);
        dt.minute = bcd_to_bin(dt.minute);
        dt.hour = bcd_to_bin(dt.hour);
        dt.day = bcd_to_bin(dt.day);
        dt.month = bcd_to_bin(dt.month);
        dt.year = bcd_to_bin(dt.year);
        dt.century = bcd_to_bin(dt.century);
    }
    
    // Convert 12-hour to 24-hour if needed
    if (!hour_24 && (dt.hour & 0x80)) {
        dt.hour = ((dt.hour & 0x7F) + 12) % 24;
    }
    
    dt.year += dt.century * 100;
    
    return dt;
}

void rtc_get_time(datetime_t *dt) {
    *dt = rtc_read();
}

void rtc_get_date(datetime_t *dt) {
    *dt = rtc_read();
}

uint64_t rtc_to_timestamp(datetime_t *dt) {
    // Simplified - not accounting for leap years fully
    uint64_t days = 0;
    
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    for (uint16_t y = 1970; y < dt->year; y++) {
        days += 365;
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) days++;
    }
    
    for (uint8_t m = 1; m < dt->month; m++) {
        days += days_in_month[m - 1];
        if (m == 2 && ((dt->year % 4 == 0 && dt->year % 100 != 0) || (dt->year % 400 == 0))) days++;
    }
    
    days += dt->day - 1;
    
    uint64_t seconds = days * 86400;
    seconds += dt->hour * 3600;
    seconds += dt->minute * 60;
    seconds += dt->second;
    
    return seconds;
}

void rtc_from_timestamp(uint64_t ts, datetime_t *dt) {
    dt->second = ts % 60; ts /= 60;
    dt->minute = ts % 60; ts /= 60;
    dt->hour = ts % 24; ts /= 24;
    
    // Simplified date calculation
    uint64_t days = ts;
    uint16_t year = 1970;
    while (days >= 365) {
        uint16_t leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) ? 366 : 365;
        if (days >= leap) {
            days -= leap;
            year++;
        } else {
            break;
        }
    }
    dt->year = year;
    
    static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t month = 1;
    while (days >= days_in_month[month - 1]) {
        uint8_t dim = days_in_month[month - 1];
        if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))) dim++;
        if (days >= dim) {
            days -= dim;
            month++;
        } else {
            break;
        }
    }
    dt->month = month;
    dt->day = days + 1;
    dt->century = dt->year / 100;
}

uint64_t timer_get_ticks(void) {
    return timer_state.ticks;
}

uint64_t timer_get_uptime_ms(void) {
    return timer_state.uptime_ms;
}

uint64_t timer_get_uptime_seconds(void) {
    return timer_state.uptime_ms / 1000;
}

void timer_delay_ms(uint32_t ms) {
    if (timer_state.hpet_available) {
        hpet_wait_ms(ms);
    } else {
        pit_wait_ms(ms);
    }
}

void timer_delay_us(uint32_t us) {
    // Use TSC for microsecond delays
    if (timer_state.tsc_available) {
        uint64_t start = timer_read_tsc();
        uint64_t ticks = (uint64_t)us * timer_state.tsc_freq / 1000000;
        while ((timer_read_tsc() - start) < ticks);
    } else {
        // Fallback to busy wait
        for (volatile uint32_t i = 0; i < us * 100; i++);
    }
}

void timer_sleep(uint32_t ms) {
    uint64_t target = timer_state.uptime_ms + ms;
    while (timer_state.uptime_ms < target) {
        __asm__ volatile("hlt");
    }
}

uint64_t timer_read_tsc(void) {
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

uint64_t timer_tsc_to_ns(uint64_t tsc) {
    return (tsc * 1000000000ULL) / timer_state.tsc_freq;
}

void timer_calibrate_tsc(void) {
    // Calibrate TSC against PIT
    uint64_t tsc_start = timer_read_tsc();
    
    // Wait ~100ms using PIT
    pit_wait_ms(100);
    
    uint64_t tsc_end = timer_read_tsc();
    uint64_t tsc_diff = tsc_end - tsc_start;
    
    // TSC frequency = ticks per 0.1s * 10
    timer_state.tsc_freq = tsc_diff * 10;
}

void timer_register_callback(timer_callback_t cb, uint32_t interval_ms) {
    if (num_callbacks >= 16) return;
    callbacks[num_callbacks] = cb;
    callback_intervals[num_callbacks] = interval_ms;
    callback_counters[num_callbacks] = 0;
    num_callbacks++;
}

void timer_unregister_callback(timer_callback_t cb) {
    for (uint32_t i = 0; i < num_callbacks; i++) {
        if (callbacks[i] == cb) {
            for (uint32_t j = i; j < num_callbacks - 1; j++) {
                callbacks[j] = callbacks[j + 1];
                callback_intervals[j] = callback_intervals[j + 1];
                callback_counters[j] = callback_counters[j + 1];
            }
            num_callbacks--;
            return;
        }
    }
}

void timer_handle_irq(void) {
    timer_state.ticks++;
    timer_state.uptime_ms += 1000 / timer_state.freq_hz;
    
    // Process callbacks
    for (uint32_t i = 0; i < num_callbacks; i++) {
        callback_counters[i] += 1000 / timer_state.freq_hz;
        if (callback_counters[i] >= callback_intervals[i]) {
            callback_counters[i] = 0;
            if (callbacks[i]) callbacks[i]();
        }
    }
}

void timer_format_time(char *buf, uint32_t max_len, datetime_t *dt) {
    // Format: YYYY-MM-DD HH:MM:SS
    if (max_len < 20) return;
    
    buf[0] = '0' + (dt->year / 1000);
    buf[1] = '0' + ((dt->year / 100) % 10);
    buf[2] = '0' + ((dt->year / 10) % 10);
    buf[3] = '0' + (dt->year % 10);
    buf[4] = '-';
    buf[5] = '0' + (dt->month / 10);
    buf[6] = '0' + (dt->month % 10);
    buf[7] = '-';
    buf[8] = '0' + (dt->day / 10);
    buf[9] = '0' + (dt->day % 10);
    buf[10] = ' ';
    buf[11] = '0' + (dt->hour / 10);
    buf[12] = '0' + (dt->hour % 10);
    buf[13] = ':';
    buf[14] = '0' + (dt->minute / 10);
    buf[15] = '0' + (dt->minute % 10);
    buf[16] = ':';
    buf[17] = '0' + (dt->second / 10);
    buf[18] = '0' + (dt->second % 10);
    buf[19] = 0;
}

void timer_format_uptime(char *buf, uint32_t max_len) {
    uint64_t seconds = timer_state.uptime_ms / 1000;
    uint64_t days = seconds / 86400;
    uint64_t hours = (seconds % 86400) / 3600;
    uint64_t minutes = (seconds % 3600) / 60;
    uint64_t secs = seconds % 60;
    
    if (days > 0) {
        // Format: Xd Yh Zm Ws
        int pos = 0;
        pos += sprintf(buf + pos, "%d", (int)days); buf[pos++] = 'd'; buf[pos++] = ' ';
        pos += sprintf(buf + pos, "%d", (int)hours); buf[pos++] = 'h'; buf[pos++] = ' ';
        pos += sprintf(buf + pos, "%d", (int)minutes); buf[pos++] = 'm'; buf[pos++] = ' ';
        pos += sprintf(buf + pos, "%d", (int)secs); buf[pos++] = 's'; buf[pos] = 0;
    } else {
        int pos = 0;
        pos += sprintf(buf + pos, "%d", (int)hours); buf[pos++] = 'h'; buf[pos++] = ' ';
        pos += sprintf(buf + pos, "%d", (int)minutes); buf[pos++] = 'm'; buf[pos++] = ' ';
        pos += sprintf(buf + pos, "%d", (int)secs); buf[pos++] = 's'; buf[pos] = 0;
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

static inline int sprintf(char *buf, const char *fmt, ...) {
    // Simplified sprintf - just handles %d
    int num = 0;
    va_list args;
    va_start(args, fmt);
    
    const char *p = fmt;
    while (*p) {
        if (*p == '%' && *(p+1) == 'd') {
            int val = va_arg(args, int);
            if (val == 0) {
                buf[num++] = '0';
            } else {
                char tmp[16];
                int i = 0;
                int neg = val < 0;
                if (neg) val = -val;
                while (val > 0) {
                    tmp[i++] = '0' + (val % 10);
                    val /= 10;
                }
                if (neg) buf[num++] = '-';
                for (int j = i - 1; j >= 0; j--) {
                    buf[num++] = tmp[j];
                }
            }
            p += 2;
        } else {
            buf[num++] = *p++;
        }
    }
    va_end(args);
    return num;
}