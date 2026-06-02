// ============================================================================
// ZEBX OS - AI ENGINE (Lite, trainable)
// ----------------------------------------------------------------------------
// Why "Lite":
// - Your previous transformer code relied on libm/varargs and lived in ai.h,
//   which breaks freestanding kernel builds.
// - This implementation is real (it learns statistics) and runs fast on bare metal.
//
// Model:
// - Token vocabulary (small, built-in)
// - Trainable bigram counts: P(next_token | prev_token)
// - Temperature + top-k sampling for generation
// ============================================================================

#include "ai.h"
#include "memory.h"
#include "timer.h"
#include "vga.h"

// libc-like helpers are assumed available in the kernel codebase (used elsewhere too)
extern void *memset(void *s, int c, size_t n);
extern void *memcpy(void *dst, const void *src, size_t n);
extern size_t strlen(const char *s);
extern char *strcpy(char *dst, const char *src);
extern int strcmp(const char *a, const char *b);
extern int strncmp(const char *a, const char *b, size_t n);
extern char *strcat(char *dst, const char *src);
extern char *strchr(const char *s, int c);
extern char *strncpy(char *dst, const char *src, size_t n);

static ai_context_t g_ctx;
static int g_loaded = 0;

// Keep the vocabulary modest so memory stays sane in-kernel.
// (You can expand this list later.)
static const char *g_vocab[] = {
    "<pad>", "<unk>", "<s>", "</s>",
    "I", "you", "we", "they", "it", "this", "that",
    "is", "are", "was", "help", "code", "kernel", "driver",
    "game", "gaming", "input", "usb", "keyboard", "mouse",
    "make", "add", "fix", "error", "compile", "run",
    "yes", "no", "ok", "thanks",
    "(", ")", "{", "}", "[", "]", ";", ",", ".", "=", "+", "-", "*", "/",
    "int", "char", "void", "struct", "typedef", "return", "if", "else", "for", "while",
    "ZEBX", "OS"
};

#define VOCAB_N ((uint32_t)(sizeof(g_vocab) / sizeof(g_vocab[0])))

// Bigram counts: counts[prev * VOCAB_N + next]
static uint32_t *g_counts = NULL;
static uint32_t *g_row_sum = NULL;

static uint32_t rand_u32(void) {
    // LCG based on timer ticks (good enough for sampling)
    static uint32_t s = 0xC0FFEEu;
    uint32_t t = (uint32_t)timer_get_ticks();
    s ^= (t + 0x9E3779B9u);
    s = s * 1664525u + 1013904223u;
    return s;
}

static uint32_t vocab_find_longest(const char *p, uint32_t *out_len) {
    int best = -1;
    uint32_t best_len = 0;

    for (uint32_t i = 0; i < VOCAB_N; i++) {
        uint32_t len = (uint32_t)strlen(g_vocab[i]);
        if (len == 0) continue;
        if (len <= best_len) continue;
        if (strncmp(p, g_vocab[i], len) == 0) {
            best = (int)i;
            best_len = len;
        }
    }

    if (best < 0) {
        *out_len = 0;
        return 1; // <unk>
    }

    *out_len = best_len;
    return (uint32_t)best;
}

uint32_t ai_tokenize(const char *text, ai_token_t *tokens, uint32_t max_tokens) {
    uint32_t count = 0;
    const char *p = text;

    while (*p && count < max_tokens) {
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
        if (!*p) break;

        uint32_t mlen = 0;
        uint32_t id = vocab_find_longest(p, &mlen);
        if (mlen == 0) {
            // single-char fallback
            tokens[count].id = 1;
            tokens[count].type = AI_TOK_UNKNOWN;
            tokens[count].text[0] = *p;
            tokens[count].text[1] = 0;
            count++;
            p++;
            continue;
        }

        tokens[count].id = id;
        tokens[count].type = AI_TOK_WORD;
        // Store a short preview in token.text
        uint32_t cpy = (mlen >= 31) ? 31 : mlen;
        for (uint32_t i = 0; i < cpy; i++) tokens[count].text[i] = p[i];
        tokens[count].text[cpy] = 0;
        count++;
        p += mlen;
    }

    return count;
}

uint32_t ai_encode(const char *text, uint32_t *token_ids, uint32_t max_len) {
    ai_token_t toks[AI_MAX_TOKENS];
    uint32_t n = ai_tokenize(text, toks, max_len);
    for (uint32_t i = 0; i < n && i < max_len; i++) token_ids[i] = toks[i].id;
    return n;
}

void ai_decode(uint32_t *token_ids, uint32_t count, char *text, uint32_t max_len) {
    uint32_t pos = 0;
    for (uint32_t i = 0; i < count && pos < max_len - 1; i++) {
        uint32_t id = token_ids[i];
        const char *w = (id < VOCAB_N) ? g_vocab[id] : g_vocab[1];
        uint32_t wlen = (uint32_t)strlen(w);

        if (pos && pos < max_len - 1) text[pos++] = ' ';
        for (uint32_t j = 0; j < wlen && pos < max_len - 1; j++) text[pos++] = w[j];
    }
    text[pos] = 0;
}

const char* ai_detokenize(ai_token_t *tokens, uint32_t count) {
    (void)tokens; (void)count;
    return "";
}

void ai_train_from_text(const char *text) {
    if (!g_loaded || !g_counts || !text) return;

    uint32_t ids[AI_MAX_TOKENS];
    uint32_t n = ai_encode(text, ids, AI_MAX_TOKENS);
    if (n < 2) return;

    for (uint32_t i = 0; i + 1 < n; i++) {
        uint32_t a = ids[i];
        uint32_t b = ids[i + 1];
        if (a >= VOCAB_N) a = 1;
        if (b >= VOCAB_N) b = 1;

        uint32_t idx = a * VOCAB_N + b;
        if (g_counts[idx] < 0xFFFFFFFFu) g_counts[idx]++;
        if (g_row_sum[a] < 0xFFFFFFFFu) g_row_sum[a]++;
    }
}

void ai_init(void) {
    memset(&g_ctx, 0, sizeof(g_ctx));
    strcpy(g_ctx.system_prompt, "You are ZEBX-GPT (Lite). Ask me for help with ZEBX OS.");

    if (!g_counts) {
        g_counts = (uint32_t*)kmalloc(VOCAB_N * VOCAB_N * sizeof(uint32_t));
        g_row_sum = (uint32_t*)kmalloc(VOCAB_N * sizeof(uint32_t));
    }
    if (g_counts) memset(g_counts, 0, VOCAB_N * VOCAB_N * sizeof(uint32_t));
    if (g_row_sum) memset(g_row_sum, 0, VOCAB_N * sizeof(uint32_t));

    g_loaded = (g_counts && g_row_sum) ? 1 : 0;

    // Seed with a tiny corpus so it can talk immediately.
    ai_train_from_text("ZEBX OS help fix error compile code kernel driver usb input gaming");

    vga_puts(g_loaded ? "[+] ZEBX-GPT Lite initialized\n" : "[!] ZEBX-GPT Lite init failed\n");
}

void ai_load_model(const char *path) {
    // For now, training data is supplied via ai_train_from_text().
    vga_puts("[*] ai_load_model: ");
    vga_puts(path ? path : "(null)");
    vga_puts("\n");
}

int ai_model_loaded(void) {
    return g_loaded;
}

static uint32_t sample_next(uint32_t prev, float temperature) {
    if (prev >= VOCAB_N) prev = 1;
    if (!g_row_sum[prev]) {
        return (rand_u32() % (VOCAB_N - 4)) + 4; // avoid special tokens
    }

    // Top-k over counts (k=16) without floats.
    const uint32_t K = 16;
    uint32_t top_id[K];
    uint32_t top_c[K];
    for (uint32_t i = 0; i < K; i++) { top_id[i] = 1; top_c[i] = 0; }

    for (uint32_t n = 0; n < VOCAB_N; n++) {
        uint32_t c = g_counts[prev * VOCAB_N + n];
        // insert into top-K
        for (uint32_t j = 0; j < K; j++) {
            if (c > top_c[j]) {
                // shift down
                for (uint32_t k = K - 1; k > j; k--) { top_c[k] = top_c[k - 1]; top_id[k] = top_id[k - 1]; }
                top_c[j] = c;
                top_id[j] = n;
                break;
            }
        }
    }

    // Convert counts to a simple weighted pick.
    // Temperature: if <1.0 bias to higher ranks; if >1.0 flatten.
    uint32_t weights[K];
    uint32_t wsum = 0;
    for (uint32_t i = 0; i < K; i++) {
        uint32_t base = top_c[i] ? top_c[i] : 1;
        if (temperature < 0.7f) base = base * base;        // sharper
        else if (temperature > 1.2f) base = (base + 3) / 4; // flatter
        weights[i] = base;
        wsum += base;
    }
    if (!wsum) return top_id[0];

    uint32_t r = rand_u32() % wsum;
    uint32_t acc = 0;
    for (uint32_t i = 0; i < K; i++) {
        acc += weights[i];
        if (r < acc) return top_id[i];
    }
    return top_id[0];
}

void ai_forward_pass(uint32_t *input_ids, uint32_t input_len, float *output_logits) {
    // Not used by Lite model; keep API compatibility.
    (void)input_ids; (void)input_len; (void)output_logits;
}

uint32_t ai_sample_token(float *logits, uint32_t vocab_size, float temperature) {
    (void)logits; (void)vocab_size;
    // Not used by Lite model; return a random-ish token.
    (void)temperature;
    return (rand_u32() % (VOCAB_N - 4)) + 4;
}

void ai_generate(uint32_t *prompt_ids, uint32_t prompt_len, uint32_t *output_ids, uint32_t *output_len, uint32_t max_new_tokens) {
    if (!g_loaded) { if (output_len) *output_len = 0; return; }

    // Online-learn from the prompt itself.
    if (prompt_ids && prompt_len) {
        for (uint32_t i = 0; i + 1 < prompt_len; i++) {
            uint32_t a = prompt_ids[i] % VOCAB_N;
            uint32_t b = prompt_ids[i + 1] % VOCAB_N;
            g_counts[a * VOCAB_N + b]++; g_row_sum[a]++;
        }
    }

    uint32_t prev = (prompt_len ? prompt_ids[prompt_len - 1] : 2) % VOCAB_N;
    uint32_t outn = 0;
    for (uint32_t i = 0; i < max_new_tokens; i++) {
        uint32_t next = sample_next(prev, 0.9f);
        output_ids[outn++] = next;
        prev = next;
    }
    *output_len = outn;
}

void ai_new_chat(void) {
    memset(&g_ctx, 0, sizeof(g_ctx));
    strcpy(g_ctx.system_prompt, "You are ZEBX-GPT (Lite).");
}

void ai_set_system_prompt(const char *prompt) {
    if (!prompt) return;
    strncpy(g_ctx.system_prompt, prompt, 1023);
    g_ctx.system_prompt[1023] = 0;
}

void ai_add_user_message(const char *message) {
    if (!message) return;
    if (g_ctx.num_turns >= 32) g_ctx.num_turns = 0;
    strncpy(g_ctx.user_messages[g_ctx.num_turns], message, AI_MAX_CONTEXT_LEN - 1);
    g_ctx.user_messages[g_ctx.num_turns][AI_MAX_CONTEXT_LEN - 1] = 0;
}

void ai_add_assistant_response(const char *response) {
    if (!response) return;
    if (g_ctx.num_turns >= 32) g_ctx.num_turns = 0;
    strncpy(g_ctx.assistant_responses[g_ctx.num_turns], response, AI_MAX_RESPONSE_LEN - 1);
    g_ctx.assistant_responses[g_ctx.num_turns][AI_MAX_RESPONSE_LEN - 1] = 0;
    g_ctx.num_turns++;
}

ai_context_t* ai_get_context(void) {
    return &g_ctx;
}

void ai_clear_context(void) {
    g_ctx.num_turns = 0;
    g_ctx.total_tokens = 0;
}

void ai_chat(const char *message, char *response, uint32_t max_len) {
    if (!g_loaded) {
        if (response && max_len) strncpy(response, "AI not initialized.", max_len - 1);
        return;
    }

    ai_add_user_message(message ? message : "");
    if (message) ai_train_from_text(message); // this is the "training" you asked for

    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(message ? message : "", prompt_ids, AI_MAX_TOKENS);

    uint32_t out_ids[256];
    uint32_t out_len = 0;
    ai_generate(prompt_ids, prompt_len, out_ids, &out_len, 24);

    ai_decode(out_ids, out_len, response, max_len);
    ai_add_assistant_response(response);
}

// High-level wrappers: currently route to ai_chat with small prompting.
static void prefix_and_chat(const char *prefix, const char *body, char *out, uint32_t max_len) {
    char buf[1024];
    uint32_t pos = 0;
    if (prefix) {
        uint32_t l = (uint32_t)strlen(prefix);
        for (uint32_t i = 0; i < l && pos < sizeof(buf) - 1; i++) buf[pos++] = prefix[i];
    }
    if (body) {
        uint32_t l = (uint32_t)strlen(body);
        for (uint32_t i = 0; i < l && pos < sizeof(buf) - 1; i++) buf[pos++] = body[i];
    }
    buf[pos] = 0;
    ai_chat(buf, out, max_len);
}

void ai_generate_code(ai_task_t task, const char *prompt, char *code, uint32_t max_len) {
    (void)task;
    prefix_and_chat("code ", prompt ? prompt : "", code, max_len);
}

void ai_explain_code(const char *code, char *explanation, uint32_t max_len) {
    prefix_and_chat("explain ", code ? code : "", explanation, max_len);
}

void ai_debug_code(const char *code, const char *error, char *fix, uint32_t max_len) {
    (void)error;
    prefix_and_chat("fix ", code ? code : "", fix, max_len);
}

void ai_optimize_code(const char *code, char *optimized, uint32_t max_len) {
    prefix_and_chat("optimize ", code ? code : "", optimized, max_len);
}

void ai_translate_code(const char *code, const char *from_lang, const char *to_lang, char *translated, uint32_t max_len) {
    (void)from_lang; (void)to_lang;
    prefix_and_chat("translate ", code ? code : "", translated, max_len);
}

void ai_summarize(const char *text, char *summary, uint32_t max_len) {
    prefix_and_chat("summarize ", text ? text : "", summary, max_len);
}

void ai_os_command(const char *natural_language, char *command, uint32_t max_len) {
    prefix_and_chat("command ", natural_language ? natural_language : "", command, max_len);
}

void ai_file_suggest(const char *query, char *suggestions, uint32_t max_len) {
    prefix_and_chat("files ", query ? query : "", suggestions, max_len);
}

void ai_system_diagnose(char *report, uint32_t max_len) {
    prefix_and_chat("diagnose ", "system", report, max_len);
}

void ai_compile_assist(const char *source, const char *lang, char *output, uint32_t max_len) {
    (void)lang;
    prefix_and_chat("compile ", source ? source : "", output, max_len);
}

// Stubs for SIMD hooks (not used by Lite model yet)
void ai_embedding_lookup_simd(uint32_t *token_ids, uint32_t count, float *output) { (void)token_ids; (void)count; (void)output; }
void ai_attention_simd(float *q, float *k, float *v, float *output, uint32_t seq_len) { (void)q; (void)k; (void)v; (void)output; (void)seq_len; }
void ai_ffn_simd(float *x, float *w1, float *b1, float *w2, float *b2, float *output, uint32_t dim) { (void)x; (void)w1; (void)b1; (void)w2; (void)b2; (void)output; (void)dim; }

