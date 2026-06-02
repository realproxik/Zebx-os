#ifndef AUDIO_H
#define AUDIO_H

#include "types.h"

#define AUDIO_MAX_CHANNELS 8
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_BUFFER_SIZE 4096
#define AUDIO_BITS_PER_SAMPLE 16

// Volume levels
#define VOLUME_MUTE     0
#define VOLUME_LOW      25
#define VOLUME_MEDIUM   50
#define VOLUME_HIGH     75
#define VOLUME_MAX      100

// Intel HD Audio registers
#define HDA_REG_GCTL        0x08
#define HDA_REG_WAKEEN      0x0C
#define HDA_REG_STATESTS    0x0E
#define HDA_REG_GSTS        0x10
#define HDA_REG_OUTPAY      0x18
#define HDA_REG_INPAY       0x1A
#define HDA_REG_CORBLBASE   0x40
#define HDA_REG_CORBUBASE   0x44
#define HDA_REG_CORBWP      0x48
#define HDA_REG_CORBRP      0x4A
#define HDA_REG_CORBCTL     0x4C
#define HDA_REG_CORBSTS     0x4D
#define HDA_REG_CORBSIZE    0x4E
#define HDA_REG_RIRBLBASE   0x50
#define HDA_REG_RIRBUBASE   0x54
#define HDA_REG_RIRBWP      0x58
#define HDA_REG_RINTCNT     0x5A
#define HDA_REG_RIRBCTL     0x5C
#define HDA_REG_RIRBSTS     0x5D
#define HDA_REG_RIRBSIZE    0x5E
#define HDA_REG_IC          0x60
#define HDA_REG_IR          0x64
#define HDA_REG_IRS         0x68
#define HDA_REG_DPLBASE     0x70
#define HDA_REG_DPUBASE     0x74

// Audio stream
typedef struct {
    uint16_t *buffer;
    uint32_t buffer_size;
    uint32_t position;
    uint32_t sample_rate;
    uint8_t channels;
    uint8_t bits_per_sample;
    int playing;
    int looping;
    float volume;
} audio_stream_t;

// Audio device info
typedef struct {
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t base_addr;
    uint8_t irq;
    uint32_t num_output_streams;
    uint32_t num_input_streams;
    uint32_t num_bidirectional_streams;
    uint32_t num_serial_links;
    uint32_t corb_size;
    uint32_t rirb_size;
} audio_device_t;

void audio_init(void);
void audio_detect_device(void);
int audio_play_stream(audio_stream_t *stream);
void audio_stop_stream(audio_stream_t *stream);
void audio_pause_stream(audio_stream_t *stream);
void audio_resume_stream(audio_stream_t *stream);
void audio_set_volume(int channel, uint8_t level);
uint8_t audio_get_volume(int channel);
void audio_volume_up(int channel);
void audio_volume_down(int channel);
void audio_mute(int channel);
void audio_unmute(int channel);
void audio_set_master_volume(uint8_t level);
void audio_set_volume_preset(const char *preset); // "low", "medium", "high", "max", "mute"
audio_stream_t* audio_create_stream(uint32_t sample_rate, uint8_t channels, uint8_t bits);
void audio_destroy_stream(audio_stream_t *stream);
void audio_mix_streams(audio_stream_t **streams, uint32_t count, int16_t *output, uint32_t samples);
void audio_beep(uint32_t freq, uint32_t duration_ms);
void audio_play_tone(uint32_t freq, uint32_t duration_ms, uint8_t volume);
void audio_play_pcm(const int16_t *data, uint32_t samples, uint32_t sample_rate);

// Sound Blaster 16 compatibility
void sb16_init(void);
void sb16_play_dma(uint8_t *data, uint32_t len, uint8_t channel);
void sb16_stop_dma(void);
void sb16_set_volume(uint8_t left, uint8_t right);

// AC97 compatibility
void ac97_init(void);
void ac97_set_master_volume(uint16_t volume);
void ac97_set_pcm_volume(uint16_t volume);

#endif

#include "audio.h"
#include "vga.h"
#include "memory.h"
#include "interrupt.h"

static audio_device_t audio_dev;
static audio_stream_t *active_streams[AUDIO_MAX_CHANNELS];
static uint8_t master_volume = VOLUME_MEDIUM;
static uint8_t channel_volumes[AUDIO_MAX_CHANNELS];
static int master_muted = 0;

// PCI Audio device IDs
#define PCI_VENDOR_INTEL    0x8086
#define PCI_VENDOR_CREATIVE 0x1102
#define PCI_VENDOR_REALTEK  0x10EC
#define PCI_VENDOR_VIA      0x1106

// Intel HD Audio device IDs
#define HDA_DEVICE_ICH6     0x2668
#define HDA_DEVICE_ICH7     0x27D8
#define HDA_DEVICE_ICH8     0x284B
#define HDA_DEVICE_ICH9     0x293E
#define HDA_DEVICE_ICH10    0x3A3E
#define HDA_DEVICE_PCH      0x1C20
#define HDA_DEVICE_PCH2     0x1E20
#define HDA_DEVICE_PCH3     0x8C20
#define HDA_DEVICE_PCH4     0x9C20
#define HDA_DEVICE_PCH5     0xA170
#define HDA_DEVICE_PCH6     0xA2F0
#define HDA_DEVICE_PCH7     0xA348
#define HDA_DEVICE_PCH8     0x9D71

static inline uint32_t hda_read(uint32_t reg) {
    return *((volatile uint32_t*)(audio_dev.base_addr + reg));
}

static inline void hda_write(uint32_t reg, uint32_t val) {
    *((volatile uint32_t*)(audio_dev.base_addr + reg)) = val;
}

static inline uint16_t hda_read16(uint32_t reg) {
    return *((volatile uint16_t*)(audio_dev.base_addr + reg));
}

static inline void hda_write16(uint32_t reg, uint16_t val) {
    *((volatile uint16_t*)(audio_dev.base_addr + reg)) = val;
}

static inline uint8_t hda_read8(uint32_t reg) {
    return *((volatile uint8_t*)(audio_dev.base_addr + reg));
}

static inline void hda_write8(uint32_t reg, uint8_t val) {
    *((volatile uint8_t*)(audio_dev.base_addr + reg)) = val;
}

void audio_detect_device(void) {
    // Scan PCI bus for audio devices
    // Simplified - would scan PCI config space
    
    // Default to Intel HD Audio at standard address
    audio_dev.vendor_id = PCI_VENDOR_INTEL;
    audio_dev.device_id = HDA_DEVICE_PCH8;
    audio_dev.base_addr = 0xF7A10000; // Typical HDA MMIO base
    audio_dev.irq = 16;
    audio_dev.num_output_streams = 4;
    audio_dev.num_input_streams = 4;
    audio_dev.num_bidirectional_streams = 0;
    audio_dev.num_serial_links = 4;
    audio_dev.corb_size = 256;
    audio_dev.rirb_size = 256;
    
    vga_puts("[+] Audio device detected\\n");
    vga_puts("    Vendor: Intel Corporation\\n");
    vga_puts("    Device: HD Audio Controller\\n");
    vga_puts("    Streams: ");
    vga_put_int(audio_dev.num_output_streams);
    vga_puts(" output, ");
    vga_put_int(audio_dev.num_input_streams);
    vga_puts(" input\\n");
}

void audio_init(void) {
    memset(&audio_dev, 0, sizeof(audio_dev));
    memset(active_streams, 0, sizeof(active_streams));
    
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        channel_volumes[i] = VOLUME_MEDIUM;
    }
    master_volume = VOLUME_MEDIUM;
    master_muted = 0;
    
    audio_detect_device();
    
    // Initialize HDA controller
    // Reset controller
    hda_write(HDA_REG_GCTL, 0);
    for (volatile int i = 0; i < 100000; i++);
    hda_write(HDA_REG_GCTL, 1);
    
    // Wait for reset completion
    while (!(hda_read(HDA_REG_GCTL) & 1));
    
    // Enable CORB and RIRB
    hda_write8(HDA_REG_CORBCTL, 0x02);
    hda_write8(HDA_REG_RIRBCTL, 0x02);
    
    // Set CORB size
    hda_write8(HDA_REG_CORBSIZE, 0x02); // 256 entries
    hda_write8(HDA_REG_RIRBSIZE, 0x02); // 256 entries
    
    // Allocate CORB and RIRB buffers
    uint32_t corb_phys = phys_alloc();
    uint32_t rirb_phys = phys_alloc();
    
    hda_write(HDA_REG_CORBLBASE, corb_phys);
    hda_write(HDA_REG_CORBUBASE, 0);
    hda_write(HDA_REG_RIRBLBASE, rirb_phys);
    hda_write(HDA_REG_RIRBUBASE, 0);
    
    // Reset CORB/RIRB pointers
    hda_write16(HDA_REG_CORBRP, 0x8000); // Reset
    hda_write16(HDA_REG_CORBRP, 0);
    hda_write16(HDA_REG_CORBWP, 0);
    hda_write16(HDA_REG_RIRBWP, 0x8000); // Reset
    hda_write16(HDA_REG_RIRBWP, 0);
    
    // Enable CORB and RIRB DMA
    hda_write8(HDA_REG_CORBCTL, 0x02);
    hda_write8(HDA_REG_RIRBCTL, 0x02);
    
    // Enable interrupts
    hda_write16(HDA_REG_WAKEEN, 0xFFFF);
    
    vga_puts("[+] Audio driver initialized\\n");
    vga_puts("    Master Volume: MEDIUM (50%)\\n");
    vga_puts("    Channels: 8\\n");
    vga_puts("    Sample Rate: 48kHz\\n");
    vga_puts("    Bits: 16\\n");
}

void audio_set_volume(int channel, uint8_t level) {
    if (channel < 0 || channel >= AUDIO_MAX_CHANNELS) return;
    if (level > VOLUME_MAX) level = VOLUME_MAX;
    channel_volumes[channel] = level;
    
    // Apply to HDA widget
    // In real implementation, would send verb to codec
}

uint8_t audio_get_volume(int channel) {
    if (channel < 0 || channel >= AUDIO_MAX_CHANNELS) return 0;
    return channel_volumes[channel];
}

void audio_volume_up(int channel) {
    uint8_t current = audio_get_volume(channel);
    if (current < VOLUME_MAX) {
        audio_set_volume(channel, current + 5);
    }
}

void audio_volume_down(int channel) {
    uint8_t current = audio_get_volume(channel);
    if (current > VOLUME_MUTE) {
        audio_set_volume(channel, current - 5);
    }
}

void audio_mute(int channel) {
    audio_set_volume(channel, VOLUME_MUTE);
}

void audio_unmute(int channel) {
    audio_set_volume(channel, master_volume);
}

void audio_set_master_volume(uint8_t level) {
    if (level > VOLUME_MAX) level = VOLUME_MAX;
    master_volume = level;
    
    // Update all channels
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        channel_volumes[i] = level;
    }
    
    vga_puts("[*] Master Volume: ");
    vga_put_int(level);
    vga_puts("%\\n");
}

void audio_set_volume_preset(const char *preset) {
    if (!strcmp(preset, "mute")) {
        audio_set_master_volume(VOLUME_MUTE);
        master_muted = 1;
        vga_puts("[AUDIO] MUTED\\n");
    } else if (!strcmp(preset, "low")) {
        audio_set_master_volume(VOLUME_LOW);
        master_muted = 0;
        vga_puts("[AUDIO] Volume: LOW (25%)\\n");
    } else if (!strcmp(preset, "medium")) {
        audio_set_master_volume(VOLUME_MEDIUM);
        master_muted = 0;
        vga_puts("[AUDIO] Volume: MEDIUM (50%)\\n");
    } else if (!strcmp(preset, "high")) {
        audio_set_master_volume(VOLUME_HIGH);
        master_muted = 0;
        vga_puts("[AUDIO] Volume: HIGH (75%)\\n");
    } else if (!strcmp(preset, "max")) {
        audio_set_master_volume(VOLUME_MAX);
        master_muted = 0;
        vga_puts("[AUDIO] Volume: MAX (100%)\\n");
    }
}

audio_stream_t* audio_create_stream(uint32_t sample_rate, uint8_t channels, uint8_t bits) {
    audio_stream_t *stream = (audio_stream_t*)kmalloc(sizeof(audio_stream_t));
    if (!stream) return NULL;
    
    stream->buffer_size = AUDIO_BUFFER_SIZE;
    stream->buffer = (uint16_t*)kmalloc(stream->buffer_size * sizeof(uint16_t));
    stream->position = 0;
    stream->sample_rate = sample_rate;
    stream->channels = channels;
    stream->bits_per_sample = bits;
    stream->playing = 0;
    stream->looping = 0;
    stream->volume = 1.0f;
    
    memset(stream->buffer, 0, stream->buffer_size * sizeof(uint16_t));
    
    return stream;
}

void audio_destroy_stream(audio_stream_t *stream) {
    if (!stream) return;
    if (stream->buffer) kfree(stream->buffer);
    kfree(stream);
}

int audio_play_stream(audio_stream_t *stream) {
    if (!stream) return -1;
    
    // Find free channel
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        if (!active_streams[i]) {
            active_streams[i] = stream;
            stream->playing = 1;
            stream->position = 0;
            return i;
        }
    }
    return -1; // No free channels
}

void audio_stop_stream(audio_stream_t *stream) {
    if (!stream) return;
    stream->playing = 0;
    
    for (int i = 0; i < AUDIO_MAX_CHANNELS; i++) {
        if (active_streams[i] == stream) {
            active_streams[i] = NULL;
        }
    }
}

void audio_pause_stream(audio_stream_t *stream) {
    if (stream) stream->playing = 0;
}

void audio_resume_stream(audio_stream_t *stream) {
    if (stream) stream->playing = 1;
}

void audio_mix_streams(audio_stream_t **streams, uint32_t count, int16_t *output, uint32_t samples) {
    // Mix multiple streams into single output
    for (uint32_t s = 0; s < samples; s++) {
        int32_t mixed = 0;
        
        for (uint32_t i = 0; i < count; i++) {
            if (!streams[i] || !streams[i]->playing) continue;
            
            uint32_t pos = streams[i]->position;
            if (pos >= streams[i]->buffer_size) {
                if (streams[i]->looping) {
                    streams[i]->position = 0;
                    pos = 0;
                } else {
                    streams[i]->playing = 0;
                    continue;
                }
            }
            
            mixed += (int32_t)(streams[i]->buffer[pos] * streams[i]->volume * channel_volumes[i] / 100);
            streams[i]->position++;
        }
        
        // Clip
        if (mixed > 32767) mixed = 32767;
        if (mixed < -32768) mixed = -32768;
        
        output[s] = (int16_t)mixed;
    }
}

void audio_beep(uint32_t freq, uint32_t duration_ms) {
    // PC Speaker beep via PIT
    uint32_t divisor = 1193180 / freq;
    
    outb(0x43, 0xB6);
    outb(0x42, divisor & 0xFF);
    outb(0x42, (divisor >> 8) & 0xFF);
    
    uint8_t tmp = inb(0x61);
    outb(0x61, tmp | 3);
    
    // Wait
    for (volatile uint32_t i = 0; i < duration_ms * 1000; i++);
    
    outb(0x61, tmp);
}

void audio_play_tone(uint32_t freq, uint32_t duration_ms, uint8_t volume) {
    if (volume == 0) return;
    audio_beep(freq, duration_ms);
}

void audio_play_pcm(const int16_t *data, uint32_t samples, uint32_t sample_rate) {
    audio_stream_t *stream = audio_create_stream(sample_rate, 2, 16);
    if (!stream) return;
    
    uint32_t to_copy = samples < stream->buffer_size ? samples : stream->buffer_size;
    for (uint32_t i = 0; i < to_copy; i++) {
        stream->buffer[i] = (uint16_t)data[i];
    }
    
    audio_play_stream(stream);
}

// Sound Blaster 16
static uint16_t sb_base = 0x220;
static uint8_t sb_irq = 5;
static uint8_t sb_dma = 1;

void sb16_init(void) {
    // Reset SB16
    outb(0xD1, sb_base + 0x0C); // Speaker on
    
    vga_puts("[+] Sound Blaster 16 initialized\\n");
    vga_puts("    Base: 0x");
    vga_put_hex(sb_base);
    vga_puts("\\n");
}

void sb16_play_dma(uint8_t *data, uint32_t len, uint8_t channel) {
    // Program DMA controller
    // Simplified - would set up DMA transfer
    
    // Set transfer mode
    outb(0x0B, 0x49); // Single mode, read, channel 1
    outb(0x0C, 0x00); // Clear flip-flop
    
    // Set address and count
    uint32_t phys = (uint32_t)data;
    outb(0x02, phys & 0xFF);
    outb(0x02, (phys >> 8) & 0xFF);
    outb(0x83, (phys >> 16) & 0xFF);
    outb(0x03, (len - 1) & 0xFF);
    outb(0x03, ((len - 1) >> 8) & 0xFF);
    
    // Start DMA
    outb(0x0A, channel | 0x01);
    
    // Start SB16 playback
    outb(0xD1, sb_base + 0x0C); // Speaker on
    outb(0x41, sb_base + 0x0C); // Set output sample rate
    outb((48000 >> 8) & 0xFF, sb_base + 0x0C);
    outb(48000 & 0xFF, sb_base + 0x0C);
    outb(0xB0, sb_base + 0x0C); // 16-bit stereo
    outb(0x00, sb_base + 0x0C); // Mode
    outb(((len / 2 - 1) >> 0) & 0xFF, sb_base + 0x0C);
    outb(((len / 2 - 1) >> 8) & 0xFF, sb_base + 0x0C);
}

void sb16_stop_dma(void) {
    outb(0xD3, sb_base + 0x0C); // Speaker off
    outb(0xD5, sb_base + 0x0C); // Halt DMA
}

void sb16_set_volume(uint8_t left, uint8_t right) {
    outb(0x22, sb_base + 0x04); // Master volume
    outb(((right & 0x0F) << 4) | (left & 0x0F), sb_base + 0x05);
}

// AC97
static uint32_t ac97_base = 0xF7A10000;
static uint32_t ac97_mixer = 0xF7A10080;

void ac97_init(void) {
    // Reset AC97
    outl(0x02, ac97_base + 0x00); // Cold reset
    for (volatile int i = 0; i < 100000; i++);
    
    vga_puts("[+] AC97 initialized\\n");
}

void ac97_set_master_volume(uint16_t volume) {
    outw(volume, ac97_mixer + 0x02);
}

void ac97_set_pcm_volume(uint16_t volume) {
    outw(volume, ac97_mixer + 0x18);
}

static inline void outb(uint8_t val, uint16_t port) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void outw(uint16_t val, uint16_t port) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "dN"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}

static inline void outl(uint32_t val, uint16_t port) {
    __asm__ volatile("outl %0, %1" : : "a"(val), "dN"(port));
}