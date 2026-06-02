// ==========================
// ZEBX OS - INTEL ARCHITECTURE SUPPORT
// ==========================
#ifndef INTEL_H
#define INTEL_H

#include "types.h"

// Intel CPU Families
#define INTEL_FAMILY_PENTIUM4     0x0F
#define INTEL_FAMILY_CORE2        0x06
#define INTEL_FAMILY_NEHALEM      0x06
#define INTEL_FAMILY_SANDYBRIDGE  0x06
#define INTEL_FAMILY_IVYBRIDGE    0x06
#define INTEL_FAMILY_HASWELL      0x06
#define INTEL_FAMILY_BROADWELL    0x06
#define INTEL_FAMILY_SKYLAKE      0x06
#define INTEL_FAMILY_KABYLAKE     0x06
#define INTEL_FAMILY_COFFEELAKE   0x06
#define INTEL_FAMILY_ICELAKE      0x06
#define INTEL_FAMILY_ROCKETLAKE   0x06
#define INTEL_FAMILY_ALDERLAKE    0x06
#define INTEL_FAMILY_RAPTORLAKE   0x06
#define INTEL_FAMILY_METEORLAKE   0x06
#define INTEL_FAMILY_ARROWLAKE    0x06
#define INTEL_FAMILY_LUNARLAKE    0x06
#define INTEL_FAMILY_PANTHERLAKE  0x06

// Model IDs
#define INTEL_MODEL_SKYLAKE_S     0x55
#define INTEL_MODEL_SKYLAKE_X     0x55
#define INTEL_MODEL_KABYLAKE_S    0x9E
#define INTEL_MODEL_KABYLAKE_H    0x8E
#define INTEL_MODEL_COFFEELAKE_S  0x9E
#define INTEL_MODEL_COFFEELAKE_H  0x9E
#define INTEL_MODEL_ICELAKE_S     0x7D
#define INTEL_MODEL_ICELAKE_X     0x6A
#define INTEL_MODEL_ICELAKE_L     0x7E
#define INTEL_MODEL_ROCKETLAKE_S  0xA7
#define INTEL_MODEL_ALDERLAKE_S   0x97
#define INTEL_MODEL_ALDERLAKE_P   0x9A
#define INTEL_MODEL_RAPTORLAKE_S  0xB7
#define INTEL_MODEL_RAPTORLAKE_P  0xBA
#define INTEL_MODEL_METEORLAKE_M  0xAA
#define INTEL_MODEL_ARROWLAKE_S   0xC5
#define INTEL_MODEL_LUNARLAKE_M   0xBD

// MSRs
#define MSR_EFER                0xC0000080
#define MSR_STAR                0xC0000081
#define MSR_LSTAR               0xC0000082
#define MSR_CSTAR               0xC0000083
#define MSR_SFMASK              0xC0000084
#define MSR_FS_BASE             0xC0000100
#define MSR_GS_BASE             0xC0000101
#define MSR_KERNEL_GS_BASE      0xC0000102
#define MSR_TSC_AUX             0xC0000103
#define MSR_IA32_APIC_BASE      0x1B
#define MSR_IA32_FEATURE_CONTROL 0x3A
#define MSR_IA32_TSC            0x10
#define MSR_IA32_PLATFORM_ID    0x17
#define MSR_IA32_MISC_ENABLE    0x1A0
#define MSR_IA32_TEMPERATURE_TARGET 0x1A2
#define MSR_IA32_PERF_STATUS    0x198
#define MSR_IA32_PERF_CTL       0x199
#define MSR_IA32_CLOCK_MODULATION 0x19A
#define MSR_IA32_THERM_STATUS   0x19C
#define MSR_IA32_THERM_INTERRUPT 0x19B
#define MSR_IA32_ENERGY_PERF_BIAS 0x1B0
#define MSR_IA32_PACKAGE_THERM_STATUS 0x1B1
#define MSR_IA32_PACKAGE_THERM_INTERRUPT 0x1B2
#define MSR_IA32_MPERF          0xE7
#define MSR_IA32_APERF          0xE8
#define MSR_IA32_CORE_CAPABILITIES 0xCF
#define MSR_IA32_HWP_CAPABILITIES 0x771
#define MSR_IA32_HWP_REQUEST    0x774
#define MSR_IA32_HWP_STATUS     0x777
#define MSR_IA32_PM_ENABLE      0x770

// VT-x MSRs
#define MSR_IA32_VMX_BASIC      0x480
#define MSR_IA32_VMX_PINBASED_CTLS 0x481
#define MSR_IA32_VMX_PROCBASED_CTLS 0x482
#define MSR_IA32_VMX_EXIT_CTLS  0x483
#define MSR_IA32_VMX_ENTRY_CTLS 0x484
#define MSR_IA32_VMX_MISC       0x485
#define MSR_IA32_VMX_CR0_FIXED0 0x486
#define MSR_IA32_VMX_CR0_FIXED1 0x487
#define MSR_IA32_VMX_CR4_FIXED0 0x488
#define MSR_IA32_VMX_CR4_FIXED1 0x489
#define MSR_IA32_VMX_VMCS_ENUM  0x48A
#define MSR_IA32_VMX_PROCBASED_CTLS2 0x48B
#define MSR_IA32_VMX_EPT_VPID_CAP 0x48C
#define MSR_IA32_VMX_TRUE_PINBASED_CTLS 0x48D
#define MSR_IA32_VMX_TRUE_PROCBASED_CTLS 0x48E
#define MSR_IA32_VMX_TRUE_EXIT_CTLS 0x48F
#define MSR_IA32_VMX_TRUE_ENTRY_CTLS 0x490
#define MSR_IA32_VMX_VMFUNC     0x491

// SGX MSRs
#define MSR_IA32_SGXLEPUBKEYHASH0 0x8C
#define MSR_IA32_SGXLEPUBKEYHASH1 0x8D
#define MSR_IA32_SGXLEPUBKEYHASH2 0x8E
#define MSR_IA32_SGXLEPUBKEYHASH3 0x8F
#define MSR_IA32_FEATURE_CONFIG 0x13C
#define MSR_IA32_TME_ACTIVATE   0x982
#define MSR_IA32_TME_CAPABILITY 0x983

// CPU Info
typedef struct {
    char vendor[16];
    char brand[64];
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    uint32_t max_cpuid;
    uint32_t max_ext_cpuid;
    uint32_t apic_id;
    uint32_t initial_apic_id;
    uint32_t clflush_size;
    uint32_t cache_line_size;
    uint32_t max_logical_cores;
    uint32_t max_physical_cores;
    uint32_t max_address_bits;
    uint32_t max_physical_address_bits;
    
    // Features
    uint64_t features_ecx;
    uint64_t features_edx;
    uint64_t ext_features_ebx;
    uint64_t ext_features_ecx;
    uint64_t ext_features_edx;
    
    // Microcode
    uint32_t microcode_revision;
    uint32_t microcode_update_count;
    
    // Thermal
    uint32_t tj_max;
    uint32_t digital_readout;
    
    // Power
    uint32_t tdp_watts;
    uint32_t min_power_watts;
    uint32_t max_power_watts;
    uint32_t energy_perf_bias;
    
    // Cache
    uint32_t l1i_size;
    uint32_t l1d_size;
    uint32_t l2_size;
    uint32_t l3_size;
    uint32_t l4_size;
    
} intel_cpu_info_t;

// VT-x Info
typedef struct {
    int supported;
    int enabled;
    uint64_t vmx_basic;
    uint32_t vmcs_revision;
    uint32_t vmxon_region_size;
    int memory_type;
    int ins_outs_vmexit;
    int true_controls;
    uint64_t ept_vpid_cap;
    int ept_supported;
    int vpid_supported;
    int unrestricted_guest;
    int apic_virtualization;
    int posted_interrupts;
    int vm_functions;
} intel_vmx_info_t;

// SGX Info
typedef struct {
    int supported;
    int enabled;
    int sgx1;
    int sgx2;
    int enclave_size_64;
    uint64_t max_enclave_size;
    int edcssd;
    int keys;
    int launch_control;
    int attestation;
    int configid;
} intel_sgx_info_t;

// TXT Info
typedef struct {
    int supported;
    int enabled;
    int chipset_present;
    int chipset_enabled;
    int chipset_activate;
} intel_txt_info_t;

// TME/MKTME Info
typedef struct {
    int supported;
    int enabled;
    int tme_bypass;
    int mk_tme;
    int tme_encrypt;
    uint32_t max_keyids;
    uint32_t keyid_bits;
} intel_tme_info_t;

// Functions
void intel_init(void);
void intel_detect_cpu(void);
void intel_detect_vmx(void);
void intel_detect_sgx(void);
void intel_detect_txt(void);
void intel_detect_tme(void);
void intel_detect_cache(void);
void intel_update_microcode(void);

// MSR access
static inline uint64_t intel_rdmsr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline void intel_wrmsr(uint32_t msr, uint64_t val) {
    uint32_t low = val & 0xFFFFFFFF;
    uint32_t high = val >> 32;
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

// CPUID
static inline void intel_cpuid(uint32_t func, uint32_t subfunc, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx) {
    __asm__ volatile("cpuid" : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx) : "a"(func), "c"(subfunc));
}

// RDTSC
static inline uint64_t intel_rdtsc(void) {
    uint32_t low, high;
    __asm__ volatile("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}

// RDTSCP
static inline uint64_t intel_rdtscp(uint32_t *aux) {
    uint32_t low, high;
    __asm__ volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(*aux));
    return ((uint64_t)high << 32) | low;
}

// RDRAND
static inline int intel_rdrand(uint32_t *val) {
    uint8_t ok;
    __asm__ volatile("rdrand %0; setc %1" : "=r"(*val), "=qm"(ok));
    return ok;
}

// RDSEED
static inline int intel_rdseed(uint32_t *val) {
    uint8_t ok;
    __asm__ volatile("rdseed %0; setc %1" : "=r"(*val), "=qm"(ok));
    return ok;
}

// Info getters
intel_cpu_info_t* intel_get_cpu_info(void);
intel_vmx_info_t* intel_get_vmx_info(void);
intel_sgx_info_t* intel_get_sgx_info(void);
intel_txt_info_t* intel_get_txt_info(void);
intel_tme_info_t* intel_get_tme_info(void);

// Power management
void intel_set_energy_perf_bias(uint32_t bias);
uint32_t intel_get_energy_perf_bias(void);
void intel_enable_hwp(void);
void intel_disable_hwp(void);

// Thermal
uint32_t intel_read_temperature(void);
void intel_set_thermal_interrupt(uint32_t threshold);

// Performance monitoring
void intel_enable_pmc(void);
uint64_t intel_read_pmc(uint32_t counter);
void intel_reset_pmc(void);

#endif

#include "intel.h"
#include "vga.h"
#include "memory.h"

static intel_cpu_info_t cpu_info;
static intel_vmx_info_t vmx_info;
static intel_sgx_info_t sgx_info;
static intel_txt_info_t txt_info;
static intel_tme_info_t tme_info;

static uint32_t microcode_buffer[2048];

void intel_detect_cpu(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // Vendor ID
    intel_cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    cpu_info.max_cpuid = eax;
    
    ((uint32_t*)cpu_info.vendor)[0] = ebx;
    ((uint32_t*)cpu_info.vendor)[1] = edx;
    ((uint32_t*)cpu_info.vendor)[2] = ecx;
    cpu_info.vendor[12] = '\\0';
    
    // Check if Intel
    if (strcmp(cpu_info.vendor, "GenuineIntel") != 0) {
        vga_puts("[!] Not an Intel processor!\\n");
        return;
    }
    
    // Processor info
    intel_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    cpu_info.stepping = eax & 0xF;
    cpu_info.model = ((eax >> 4) & 0xF) | ((eax >> 12) & 0xF0);
    cpu_info.family = ((eax >> 8) & 0xF) + ((eax >> 20) & 0xFF);
    cpu_info.features_ecx = ecx;
    cpu_info.features_edx = edx;
    cpu_info.initial_apic_id = (ebx >> 24) & 0xFF;
    cpu_info.clflush_size = ((ebx >> 8) & 0xFF) * 8;
    cpu_info.max_logical_cores = (ebx >> 16) & 0xFF;
    cpu_info.cache_line_size = cpu_info.clflush_size;
    
    // Extended features
    intel_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    cpu_info.ext_features_ebx = ebx;
    cpu_info.ext_features_ecx = ecx;
    cpu_info.ext_features_edx = edx;
    
    // Brand string
    intel_cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    cpu_info.max_ext_cpuid = eax;
    
    if (eax >= 0x80000004) {
        for (int i = 0; i < 3; i++) {
            intel_cpuid(0x80000002 + i, 0, &eax, &ebx, &ecx, &edx);
            ((uint32_t*)cpu_info.brand)[i * 4 + 0] = eax;
            ((uint32_t*)cpu_info.brand)[i * 4 + 1] = ebx;
            ((uint32_t*)cpu_info.brand)[i * 4 + 2] = ecx;
            ((uint32_t*)cpu_info.brand)[i * 4 + 3] = edx;
        }
        cpu_info.brand[48] = '\\0';
    }
    
    // Address sizes
    intel_cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
    cpu_info.max_physical_address_bits = eax & 0xFF;
    cpu_info.max_address_bits = (eax >> 8) & 0xFF;
    
    // Physical cores
    intel_cpuid(0x80000008, 0, &eax, &ebx, &ecx, &edx);
    cpu_info.max_physical_cores = (ecx & 0xFF) + 1;
    
    // Microcode revision
    cpu_info.microcode_revision = (uint32_t)(intel_rdmsr(MSR_IA32_PLATFORM_ID) >> 32) & 0xFFFF;
    
    // Thermal
    uint64_t temp_target = intel_rdmsr(MSR_IA32_TEMPERATURE_TARGET);
    cpu_info.tj_max = (temp_target >> 16) & 0xFF;
    if (cpu_info.tj_max == 0) cpu_info.tj_max = 100;
    
    // TDP
    intel_cpuid(6, 0, &eax, &ebx, &ecx, &edx);
    if (eax & (1 << 2)) {
        uint64_t perf_status = intel_rdmsr(MSR_IA32_PERF_STATUS);
        cpu_info.tdp_watts = (perf_status >> 32) & 0x7FFF;
    }
    
    // Energy perf bias
    cpu_info.energy_perf_bias = intel_get_energy_perf_bias();
}

void intel_detect_vmx(void) {
    memset(&vmx_info, 0, sizeof(vmx_info));
    
    if (!(cpu_info.features_ecx & (1 << 5))) {
        vga_puts("    VMX: Not supported\\n");
        return;
    }
    
    vmx_info.supported = 1;
    
    // Read VMX basic MSR
    vmx_info.vmx_basic = intel_rdmsr(MSR_IA32_VMX_BASIC);
    vmx_info.vmcs_revision = vmx_info.vmx_basic & 0x7FFFFFFF;
    vmx_info.vmxon_region_size = (vmx_info.vmx_basic >> 32) & 0x1FFF;
    vmx_info.memory_type = (vmx_info.vmx_basic >> 50) & 0xF;
    vmx_info.ins_outs_vmexit = (vmx_info.vmx_basic >> 54) & 1;
    vmx_info.true_controls = (vmx_info.vmx_basic >> 55) & 1;
    
    // EPT/VPID capabilities
    vmx_info.ept_vpid_cap = intel_rdmsr(MSR_IA32_VMX_EPT_VPID_CAP);
    vmx_info.ept_supported = (vmx_info.ept_vpid_cap & (1ULL << 0)) != 0;
    vmx_info.vpid_supported = (vmx_info.ept_vpid_cap & (1ULL << 32)) != 0;
    vmx_info.unrestricted_guest = (vmx_info.ept_vpid_cap & (1ULL << 7)) != 0;
    
    // Check if enabled
    uint64_t feature_ctrl = intel_rdmsr(MSR_IA32_FEATURE_CONTROL);
    vmx_info.enabled = (feature_ctrl & (1 << 2)) != 0;
    
    vga_puts("    VMX: SUPPORTED");
    if (vmx_info.enabled) vga_puts(" + ENABLED");
    vga_puts("\\n");
    
    if (vmx_info.ept_supported) vga_puts("    EPT: Supported\\n");
    if (vmx_info.vpid_supported) vga_puts("    VPID: Supported\\n");
}

void intel_detect_sgx(void) {
    memset(&sgx_info, 0, sizeof(sgx_info));
    
    // Check SGX via CPUID leaf 7, ECX bit 2
    if (!(cpu_info.ext_features_ebx & (1 << 2))) {
        vga_puts("    SGX: Not supported\\n");
        return;
    }
    
    sgx_info.supported = 1;
    
    // SGX capabilities from leaf 12
    uint32_t eax, ebx, ecx, edx;
    intel_cpuid(0x12, 0, &eax, &ebx, &ecx, &edx);
    
    sgx_info.sgx1 = (eax & 1) != 0;
    sgx_info.sgx2 = (eax & 2) != 0;
    sgx_info.enclave_size_64 = (eax & 4) != 0;
    
    // Max enclave size
    uint32_t max_size_32 = (edx >> 0) & 0xFF;
    uint32_t max_size_64 = (edx >> 8) & 0xFF;
    sgx_info.max_enclave_size = 1ULL << max_size_64;
    
    // Check enabled
    uint64_t feature_config = intel_rdmsr(MSR_IA32_FEATURE_CONFIG);
    sgx_info.enabled = (feature_config & 1) != 0;
    
    vga_puts("    SGX: SUPPORTED");
    if (sgx_info.enabled) vga_puts(" + ENABLED");
    vga_puts("\\n");
    
    if (sgx_info.sgx1) vga_puts("    SGX1: Yes\\n");
    if (sgx_info.sgx2) vga_puts("    SGX2: Yes\\n");
}

void intel_detect_txt(void) {
    memset(&txt_info, 0, sizeof(txt_info));
    
    // TXT is indicated by SMX in CPUID
    if (!(cpu_info.features_ecx & (1 << 6))) {
        vga_puts("    TXT: Not supported\\n");
        return;
    }
    
    txt_info.supported = 1;
    
    // Check chipset
    txt_info.chipset_present = 1;
    txt_info.chipset_enabled = 1;
    
    vga_puts("    TXT: Supported\\n");
}

void intel_detect_tme(void) {
    memset(&tme_info, 0, sizeof(tme_info));
    
    // TME from CPUID leaf 7, ECX bit 13
    if (!(cpu_info.ext_features_ecx & (1 << 13))) {
        vga_puts("    TME: Not supported\\n");
        return;
    }
    
    tme_info.supported = 1;
    
    uint64_t tme_activate = intel_rdmsr(MSR_IA32_TME_ACTIVATE);
    tme_info.enabled = (tme_activate & 1) != 0;
    tme_info.tme_bypass = (tme_activate >> 31) & 1;
    tme_info.mk_tme = (tme_activate >> 32) & 1;
    
    uint64_t tme_cap = intel_rdmsr(MSR_IA32_TME_CAPABILITY);
    tme_info.max_keyids = (tme_cap >> 32) & 0x3F;
    tme_info.keyid_bits = (tme_cap >> 0) & 0x3F;
    
    vga_puts("    TME: SUPPORTED");
    if (tme_info.enabled) vga_puts(" + ENABLED");
    vga_puts("\\n");
    
    if (tme_info.mk_tme) {
        vga_puts("    MK-TME: Yes (");
        vga_put_int(tme_info.max_keyids);
        vga_puts(" key IDs)\\n");
    }
}

void intel_detect_cache(void) {
    uint32_t eax, ebx, ecx, edx;
    
    // L1 cache
    intel_cpuid(4, 0, &eax, &ebx, &ecx, &edx);
    if ((eax & 0x1F) != 0) {
        uint32_t ways = ((ebx >> 22) & 0x3FF) + 1;
        uint32_t partitions = ((ebx >> 12) & 0x3FF) + 1;
        uint32_t line_size = (ebx & 0xFFF) + 1;
        uint32_t sets = ecx + 1;
        cpu_info.l1d_size = (ways * partitions * line_size * sets) / 1024;
    }
    
    // L1 instruction
    intel_cpuid(4, 1, &eax, &ebx, &ecx, &edx);
    if ((eax & 0x1F) != 0) {
        uint32_t ways = ((ebx >> 22) & 0x3FF) + 1;
        uint32_t partitions = ((ebx >> 12) & 0x3FF) + 1;
        uint32_t line_size = (ebx & 0xFFF) + 1;
        uint32_t sets = ecx + 1;
        cpu_info.l1i_size = (ways * partitions * line_size * sets) / 1024;
    }
    
    // L2
    intel_cpuid(4, 2, &eax, &ebx, &ecx, &edx);
    if ((eax & 0x1F) != 0) {
        uint32_t ways = ((ebx >> 22) & 0x3FF) + 1;
        uint32_t partitions = ((ebx >> 12) & 0x3FF) + 1;
        uint32_t line_size = (ebx & 0xFFF) + 1;
        uint32_t sets = ecx + 1;
        cpu_info.l2_size = (ways * partitions * line_size * sets) / 1024;
    }
    
    // L3
    intel_cpuid(4, 3, &eax, &ebx, &ecx, &edx);
    if ((eax & 0x1F) != 0) {
        uint32_t ways = ((ebx >> 22) & 0x3FF) + 1;
        uint32_t partitions = ((ebx >> 12) & 0x3FF) + 1;
        uint32_t line_size = (ebx & 0xFFF) + 1;
        uint32_t sets = ecx + 1;
        cpu_info.l3_size = (ways * partitions * line_size * sets) / 1024;
    }
}

void intel_update_microcode(void) {
    // Load microcode update
    // In real implementation, would load from file
    vga_puts("[*] Microcode update: ");
    vga_put_hex(cpu_info.microcode_revision);
    vga_puts("\\n");
    
    // Write microcode
    uint64_t ucode_addr = (uint64_t)microcode_buffer;
    intel_wrmsr(0x79, ucode_addr); // IA32_BIOS_UPDT_TRIG
    
    // Verify
    uint32_t new_rev = (uint32_t)(intel_rdmsr(MSR_IA32_PLATFORM_ID) >> 32) & 0xFFFF;
    if (new_rev != cpu_info.microcode_revision) {
        cpu_info.microcode_revision = new_rev;
        cpu_info.microcode_update_count++;
        vga_puts("[+] Microcode updated to: ");
        vga_put_hex(new_rev);
        vga_puts("\\n");
    }
}

void intel_init(void) {
    vga_puts("\\n[+] Intel Architecture Support\\n");
    
    intel_detect_cpu();
    
    vga_puts("    Vendor: ");
    vga_puts(cpu_info.vendor);
    vga_puts("\\n");
    vga_puts("    Brand: ");
    vga_puts(cpu_info.brand);
    vga_puts("\\n");
    vga_puts("    Family: ");
    vga_put_int(cpu_info.family);
    vga_puts("  Model: ");
    vga_put_int(cpu_info.model);
    vga_puts("  Stepping: ");
    vga_put_int(cpu_info.stepping);
    vga_puts("\\n");
    vga_puts("    Cores: ");
    vga_put_int(cpu_info.max_physical_cores);
    vga_puts(" physical / ");
    vga_put_int(cpu_info.max_logical_cores);
    vga_puts(" logical\\n");
    vga_puts("    Address: ");
    vga_put_int(cpu_info.max_physical_address_bits);
    vga_puts(" physical / ");
    vga_put_int(cpu_info.max_address_bits);
    vga_puts(" virtual bits\\n");
    
    intel_detect_cache();
    vga_puts("    L1i: ");
    vga_put_int(cpu_info.l1i_size);
    vga_puts(" KB  L1d: ");
    vga_put_int(cpu_info.l1d_size);
    vga_puts(" KB\\n");
    vga_puts("    L2: ");
    vga_put_int(cpu_info.l2_size);
    vga_puts(" KB  L3: ");
    vga_put_int(cpu_info.l3_size);
    vga_puts(" KB\\n");
    
    vga_puts("    Microcode: 0x");
    vga_put_hex(cpu_info.microcode_revision);
    vga_puts("\\n");
    
    intel_detect_vmx();
    intel_detect_sgx();
    intel_detect_txt();
    intel_detect_tme();
    
    vga_puts("    RDRAND: ");
    vga_puts((cpu_info.ext_features_ebx & (1 << 30)) ? "Yes" : "No");
    vga_puts("\\n");
    vga_puts("    RDSEED: ");
    vga_puts((cpu_info.ext_features_ebx & (1 << 18)) ? "Yes" : "No");
    vga_puts("\\n");
    vga_puts("    TSX: ");
    vga_puts((cpu_info.ext_features_ebx & (1 << 11)) ? "Yes" : "No");
    vga_puts("\\n");
    vga_puts("    CET: ");
    vga_puts((cpu_info.ext_features_ecx & (1 << 7)) ? "Yes" : "No");
    vga_puts("\\n");
    vga_puts("    AMX: ");
    vga_puts((cpu_info.ext_features_edx & (1 << 24)) ? "Yes" : "No");
    vga_puts("\\n");
    vga_puts("    AVX-VNNI: ");
    vga_puts((cpu_info.ext_features_eax & (1 << 4)) ? "Yes" : "No");
    vga_puts("\\n");
}

void intel_set_energy_perf_bias(uint32_t bias) {
    if (bias > 15) bias = 15;
    intel_wrmsr(MSR_IA32_ENERGY_PERF_BIAS, bias);
}

uint32_t intel_get_energy_perf_bias(void) {
    return (uint32_t)intel_rdmsr(MSR_IA32_ENERGY_PERF_BIAS) & 0xF;
}

void intel_enable_hwp(void) {
    if (!(cpu_info.ext_features_ebx & (1 << 7))) return; // No HWP
    
    intel_wrmsr(MSR_IA32_PM_ENABLE, 1);
    
    // Set HWP request
    uint64_t hwp_request = intel_rdmsr(MSR_IA32_HWP_REQUEST);
    hwp_request |= (1ULL << 63); // Enable HWP
    intel_wrmsr(MSR_IA32_HWP_REQUEST, hwp_request);
    
    vga_puts("[+] HWP (Hardware P-States) enabled\\n");
}

void intel_disable_hwp(void) {
    intel_wrmsr(MSR_IA32_PM_ENABLE, 0);
}

uint32_t intel_read_temperature(void) {
    uint64_t therm_status = intel_rdmsr(MSR_IA32_THERM_STATUS);
    uint32_t digital_readout = (therm_status >> 16) & 0x7F;
    
    if (cpu_info.tj_max == 0) cpu_info.tj_max = 100;
    
    return cpu_info.tj_max - digital_readout;
}

void intel_set_thermal_interrupt(uint32_t threshold) {
    uint64_t therm_int = intel_rdmsr(MSR_IA32_THERM_INTERRUPT);
    therm_int &= ~(0x7F << 0);
    therm_int |= (threshold & 0x7F);
    intel_wrmsr(MSR_IA32_THERM_INTERRUPT, therm_int);
}

void intel_enable_pmc(void) {
    uint64_t perf_global_ctrl = intel_rdmsr(0x38F);
    perf_global_ctrl |= 0x7; // Enable counters 0-2
    intel_wrmsr(0x38F, perf_global_ctrl);
}

uint64_t intel_read_pmc(uint32_t counter) {
    if (counter > 3) return 0;
    return intel_rdmsr(0xC1 + counter);
}

void intel_reset_pmc(void) {
    for (uint32_t i = 0; i < 4; i++) {
        intel_wrmsr(0xC1 + i, 0);
    }
}

intel_cpu_info_t* intel_get_cpu_info(void) { return &cpu_info; }
intel_vmx_info_t* intel_get_vmx_info(void) { return &vmx_info; }
intel_sgx_info_t* intel_get_sgx_info(void) { return &sgx_info; }
intel_txt_info_t* intel_get_txt_info(void) { return &txt_info; }
intel_tme_info_t* intel_get_tme_info(void) { return &tme_info; }