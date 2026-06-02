#ifndef AI_H
#define AI_H

#include "types.h"

#define AI_MAX_CONTEXT_LEN      8192
#define AI_MAX_RESPONSE_LEN     32768
#define AI_MAX_TOKENS           4096
#define AI_VOCAB_SIZE           50000
#define AI_EMBEDDING_DIM        768
#define AI_NUM_HEADS            12
#define AI_NUM_LAYERS           12
#define AI_FF_DIM               3072
#define AI_MAX_BATCH            4

// Token types
typedef enum {
    AI_TOK_UNKNOWN,
    AI_TOK_WORD,
    AI_TOK_NUMBER,
    AI_TOK_SYMBOL,
    AI_TOK_NEWLINE,
    AI_TOK_SPACE,
    AI_TOK_CODE_BLOCK,
    AI_TOK_INLINE_CODE,
    AI_TOK_SPECIAL,
} ai_token_type_t;

// Token
typedef struct {
    uint32_t id;
    ai_token_type_t type;
    char text[32];
    float embedding[AI_EMBEDDING_DIM];
} ai_token_t;

// Attention head
typedef struct {
    float *wq; // Query weights [EMBEDDING_DIM x EMBEDDING_DIM/NUM_HEADS]
    float *wk; // Key weights
    float *wv; // Value weights
    float *wo; // Output weights
} ai_attention_head_t;

// Transformer layer
typedef struct {
    ai_attention_head_t heads[AI_NUM_HEADS];
    float *ff_w1; // Feed-forward weights [EMBEDDING_DIM x FF_DIM]
    float *ff_w2; // Feed-forward weights [FF_DIM x EMBEDDING_DIM]
    float *ff_b1; // Biases
    float *ff_b2;
    float *ln_gamma1; // Layer norm parameters
    float *ln_beta1;
    float *ln_gamma2;
    float *ln_beta2;
} ai_transformer_layer_t;

// Model
typedef struct {
    ai_token_t vocab[AI_VOCAB_SIZE];
    uint32_t vocab_size;
    float *token_embeddings;
    float *position_embeddings;
    ai_transformer_layer_t layers[AI_NUM_LAYERS];
    float *output_norm_gamma;
    float *output_norm_beta;
    float *lm_head; // Language model head weights
    uint32_t num_params;
} ai_model_t;

// Conversation context
typedef struct {
    char system_prompt[1024];
    char user_messages[32][AI_MAX_CONTEXT_LEN];
    char assistant_responses[32][AI_MAX_RESPONSE_LEN];
    uint32_t num_turns;
    uint32_t total_tokens;
} ai_context_t;

// Task types
typedef enum {
    AI_TASK_CHAT,
    AI_TASK_CODE_C,
    AI_TASK_CODE_CPP,
    AI_TASK_CODE_PYTHON,
    AI_TASK_CODE_CSHARP,
    AI_TASK_EXPLAIN,
    AI_TASK_DEBUG,
    AI_TASK_OPTIMIZE,
    AI_TASK_TRANSLATE,
    AI_TASK_SUMMARIZE,
    AI_TASK_OS_COMMAND,
    AI_TASK_FILE_MANAGE,
    AI_TASK_COMPILE,
    AI_TASK_SYSTEM_INFO,
} ai_task_t;

// Functions
void ai_init(void);
void ai_load_model(const char *path);
int ai_model_loaded(void);
void ai_train_from_text(const char *text);

// Tokenization
uint32_t ai_tokenize(const char *text, ai_token_t *tokens, uint32_t max_tokens);
const char* ai_detokenize(ai_token_t *tokens, uint32_t count);
uint32_t ai_encode(const char *text, uint32_t *token_ids, uint32_t max_len);
void ai_decode(uint32_t *token_ids, uint32_t count, char *text, uint32_t max_len);

// Inference
void ai_forward_pass(uint32_t *input_ids, uint32_t input_len, float *output_logits);
uint32_t ai_sample_token(float *logits, uint32_t vocab_size, float temperature);
void ai_generate(uint32_t *prompt_ids, uint32_t prompt_len, uint32_t *output_ids, uint32_t *output_len, uint32_t max_new_tokens);

// High-level API
void ai_chat(const char *message, char *response, uint32_t max_len);
void ai_generate_code(ai_task_t task, const char *prompt, char *code, uint32_t max_len);
void ai_explain_code(const char *code, char *explanation, uint32_t max_len);
void ai_debug_code(const char *code, const char *error, char *fix, uint32_t max_len);
void ai_optimize_code(const char *code, char *optimized, uint32_t max_len);
void ai_translate_code(const char *code, const char *from_lang, const char *to_lang, char *translated, uint32_t max_len);
void ai_summarize(const char *text, char *summary, uint32_t max_len);

// OS-specific tasks
void ai_os_command(const char *natural_language, char *command, uint32_t max_len);
void ai_file_suggest(const char *query, char *suggestions, uint32_t max_len);
void ai_system_diagnose(char *report, uint32_t max_len);
void ai_compile_assist(const char *source, const char *lang, char *output, uint32_t max_len);

// Context management
void ai_new_chat(void);
void ai_set_system_prompt(const char *prompt);
void ai_add_user_message(const char *message);
void ai_add_assistant_response(const char *response);
ai_context_t* ai_get_context(void);
void ai_clear_context(void);

// Attention mechanism
void ai_compute_attention(float *q, float *k, float *v, float *output, uint32_t seq_len, uint32_t head_dim);
void ai_softmax(float *x, uint32_t len);
void ai_layer_norm(float *x, uint32_t len, float gamma, float beta);
void ai_gelu(float *x, uint32_t len);
void ai_matmul(float *a, float *b, float *c, uint32_t m, uint32_t n, uint32_t k);

// Utilities
float ai_random_normal(void);
void ai_init_weights(float *weights, uint32_t count, float scale);
float ai_cosine_similarity(float *a, float *b, uint32_t len);

// SIMD-accelerated operations
void ai_embedding_lookup_simd(uint32_t *token_ids, uint32_t count, float *output);
void ai_attention_simd(float *q, float *k, float *v, float *output, uint32_t seq_len);
void ai_ffn_simd(float *x, float *w1, float *b1, float *w2, float *b2, float *output, uint32_t dim);

#endif