#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "types.h"

#define CALC_MAX_EXPRESSION 1024
#define CALC_MAX_HISTORY 100
#define CALC_PRECISION 15

// Calculator modes
typedef enum {
    CALC_MODE_STANDARD,
    CALC_MODE_SCIENTIFIC,
    CALC_MODE_PROGRAMMER,
    CALC_MODE_GRAPHING,
    CALC_MODE_DATE,
    CALC_MODE_CONVERTER,
} calc_mode_t;

// Number bases
typedef enum {
    BASE_DECIMAL,
    BASE_BINARY,
    BASE_OCTAL,
    BASE_HEXADECIMAL,
} calc_base_t;

// Angle modes
typedef enum {
    ANGLE_DEGREE,
    ANGLE_RADIAN,
    ANGLE_GRADIAN,
} calc_angle_t;

// Token types for expression parser
typedef enum {
    CALC_TOK_NUMBER,
    CALC_TOK_OPERATOR,
    CALC_TOK_FUNCTION,
    CALC_TOK_LPAREN,
    CALC_TOK_RPAREN,
    CALC_TOK_VARIABLE,
    CALC_TOK_COMMA,
    CALC_TOK_EOF,
} calc_token_type_t;

typedef struct {
    calc_token_type_t type;
    double value;
    char str[32];
} calc_token_t;

// History entry
typedef struct {
    char expression[CALC_MAX_EXPRESSION];
    double result;
    calc_mode_t mode;
    uint64_t timestamp;
} calc_history_entry_t;

// Graph data
typedef struct {
    char expression[CALC_MAX_EXPRESSION];
    double *x_values;
    double *y_values;
    uint32_t num_points;
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    uint32_t color;
} calc_graph_t;

// Unit conversion
typedef enum {
    UNIT_LENGTH,
    UNIT_WEIGHT,
    UNIT_TEMPERATURE,
    UNIT_VOLUME,
    UNIT_AREA,
    UNIT_SPEED,
    UNIT_TIME,
    UNIT_DATA,
    UNIT_PRESSURE,
    UNIT_ENERGY,
    UNIT_POWER,
} calc_unit_category_t;

typedef struct {
    char name[32];
    char symbol[8];
    double to_base;
} calc_unit_t;

// Calculator state
typedef struct {
    calc_mode_t mode;
    calc_base_t base;
    calc_angle_t angle;
    
    char display[CALC_MAX_EXPRESSION];
    char expression[CALC_MAX_EXPRESSION];
    double memory;
    double last_result;
    
    // History
    calc_history_entry_t history[CALC_MAX_HISTORY];
    uint32_t history_count;
    
    // Graphing
    calc_graph_t graphs[8];
    uint32_t num_graphs;
    
    // Programmer
    uint64_t bit_display;
    int word_size; // 8, 16, 32, 64
    
    // Settings
    int digit_grouping;
    int fixed_decimal;
    int decimal_places;
    
} calculator_t;

// Functions
void calculator_init(void);
calculator_t* calculator_get_state(void);

// Input
void calculator_input_digit(calculator_t *calc, int digit);
void calculator_input_operator(calculator_t *calc, char op);
void calculator_input_function(calculator_t *calc, const char *func);
void calculator_input_decimal(calculator_t *calc);
void calculator_input_backspace(calculator_t *calc);
void calculator_input_clear(calculator_t *calc);
void calculator_input_clear_entry(calculator_t *calc);
void calculator_input_equals(calculator_t *calc);
void calculator_input_parenthesis(calculator_t *calc, char paren);
void calculator_input_percent(calculator_t *calc);
void calculator_input_sign(calculator_t *calc);

// Memory
void calculator_memory_store(calculator_t *calc);
void calculator_memory_recall(calculator_t *calc);
void calculator_memory_add(calculator_t *calc);
void calculator_memory_subtract(calculator_t *calc);
void calculator_memory_clear(calculator_t *calc);

// Mode switching
void calculator_set_mode(calculator_t *calc, calc_mode_t mode);
void calculator_set_base(calculator_t *calc, calc_base_t base);
void calculator_set_angle(calculator_t *calc, calc_angle_t angle);

// Scientific functions
double calculator_sin(double x, calc_angle_t angle);
double calculator_cos(double x, calc_angle_t angle);
double calculator_tan(double x, calc_angle_t angle);
double calculator_asin(double x, calc_angle_t angle);
double calculator_acos(double x, calc_angle_t angle);
double calculator_atan(double x, calc_angle_t angle);
double calculator_log(double x);
double calculator_log10(double x);
double calculator_ln(double x);
double calculator_exp(double x);
double calculator_sqrt(double x);
double calculator_cbrt(double x);
double calculator_pow(double base, double exp);
double calculator_factorial(double n);
double calculator_mod(double a, double b);
double calculator_abs(double x);
double calculator_floor(double x);
double calculator_ceil(double x);
double calculator_round(double x);
double calculator_random(void);

// Programmer functions
char* calculator_to_binary(uint64_t n, int bits);
char* calculator_to_octal(uint64_t n);
char* calculator_to_hex(uint64_t n);
uint64_t calculator_from_binary(const char *s);
uint64_t calculator_from_octal(const char *s);
uint64_t calculator_from_hex(const char *s);
uint64_t calculator_bitwise_and(uint64_t a, uint64_t b);
uint64_t calculator_bitwise_or(uint64_t a, uint64_t b);
uint64_t calculator_bitwise_xor(uint64_t a, uint64_t b);
uint64_t calculator_bitwise_not(uint64_t n, int bits);
uint64_t calculator_shift_left(uint64_t n, int bits);
uint64_t calculator_shift_right(uint64_t n, int bits);
uint64_t calculator_rotate_left(uint64_t n, int bits, int width);
uint64_t calculator_rotate_right(uint64_t n, int bits, int width);

// Expression evaluation
double calculator_evaluate(const char *expr);
double calculator_evaluate_rpn(calc_token_t *tokens, uint32_t count);
int calculator_infix_to_rpn(const char *expr, calc_token_t *output, uint32_t *count);
int calculator_tokenize(const char *expr, calc_token_t *tokens, uint32_t *count);
int calculator_get_precedence(const char *op);

// Graphing
void calculator_graph_add(calculator_t *calc, const char *expr, double x_min, double x_max, uint32_t points);
void calculator_graph_evaluate(calc_graph_t *graph);
void calculator_graph_render(calc_graph_t *graph, uint32_t *buffer, int w, int h);
void calculator_graph_clear(calculator_t *calc);

// Unit conversion
double calculator_convert(double value, calc_unit_category_t category, int from_unit, int to_unit);
const calc_unit_t* calculator_get_units(calc_unit_category_t category, uint32_t *count);

// History
void calculator_add_history(calculator_t *calc, const char *expr, double result);
calc_history_entry_t* calculator_get_history(calculator_t *calc, uint32_t *count);
void calculator_clear_history(calculator_t *calc);

// GUI rendering
void calculator_render_standard(calculator_t *calc, uint32_t *buffer, int w, int h);
void calculator_render_scientific(calculator_t *calc, uint32_t *buffer, int w, int h);
void calculator_render_programmer(calculator_t *calc, uint32_t *buffer, int w, int h);
void calculator_render_graphing(calculator_t *calc, uint32_t *buffer, int w, int h);
void calculator_render_button(int x, int y, int w, int h, const char *label, uint32_t bg, uint32_t fg, uint32_t *buffer, int bw, int bh);
void calculator_render_display(const char *text, int x, int y, int w, int h, uint32_t *buffer, int bw, int bh);

#endif

#include "calculator.h"
#include "vga.h"
#include "memory.h"
#include "simd.h"
#include <math.h>

static calculator_t calc_state;

// Unit definitions
static calc_unit_t length_units[] = {
    {"Nanometer", "nm", 1e-9},
    {"Micrometer", "um", 1e-6},
    {"Millimeter", "mm", 1e-3},
    {"Centimeter", "cm", 1e-2},
    {"Meter", "m", 1.0},
    {"Kilometer", "km", 1e3},
    {"Inch", "in", 0.0254},
    {"Foot", "ft", 0.3048},
    {"Yard", "yd", 0.9144},
    {"Mile", "mi", 1609.344},
    {"Nautical Mile", "nmi", 1852.0},
};

static calc_unit_t weight_units[] = {
    {"Milligram", "mg", 1e-6},
    {"Gram", "g", 1e-3},
    {"Kilogram", "kg", 1.0},
    {"Metric Ton", "t", 1e3},
    {"Ounce", "oz", 0.0283495},
    {"Pound", "lb", 0.453592},
    {"Stone", "st", 6.35029},
    {"US Ton", "ton", 907.185},
};

static calc_unit_t temperature_units[] = {
    {"Celsius", "C", 0},
    {"Fahrenheit", "F", 0},
    {"Kelvin", "K", 0},
};

static calc_unit_t data_units[] = {
    {"Bit", "b", 0.125},
    {"Byte", "B", 1.0},
    {"Kilobyte", "KB", 1024.0},
    {"Megabyte", "MB", 1048576.0},
    {"Gigabyte", "GB", 1073741824.0},
    {"Terabyte", "TB", 1099511627776.0},
    {"Petabyte", "PB", 1125899906842624.0},
};

void calculator_init(void) {
    memset(&calc_state, 0, sizeof(calculator_t));
    calc_state.mode = CALC_MODE_STANDARD;
    calc_state.base = BASE_DECIMAL;
    calc_state.angle = ANGLE_DEGREE;
    calc_state.word_size = 64;
    calc_state.decimal_places = 10;
    calc_state.digit_grouping = 1;
    
    strcpy(calc_state.display, "0");
    
    vga_puts("[+] Calculator App initialized\\n");
    vga_puts("    Modes: Standard, Scientific, Programmer, Graphing\\n");
    vga_puts("    Bases: DEC, BIN, OCT, HEX\\n");
}

calculator_t* calculator_get_state(void) {
    return &calc_state;
}

void calculator_input_digit(calculator_t *calc, int digit) {
    if (!strcmp(calc->display, "0")) {
        calc->display[0] = '0' + digit;
        calc->display[1] = '\\0';
    } else {
        int len = strlen(calc->display);
        if (len < CALC_MAX_EXPRESSION - 1) {
            calc->display[len] = '0' + digit;
            calc->display[len + 1] = '\\0';
        }
    }
}

void calculator_input_operator(calculator_t *calc, char op) {
    int len = strlen(calc->display);
    if (len < CALC_MAX_EXPRESSION - 2) {
        calc->display[len] = ' ';
        calc->display[len + 1] = op;
        calc->display[len + 2] = ' ';
        calc->display[len + 3] = '\\0';
    }
}

void calculator_input_function(calculator_t *calc, const char *func) {
    int len = strlen(calc->display);
    int flen = strlen(func);
    if (len + flen + 1 < CALC_MAX_EXPRESSION) {
        strcpy(calc->display + len, func);
        calc->display[len + flen] = '(';
        calc->display[len + flen + 1] = '\\0';
    }
}

void calculator_input_decimal(calculator_t *calc) {
    int len = strlen(calc->display);
    // Check if last number already has decimal
    int has_decimal = 0;
    for (int i = len - 1; i >= 0; i--) {
        if (calc->display[i] == '.') {
            has_decimal = 1;
            break;
        }
        if (calc->display[i] == ' ' || calc->display[i] == '+' ||
            calc->display[i] == '-' || calc->display[i] == '*' ||
            calc->display[i] == '/') {
            break;
        }
    }
    
    if (!has_decimal && len < CALC_MAX_EXPRESSION - 1) {
        calc->display[len] = '.';
        calc->display[len + 1] = '\\0';
    }
}

void calculator_input_backspace(calculator_t *calc) {
    int len = strlen(calc->display);
    if (len > 0) {
        calc->display[len - 1] = '\\0';
    }
    if (len == 1) {
        strcpy(calc->display, "0");
    }
}

void calculator_input_clear(calculator_t *calc) {
    strcpy(calc->display, "0");
    calc->expression[0] = '\\0';
    calc->last_result = 0;
}

void calculator_input_clear_entry(calculator_t *calc) {
    strcpy(calc->display, "0");
}

void calculator_input_equals(calculator_t *calc) {
    strcpy(calc->expression, calc->display);
    double result = calculator_evaluate(calc->expression);
    calc->last_result = result;
    
    // Format result
    if (result == (int64_t)result) {
        // Integer
        if (calc->base == BASE_HEXADECIMAL) {
            char hex[32];
            calculator_to_hex((uint64_t)result, hex);
            strcpy(calc->display, hex);
        } else if (calc->base == BASE_BINARY) {
            char bin[128];
            calculator_to_binary((uint64_t)result, calc->word_size, bin);
            strcpy(calc->display, bin);
        } else if (calc->base == BASE_OCTAL) {
            char oct[32];
            calculator_to_octal((uint64_t)result, oct);
            strcpy(calc->display, oct);
        } else {
            // Decimal integer
            int64_t val = (int64_t)result;
            if (val < 0) {
                calc->display[0] = '-';
                val = -val;
                // Convert to string
                char buf[32];
                int i = 0;
                do {
                    buf[i++] = '0' + (val % 10);
                    val /= 10;
                } while (val > 0);
                buf[i] = '\\0';
                // Reverse
                for (int j = 0; j < i; j++) {
                    calc->display[1 + j] = buf[i - 1 - j];
                }
                calc->display[1 + i] = '\\0';
            } else {
                char buf[32];
                int i = 0;
                do {
                    buf[i++] = '0' + ((uint64_t)val % 10);
                    val /= 10;
                } while (val > 0);
                buf[i] = '\\0';
                for (int j = 0; j < i; j++) {
                    calc->display[j] = buf[i - 1 - j];
                }
                calc->display[i] = '\\0';
            }
        }
    } else {
        // Floating point
        // Simple formatting
        int64_t whole = (int64_t)result;
        double frac = result - whole;
        if (frac < 0) frac = -frac;
        
        char buf[64];
        int pos = 0;
        
        if (result < 0) {
            buf[pos++] = '-';
            whole = -whole;
        }
        
        // Whole part
        char whole_buf[32];
        int wi = 0;
        do {
            whole_buf[wi++] = '0' + (whole % 10);
            whole /= 10;
        } while (whole > 0);
        
        for (int j = wi - 1; j >= 0; j--) {
            buf[pos++] = whole_buf[j];
        }
        
        // Fractional part
        buf[pos++] = '.';
        for (int i = 0; i < calc->decimal_places && frac > 0; i++) {
            frac *= 10;
            int digit = (int)frac;
            buf[pos++] = '0' + digit;
            frac -= digit;
        }
        buf[pos] = '\\0';
        
        strcpy(calc->display, buf);
    }
    
    calculator_add_history(calc, calc->expression, calc->last_result);
}

void calculator_input_parenthesis(calculator_t *calc, char paren) {
    int len = strlen(calc->display);
    if (len < CALC_MAX_EXPRESSION - 1) {
        calc->display[len] = paren;
        calc->display[len + 1] = '\\0';
    }
}

void calculator_input_percent(calculator_t *calc) {
    double val = calculator_evaluate(calc->display);
    val = val / 100.0;
    calc->last_result = val;
    
    // Format
    char buf[64];
    int pos = 0;
    int64_t whole = (int64_t)val;
    double frac = val - whole;
    if (frac < 0) frac = -frac;
    
    if (val < 0) {
        buf[pos++] = '-';
        whole = -whole;
    }
    
    char whole_buf[32];
    int wi = 0;
    do {
        whole_buf[wi++] = '0' + (whole % 10);
        whole /= 10;
    } while (whole > 0);
    
    for (int j = wi - 1; j >= 0; j--) {
        buf[pos++] = whole_buf[j];
    }
    
    buf[pos++] = '.';
    for (int i = 0; i < 4 && frac > 0; i++) {
        frac *= 10;
        int digit = (int)frac;
        buf[pos++] = '0' + digit;
        frac -= digit;
    }
    buf[pos] = '\\0';
    
    strcpy(calc->display, buf);
}

void calculator_input_sign(calculator_t *calc) {
    if (calc->display[0] == '-') {
        memmove(calc->display, calc->display + 1, strlen(calc->display));
    } else if (strcmp(calc->display, "0") != 0) {
        memmove(calc->display + 1, calc->display, strlen(calc->display) + 1);
        calc->display[0] = '-';
    }
}

// Memory functions
void calculator_memory_store(calculator_t *calc) {
    calc->memory = calculator_evaluate(calc->display);
}

void calculator_memory_recall(calculator_t *calc) {
    // Format memory value to display
    int64_t whole = (int64_t)calc->memory;
    if (calc->memory == whole) {
        char buf[32];
        int i = 0;
        int64_t val = whole;
        if (val < 0) {
            buf[i++] = '-';
            val = -val;
        }
        int start = i;
        do {
            buf[i++] = '0' + (val % 10);
            val /= 10;
        } while (val > 0);
        // Reverse
        for (int j = 0; j < (i - start) / 2; j++) {
            char tmp = buf[start + j];
            buf[start + j] = buf[i - 1 - j];
            buf[i - 1 - j] = tmp;
        }
        buf[i] = '\\0';
        strcpy(calc->display, buf);
    }
}

void calculator_memory_add(calculator_t *calc) {
    calc->memory += calculator_evaluate(calc->display);
}

void calculator_memory_subtract(calculator_t *calc) {
    calc->memory -= calculator_evaluate(calc->display);
}

void calculator_memory_clear(calculator_t *calc) {
    calc->memory = 0;
}

// Mode switching
void calculator_set_mode(calculator_t *calc, calc_mode_t mode) {
    calc->mode = mode;
    calculator_input_clear(calc);
}

void calculator_set_base(calculator_t *calc, calc_base_t base) {
    calc->base = base;
    calculator_input_clear(calc);
}

void calculator_set_angle(calculator_t *calc, calc_angle_t angle) {
    calc->angle = angle;
}

// Scientific functions
double calculator_sin(double x, calc_angle_t angle) {
    if (angle == ANGLE_DEGREE) x = x * 3.14159265358979323846 / 180.0;
    else if (angle == ANGLE_GRADIAN) x = x * 3.14159265358979323846 / 200.0;
    
    // Taylor series approximation
    double result = x;
    double term = x;
    for (int n = 1; n < 20; n++) {
        term *= -x * x / ((2 * n) * (2 * n + 1));
        result += term;
    }
    return result;
}

double calculator_cos(double x, calc_angle_t angle) {
    if (angle == ANGLE_DEGREE) x = x * 3.14159265358979323846 / 180.0;
    else if (angle == ANGLE_GRADIAN) x = x * 3.14159265358979323846 / 200.0;
    
    double result = 1.0;
    double term = 1.0;
    for (int n = 1; n < 20; n++) {
        term *= -x * x / ((2 * n - 1) * (2 * n));
        result += term;
    }
    return result;
}

double calculator_tan(double x, calc_angle_t angle) {
    double s = calculator_sin(x, angle);
    double c = calculator_cos(x, angle);
    if (c == 0) return 0;
    return s / c;
}

double calculator_sqrt(double x) {
    if (x < 0) return 0;
    if (x == 0) return 0;
    
    double guess = x;
    for (int i = 0; i < 50; i++) {
        guess = (guess + x / guess) / 2.0;
    }
    return guess;
}

double calculator_pow(double base, double exp) {
    if (exp == 0) return 1;
    if (exp == 1) return base;
    if (base == 0) return 0;
    
    // Integer exponent
    if (exp == (int64_t)exp && exp > 0) {
        double result = 1;
        int64_t e = (int64_t)exp;
        while (e > 0) {
            if (e & 1) result *= base;
            base *= base;
            e >>= 1;
        }
        return result;
    }
    
    // Use exp(log(x)*y)
    return calculator_exp(exp * calculator_ln(base));
}

double calculator_ln(double x) {
    if (x <= 0) return 0;
    
    // Use log2 approximation then convert
    double log2_val = 0;
    while (x >= 2.0) { x /= 2.0; log2_val += 1.0; }
    while (x < 1.0) { x *= 2.0; log2_val -= 1.0; }
    
    // Taylor series for ln(1 + y) where y = x - 1
    double y = x - 1.0;
    double result = y;
    double term = y;
    for (int n = 2; n < 50; n++) {
        term *= -y;
        result += term / n;
    }
    
    return (log2_val + result) * 0.6931471805599453;
}

double calculator_log(double x) {
    return calculator_ln(x);
}

double calculator_log10(double x) {
    return calculator_ln(x) / 2.302585092994046;
}

double calculator_exp(double x) {
    double result = 1.0;
    double term = 1.0;
    for (int n = 1; n < 50; n++) {
        term *= x / n;
        result += term;
    }
    return result;
}

double calculator_factorial(double n) {
    if (n < 0) return 0;
    if (n == 0 || n == 1) return 1;
    
    double result = 1;
    for (double i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

double calculator_abs(double x) {
    return x < 0 ? -x : x;
}

double calculator_floor(double x) {
    int64_t i = (int64_t)x;
    if (x < 0 && x != i) i--;
    return (double)i;
}

double calculator_ceil(double x) {
    int64_t i = (int64_t)x;
    if (x > 0 && x != i) i++;
    return (double)i;
}

double calculator_round(double x) {
    if (x >= 0) return calculator_floor(x + 0.5);
    return calculator_ceil(x - 0.5);
}

double calculator_random(void) {
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345;
    return (double)(seed & 0x7FFF) / 32768.0;
}

// Programmer functions
char* calculator_to_binary(uint64_t n, int bits, char *out) {
    int pos = 0;
    for (int i = bits - 1; i >= 0; i--) {
        out[pos++] = (n & (1ULL << i)) ? '1' : '0';
        if (i % 4 == 0 && i > 0) out[pos++] = ' ';
    }
    out[pos] = '\\0';
    return out;
}

char* calculator_to_octal(uint64_t n, char *out) {
    if (n == 0) {
        strcpy(out, "0");
        return out;
    }
    
    char buf[32];
    int i = 0;
    while (n > 0) {
        buf[i++] = '0' + (n & 7);
        n >>= 3;
    }
    
    int pos = 0;
    out[pos++] = '0';
    for (int j = i - 1; j >= 0; j--) {
        out[pos++] = buf[j];
    }
    out[pos] = '\\0';
    return out;
}

char* calculator_to_hex(uint64_t n, char *out) {
    if (n == 0) {
        strcpy(out, "0x0");
        return out;
    }
    
    const char *hex = "0123456789ABCDEF";
    char buf[32];
    int i = 0;
    while (n > 0) {
        buf[i++] = hex[n & 0xF];
        n >>= 4;
    }
    
    int pos = 0;
    out[pos++] = '0';
    out[pos++] = 'x';
    for (int j = i - 1; j >= 0; j--) {
        out[pos++] = buf[j];
    }
    out[pos] = '\\0';
    return out;
}

uint64_t calculator_from_binary(const char *s) {
    uint64_t result = 0;
    while (*s) {
        if (*s == '0' || *s == '1') {
            result = (result << 1) | (*s - '0');
        }
        s++;
    }
    return result;
}

uint64_t calculator_from_octal(const char *s) {
    uint64_t result = 0;
    while (*s) {
        if (*s >= '0' && *s <= '7') {
            result = (result << 3) | (*s - '0');
        }
        s++;
    }
    return result;
}

uint64_t calculator_from_hex(const char *s) {
    uint64_t result = 0;
    while (*s) {
        char c = *s;
        if (c >= '0' && c <= '9') result = (result << 4) | (c - '0');
        else if (c >= 'A' && c <= 'F') result = (result << 4) | (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') result = (result << 4) | (c - 'a' + 10);
        s++;
    }
    return result;
}

uint64_t calculator_bitwise_and(uint64_t a, uint64_t b) { return a & b; }
uint64_t calculator_bitwise_or(uint64_t a, uint64_t b) { return a | b; }
uint64_t calculator_bitwise_xor(uint64_t a, uint64_t b) { return a ^ b; }
uint64_t calculator_bitwise_not(uint64_t n, int bits) {
    uint64_t mask = (bits == 64) ? 0xFFFFFFFFFFFFFFFFULL : ((1ULL << bits) - 1);
    return (~n) & mask;
}
uint64_t calculator_shift_left(uint64_t n, int bits) { return n << bits; }
uint64_t calculator_shift_right(uint64_t n, int bits) { return n >> bits; }

// Expression evaluation
double calculator_evaluate(const char *expr) {
    calc_token_t tokens[256];
    uint32_t count = 0;
    
    if (calculator_tokenize(expr, tokens, &count) != 0) {
        return 0;
    }
    
    calc_token_t rpn[256];
    uint32_t rpn_count = 0;
    
    if (calculator_infix_to_rpn(tokens, count, rpn, &rpn_count) != 0) {
        return 0;
    }
    
    return calculator_evaluate_rpn(rpn, rpn_count);
}

int calculator_tokenize(const char *expr, calc_token_t *tokens, uint32_t *count) {
    const char *p = expr;
    uint32_t i = 0;
    
    while (*p && i < 255) {
        // Skip whitespace
        while (*p == ' ' || *p == '\\t') p++;
        if (!*p) break;
        
        // Number
        if ((*p >= '0' && *p <= '9') || (*p == '.' && *(p+1) >= '0' && *(p+1) <= '9')) {
            tokens[i].type = CALC_TOK_NUMBER;
            tokens[i].value = 0;
            int pos = 0;
            int has_dot = 0;
            double frac_div = 10.0;
            
            while ((*p >= '0' && *p <= '9') || (*p == '.' && !has_dot)) {
                if (*p == '.') {
                    has_dot = 1;
                } else if (!has_dot) {
                    tokens[i].value = tokens[i].value * 10 + (*p - '0');
                } else {
                    tokens[i].value += (*p - '0') / frac_div;
                    frac_div *= 10.0;
                }
                tokens[i].str[pos++] = *p++;
            }
            tokens[i].str[pos] = '\\0';
            i++;
            continue;
        }
        
        // Hex number
        if (*p == '0' && (*(p+1) == 'x' || *(p+1) == 'X')) {
            p += 2;
            tokens[i].type = CALC_TOK_NUMBER;
            tokens[i].value = 0;
            int pos = 0;
            tokens[i].str[pos++] = '0';
            tokens[i].str[pos++] = 'x';
            
            while ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')) {
                char c = *p;
                int val;
                if (c >= '0' && c <= '9') val = c - '0';
                else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
                else val = c - 'a' + 10;
                
                tokens[i].value = tokens[i].value * 16 + val;
                tokens[i].str[pos++] = *p++;
            }
            tokens[i].str[pos] = '\\0';
            i++;
            continue;
        }
        
        // Binary number
        if (*p == '0' && (*(p+1) == 'b' || *(p+1) == 'B')) {
            p += 2;
            tokens[i].type = CALC_TOK_NUMBER;
            tokens[i].value = 0;
            int pos = 0;
            tokens[i].str[pos++] = '0';
            tokens[i].str[pos++] = 'b';
            
            while (*p == '0' || *p == '1') {
                tokens[i].value = tokens[i].value * 2 + (*p - '0');
                tokens[i].str[pos++] = *p++;
            }
            tokens[i].str[pos] = '\\0';
            i++;
            continue;
        }
        
        // Operator
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == '%' ||
            *p == '^' || *p == '&' || *p == '|' || *p == '~' || *p == '!' ||
            *p == '<' || *p == '>') {
            tokens[i].type = CALC_TOK_OPERATOR;
            tokens[i].value = 0;
            tokens[i].str[0] = *p;
            tokens[i].str[1] = '\\0';
            
            // Check for two-char operators
            if ((*p == '<' && *(p+1) == '<') || (*p == '>' && *(p+1) == '>') ||
                (*p == '=' && *(p+1) == '=') || (*p == '!' && *(p+1) == '=')) {
                tokens[i].str[1] = *(p+1);
                tokens[i].str[2] = '\\0';
                p++;
            }
            
            i++;
            p++;
            continue;
        }
        
        // Parentheses
        if (*p == '(') {
            tokens[i].type = CALC_TOK_LPAREN;
            tokens[i].str[0] = '(';
            tokens[i].str[1] = '\\0';
            i++; p++;
            continue;
        }
        if (*p == ')') {
            tokens[i].type = CALC_TOK_RPAREN;
            tokens[i].str[0] = ')';
            tokens[i].str[1] = '\\0';
            i++; p++;
            continue;
        }
        
        // Function or variable
        if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
            int pos = 0;
            while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
                tokens[i].str[pos++] = *p++;
            }
            tokens[i].str[pos] = '\\0';
            
            // Check if function
            if (!strcmp(tokens[i].str, "sin") || !strcmp(tokens[i].str, "cos") ||
                !strcmp(tokens[i].str, "tan") || !strcmp(tokens[i].str, "asin") ||
                !strcmp(tokens[i].str, "acos") || !strcmp(tokens[i].str, "atan") ||
                !strcmp(tokens[i].str, "sqrt") || !strcmp(tokens[i].str, "cbrt") ||
                !strcmp(tokens[i].str, "log") || !strcmp(tokens[i].str, "ln") ||
                !strcmp(tokens[i].str, "exp") || !strcmp(tokens[i].str, "abs") ||
                !strcmp(tokens[i].str, "floor") || !strcmp(tokens[i].str, "ceil") ||
                !strcmp(tokens[i].str, "round") || !strcmp(tokens[i].str, "fact") ||
                !strcmp(tokens[i].str, "rand")) {
                tokens[i].type = CALC_TOK_FUNCTION;
            } else {
                tokens[i].type = CALC_TOK_VARIABLE;
            }
            
            i++;
            continue;
        }
        
        // Comma
        if (*p == ',') {
            tokens[i].type = CALC_TOK_COMMA;
            tokens[i].str[0] = ',';
            tokens[i].str[1] = '\\0';
            i++; p++;
            continue;
        }
        
        p++; // Skip unknown
    }
    
    tokens[i].type = CALC_TOK_EOF;
    *count = i;
    return 0;
}

int calculator_get_precedence(const char *op) {
    if (!strcmp(op, "~") || !strcmp(op, "!")) return 7;
    if (!strcmp(op, "^")) return 6;
    if (!strcmp(op, "*") || !strcmp(op, "/") || !strcmp(op, "%")) return 5;
    if (!strcmp(op, "+") || !strcmp(op, "-")) return 4;
    if (!strcmp(op, "<<") || !strcmp(op, ">>")) return 3;
    if (!strcmp(op, "&")) return 2;
    if (!strcmp(op, "^")) return 2;
    if (!strcmp(op, "|")) return 1;
    return 0;
}

int calculator_infix_to_rpn(calc_token_t *tokens, uint32_t count, calc_token_t *output, uint32_t *out_count) {
    calc_token_t stack[256];
    uint32_t stack_top = 0;
    uint32_t out_i = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        switch (tokens[i].type) {
            case CALC_TOK_NUMBER:
            case CALC_TOK_VARIABLE:
                output[out_i++] = tokens[i];
                break;
                
            case CALC_TOK_FUNCTION:
                stack[stack_top++] = tokens[i];
                break;
                
            case CALC_TOK_OPERATOR:
                while (stack_top > 0 && stack[stack_top - 1].type == CALC_TOK_OPERATOR &&
                       calculator_get_precedence(stack[stack_top - 1].str) >=
                       calculator_get_precedence(tokens[i].str)) {
                    output[out_i++] = stack[--stack_top];
                }
                stack[stack_top++] = tokens[i];
                break;
                
            case CALC_TOK_LPAREN:
                stack[stack_top++] = tokens[i];
                break;
                
            case CALC_TOK_RPAREN:
                while (stack_top > 0 && stack[stack_top - 1].type != CALC_TOK_LPAREN) {
                    output[out_i++] = stack[--stack_top];
                }
                if (stack_top > 0) stack_top--; // Pop LPAREN
                // Check for function
                if (stack_top > 0 && stack[stack_top - 1].type == CALC_TOK_FUNCTION) {
                    output[out_i++] = stack[--stack_top];
                }
                break;
                
            default:
                break;
        }
    }
    
    while (stack_top > 0) {
        output[out_i++] = stack[--stack_top];
    }
    
    *out_count = out_i;
    return 0;
}

double calculator_evaluate_rpn(calc_token_t *tokens, uint32_t count) {
    double stack[256];
    uint32_t stack_top = 0;
    
    for (uint32_t i = 0; i < count; i++) {
        switch (tokens[i].type) {
            case CALC_TOK_NUMBER:
                stack[stack_top++] = tokens[i].value;
                break;
                
            case CALC_TOK_OPERATOR: {
                if (stack_top < 2) return 0;
                double b = stack[--stack_top];
                double a = stack[--stack_top];
                double result = 0;
                
                if (!strcmp(tokens[i].str, "+")) result = a + b;
                else if (!strcmp(tokens[i].str, "-")) result = a - b;
                else if (!strcmp(tokens[i].str, "*")) result = a * b;
                else if (!strcmp(tokens[i].str, "/")) result = (b != 0) ? a / b : 0;
                else if (!strcmp(tokens[i].str, "%")) result = (b != 0) ? (int64_t)a % (int64_t)b : 0;
                else if (!strcmp(tokens[i].str, "^")) result = calculator_pow(a, b);
                else if (!strcmp(tokens[i].str, "&")) result = (int64_t)a & (int64_t)b;
                else if (!strcmp(tokens[i].str, "|")) result = (int64_t)a | (int64_t)b;
                else if (!strcmp(tokens[i].str, "<<")) result = (int64_t)a << (int)b;
                else if (!strcmp(tokens[i].str, ">>")) result = (int64_t)a >> (int)b;
                
                stack[stack_top++] = result;
                break;
            }
                
            case CALC_TOK_FUNCTION: {
                if (stack_top < 1) return 0;
                double a = stack[--stack_top];
                double result = 0;
                
                if (!strcmp(tokens[i].str, "sin")) result = calculator_sin(a, ANGLE_DEGREE);
                else if (!strcmp(tokens[i].str, "cos")) result = calculator_cos(a, ANGLE_DEGREE);
                else if (!strcmp(tokens[i].str, "tan")) result = calculator_tan(a, ANGLE_DEGREE);
                else if (!strcmp(tokens[i].str, "sqrt")) result = calculator_sqrt(a);
                else if (!strcmp(tokens[i].str, "log")) result = calculator_log10(a);
                else if (!strcmp(tokens[i].str, "ln")) result = calculator_ln(a);
                else if (!strcmp(tokens[i].str, "exp")) result = calculator_exp(a);
                else if (!strcmp(tokens[i].str, "abs")) result = calculator_abs(a);
                else if (!strcmp(tokens[i].str, "floor")) result = calculator_floor(a);
                else if (!strcmp(tokens[i].str, "ceil")) result = calculator_ceil(a);
                else if (!strcmp(tokens[i].str, "round")) result = calculator_round(a);
                else if (!strcmp(tokens[i].str, "fact")) result = calculator_factorial(a);
                else if (!strcmp(tokens[i].str, "rand")) result = calculator_random();
                
                stack[stack_top++] = result;
                break;
            }
                
            default:
                break;
        }
    }
    
    return (stack_top > 0) ? stack[stack_top - 1] : 0;
}

// History
void calculator_add_history(calculator_t *calc, const char *expr, double result) {
    if (calc->history_count >= CALC_MAX_HISTORY) {
        // Shift
        for (uint32_t i = 0; i < CALC_MAX_HISTORY - 1; i++) {
            calc->history[i] = calc->history[i + 1];
        }
        calc->history_count = CALC_MAX_HISTORY - 1;
    }
    
    calc_history_entry_t *entry = &calc->history[calc->history_count++];
    strncpy(entry->expression, expr, CALC_MAX_EXPRESSION - 1);
    entry->result = result;
    entry->mode = calc->mode;
    entry->timestamp = 0;
}

calc_history_entry_t* calculator_get_history(calculator_t *calc, uint32_t *count) {
    *count = calc->history_count;
    return calc->history;
}

void calculator_clear_history(calculator_t *calc) {
    calc->history_count = 0;
}

// Unit conversion
double calculator_convert(double value, calc_unit_category_t category, int from_unit, int to_unit) {
    calc_unit_t *units = NULL;
    uint32_t count = 0;
    
    switch (category) {
        case UNIT_LENGTH:
            units = length_units;
            count = sizeof(length_units) / sizeof(calc_unit_t);
            break;
        case UNIT_WEIGHT:
            units = weight_units;
            count = sizeof(weight_units) / sizeof(calc_unit_t);
            break;
        case UNIT_DATA:
            units = data_units;
            count = sizeof(data_units) / sizeof(calc_unit_t);
            break;
        default:
            return value;
    }
    
    if (from_unit < 0 || from_unit >= (int)count || to_unit < 0 || to_unit >= (int)count) {
        return value;
    }
    
    // Convert to base then to target
    double base_value = value * units[from_unit].to_base;
    return base_value / units[to_unit].to_base;
}

const calc_unit_t* calculator_get_units(calc_unit_category_t category, uint32_t *count) {
    switch (category) {
        case UNIT_LENGTH:
            *count = sizeof(length_units) / sizeof(calc_unit_t);
            return length_units;
        case UNIT_WEIGHT:
            *count = sizeof(weight_units) / sizeof(calc_unit_t);
            return weight_units;
        case UNIT_DATA:
            *count = sizeof(data_units) / sizeof(calc_unit_t);
            return data_units;
        default:
            *count = 0;
            return NULL;
    }
}

// Graphing
void calculator_graph_add(calculator_t *calc, const char *expr, double x_min, double x_max, uint32_t points) {
    if (calc->num_graphs >= 8) return;
    
    calc_graph_t *graph = &calc->graphs[calc->num_graphs++];
    strncpy(graph->expression, expr, CALC_MAX_EXPRESSION - 1);
    graph->x_min = x_min;
    graph->x_max = x_max;
    graph->num_points = points;
    graph->x_values = (double*)kmalloc(points * sizeof(double));
    graph->y_values = (double*)kmalloc(points * sizeof(double));
    graph->color = 0xFF00FF00;
    
    calculator_graph_evaluate(graph);
}

void calculator_graph_evaluate(calc_graph_t *graph) {
    if (!graph->x_values || !graph->y_values) return;
    
    double step = (graph->x_max - graph->x_min) / (graph->num_points - 1);
    
    graph->y_min = 1e308;
    graph->y_max = -1e308;
    
    for (uint32_t i = 0; i < graph->num_points; i++) {
        graph->x_values[i] = graph->x_min + i * step;
        
        // Evaluate expression at x
        // For demo, use simple functions
        double x = graph->x_values[i];
        double y = calculator_sin(x, ANGLE_RADIAN); // Default to sin(x)
        
        graph->y_values[i] = y;
        
        if (y < graph->y_min) graph->y_min = y;
        if (y > graph->y_max) graph->y_max = y;
    }
}

void calculator_graph_render(calc_graph_t *graph, uint32_t *buffer, int w, int h) {
    if (!graph->x_values || !graph->y_values) return;
    
    // Clear
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    // Draw axes
    int origin_x = w / 2;
    int origin_y = h / 2;
    
    for (int x = 0; x < w; x++) buffer[origin_y * w + x] = 0xFF444444;
    for (int y = 0; y < h; y++) buffer[y * w + origin_x] = 0xFF444444;
    
    // Scale
    double x_range = graph->x_max - graph->x_min;
    double y_range = graph->y_max - graph->y_min;
    if (y_range == 0) y_range = 1;
    
    // Draw graph
    for (uint32_t i = 0; i < graph->num_points - 1; i++) {
        int x1 = (int)((graph->x_values[i] - graph->x_min) / x_range * w);
        int y1 = h - (int)((graph->y_values[i] - graph->y_min) / y_range * h);
        int x2 = (int)((graph->x_values[i + 1] - graph->x_min) / x_range * w);
        int y2 = h - (int)((graph->y_values[i + 1] - graph->y_min) / y_range * h);
        
        // Simple line
        if (x1 >= 0 && x1 < w && y1 >= 0 && y1 < h) buffer[y1 * w + x1] = graph->color;
        if (x2 >= 0 && x2 < w && y2 >= 0 && y2 < h) buffer[y2 * w + x2] = graph->color;
    }
}

void calculator_graph_clear(calculator_t *calc) {
    for (uint32_t i = 0; i < calc->num_graphs; i++) {
        if (calc->graphs[i].x_values) kfree(calc->graphs[i].x_values);
        if (calc->graphs[i].y_values) kfree(calc->graphs[i].y_values);
    }
    calc->num_graphs = 0;
}

// GUI rendering
void calculator_render_standard(calculator_t *calc, uint32_t *buffer, int w, int h) {
    // Background
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF2D2D2D;
    
    // Display area
    calculator_render_display(calc->display, 10, 10, w - 20, 60, buffer, w, h);
    
    // Button grid
    const char *buttons[] = {
        "MC", "MR", "M+", "M-", "MS",
        "%", "CE", "C", "DEL", "/",
        "7", "8", "9", "x", "sqrt",
        "4", "5", "6", "-", "x^2",
        "1", "2", "3", "+", "1/x",
        "+/-", "0", ".", "=", "pi"
    };
    
    int btn_w = (w - 40) / 5;
    int btn_h = (h - 100) / 6;
    int start_y = 90;
    
    for (int row = 0; row < 6; row++) {
        for (int col = 0; col < 5; col++) {
            int idx = row * 5 + col;
            if (idx >= 30) break;
            
            int bx = 20 + col * btn_w;
            int by = start_y + row * btn_h;
            
            uint32_t bg = 0xFF3D3D3D;
            uint32_t fg = 0xFFFFFFFF;
            
            if (row == 0) { bg = 0xFF4D4D4D; }
            else if (col == 4) { bg = 0xFF5D5D5D; }
            else if (col == 3) { bg = 0xFFFF9500; fg = 0xFFFFFFFF; }
            else if (row == 5 && col == 4) { bg = 0xFFFF9500; }
            
            calculator_render_button(bx, by, btn_w - 4, btn_h - 4, buttons[idx], bg, fg, buffer, w, h);
        }
    }
}

void calculator_render_display(const char *text, int x, int y, int w, int h, uint32_t *buffer, int bw, int bh) {
    // Background
    for (int row = y; row < y + h && row < bh; row++) {
        for (int col = x; col < x + w && col < bw; col++) {
            buffer[row * bw + col] = 0xFF1A1A1A;
        }
    }
    
    // Border
    for (int col = x; col < x + w && col < bw; col++) {
        buffer[y * bw + col] = 0xFF555555;
        buffer[(y + h - 1) * bw + col] = 0xFF555555;
    }
    for (int row = y; row < y + h && row < bh; row++) {
        buffer[row * bw + x] = 0xFF555555;
        buffer[row * bw + x + w - 1] = 0xFF555555;
    }
    
    // Text (simplified - just draw pixels)
    int tx = x + w - 20;
    int ty = y + h / 2;
    
    // Draw text right-aligned (simplified)
    // In real implementation, would use proper font rendering
}

void calculator_render_button(int x, int y, int w, int h, const char *label, uint32_t bg, uint32_t fg, uint32_t *buffer, int bw, int bh) {
    // Button background
    for (int row = y; row < y + h && row < bh; row++) {
        for (int col = x; col < x + w && col < bw; col++) {
            buffer[row * bw + col] = bg;
        }
    }
    
    // Border
    for (int col = x; col < x + w && col < bw; col++) {
        buffer[y * bw + col] = 0xFF555555;
        buffer[(y + h - 1) * bw + col] = 0xFF555555;
    }
    for (int row = y; row < y + h && row < bh; row++) {
        buffer[row * bw + x] = 0xFF555555;
        buffer[row * bw + x + w - 1] = 0xFF555555;
    }
    
    // Label (centered - simplified)
    // In real implementation, would render text
}

void calculator_render_scientific(calculator_t *calc, uint32_t *buffer, int w, int h) {
    calculator_render_standard(calc, buffer, w, h);
    // Additional scientific buttons would be rendered here
}

void calculator_render_programmer(calculator_t *calc, uint32_t *buffer, int w, int h) {
    // Programmer layout with bit display
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF2D2D2D;
    
    // Bit display at top
    char bits[128];
    calculator_to_binary(calc->bit_display, calc->word_size, bits);
    calculator_render_display(bits, 10, 10, w - 20, 40, buffer, w, h);
    
    // Hex display
    char hex[32];
    calculator_to_hex(calc->bit_display, hex);
    calculator_render_display(hex, 10, 55, w - 20, 30, buffer, w, h);
    
    // Buttons
    const char *buttons[] = {
        "AND", "OR", "XOR", "NOT", "NAND",
        "LSH", "RSH", "ROL", "ROR", "MOD",
        "A", "B", "C", "D", "E",
        "F", "7", "8", "9", "+",
        "4", "5", "6", "-", "*",
        "1", "2", "3", "/", "=",
        "0", ".", "CE", "C", "DEL"
    };
    
    int btn_w = (w - 40) / 5;
    int btn_h = (h - 120) / 7;
    int start_y = 100;
    
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 5; col++) {
            int idx = row * 5 + col;
            if (idx >= 35) break;
            
            int bx = 20 + col * btn_w;
            int by = start_y + row * btn_h;
            
            uint32_t bg = 0xFF3D3D3D;
            uint32_t fg = 0xFFFFFFFF;
            
            if (row < 2) { bg = 0xFF4D4D4D; }
            else if (col == 4) { bg = 0xFFFF9500; }
            
            calculator_render_button(bx, by, btn_w - 4, btn_h - 4, buttons[idx], bg, fg, buffer, w, h);
        }
    }
}

void calculator_render_graphing(calculator_t *calc, uint32_t *buffer, int w, int h) {
    // Graph area
    int graph_h = h - 100;
    
    for (int i = 0; i < w * h; i++) buffer[i] = 0xFF1A1A2E;
    
    // Render all graphs
    for (uint32_t i = 0; i < calc->num_graphs; i++) {
        calculator_graph_render(&calc->graphs[i], buffer, w, graph_h);
    }
    
    // Input area at bottom
    calculator_render_display(calc->display, 10, graph_h + 10, w - 20, 40, buffer, w, h);
    
    // Function buttons
    const char *funcs[] = {"sin", "cos", "tan", "log", "ln", "exp", "sqrt", "pi", "atom", "pr"};
    int btn_w = (w - 20) / 10;
    for (int i = 0; i < 10; i++) {
        calculator_render_button(10 + i * btn_w, graph_h + 55, btn_w - 4, 35, funcs[i], 0xFF4D4D4D, 0xFFFFFFFF, buffer, w, h);
    }
}
