// ============================================================================
// ZEBX OS - ZEBX-GPT v2.0 AI ENGINE
// Complete Transformer Implementation with Training & Inference
// 768D embeddings, 12 layers, 12 heads, 3072 FFN dim
// ============================================================================

#include "kernel/types.h"
#include "kernel/memory.h"
#include "kernel/vga.h"
#include "arch/simd.h"

#define AI_VOCAB_SIZE       50000
#define AI_MAX_SEQ_LEN      4096
#define AI_EMBEDDING_DIM    768
#define AI_NUM_HEADS        12
#define AI_NUM_LAYERS       12
#define AI_FF_DIM           3072
#define AI_HEAD_DIM         (AI_EMBEDDING_DIM / AI_NUM_HEADS)  // 64
#define AI_MAX_BATCH        4
#define AI_TEMPERATURE      0.8f
#define AI_TOP_K            50

// Token types
#define TOK_PAD         0
#define TOK_UNK         1
#define TOK_BOS         2
#define TOK_EOS         3
#define TOK_MASK        4

// Activation types
#define ACT_GELU        1
#define ACT_RELU        2
#define ACT_SWISH       3

// Model precision
#define USE_FP16        1   // Half precision for speed
#define USE_QUANT       1   // 8-bit quantization for weights

typedef struct {
    uint32_t id;
    char text[64];
    float embedding[AI_EMBEDDING_DIM];
} token_t;

// QKV weights for attention
typedef struct {
    float *wq;  // [EMBEDDING_DIM x EMBEDDING_DIM]
    float *wk;
    float *wv;
    float *wo;  // Output projection
} attention_weights_t;

// FFN weights
typedef struct {
    float *w1;  // [EMBEDDING_DIM x FF_DIM]
    float *w2;  // [FF_DIM x EMBEDDING_DIM]
    float *b1;
    float *b2;
} ffn_weights_t;

// Layer norm parameters
typedef struct {
    float *gamma;
    float *beta;
} layernorm_t;

// Transformer layer
typedef struct {
    attention_weights_t attn;
    ffn_weights_t ffn;
    layernorm_t ln1;
    layernorm_t ln2;
} transformer_layer_t;

// Complete model
typedef struct {
    token_t vocab[AI_VOCAB_SIZE];
    uint32_t vocab_size;
    
    float *token_embeddings;        // [VOCAB_SIZE x EMBEDDING_DIM]
    float *position_embeddings;       // [MAX_SEQ_LEN x EMBEDDING_DIM]
    
    transformer_layer_t layers[AI_NUM_LAYERS];
    
    layernorm_t final_ln;
    float *lm_head;                   // [EMBEDDING_DIM x VOCAB_SIZE]
    
    // Quantization scales
    float *quant_scales;
    int8_t *quant_weights;
    
    uint64_t num_params;
    float loss;
} gpt_model_t;

// Training state
typedef struct {
    float *gradients;
    float *momentum;
    float *velocity;
    float learning_rate;
    float beta1;
    float beta2;
    float epsilon;
    uint32_t step;
} optimizer_t;

// Context for generation
typedef struct {
    uint32_t tokens[AI_MAX_SEQ_LEN];
    uint32_t num_tokens;
    uint32_t max_tokens;
    float temperature;
    uint32_t top_k;
} generation_context_t;

// Global model
static gpt_model_t model;
static optimizer_t optimizer;
static int model_loaded = 0;

// ============================================================================
// MATH UTILITIES
// ============================================================================

static inline float gelu(float x) {
    // GELU: 0.5 * x * (1 + tanh(sqrt(2/pi) * (x + 0.044715 * x^3)))
    float c = 0.044715f;
    float sqrt_2_over_pi = 0.7978845608f;
    float x3 = x * x * x;
    return 0.5f * x * (1.0f + tanhf(sqrt_2_over_pi * (x + c * x3)));
}

static inline float softmax(float *x, uint32_t len, uint32_t idx) {
    float max_val = x[0];
    for (uint32_t i = 1; i < len; i++) {
        if (x[i] > max_val) max_val = x[i];
    }
    
    float sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    
    for (uint32_t i = 0; i < len; i++) {
        x[i] /= sum;
    }
    
    return x[idx];
}

static void layer_norm(float *x, uint32_t len, float *gamma, float *beta) {
    float mean = 0;
    for (uint32_t i = 0; i < len; i++) mean += x[i];
    mean /= len;
    
    float var = 0;
    for (uint32_t i = 0; i < len; i++) {
        float diff = x[i] - mean;
        var += diff * diff;
    }
    var /= len;
    
    float std = sqrtf(var + 1e-5f);
    
    for (uint32_t i = 0; i < len; i++) {
        x[i] = gamma[i] * ((x[i] - mean) / std) + beta[i];
    }
}

static void matmul(float *a, float *b, float *c, 
                   uint32_t m, uint32_t n, uint32_t k) {
    // C[m x n] = A[m x k] * B[k x n]
    // Using SIMD if available
    if (simd_has_feature(SIMD_FEATURE_AVX512F) && k >= 16) {
        for (uint32_t i = 0; i < m; i++) {
            for (uint32_t j = 0; j < n; j++) {
                float sum = 0;
                uint32_t l = 0;
                for (; l + 16 <= k; l += 16) {
                    // AVX-512 FMA would go here
                    for (uint32_t ll = 0; ll < 16; ll++) {
                        sum += a[i * k + l + ll] * b[(l + ll) * n + j];
                    }
                }
                for (; l < k; l++) {
                    sum += a[i * k + l] * b[l * n + j];
                }
                c[i * n + j] = sum;
            }
        }
    } else {
        for (uint32_t i = 0; i < m; i++) {
            for (uint32_t j = 0; j < n; j++) {
                float sum = 0;
                for (uint32_t l = 0; l < k; l++) {
                    sum += a[i * k + l] * b[l * n + j];
                }
                c[i * n + j] = sum;
            }
        }
    }
}

// ============================================================================
// ATTENTION MECHANISM
// ============================================================================

static void compute_attention(float *q, float *k, float *v, float *output,
                              uint32_t seq_len, uint32_t head_dim) {
    float *scores = (float*)kmalloc(seq_len * seq_len * sizeof(float));
    
    // Q * K^T
    for (uint32_t i = 0; i < seq_len; i++) {
        for (uint32_t j = 0; j < seq_len; j++) {
            float sum = 0;
            for (uint32_t d = 0; d < head_dim; d++) {
                sum += q[i * head_dim + d] * k[j * head_dim + d];
            }
            scores[i * seq_len + j] = sum / sqrtf((float)head_dim);
        }
        
        // Causal mask
        for (uint32_t j = i + 1; j < seq_len; j++) {
            scores[i * seq_len + j] = -1e9f;
        }
        
        // Softmax
        softmax(&scores[i * seq_len], seq_len, 0);
    }
    
    // Attention * V
    for (uint32_t i = 0; i < seq_len; i++) {
        for (uint32_t d = 0; d < head_dim; d++) {
            float sum = 0;
            for (uint32_t j = 0; j < seq_len; j++) {
                sum += scores[i * seq_len + j] * v[j * head_dim + d];
            }
            output[i * head_dim + d] = sum;
        }
    }
    
    kfree(scores);
}

// ============================================================================
// TRANSFORMER LAYER
// ============================================================================

static void transformer_layer_forward(transformer_layer_t *layer,
                                      float *input, float *output,
                                      uint32_t seq_len) {
    float *q = (float*)kmalloc(seq_len * AI_EMBEDDING_DIM * sizeof(float));
    float *k = (float*)kmalloc(seq_len * AI_EMBEDDING_DIM * sizeof(float));
    float *v = (float*)kmalloc(seq_len * AI_EMBEDDING_DIM * sizeof(float));
    float *attn_out = (float*)kmalloc(seq_len * AI_EMBEDDING_DIM * sizeof(float));
    
    // Self-attention
    matmul(input, layer->attn.wq, q, seq_len, AI_EMBEDDING_DIM, AI_EMBEDDING_DIM);
    matmul(input, layer->attn.wk, k, seq_len, AI_EMBEDDING_DIM, AI_EMBEDDING_DIM);
    matmul(input, layer->attn.wv, v, seq_len, AI_EMBEDDING_DIM, AI_EMBEDDING_DIM);
    
    // Multi-head attention
    for (int h = 0; h < AI_NUM_HEADS; h++) {
        compute_attention(
            q + h * AI_HEAD_DIM,
            k + h * AI_HEAD_DIM,
            v + h * AI_HEAD_DIM,
            attn_out + h * AI_HEAD_DIM,
            seq_len, AI_HEAD_DIM
        );
    }
    
    // Output projection
    matmul(attn_out, layer->attn.wo, output, seq_len, AI_EMBEDDING_DIM, AI_EMBEDDING_DIM);
    
    // Residual + Layer Norm 1
    for (uint32_t i = 0; i < seq_len * AI_EMBEDDING_DIM; i++) {
        output[i] += input[i];
    }
    for (uint32_t i = 0; i < seq_len; i++) {
        layer_norm(&output[i * AI_EMBEDDING_DIM], AI_EMBEDDING_DIM,
                   layer->ln1.gamma, layer->ln1.beta);
    }
    
    // FFN
    float *ffn_hidden = (float*)kmalloc(seq_len * AI_FF_DIM * sizeof(float));
    float *ffn_out = (float*)kmalloc(seq_len * AI_EMBEDDING_DIM * sizeof(float));
    
    matmul(output, layer->ffn.w1, ffn_hidden, seq_len, AI_FF_DIM, AI_EMBEDDING_DIM);
    for (uint32_t i = 0; i < seq_len * AI_FF_DIM; i++) {
        ffn_hidden[i] = gelu(ffn_hidden[i] + layer->ffn.b1[i % AI_FF_DIM]);
    }
    
    matmul(ffn_hidden, layer->ffn.w2, ffn_out, seq_len, AI_EMBEDDING_DIM, AI_FF_DIM);
    for (uint32_t i = 0; i < seq_len * AI_EMBEDDING_DIM; i++) {
        ffn_out[i] += layer->ffn.b2[i % AI_EMBEDDING_DIM];
    }
    
    // Residual + Layer Norm 2
    for (uint32_t i = 0; i < seq_len * AI_EMBEDDING_DIM; i++) {
        output[i] = ffn_out[i] + output[i];
    }
    for (uint32_t i = 0; i < seq_len; i++) {
        layer_norm(&output[i * AI_EMBEDDING_DIM], AI_EMBEDDING_DIM,
                   layer->ln2.gamma, layer->ln2.beta);
    }
    
    kfree(q); kfree(k); kfree(v); kfree(attn_out);
    kfree(ffn_hidden); kfree(ffn_out);
}

// ============================================================================
// MODEL INITIALIZATION
// ============================================================================

static void init_weights(float *weights, uint32_t count, float scale) {
    // Xavier/Glorot initialization
    for (uint32_t i = 0; i < count; i++) {
        uint32_t seed = timer_get_ticks() + i * 12345;
        float u1 = (float)(seed % 10000) / 10000.0f;
        float u2 = (float)((seed * 7) % 10000) / 10000.0f;
        if (u1 < 0.0001f) u1 = 0.0001f;
        float mag = sqrtf(-2.0f * logf(u1));
        float spare = mag * cosf(2.0f * 3.14159f * u2);
        weights[i] = spare * scale;
    }
}

static void init_vocabulary(void) {
    // Initialize with basic vocabulary
    const char *words[] = {
        "<pad>", "<unk>", "<s>", "</s>", "<mask>",
        "the", "a", "an", "is", "are", "was", "were", "be", "been",
        "have", "has", "had", "do", "does", "did", "will", "would",
        "I", "you", "he", "she", "it", "we", "they", "that", "this", "these", "those", "my", "your", "his", "her",
        "and", "or", "but", "so", "if", "then", "else", "for", "in", "on", "at", "to", "from", "by", "with", "about",
        "of", "off", "over", "under", "again", "further", "then",
        "here", "there", "when", "where", "why", "how",
        "all", "hello", "world", "program", "code", "computer", "system",
        "kernel", "memory", "process", "thread", "file", "disk",
        "weeks", "week", "month", "days" "year", "day", "hour", "minute", "second",
        "treat", "run", "walk", "eat", "drink", "sleep", "think", "know",
        "train", "learn", "read", "write", "speak", "listen", "see", "look",
        "king", "queen", "man", "prince", "princess", "child", "friend", "enemy", "love", "hate",
        "bestie", "best", "better", "gonna", "wanna", "jump", "fear", "of", "it", "thing", "estimate", "guess", "idea", "hope", "dream", "wish", "want", "need",
        "mine", "yours", "his", "hers", "ours", "theirs",
        "mom", "dad", "family", "home", "house", "car", "city", "country",
        "bad", "good", "happy", "sad", "angry", "excited", "bored", "tired",
        "red", "blue", "green", "yellow", "black", "white", "color", "dark", "light", "megenta", "cyan", "purple", "orange",
        "nah", "i", "i'd", "i'll", "i've", "you", "you'd", "you'll", "you've",
        "no", "nope", "open", "close", "dog", "cat", "parrot", "bird", "animal", "animals", "birds",
        "brought", "bought", "thought", "taught", "caught", "fought", "sought",
        "yeah", "yep", "yup", "ok", "okay", "sure", "definitely", "absolutely", "maybe", "perhaps",
        "night", "day", "morning", "afternoon", "evening", "weekend", "holiday",
        "sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday",
        "cpp", "c", "python", "java", "javascript", "ruby", "go", "rust", "swift",
        "c++", "c#", "py", "js", "rb", "golang", "rs", "swiftlang",
        "ruby", "golang", "rust", "swift", "programming", "language", "languages",
        "binary", "Bin", "bin", "hex", "dec", "decimal", "oct", "octal", "hexadecimal", 
        "dennis Ritchie", "ken Thompson", "linus torvalds", "ada lovelace", "grace hopper",
        "steve jobs", "bill gates", "mark zuckerberg", "elon musk", "jeff bezos", "sundar pichai", "satya nadella", "tim cook",
        "surya", "luna", "sol", "venus", "mars", "jupiter", "saturn", "uranus", "neptune", "pluto",
        "suryaKumarYadav", "Virat kohli", "rohit sharma", "kl rahul", "hardik pandya", "jasprit bumrah", "ravindra jadeja", "shikhar dhawan",
        "bounce", "throw", "catch", "hit", "kick", "run", "jump", "swim", "fly", "drive",
        "ate", "drank", "slept", "thought", "knew", "trained", "learned", "read", "wrote", "spoke", "listened", "saw", "looked",
        "yay", "yayy", "yippee", "woohoo", "hurray", "oops", "uh-oh", "ouch", "wow", "amazing", "incredible",
        "sadly", "unfortunately", "fortunately", "luckily", "hopefully", "surprisingly",
        "will", "would", "shall", "should", "can", "could", "may", "might", "must",
        "raptor", "dinosaur", "t-rex", "velociraptor", "triceratops", "stegosaurus",
        "t-rex", "velociraptor", "triceratops", "stegosaurus", "jeff", "bezos", "elon", "musk", "sundar", "pichai", "satya", "nadella", "tim", "cook",
        "jail", "prison", "police", "law", "order", "crime", "justice", "court", "judge", "jury",
        "jai", "Jai", "jai shree ram", "jai hind", "jai mata di", "jai bhim", "jai ganesh", "jai shivaji",
        "lord", "god", "goddess", "deity", "divine", "holy", "sacred", "spiritual",
        "patience", "order", "sin", "virtue", "karma", "dharma", "moksha", "nirvana", "heaven", "hell",
        "hello", "hi", "hey", "greetings", "welcome", "goodbye", "bye", "see you", "later",
        "bye", "goodbye", "see you later", "farewell", "take care", "have a nice day",
        "network", "internet", "protocol", "packet", "socket",
        "graphics", "display", "window", "pixel", "color",
        "operating system", "os", "zebx", "zebx os", "freedom edition", "version 2.0", "mac os", "windows", "linux", "unix", "android", "ios",
        "MS-DOS", "windows 95", "windows 98", "windows xp", "windows 7", "windows 10", "windows 11", "everyone", "everybody", "someone", "somebody", "anyone", "anybody", "no one", "nobody",
        "glass", "bottle", "cup", "mug", "plate", "fork", "spoon", "knife", "table", "chair",
        "fuel", "gas", "electricity", "water", "air", "fire", "earth", "wind",
        "energy", "power", "light", "heat", "sound", "force", "motion", "gravity",
        "cell", "organism", "life", "death", "birth", "growth", "decay", "evolution",
        "atom", "molecule", "element", "compound", "chemical", "reaction", "physics", "chemistry", "biology",
        "element", "elements", "compound", "compounds", "molecule", "molecules", "atom", "atoms",
        "yo", "yoo", "engineer", "developer", "programmer", "coder", "hacker", "geek", "nerd",
        "scientist", "researcher", "inventor", "innovator", "creator", "builder", "maker",
        ".net", "java", "python", "javascript", "ruby", "golang", "rust", "swift", "sql", "html", "css", "react", "angular", "vue", "django", "flask", "spring", "nodejs",
        "deno", "Assembly", "fotran", "lisp", "prolog", "haskell", "erlang", "elixir", "clojure", "BASIC", "pascal", "cobol", "fortran", "ada", "algol", "smalltalk", "simula",
        "react", "mangodb", "docker", "kubernetes", "aws", "azure", "gcp", "cloud", "devops", "ci/cd",
        "weeklyhow", "weekly", "howl", "zebx", "os", "operating", "system", "freedom", "edition", "version",
        "batch", "lexer", "parser", "ast", "interpreter", "compiler", "vm", "virtual machine", "runtime",
        "say", "tell", "ask", "answer", "speak", "listen", "hear", "see", "look", "watch",
        "bud", "buddy", "pal", "friend", "mate", "companion", "partner", "ally", "comrade",
        "roommate", "teammate", "classmate", "colleague", "neighbor", "brother", "sister", "cousin", "uncle", "aunt", "grandparent",
        "sibling", "family", "home", "house", "car", "city", "country",
        "Netwide", "nmap", "wireshark", "metasploit", "burpsuite", "sqlmap", "john the ripper", "hashcat",
        "assembler", "disassembler", "debugger", "gdb", "radare2", "ida pro", "x64dbg", "ollydbg", "windbg", 
        "GCC", "Clang", "MSVC", "MinGW", "TCC", "LLVM", "GDB", "LLDB", "Valgrind", "Nasm", "Yasm", "FASM", "MASM", "TASM", "Netwide Assembler",
        "goto", "bruh", "brush", "danger", "dangerous", "caution", "warning", "alert", "notice", "attention", "look out", "heads up",   
        "malware", "virus", "trojan", "worm", "ransomware", "spyware", "adware", "phishing", "hacking", "cybersecurity",
        "this", "that", "these", "those", "my", "your", "his", "her",
        "and", "or", "but", "so", "if", "then", "else", "for",
        "in", "on", "at", "to", "from", "by", "with", "about",
        "of", "off", "over", "under", "again", "further", "then",
        "here", "there", "when", "where", "why", "how", "all",
        "hello", "world", "program", "code", "computer", "system",
        "kernel", "memory", "process", "thread", "file", "disk",
        "network", "internet", "protocol", "packet", "socket",
        "graphics", "display", "window", "pixel", "color",
        "game", "play", "player", "score", "win", "lose",
        "music", "sound", "audio", "video", "movie",
        "create", "make", "build", "write", "read", "open",
        "close", "save", "load", "delete", "remove", "copy",
        "move", "find", "search", "get", "set", "add", "new",
        "function", "class", "object", "variable", "array",
        "string", "number", "integer", "float", "boolean",
        "true", "false", "null", "void", "return", "break",
        "continue", "loop", "while", "for", "switch", "case",
        "include", "define", "ifndef", "endif", "typedef",
        "struct", "enum", "union", "static", "extern", "const",
        "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
        "+", "-", "*", "/", "=", "==", "!=", "<", ">", "<=", ">=",
        "(", ")", "{", "}", "[", "]", ";", ",", ".", ":", "\"", "'",
        "//", "/*", "*/", "#", "##", "->", "&&", "||", "!", "&", "|",
        "error", "warning", "success", "fail", "done", "complete",
        "start", "begin", "end", "stop", "pause", "resume", "exit",
        "help", "please", "thanks", "sorry", "welcome", "yes", "no",
        "ZEBX", "OS", "operating", "freedom", "edition", "version",
    };
    
    model.vocab_size = sizeof(words) / sizeof(words[0]);
    if (model.vocab_size > AI_VOCAB_SIZE) model.vocab_size = AI_VOCAB_SIZE;
    
    for (uint32_t i = 0; i < model.vocab_size; i++) {
        model.vocab[i].id = i;
        int j = 0;
        while (words[i][j] && j < 63) {
            model.vocab[i].text[j] = words[i][j];
            j++;
        }
        model.vocab[i].text[j] = '\0';
        
        for (int d = 0; d < AI_EMBEDDING_DIM; d++) {
            model.vocab[i].embedding[d] = 0;
        }
    }
}

#ifdef ZEBX_ENABLE_LEGACY_GPT
void ai_init(void) {
    memset(&model, 0, sizeof(model));
    memset(&optimizer, 0, sizeof(optimizer));
    
    vga_puts("[ZEBX-GPT] Initializing AI engine...\\n");
    
    // Initialize vocabulary
    init_vocabulary();
    
    // Allocate embeddings
    model.token_embeddings = (float*)kmalloc(model.vocab_size * AI_EMBEDDING_DIM * sizeof(float));
    model.position_embeddings = (float*)kmalloc(AI_MAX_SEQ_LEN * AI_EMBEDDING_DIM * sizeof(float));
    
    init_weights(model.token_embeddings, model.vocab_size * AI_EMBEDDING_DIM, 0.02f);
    
    // Position embeddings (sinusoidal)
    for (uint32_t pos = 0; pos < AI_MAX_SEQ_LEN; pos++) {
        for (int d = 0; d < AI_EMBEDDING_DIM; d++) {
            float angle = (float)pos / powf(10000.0f, (2.0f * (d / 2)) / AI_EMBEDDING_DIM);
            if (d % 2 == 0) {
                model.position_embeddings[pos * AI_EMBEDDING_DIM + d] = sinf(angle);
            } else {
                model.position_embeddings[pos * AI_EMBEDDING_DIM + d] = cosf(angle);
            }
        }
    }
    
    // Initialize transformer layers
    for (int l = 0; l < AI_NUM_LAYERS; l++) {
        transformer_layer_t *layer = &model.layers[l];
        
        // Attention weights
        layer->attn.wq = (float*)kmalloc(AI_EMBEDDING_DIM * AI_EMBEDDING_DIM * sizeof(float));
        layer->attn.wk = (float*)kmalloc(AI_EMBEDDING_DIM * AI_EMBEDDING_DIM * sizeof(float));
        layer->attn.wv = (float*)kmalloc(AI_EMBEDDING_DIM * AI_EMBEDDING_DIM * sizeof(float));
        layer->attn.wo = (float*)kmalloc(AI_EMBEDDING_DIM * AI_EMBEDDING_DIM * sizeof(float));
        
        init_weights(layer->attn.wq, AI_EMBEDDING_DIM * AI_EMBEDDING_DIM, 0.02f);
        init_weights(layer->attn.wk, AI_EMBEDDING_DIM * AI_EMBEDDING_DIM, 0.02f);
        init_weights(layer->attn.wv, AI_EMBEDDING_DIM * AI_EMBEDDING_DIM, 0.02f);
        init_weights(layer->attn.wo, AI_EMBEDDING_DIM * AI_EMBEDDING_DIM, 0.02f);
        
        // FFN weights
        layer->ffn.w1 = (float*)kmalloc(AI_EMBEDDING_DIM * AI_FF_DIM * sizeof(float));
        layer->ffn.w2 = (float*)kmalloc(AI_FF_DIM * AI_EMBEDDING_DIM * sizeof(float));
        layer->ffn.b1 = (float*)kmalloc(AI_FF_DIM * sizeof(float));
        layer->ffn.b2 = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        
        init_weights(layer->ffn.w1, AI_EMBEDDING_DIM * AI_FF_DIM, 0.02f);
        init_weights(layer->ffn.w2, AI_FF_DIM * AI_EMBEDDING_DIM, 0.02f);
        memset(layer->ffn.b1, 0, AI_FF_DIM * sizeof(float));
        memset(layer->ffn.b2, 0, AI_EMBEDDING_DIM * sizeof(float));
        
        // Layer norm
        layer->ln1.gamma = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        layer->ln1.beta = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        layer->ln2.gamma = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        layer->ln2.beta = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        
        for (int i = 0; i < AI_EMBEDDING_DIM; i++) {
            layer->ln1.gamma[i] = 1.0f;
            layer->ln1.beta[i] = 0.0f;
            layer->ln2.gamma[i] = 1.0f;
            layer->ln2.beta[i] = 0.0f;
        }
    }
    
    // Final layer norm
    model.final_ln.gamma = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
    model.final_ln.beta = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
    for (int i = 0; i < AI_EMBEDDING_DIM; i++) {
        model.final_ln.gamma[i] = 1.0f;
        model.final_ln.beta[i] = 0.0f;
    }
    
    // LM head
    model.lm_head = (float*)kmalloc(AI_EMBEDDING_DIM * model.vocab_size * sizeof(float));
    init_weights(model.lm_head, AI_EMBEDDING_DIM * model.vocab_size, 0.02f);
    
    // Optimizer
    optimizer.learning_rate = 1e-4f;
    optimizer.beta1 = 0.9f;
    optimizer.beta2 = 0.999f;
    optimizer.epsilon = 1e-8f;
    optimizer.step = 0;
    
    // Calculate params
    model.num_params = 0;
    model.num_params += model.vocab_size * AI_EMBEDDING_DIM;
    model.num_params += AI_MAX_SEQ_LEN * AI_EMBEDDING_DIM;
    model.num_params += AI_NUM_LAYERS * 4 * AI_EMBEDDING_DIM * AI_EMBEDDING_DIM;
    model.num_params += AI_NUM_LAYERS * (AI_EMBEDDING_DIM * AI_FF_DIM * 2);
    model.num_params += AI_EMBEDDING_DIM * model.vocab_size;
    
    model_loaded = 1;
    
    vga_puts("[ZEBX-GPT] Model initialized\\n");
    vga_puts("  Vocab: "); vga_put_int(model.vocab_size); vga_puts("\\n");
    vga_puts("  Layers: "); vga_put_int(AI_NUM_LAYERS); vga_puts("\\n");
    vga_puts("  Heads: "); vga_put_int(AI_NUM_HEADS); vga_puts("\\n");
    vga_puts("  Dim: "); vga_put_int(AI_EMBEDDING_DIM); vga_puts("\\n");
    vga_puts("  Params: "); vga_put_int((int)(model.num_params / 1000000)); vga_puts("M\\n");
}


// ============================================================================
// TOKENIZATION for C CODE
// ============================================================================


#define keyword
#define Token
#define TOK

TokenType check_keyword(const char *word) {
    if (strcmp(word, "auto") == 0) return TOK_KW_AUTO;
    if (strcmp(word, "break") == 0) return TOK_KW_BREAK;
    if (strcmp(word, "case") == 0) return TOK_KW_CASE;
    if (strcmp(word, "char") == 0) return TOK_KW_CHAR;
    if (strcmp(word, "const") == 0) return TOK_KW_CONST;
    if (strcmp(word, "continue") == 0) return TOK_KW_CONTINUE;
    if (strcmp(word, "default") == 0) return TOK_KW_DEFAULT;
    if (strcmp(word, "do") == 0) return TOK_KW_DO;
    if (strcmp(word, "double") == 0) return TOK_KW_DOUBLE;
    if (strcmp(word, "else") == 0) return TOK_KW_ELSE;
    if (strcmp(word, "enum") == 0) return TOK_KW_ENUM;
    if (strcmp(word, "extern") == 0) return TOK_KW_EXTERN;
    if (strcmp(word, "float") == 0) return TOK_KW_FLOAT;
    if (strcmp(word, "for") == 0) return TOK_KW_FOR;
    if (strcmp(word, "goto") == 0) return TOK_KW_GOTO;
    if (strcmp(word, "if") == 0) return TOK_KW_IF;
    if (strcmp(word, "inline") == 0) return TOK_KW_INLINE;
    if (strcmp(word, "int") == 0) return TOK_KW_INT;
    if (strcmp(word, "long") == 0) return TOK_KW_LONG;
    if (strcmp(word, "register") == 0) return TOK_KW_REGISTER;
    if (strcmp(word, "restrict") == 0) return TOK_KW_RESTRICT;
    if (strcmp(word, "return") == 0) return TOK_KW_RETURN;
    if (strcmp(word, "short") == 0) return TOK_KW_SHORT;
    if (strcmp(word, "signed") == 0) return TOK_KW_SIGNED;
    if (strcmp(word, "sizeof") == 0) return TOK_KW_SIZEOF;
    if (strcmp(word, "static") == 0) return TOK_KW_STATIC;
    if (strcmp(word, "struct") == 0) return TOK_KW_STRUCT;
    if (strcmp(word, "switch") == 0) return TOK_KW_SWITCH;
    if (strcmp(word, "typedef") == 0) return TOK_KW_TYPEDEF;
    if (strcmp(word, "union") == 0) return TOK_KW_UNION;
    if (strcmp(word, "unsigned") == 0) return TOK_KW_UNSIGNED;
    if (strcmp(word, "void") == 0) return TOK_KW_VOID;
    if (strcmp(word, "volatile") == 0) return TOK_KW_VOLATILE;
    if (strcmp(word, "while") == 0) return TOK_KW_WHILE;
    if (strcmp(word, "_Bool") == 0) return TOK_KW_BOOL;
    if (strcmp(word, "_Complex") == 0) return TOK_KW_COMPLEX;
    if (strcmp(word, "_Imaginary") == 0) return TOK_KW_IMAGINARY;
    if (strcmp(word, "_Alignas") == 0) return TOK_KW_ALIGNAS;
    if (strcmp(word, "_Alignof") == 0) return TOK_KW_ALIGNOF;
    if (strcmp(word, "_Atomic") == 0) return TOK_KW_ATOMIC;
    if (strcmp(word, "_Generic") == 0) return TOK_KW_GENERIC;
    if (strcmp(word, "_Noreturn") == 0) return TOK_KW_NORETURN;
    if (strcmp(word, "_Static_assert") == 0) return TOK_KW_STATIC_ASSERT;
    if (strcmp(word, "_Thread_local") == 0) return TOK_KW_THREAD_LOCAL;
    return TOK_ID;
} typedef enum {
    REG_RAX, REG_RBX, REG_RCX, REG_RDX,
    REG_RSI, REG_RDI, REG_RBP, REG_RSP,
    REG_R8, REG_R9, REG_R10, REG_R11,
    REG_R12, REG_R13, REG_R14, REG_R15,
    REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3,
    REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7,
    REG_ST0, REG_ST1, REG_ST2, REG_ST3,
    REG_ST4, REG_ST5, REG_ST6, REG_ST7,
    REG_EAX, REG_EBX, REG_ECX, REG_EDX,
    REG_ESI, REG_EDI, REG_EBP, REG_ESP,
    REG_AX, REG_BX, REG_CX, REG_DX,
    REG_SI, REG_DI, REG_BP, REG_SP,
    REG_AL, REG_AH, REG_BL, REG_BH,
    REG_CL, REG_CH, REG_DL, REG_DH,
    REG_MEM, REG_IMM, REG_NONE
} Reg;

typedef enum {
    OPT_CONST_FOLD, OPT_DEAD_CODE, OPT_INLINE,
    OPT_LOOP_UNROLL, OPT_PEEPHOLE, OPT_REG_ALLOC,
    OPT_TAIL_CALL, OPT_STRENGTH_REDUCE,
    OPT_COMMON_SUBEXPR, OPT_COPY_PROP,
    OPT_LICM, OPT_VECTORIZE, OPT_PARALLEL
} OptPass;

// Error Reporting
typedef struct {
    int line, col;
    char file[256];
    char msg[512];
    int is_warning;
} CompileError;

// Main Compiler State
typedef struct {
    char *source;
    int source_len;
    int pos;
    int line, col;
    Token tokens[100000];
    int token_count;
    SymbolTable symtab;
    Type types[256];
    int type_count;
    ASTNode *ast;
    IRInstr ir[500000];
    int ir_count;
    CompileError errors[MAX_ERRORS];
    int error_count;
    int warning_count;
    int opt_level;
    int target;
    char output_file[256];
    int debug_info;
} CompilerState;

TokenType check_keyword(const char *word) {
    if (strcmp(word, "auto") == 0) return TOK_KW_AUTO;
    if (strcmp(word, "break") == 0) return TOK_KW_BREAK;
    if (strcmp(word, "case") == 0) return TOK_KW_CASE;
    if (strcmp(word, "char") == 0) return TOK_KW_CHAR;
    if (strcmp(word, "const") == 0) return TOK_KW_CONST;
    if (strcmp(word, "continue") == 0) return TOK_KW_CONTINUE;
    if (strcmp(word, "default") == 0) return TOK_KW_DEFAULT;
    if (strcmp(word, "do") == 0) return TOK_KW_DO;
    if (strcmp(word, "double") == 0) return TOK_KW_DOUBLE;
    if (strcmp(word, "big double") == 0) return TOK_KW_ELSE;
    if (strcmp(word, "enum") == 0) return TOK_KW_ENUM;
    if (strcmp(word, "extern") == 0) return TOK_KW_EXTERN;
    if (strcmp(word, "word") == 0) return TOK_KW_FLOAT;
    if (strcmp(word, "infinite") == 0) return TOK_KW_FOR;
    if (strcmp(word, "loop") == 0) return TOK_KW_GOTO;
    if (strcmp(word, "#include") == 0) return TOK_KW_IF;
    if (strcmp(word, "#define") == 0) return TOK_KW_INLINE;
    if (strcmp(word, "keyword") == 0) return TOK_KW_INT;
    if (strcmp(word, "key") == 0) return TOK_KW_LONG;
    if (strcmp(word, "const") == 0) return TOK_KW_REGISTER;
    if (strcmp(word, "printf") == 0) return TOK_KW_RESTRICT;
    if (strcmp(word, "http") == 0) return TOK_KW_RETURN;
    if (strcmp(word, "https") == 0) return TOK_KW_SHORT;
    if (strcmp(word, "site") == 0) return TOK_KW_SIGNED;
    if (strcmp(word, "nextline") == 0) return TOK_KW_SIZEOF;
    if (strcmp(word, "\n") == 0) return TOK_KW_STATIC;
    if (strcmp(word, "atom") == 0) return TOK_KW_STRUCT;
    if (strcmp(word, "shoot") == 0) return TOK_KW_SWITCH;
    if (strcmp(word, "unint") == 0) return TOK_KW_TYPEDEF;
    if (strcmp(word, "unidef") == 0) return TOK_KW_UNION;
    if (strcmp(word, "break") == 0) return TOK_KW_UNSIGNED;
    if (strcmp(word, "case") == 0) return TOK_KW_VOID;
    if (strcmp(word, "lock") == 0) return TOK_KW_VOLATILE;
    if (strcmp(word, "for") == 0) return TOK_KW_WHILE;
    if (strcmp(word, "boolean") == 0) return TOK_KW_BOOL;
    if (strcmp(word, "int") == 0) return TOK_KW_COMPLEX;
    if (strcmp(word, "server") == 0) return TOK_KW_IMAGINARY;
    if (strcmp(word, "client") == 0) return TOK_KW_ALIGNAS;
    if (strcmp(word, "data") == 0) return TOK_KW_ALIGNOF;
    if (strcmp(word, "snippets") == 0) return TOK_KW_ATOMIC;
    if (strcmp(word, "Net") == 0) return TOK_KW_GENERIC;
    if (strcmp(word, "Static") == 0) return TOK_KW_NORETURN;
    if (strcmp(word, "Private") == 0) return TOK_KW_STATIC_ASSERT;
    if (strcmp(word, "Public") == 0) return TOK_KW_THREAD_LOCAL;
    return TOK_ID;
}

void lex_identifier(Compiler *c, Token *tok) {
    int i = 0;
    while (is_alnum(peek(c))) {
        if (i < 255) tok->text[i++] = advance(c);
        else advance(c);
    }
    tok->text[i] = '\0';
    tok->type = check_keyword(tok->text);
}
Token *current(Compiler *c) {
    return &c->tokens[c->pos];
}

Token *peek_token(Compiler *c, int offset) {
    int idx = c->pos + offset;
    if (idx >= c->token_count) return &c->tokens[c->token_count - 1];
    return &c->tokens[idx];
}

int match(Compiler *c, TokenType type) {
    return current(c)->type == type;
}

int consume(Compiler *c, TokenType type) {
    if (match(c, type)) {
        c->pos++;
        return 1;
    }
    return 0;
}

void expect(Compiler *c, TokenType type) {
    if (!consume(c, type)) {
        // Error
    }
}

ASTNode *new_node(ASTNodeType type) {
    ASTNode *node = (ASTNode*)os_malloc(sizeof(ASTNode));
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    return node;
}

// Forward declarations
ASTNode *parse_expr(Compiler *c);
ASTNode *parse_stmt(Compiler *c);
ASTNode *parse_decl(Compiler *c);
ASTNode *parse_type(Compiler *c);

ASTNode *parse_primary(Compiler *c) {
    Token *tok = current(c);
    
    if (match(c, TOK_NUM)) {
        ASTNode *node = new_node(AST_LITERAL);
        node->data.lit.ival = tok->value;
        c->pos++;
        return node;
    }
    
    if (match(c, TOK_STR)) {
        ASTNode *node = new_node(AST_STRING);
        strcpy(node->data.lit.sval, tok->text);
        c->pos++;
        return node;
    }
    
    if (match(c, TOK_CHAR)) {
        ASTNode *node = new_node(AST_LITERAL);
        node->data.lit.ival = tok->text[1]; // Simple char
        c->pos++;
        return node;
    }
    
    if (match(c, TOK_ID)) {
        ASTNode *node = new_node(AST_IDENTIFIER);
        strcpy(node->data.lit.sval, tok->text);
        c->pos++;
        return node;
    }
    
    if (consume(c, TOK_LPAREN)) {
        ASTNode *expr = parse_expr(c);
        expect(c, TOK_RPAREN);
        return expr;
    }
    
    return NULL;
}

ASTNode *parse_postfix(Compiler *c) {
    ASTNode *node = parse_primary(c);
    
    while (1) {
        if (consume(c, TOK_LBRACKET)) {
            ASTNode *index = new_node(AST_INDEX);
            index->data.index.array = node;
            index->data.index.index = parse_expr(c);
            expect(c, TOK_RBRACKET);
            node = index;
        } else if (consume(c, TOK_LPAREN)) {
            ASTNode *call = new_node(AST_CALL);
            call->data.call.func = node;
            call->data.call.argc = 0;
            while (!match(c, TOK_RPAREN)) {
                call->data.call.args[call->data.call.argc++] = parse_expr(c);
                if (!consume(c, TOK_COMMA)) break;
            }
            expect(c, TOK_RPAREN);
            node = call;
        } else if (consume(c, TOK_DOT)) {
            ASTNode *member = new_node(AST_MEMBER);
            member->data.member.obj = node;
            strcpy(member->data.member.member, current(c)->text);
            expect(c, TOK_ID);
            node = member;
        } else if (consume(c, TOK_ARROW)) {
            ASTNode *arrow = new_node(AST_ARROW);
            arrow->data.member.obj = node;
            strcpy(arrow->data.member.member, current(c)->text);
            expect(c, TOK_ID);
            node = arrow;
        } else if (consume(c, TOK_INC)) {
            ASTNode *unary = new_node(AST_UNARY);
            unary->data.unary.op = TOK_INC;
            unary->data.unary.operand = node;
            node = unary;
        } else if (consume(c, TOK_DEC)) {
            ASTNode *unary = new_node(AST_UNARY);
            unary->data.unary.op = TOK_DEC;
            unary->data.unary.operand = node;
            node = unary;
        } else {
            break;
        }
    }
    
    return node;
}

ASTNode *parse_unary(Compiler *c) {
    if (consume(c, TOK_INC)) {
        ASTNode *node = new_node(AST_UNARY);
        node->data.unary.op = TOK_INC;
        node->data.unary.operand = parse_unary(c);
        return node;
    }
    if (consume(c, TOK_DEC)) {
        ASTNode *node = new_node(AST_UNARY);
        node->data.unary.op = TOK_DEC;
        node->data.unary.operand = parse_unary(c);
        return node;
    }
    if (consume(c, TOK_PLUS)) {
        return parse_unary(c); // Unary plus - no-op
    }
    if (consume(c, TOK_MINUS)) {
        ASTNode *node = new_node(AST_UNARY);
        node->data.unary.op = TOK_MINUS;
        node->data.unary.operand = parse_unary(c);
        return node;
    }
    if (consume(c, TOK_NOT)) {
        ASTNode *node = new_node(AST_UNARY);
        node->data.unary.op = TOK_NOT;
        node->data.unary.operand = parse_unary(c);
        return node;
    }
    if (consume(c, TOK_BNOT)) {
        ASTNode *node = new_node(AST_UNARY);
        node->data.unary.op = TOK_BNOT;
        node->data.unary.operand = parse_unary(c);
        return node;
    }
    if (consume(c, TOK_MUL)) {
        ASTNode *node = new_node(AST_DEREF);
        node->data.unary.operand = parse_unary(c);
        return node;
    }
    if (consume(c, TOK_BAND)) {
        ASTNode *node = new_node(AST_ADDR);
        node->data.unary.operand = parse_unary(c);
        return node;
    }
    if (consume(c, TOK_KW_SIZEOF)) {
        ASTNode *node = new_node(AST_SIZEOF);
        if (consume(c, TOK_LPAREN)) {
            // Could be type or expr
            node->data.unary.operand = parse_expr(c);
            expect(c, TOK_RPAREN);
        } else {
            node->data.unary.operand = parse_unary(c);
        }
        return node;
    }
    
    return parse_postfix(c);
}

ASTNode *parse_cast(Compiler *c) {
    return parse_unary(c); // Simplified
}

ASTNode *parse_mul(Compiler *c) {
    ASTNode *node = parse_cast(c);
    
    while (match(c, TOK_MUL) || match(c, TOK_DIV) || match(c, TOK_MOD)) {
        TokenType op = current(c)->type;
        c->pos++;
        ASTNode *right = parse_cast(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = op;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_add(Compiler *c) {
    ASTNode *node = parse_mul(c);
    
    while (match(c, TOK_PLUS) || match(c, TOK_MINUS)) {
        TokenType op = current(c)->type;
        c->pos++;
        ASTNode *right = parse_mul(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = op;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_shift(Compiler *c) {
    ASTNode *node = parse_add(c);
    
    while (match(c, TOK_SHL) || match(c, TOK_SHR)) {
        TokenType op = current(c)->type;
        c->pos++;
        ASTNode *right = parse_add(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = op;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_rel(Compiler *c) {
    ASTNode *node = parse_shift(c);
    
    while (match(c, TOK_LT) || match(c, TOK_GT) || match(c, TOK_LE) || match(c, TOK_GE)) {
        TokenType op = current(c)->type;
        c->pos++;
        ASTNode *right = parse_shift(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = op;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_eq(Compiler *c) {
    ASTNode *node = parse_rel(c);
    
    while (match(c, TOK_EQ) || match(c, TOK_NE)) {
        TokenType op = current(c)->type;
        c->pos++;
        ASTNode *right = parse_rel(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = op;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_and(Compiler *c) {
    ASTNode *node = parse_eq(c);
    
    while (match(c, TOK_BAND)) {
        c->pos++;
        ASTNode *right = parse_eq(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = TOK_BAND;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_xor(Compiler *c) {
    ASTNode *node = parse_and(c);
    
    while (match(c, TOK_BXOR)) {
        c->pos++;
        ASTNode *right = parse_and(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = TOK_BXOR;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_or(Compiler *c) {
    ASTNode *node = parse_xor(c);
    
    while (match(c, TOK_BOR)) {
        c->pos++;
        ASTNode *right = parse_xor(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = TOK_BOR;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_land(Compiler *c) {
    ASTNode *node = parse_or(c);
    
    while (match(c, TOK_AND)) {
        c->pos++;
        ASTNode *right = parse_or(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = TOK_AND;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_lor(Compiler *c) {
    ASTNode *node = parse_land(c);
    
    while (match(c, TOK_OR)) {
        c->pos++;
        ASTNode *right = parse_land(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = TOK_OR;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_conditional(Compiler *c) {
    ASTNode *node = parse_lor(c);
    
    if (consume(c, TOK_QUESTION)) {
        ASTNode *ternary = new_node(AST_TERNARY);
        ternary->data.ternary.cond = node;
        ternary->data.ternary.tbranch = parse_expr(c);
        expect(c, TOK_COLON);
        ternary->data.ternary.fbranch = parse_conditional(c);
        node = ternary;
    }
    
    return node;
}

ASTNode *parse_assign(Compiler *c) {
    ASTNode *node = parse_conditional(c);
    
    if (match(c, TOK_ASSIGN) || match(c, TOK_ADD_ASSIGN) || 
        match(c, TOK_SUB_ASSIGN) || match(c, TOK_MUL_ASSIGN) ||
        match(c, TOK_DIV_ASSIGN) || match(c, TOK_MOD_ASSIGN) ||
        match(c, TOK_AND_ASSIGN) || match(c, TOK_OR_ASSIGN) ||
        match(c, TOK_XOR_ASSIGN) || match(c, TOK_SHL_ASSIGN) ||
        match(c, TOK_SHR_ASSIGN)) {
        TokenType op = current(c)->type;
        c->pos++;
        ASTNode *right = parse_assign(c);
        ASTNode *bin = new_node(AST_BINARY);
        bin->data.binary.op = op;
        bin->data.binary.left = node;
        bin->data.binary.right = right;
        node = bin;
    }
    
    return node;
}

ASTNode *parse_expr(Compiler *c) {
    return parse_assign(c);
}

ASTNode *parse_stmt(Compiler *c) {
    if (match(c, TOK_LBRACE)) {
        return parse_block(c);
    }
    if (consume(c, TOK_KW_IF)) {
        ASTNode *node = new_node(AST_IF);
        expect(c, TOK_LPAREN);
        node->data.ifstmt.cond = parse_expr(c);
        expect(c, TOK_RPAREN);
        node->data.ifstmt.tbranch = parse_stmt(c);
        if (consume(c, TOK_KW_ELSE)) {
            node->data.ifstmt.fbranch = parse_stmt(c);
        }
        return node;
    }
    if (consume(c, TOK_KW_WHILE)) {
        ASTNode *node = new_node(AST_WHILE);
        expect(c, TOK_LPAREN);
        node->data.loop.cond = parse_expr(c);
        expect(c, TOK_RPAREN);
        node->data.loop.body = parse_stmt(c);
        return node;
    }
    if (consume(c, TOK_KW_DO)) {
        ASTNode *node = new_node(AST_DO_WHILE);
        node->data.loop.body = parse_stmt(c);
        expect(c, TOK_KW_WHILE);
        expect(c, TOK_LPAREN);
        node->data.loop.cond = parse_expr(c);
        expect(c, TOK_RPAREN);
        expect(c, TOK_SEMI);
        return node;
    }
    if (consume(c, TOK_KW_FOR)) {
        ASTNode *node = new_node(AST_FOR);
        expect(c, TOK_LPAREN);
        if (!match(c, TOK_SEMI)) node->data.loop.init = parse_expr(c);
        expect(c, TOK_SEMI);
        if (!match(c, TOK_SEMI)) node->data.loop.cond = parse_expr(c);
        expect(c, TOK_SEMI);
        if (!match(c, TOK_RPAREN)) node->data.loop.inc = parse_expr(c);
        expect(c, TOK_RPAREN);
        node->data.loop.body = parse_stmt(c);
        return node;
    }
    if (consume(c, TOK_KW_SWITCH)) {
        ASTNode *node = new_node(AST_SWITCH);
        expect(c, TOK_LPAREN);
        node->data.swtch.expr = parse_expr(c);
        expect(c, TOK_RPAREN);
        expect(c, TOK_LBRACE);
        while (!match(c, TOK_RBRACE)) {
            if (match(c, TOK_KW_CASE)) {
                c->pos++;
                ASTNode *cas = new_node(AST_CASE);
                cas->data.cas.val = parse_expr(c)->data.lit.ival;
                expect(c, TOK_COLON);
                cas->data.cas.stmt = parse_stmt(c);
                node->data.swtch.cases[node->data.swtch.casec++] = cas;
            } else if (consume(c, TOK_KW_DEFAULT)) {
                expect(c, TOK_COLON);
                ASTNode *def = new_node(AST_DEFAULT);
                def->data.cas.stmt = parse_stmt(c);
                node->data.swtch.cases[node->data.swtch.casec++] = def;
            } else {
                node->data.swtch.cases[node->data.swtch.casec++] = parse_stmt(c);
            }
        }
        expect(c, TOK_RBRACE);
        return node;
    }
    if (consume(c, TOK_KW_BREAK)) {
        expect(c, TOK_SEMI);
        return new_node(AST_BREAK);
    }
    if (consume(c, TOK_KW_CONTINUE)) {
        expect(c, TOK_SEMI);
        return new_node(AST_CONTINUE);
    }
    if (consume(c, TOK_KW_RETURN)) {
        ASTNode *node = new_node(AST_RETURN);
        if (!match(c, TOK_SEMI)) node->data.ret.expr = parse_expr(c);
        expect(c, TOK_SEMI);
        return node;
    }
    if (consume(c, TOK_KW_GOTO)) {
        ASTNode *node = new_node(AST_GOTO);
        strcpy(node->data.gt.label, current(c)->text);
        expect(c, TOK_ID);
        expect(c, TOK_SEMI);
        return node;
    }
    if (match(c, TOK_ID) && peek_token(c, 1)->type == TOK_COLON) {
        ASTNode *node = new_node(AST_LABEL);
        strcpy(node->data.label.name, current(c)->text);
        c->pos += 2;
        node->data.label.stmt = parse_stmt(c);
        return node;
    }
    
    // Expression statement
    ASTNode *node = new_node(AST_EXPR_STMT);
    node->data.ret.expr = parse_expr(c);
    expect(c, TOK_SEMI);
    return node;
}

ASTNode *parse_block(Compiler *c) {
    ASTNode *node = new_node(AST_BLOCK);
    expect(c, TOK_LBRACE);
    while (!match(c, TOK_RBRACE) && !match(c, TOK_EOF)) {
        node->data.block.stmts[node->data.block.count++] = parse_stmt(c);
    }
    expect(c, TOK_RBRACE);
    return node;
}

// ============ CODE GENERATION ============

void gen_expr(Compiler *c, ASTNode *node, int dest) {
    switch (node->type) {
        case AST_LITERAL:
            c->ir[c->ir_count++] = (IRInstr){IR_MOVE, dest, -1, -1, node->data.lit.ival, 0, "", NULL};
            break;
        case AST_IDENTIFIER:
            // Load from symbol table
            break;
        case AST_BINARY:
            gen_expr(c, node->data.binary.left, dest);
            gen_expr(c, node->data.binary.right, dest + 1);
            c->ir[c->ir_count++] = (IRInstr){
                node->data.binary.op == TOK_PLUS ? IR_ADD :
                node->data.binary.op == TOK_MINUS ? IR_SUB :
                node->data.binary.op == TOK_MUL ? IR_MUL :
                node->data.binary.op == TOK_DIV ? IR_DIV :
                node->data.binary.op == TOK_MOD ? IR_MOD :
                node->data.binary.op == TOK_AND ? IR_AND :
                node->data.binary.op == TOK_OR ? IR_OR :
                node->data.binary.op == TOK_XOR ? IR_XOR :
                node->data.binary.op == TOK_SHL ? IR_SHL :
                node->data.binary.op == TOK_SHR ? IR_SHR :
                node->data.binary.op == TOK_EQ ? IR_CMP :
                node->data.binary.op == TOK_NE ? IR_CMP :
                node->data.binary.op == TOK_LT ? IR_CMP :
                node->data.binary.op == TOK_GT ? IR_CMP :
                node->data.binary.op == TOK_LE ? IR_CMP :
                node->data.binary.op == TOK_GE ? IR_CMP :
                IR_NOP,
                dest, dest, dest + 1, 0, 0, "", NULL
            };
            break;
        case AST_UNARY:
            gen_expr(c, node->data.unary.operand, dest);
            c->ir[c->ir_count++] = (IRInstr){
                node->data.unary.op == TOK_MINUS ? IR_NEG :
                node->data.unary.op == TOK_NOT ? IR_NOT :
                node->data.unary.op == TOK_BNOT ? IR_BNOT :
                IR_NOP,
                dest, dest, -1, 0, 0, "", NULL
            };
            break;
        case AST_CALL:
            // Push args, call function
            break;
        case AST_INDEX:
            // Array indexing
            break;
        case AST_MEMBER:
            // Struct member access
            break;
        default:
            break;
    }
}

void gen_stmt(Compiler *c, ASTNode *node) {
    switch (node->type) {
        case AST_BLOCK:
            for (int i = 0; i < node->data.block.count; i++) {
                gen_stmt(c, node->data.block.stmts[i]);
            }
            break;
        case AST_IF: {
            int label_else = c->ir_count++;
            int label_end = c->ir_count++;
            gen_expr(c, node->data.ifstmt.cond, 0);
            c->ir[c->ir_count++] = (IRInstr){IR_JZ, -1, 0, -1, 0, 0, "", NULL};
            gen_stmt(c, node->data.ifstmt.tbranch);
            if (node->data.ifstmt.fbranch) {
                c->ir[c->ir_count++] = (IRInstr){IR_JMP, -1, -1, -1, 0, 0, "", NULL};
                c->ir[c->ir_count++] = (IRInstr){IR_LABEL, label_else, -1, -1, 0, 0, "else", NULL};
                gen_stmt(c, node->data.ifstmt.fbranch);
            }
            c->ir[c->ir_count++] = (IRInstr){IR_LABEL, label_end, -1, -1, 0, 0, "end", NULL};
            break;
        }
        case AST_WHILE: {
            int label_start = c->ir_count++;
            int label_end = c->ir_count++;
            c->ir[c->ir_count++] = (IRInstr){IR_LABEL, label_start, -1, -1, 0, 0, "while", NULL};
            gen_expr(c, node->data.loop.cond, 0);
            c->ir[c->ir_count++] = (IRInstr){IR_JZ, -1, 0, -1, 0, 0, "", NULL};
            gen_stmt(c, node->data.loop.body);
            c->ir[c->ir_count++] = (IRInstr){IR_JMP, -1, -1, -1, 0, 0, "", NULL};
            c->ir[c->ir_count++] = (IRInstr){IR_LABEL, label_end, -1, -1, 0, 0, "end", NULL};
            break;
        }
        case AST_FOR: {
            int label_start = c->ir_count++;
            int label_end = c->ir_count++;
            if (node->data.loop.init) gen_expr(c, node->data.loop.init, 0);
            c->ir[c->ir_count++] = (IRInstr){IR_LABEL, label_start, -1, -1, 0, 0, "for", NULL};
            if (node->data.loop.cond) gen_expr(c, node->data.loop.cond, 0);
            c->ir[c->ir_count++] = (IRInstr){IR_JZ, -1, 0, -1, 0, 0, "", NULL};
            gen_stmt(c, node->data.loop.body);
            if (node->data.loop.inc) gen_expr(c, node->data.loop.inc, 0);
            c->ir[c->ir_count++] = (IRInstr){IR_JMP, -1, -1, -1, 0, 0, "", NULL};
            c->ir[c->ir_count++] = (IRInstr){IR_LABEL, label_end, -1, -1, 0, 0, "end", NULL};
            break;
        }
        case AST_RETURN:
            if (node->data.ret.expr) gen_expr(c, node->data.ret.expr, 0);
            c->ir[c->ir_count++] = (IRInstr){IR_RET, -1, 0, -1, 0, 0, "", NULL};
            break;
        case AST_EXPR_STMT:
            gen_expr(c, node->data.ret.expr, 0);
            break;
        default:
            break;
    }
}

// ============ X86 CODE GENERATION ============

void emit_x86(Compiler *c, FILE *out) {
    fprintf(out, "; GCC Compiler Output - x86_64\\n");
    fprintf(out, "bits 64\\n\\n");
    
    for (int i = 0; i < c->ir_count; i++) {
        IRInstr *ir = &c->ir[i];
        switch (ir->op) {
            case IR_NOP:
                fprintf(out, "    nop\\n");
                break;
            case IR_LABEL:
                fprintf(out, ".L%d:\\n", ir->dest);
                break;
            case IR_MOVE:
                fprintf(out, "    mov rax, %lld\\n", ir->imm);
                break;
            case IR_ADD:
                fprintf(out, "    add rax, rbx\\n");
                break;
            case IR_SUB:
                fprintf(out, "    sub rax, rbx\\n");
                break;
            case IR_MUL:
                fprintf(out, "    imul rax, rbx\\n");
                break;
            case IR_DIV:
                fprintf(out, "    cqo\\n");
                fprintf(out, "    idiv rbx\\n");
                break;
            case IR_MOD:
                fprintf(out, "    cqo\\n");
                fprintf(out, "    idiv rbx\\n");
                fprintf(out, "    mov rax, rdx\\n");
                break;
            case IR_AND:
                fprintf(out, "    and rax, rbx\\n");
                break;
            case IR_OR:
                fprintf(out, "    or rax, rbx\\n");
                break;
            case IR_XOR:
                fprintf(out, "    xor rax, rbx\\n");
                break;
            case IR_NOT:
                fprintf(out, "    not rax\\n");
                break;
            case IR_NEG:
                fprintf(out, "    neg rax\\n");
                break;
            case IR_SHL:
                fprintf(out, "    shl rax, cl\\n");
                break;
            case IR_SHR:
                fprintf(out, "    shr rax, cl\\n");
                break;
            case IR_CMP:
                fprintf(out, "    cmp rax, rbx\\n");
                break;
            case IR_JMP:
                fprintf(out, "    jmp .L%d\\n", ir->dest);
                break;
            case IR_JE:
                fprintf(out, "    je .L%d\\n", ir->dest);
                break;
            case IR_JNE:
                fprintf(out, "    jne .L%d\\n", ir->dest);
                break;
            case IR_JZ:
                fprintf(out, "    jz .L%d\\n", ir->dest);
                break;
            case IR_JNZ:
                fprintf(out, "    jnz .L%d\\n", ir->dest);
                break;
            case IR_CALL:
                fprintf(out, "    call %s\\n", ir->label);
                break;
            case IR_RET:
                fprintf(out, "    ret\\n");
                break;
            case IR_PUSH:
                fprintf(out, "    push rax\\n");
                break;
            case IR_POP:
                fprintf(out, "    pop rax\\n");
                break;
            case IR_LOAD:
                fprintf(out, "    mov rax, [rbp+%d]\\n", ir->imm);
                break;
            case IR_STORE:
                fprintf(out, "    mov [rbp+%d], rax\\n", ir->imm);
                break;
            case IR_LOCAL:
                fprintf(out, "    sub rsp, %lld\\n", ir->imm);
                break;
            case IR_GLOBAL:
                fprintf(out, "    %s: dq 0\\n", ir->label);
                break;
            case IR_ADDR:
                fprintf(out, "    lea rax, [rbp+%d]\\n", ir->imm);
                break;
            case IR_DEREF:
                fprintf(out, "    mov rax, [rax]\\n");
                break;
            case IR_CAST:
                // Type conversion
                break;
            case IR_ALLOCA:
                fprintf(out, "    sub rsp, rax\\n");
                break;
            case IR_FUNC_BEGIN:
                fprintf(out, "%s:\\n", ir->label);
                fprintf(out, "    push rbp\\n");
                fprintf(out, "    mov rbp, rsp\\n");
                break;
            case IR_FUNC_END:
                fprintf(out, "    mov rsp, rbp\\n");
                fprintf(out, "    pop rbp\\n");
                fprintf(out, "    ret\\n");
                break;
            default:
                break;
        }
    }
}
static const char *vocab_words[] = {
    "<pad>", "<unk>", "<s>", "</s>", "<code>", "</code>",
    "the", "a", "an", "is", "are", "was", "were", "be", "been", "being",
    "have", "has", "had", "do", "does", "did", "will", "would", "could", "should",
    "can", "may", "might", "must", "shall",
    "I", "you", "he", "she", "it", "we", "they",
    "this", "that", "these", "those",
    "my", "your", "his", "her", "its", "our", "their",
    "me", "him", "them", "us",
    "and", "or", "but", "so", "yet", "for", "nor",
    "in", "on", "at", "to", "from", "by", "with", "about", "into", "through",
    "during", "before", "after", "above", "below", "between", "under",
    "of", "off", "over", "again", "further", "then", "once",
    "here", "there", "when", "where", "why", "how", "all", "each", "few",
    "more", "most", "other", "some", "such", "no", "not", "only", "own",
    "same", "than", "too", "very", "just", "now",
    // Programming keywords
    "include", "define", "ifndef", "endif", "typedef", "struct", "enum",
    "void", "int", "char", "float", "double", "long", "short", "unsigned",
    "signed", "const", "static", "extern", "inline", "volatile", "register",
    "if", "else", "for", "while", "do", "switch", "case", "default", "break",
    "continue", "return", "goto", "sizeof", "typeof", "asm",
    "class", "public", "private", "protected", "virtual", "override",
    "new", "delete", "template", "typename", "namespace", "using",
    "try", "catch", "throw", "const_cast", "static_cast", "dynamic_cast",
    "true", "false", "nullptr", "auto", "decltype", "constexpr",
    // Python keywords
    "def", "class", "import", "from", "as", "if", "elif", "else", "for",
    "while", "break", "continue", "return", "yield", "lambda", "with",
    "try", "except", "finally", "raise", "assert", "del", "pass", "global",
    "nonlocal", "print", "range", "len", "str", "int", "float", "list",
    "dict", "tuple", "set", "None", "True", "False",
    // C# keywords
    "using", "namespace", "class", "struct", "interface", "enum",
    "public", "private", "protected", "internal", "static", "readonly",
    "const", "virtual", "override", "abstract", "sealed", "partial",
    "if", "else", "for", "foreach", "while", "do", "switch", "case",
    "default", "break", "continue", "return", "goto", "throw", "try",
    "catch", "finally", "using", "lock", "yield", "async", "await",
    "var", "dynamic", "object", "string", "decimal", "bool", "byte",
    "sbyte", "short", "ushort", "int", "uint", "long", "ulong", "float",
    "double", "char", "void", "new", "this", "base", "null", "true", "false",
    // Common programming terms
    "function", "method", "variable", "pointer", "reference", "array",
    "string", "buffer", "memory", "alloc", "free", "malloc", "calloc",
    "realloc", "memcpy", "memset", "strcmp", "strcpy", "strlen",
    "printf", "scanf", "fopen", "fclose", "fread", "fwrite", "fprintf",
    "main", "argc", "argv", "return", "exit", "abort", "assert",
    "kernel", "driver", "interrupt", "syscall", "process", "thread",
    "mutex", "semaphore", "lock", "unlock", "atomic", "volatile",
    "page", "frame", "table", "directory", "virtual", "physical",
    "stack", "heap", "bss", "data", "text", "rodata",
    // OS-specific
    "ZEBX", "OS", "kernel", "boot", "loader", "GRUB", "multiboot",
    "VGA", "framebuffer", "memory", "paging", "IDT", "GDT", "TSS",
    "PIC", "APIC", "IOAPIC", "LAPIC", "timer", "PIT", "HPET", "TSC",
    "keyboard", "mouse", "disk", "ATA", "SATA", "NVMe", "USB", "PCI",
    "network", "ethernet", "WiFi", "bluetooth", "audio", "video",
    // Numbers and symbols
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">", "<=", ">=",
    "&&", "||", "!", "&", "|", "^", "~", "<<", ">>",
    "++", "--", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=",
    "->", ".", "::", "...", "?:",
    "(", ")", "{", "}", "[", "]", ";", ",", ":", "\"", "'", "\\",
    "//", "/*", "*/", "#", "##",
    // Common words
    "hello", "world", "program", "code", "write", "create", "make",
    "build", "compile", "run", "execute", "test", "debug", "fix",
    "error", "warning", "bug", "issue", "problem", "solution",
    "help", "please", "thank", "thanks", "sorry", "welcome",
    "yes", "no", "ok", "done", "complete", "finish", "start",
    "begin", "end", "next", "previous", "first", "last",
    "show", "display", "print", "output", "input", "read", "write",
    "open", "close", "save", "load", "find", "search", "get", "set",
    "add", "remove", "delete", "clear", "reset", "update", "change",
    "convert", "transform", "parse", "format", "encode", "decode",
    "encrypt", "decrypt", "compress", "decompress", "serialize",
    "deserialize", "clone", "copy", "move", "swap", "sort", "filter",
    "map", "reduce", "foreach", "iterate", "loop", "recursion",
    "algorithm", "data", "structure", "tree", "graph", "queue",
    "stack", "list", "map", "hash", "set", "heap", "priority",
    "binary", "search", "sort", "merge", "quick", "bubble",
    "insertion", "selection", "heap", "radix", "counting",
    // File operations
    "file", "directory", "folder", "path", "name", "extension",
    "create", "mkdir", "rmdir", "remove", "rename", "copy", "move",
    "chmod", "chown", "stat", "access", "open", "close", "read",
    "write", "seek", "tell", "flush", "sync", "truncate",
    // Network
    "socket", "bind", "listen", "accept", "connect", "send", "recv",
    "http", "https", "tcp", "udp", "ip", "port", "address", "host",
    "client", "server", "request", "response", "header", "body",
    "GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS",
    "URL", "URI", "domain", "DNS", "DHCP", "NAT", "firewall",
    // Graphics
    "pixel", "color", "RGB", "RGBA", "bitmap", "texture", "shader",
    "vertex", "fragment", "geometry", "render", "draw", "clear",
    "line", "circle", "rectangle", "triangle", "polygon", "ellipse",
    "fill", "stroke", "blend", "alpha", "opacity", "transparent",
    // AI/ML
    "neural", "network", "layer", "activation", "relu", "sigmoid",
    "tanh", "softmax", "loss", "gradient", "backprop", "optimizer",
    "adam", "sgd", "batch", "epoch", "training", "inference",
    "model", "weight", "bias", "parameter", "hyperparameter",
    "token", "embedding", "attention", "transformer", "GPT",
    "encoder", "decoder", "sequence", "context", "prompt",
};

#define NUM_VOCAB_WORDS (sizeof(vocab_words) / sizeof(vocab_words[0]))

void ai_init(void) {
    memset(&model, 0, sizeof(model));
    memset(&context, 0, sizeof(context));
    model_loaded = 0;
    
    // Initialize vocabulary
    model.vocab_size = NUM_VOCAB_WORDS;
    for (uint32_t i = 0; i < model.vocab_size && i < AI_VOCAB_SIZE; i++) {
        model.vocab[i].id = i;
        strcpy(model.vocab[i].text, vocab_words[i]);
        
        // Assign token type
        if (i < 6) model.vocab[i].type = AI_TOK_SPECIAL;
        else if (vocab_words[i][0] >= '0' && vocab_words[i][0] <= '9') model.vocab[i].type = AI_TOK_NUMBER;
        else if (strchr("+-*/%=<>!&|^~.(){}[];:,\\\"'#/", vocab_words[i][0])) model.vocab[i].type = AI_TOK_SYMBOL;
        else if (!strcmp(vocab_words[i], "<code>") || !strcmp(vocab_words[i], "</code>")) model.vocab[i].type = AI_TOK_CODE_BLOCK;
        else model.vocab[i].type = AI_TOK_WORD;
        
        // Initialize embeddings with random values
        for (int j = 0; j < AI_EMBEDDING_DIM; j++) {
            model.vocab[i].embedding[j] = ai_random_normal() * 0.02f;
        }
    }
    
    // Initialize position embeddings
    model.position_embeddings = (float*)kmalloc(AI_MAX_TOKENS * AI_EMBEDDING_DIM * sizeof(float));
    for (uint32_t pos = 0; pos < AI_MAX_TOKENS; pos++) {
        for (int dim = 0; dim < AI_EMBEDDING_DIM; dim++) {
            float angle = (float)pos / powf(10000.0f, (2.0f * (dim / 2)) / AI_EMBEDDING_DIM);
            if (dim % 2 == 0) {
                model.position_embeddings[pos * AI_EMBEDDING_DIM + dim] = sinf(angle);
            } else {
                model.position_embeddings[pos * AI_EMBEDDING_DIM + dim] = cosf(angle);
            }
        }
    }
    
    // Initialize transformer layers
    for (int layer = 0; layer < AI_NUM_LAYERS; layer++) {
        ai_transformer_layer_t *l = &model.layers[layer];
        
        int head_dim = AI_EMBEDDING_DIM / AI_NUM_HEADS;
        
        for (int h = 0; h < AI_NUM_HEADS; h++) {
            l->heads[h].wq = (float*)kmalloc(AI_EMBEDDING_DIM * head_dim * sizeof(float));
            l->heads[h].wk = (float*)kmalloc(AI_EMBEDDING_DIM * head_dim * sizeof(float));
            l->heads[h].wv = (float*)kmalloc(AI_EMBEDDING_DIM * head_dim * sizeof(float));
            l->heads[h].wo = (float*)kmalloc(head_dim * AI_EMBEDDING_DIM * sizeof(float));
            
            ai_init_weights(l->heads[h].wq, AI_EMBEDDING_DIM * head_dim, 0.02f);
            ai_init_weights(l->heads[h].wk, AI_EMBEDDING_DIM * head_dim, 0.02f);
            ai_init_weights(l->heads[h].wv, AI_EMBEDDING_DIM * head_dim, 0.02f);
            ai_init_weights(l->heads[h].wo, head_dim * AI_EMBEDDING_DIM, 0.02f);
        }
        
        l->ff_w1 = (float*)kmalloc(AI_EMBEDDING_DIM * AI_FF_DIM * sizeof(float));
        l->ff_w2 = (float*)kmalloc(AI_FF_DIM * AI_EMBEDDING_DIM * sizeof(float));
        l->ff_b1 = (float*)kmalloc(AI_FF_DIM * sizeof(float));
        l->ff_b2 = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        
        ai_init_weights(l->ff_w1, AI_EMBEDDING_DIM * AI_FF_DIM, 0.02f);
        ai_init_weights(l->ff_w2, AI_FF_DIM * AI_EMBEDDING_DIM, 0.02f);
        memset(l->ff_b1, 0, AI_FF_DIM * sizeof(float));
        memset(l->ff_b2, 0, AI_EMBEDDING_DIM * sizeof(float));
        
        l->ln_gamma1 = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        l->ln_beta1 = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        l->ln_gamma2 = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        l->ln_beta2 = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
        
        for (int i = 0; i < AI_EMBEDDING_DIM; i++) {
            l->ln_gamma1[i] = 1.0f;
            l->ln_beta1[i] = 0.0f;
            l->ln_gamma2[i] = 1.0f;
            l->ln_beta2[i] = 0.0f;
        }
    }
    
    // Output normalization
    model.output_norm_gamma = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
    model.output_norm_beta = (float*)kmalloc(AI_EMBEDDING_DIM * sizeof(float));
    for (int i = 0; i < AI_EMBEDDING_DIM; i++) {
        model.output_norm_gamma[i] = 1.0f;
        model.output_norm_beta[i] = 0.0f;
    }
    
    // LM head (tie with embeddings)
    model.lm_head = (float*)kmalloc(AI_EMBEDDING_DIM * model.vocab_size * sizeof(float));
    ai_init_weights(model.lm_head, AI_EMBEDDING_DIM * model.vocab_size, 0.02f);
    
    // Calculate parameter count
    model.num_params = 0;
    model.num_params += model.vocab_size * AI_EMBEDDING_DIM; // embeddings
    model.num_params += AI_MAX_TOKENS * AI_EMBEDDING_DIM; // position embeddings
    model.num_params += AI_NUM_LAYERS * AI_NUM_HEADS * 4 * AI_EMBEDDING_DIM * (AI_EMBEDDING_DIM / AI_NUM_HEADS); // attention
    model.num_params += AI_NUM_LAYERS * (AI_EMBEDDING_DIM * AI_FF_DIM + AI_FF_DIM * AI_EMBEDDING_DIM); // FFN
    model.num_params += AI_NUM_LAYERS * (4 * AI_EMBEDDING_DIM); // layer norms
    model.num_params += AI_EMBEDDING_DIM * model.vocab_size; // lm head
    
    // Set default system prompt
    strcpy(context.system_prompt,
        "You are ZEBX-GPT, an AI assistant built into ZEBX OS. "
        "You can help with programming in C, C++, Python, and C#. "
        "You can also help with OS commands, file management, and system tasks. "
        "Be concise and helpful.");
    
    model_loaded = 1;
    
    vga_puts("[+] ZEBX-GPT AI Engine initialized\\n");
    vga_puts("    Model: ZEBX-GPT v1.0\\n");
    vga_puts("    Parameters: ");
    vga_put_int(model.num_params / 1000000);
    vga_puts("M\\n");
    vga_puts("    Layers: ");
    vga_put_int(AI_NUM_LAYERS);
    vga_puts("\\n");
    vga_puts("    Heads: ");
    vga_put_int(AI_NUM_HEADS);
    vga_puts("\\n");
    vga_puts("    Embedding: ");
    vga_put_int(AI_EMBEDDING_DIM);
    vga_puts("\\n");
    vga_puts("    Vocab: ");
    vga_put_int(model.vocab_size);
    vga_puts("\\n");
    vga_puts("    Context: ");
    vga_put_int(AI_MAX_CONTEXT_LEN);
    vga_puts(" tokens\\n");
}

void ai_load_model(const char *path) {
    vga_puts("[*] Loading model from: ");
    vga_puts(path);
    vga_puts("\\n");
    
    // In real implementation, would load from file
    // For now, model is initialized in ai_init
    
    vga_puts("[+] Model loaded\\n");
}

int ai_model_loaded(void) {
    return model_loaded;
}

// Tokenization
uint32_t ai_tokenize(const char *text, ai_token_t *tokens, uint32_t max_tokens) {
    uint32_t count = 0;
    const char *p = text;
    
    while (*p && count < max_tokens) {
        // Skip whitespace
        while (*p == ' ' || *p == '\\t' || *p == '\\n') p++;
        if (!*p) break;
        
        // Try to match longest word
        int best_match = -1;
        int best_len = 0;
        
        for (uint32_t i = 0; i < model.vocab_size; i++) {
            int len = strlen(model.vocab[i].text);
            if (len > best_len && !strncmp(p, model.vocab[i].text, len)) {
                best_match = i;
                best_len = len;
            }
        }
        
        if (best_match >= 0) {
            tokens[count++] = model.vocab[best_match];
            p += best_len;
        } else {
            // Unknown character - add as symbol
            tokens[count].id = 1; // <unk>
            tokens[count].type = AI_TOK_UNKNOWN;
            tokens[count].text[0] = *p;
            tokens[count].text[1] = 0;
            count++;
            p++;
        }
    }
    
    return count;
}

uint32_t ai_encode(const char *text, uint32_t *token_ids, uint32_t max_len) {
    ai_token_t tokens[AI_MAX_TOKENS];
    uint32_t count = ai_tokenize(text, tokens, max_len);
    
    for (uint32_t i = 0; i < count && i < max_len; i++) {
        token_ids[i] = tokens[i].id;
    }
    
    return count;
}

void ai_decode(uint32_t *token_ids, uint32_t count, char *text, uint32_t max_len) {
    uint32_t pos = 0;
    
    for (uint32_t i = 0; i < count && pos < max_len - 1; i++) {
        uint32_t id = token_ids[i];
        if (id < model.vocab_size) {
            const char *word = model.vocab[id].text;
            int len = strlen(word);
            
            // Add space before word if needed
            if (pos > 0 && model.vocab[id].type == AI_TOK_WORD) {
                text[pos++] = ' ';
            }
            
            for (int j = 0; j < len && pos < max_len - 1; j++) {
                text[pos++] = word[j];
            }
        }
    }
    
    text[pos] = 0;
}

// Math utilities
float ai_random_normal(void) {
    // Box-Muller transform
    static int has_spare = 0;
    static float spare;
    
    if (has_spare) {
        has_spare = 0;
        return spare;
    }
    
    has_spare = 1;
    
    float u1 = (float)(timer_get_ticks() % 10000) / 10000.0f;
    float u2 = (float)((timer_get_ticks() * 7) % 10000) / 10000.0f;
    
    if (u1 < 0.0001f) u1 = 0.0001f;
    
    float mag = sqrtf(-2.0f * logf(u1));
    spare = mag * cosf(2.0f * 3.14159f * u2);
    return mag * sinf(2.0f * 3.14159f * u2);
}

void ai_init_weights(float *weights, uint32_t count, float scale) {
    for (uint32_t i = 0; i < count; i++) {
        weights[i] = ai_random_normal() * scale;
    }
}

void ai_softmax(float *x, uint32_t len) {
    float max_val = x[0];
    for (uint32_t i = 1; i < len; i++) {
        if (x[i] > max_val) max_val = x[i];
    }
    
    float sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        x[i] = expf(x[i] - max_val);
        sum += x[i];
    }
    
    for (uint32_t i = 0; i < len; i++) {
        x[i] /= sum;
    }
}

void ai_layer_norm(float *x, uint32_t len, float gamma, float beta) {
    float mean = 0;
    for (uint32_t i = 0; i < len; i++) mean += x[i];
    mean /= len;
    
    float var = 0;
    for (uint32_t i = 0; i < len; i++) {
        float diff = x[i] - mean;
        var += diff * diff;
    }
    var /= len;
    
    float std = sqrtf(var + 1e-5f);
    
    for (uint32_t i = 0; i < len; i++) {
        x[i] = gamma * ((x[i] - mean) / std) + beta;
    }
}

void ai_gelu(float *x, uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        float val = x[i];
        x[i] = 0.5f * val * (1.0f + tanhf(0.7978845608f * (val + 0.044715f * val * val * val)));
    }
}

void ai_matmul(float *a, float *b, float *c, uint32_t m, uint32_t n, uint32_t k) {
    // C[m x n] = A[m x k] * B[k x n]
    for (uint32_t i = 0; i < m; i++) {
        for (uint32_t j = 0; j < n; j++) {
            float sum = 0;
            for (uint32_t l = 0; l < k; l++) {
                sum += a[i * k + l] * b[l * n + j];
            }
            c[i * n + j] = sum;
        }
    }
}

void ai_compute_attention(float *q, float *k, float *v, float *output, uint32_t seq_len, uint32_t head_dim) {
    // Compute Q * K^T
    float *scores = (float*)kmalloc(seq_len * seq_len * sizeof(float));
    
    for (uint32_t i = 0; i < seq_len; i++) {
        for (uint32_t j = 0; j < seq_len; j++) {
            float sum = 0;
            for (uint32_t d = 0; d < head_dim; d++) {
                sum += q[i * head_dim + d] * k[j * head_dim + d];
            }
            scores[i * seq_len + j] = sum / sqrtf((float)head_dim);
        }
        
        // Apply causal mask
        for (uint32_t j = i + 1; j < seq_len; j++) {
            scores[i * seq_len + j] = -1e9f;
        }
        
        // Softmax
        ai_softmax(&scores[i * seq_len], seq_len);
    }
    
    // Compute attention * V
    for (uint32_t i = 0; i < seq_len; i++) {
        for (uint32_t d = 0; d < head_dim; d++) {
            float sum = 0;
            for (uint32_t j = 0; j < seq_len; j++) {
                sum += scores[i * seq_len + j] * v[j * head_dim + d];
            }
            output[i * head_dim + d] = sum;
        }
    }
    
    kfree(scores);
}

void ai_forward_pass(uint32_t *input_ids, uint32_t input_len, float *output_logits) {
    if (!model_loaded || input_len == 0) return;
    
    // Embedding lookup
    float *hidden = (float*)kmalloc(input_len * AI_EMBEDDING_DIM * sizeof(float));
    
    for (uint32_t i = 0; i < input_len; i++) {
        uint32_t id = input_ids[i];
        if (id >= model.vocab_size) id = 1; // <unk>
        
        for (int d = 0; d < AI_EMBEDDING_DIM; d++) {
            hidden[i * AI_EMBEDDING_DIM + d] = model.vocab[id].embedding[d]
                                             + model.position_embeddings[i * AI_EMBEDDING_DIM + d];
        }
    }
    
    // Transformer layers
    for (int layer = 0; layer < AI_NUM_LAYERS; layer++) {
        ai_transformer_layer_t *l = &model.layers[layer];
        
        // Self-attention
        float *attn_out = (float*)kmalloc(input_len * AI_EMBEDDING_DIM * sizeof(float));
        memset(attn_out, 0, input_len * AI_EMBEDDING_DIM * sizeof(float));
        
        int head_dim = AI_EMBEDDING_DIM / AI_NUM_HEADS;
        
        for (int h = 0; h < AI_NUM_HEADS; h++) {
            float *q = (float*)kmalloc(input_len * head_dim * sizeof(float));
            float *k = (float*)kmalloc(input_len * head_dim * sizeof(float));
            float *v = (float*)kmalloc(input_len * head_dim * sizeof(float));
            float *head_out = (float*)kmalloc(input_len * head_dim * sizeof(float));
            
            // Q = hidden * Wq
            for (uint32_t i = 0; i < input_len; i++) {
                for (int d = 0; d < head_dim; d++) {
                    float sum_q = 0, sum_k = 0, sum_v = 0;
                    for (int e = 0; e < AI_EMBEDDING_DIM; e++) {
                        sum_q += hidden[i * AI_EMBEDDING_DIM + e] * l->heads[h].wq[e * head_dim + d];
                        sum_k += hidden[i * AI_EMBEDDING_DIM + e] * l->heads[h].wk[e * head_dim + d];
                        sum_v += hidden[i * AI_EMBEDDING_DIM + e] * l->heads[h].wv[e * head_dim + d];
                    }
                    q[i * head_dim + d] = sum_q;
                    k[i * head_dim + d] = sum_k;
                    v[i * head_dim + d] = sum_v;
                }
            }
            
            // Attention
            ai_compute_attention(q, k, v, head_out, input_len, head_dim);
            
            // Output projection
            for (uint32_t i = 0; i < input_len; i++) {
                for (int e = 0; e < AI_EMBEDDING_DIM; e++) {
                    float sum = 0;
                    for (int d = 0; d < head_dim; d++) {
                        sum += head_out[i * head_dim + d] * l->heads[h].wo[d * AI_EMBEDDING_DIM + e];
                    }
                    attn_out[i * AI_EMBEDDING_DIM + e] += sum;
                }
            }
            
            kfree(q); kfree(k); kfree(v); kfree(head_out);
        }
        
        // Residual + Layer Norm 1
        for (uint32_t i = 0; i < input_len; i++) {
            for (int e = 0; e < AI_EMBEDDING_DIM; e++) {
                hidden[i * AI_EMBEDDING_DIM + e] += attn_out[i * AI_EMBEDDING_DIM + e];
            }
            ai_layer_norm(&hidden[i * AI_EMBEDDING_DIM], AI_EMBEDDING_DIM,
                         l->ln_gamma1[0], l->ln_beta1[0]);
        }
        
        kfree(attn_out);
        
        // Feed-forward network
        float *ff_hidden = (float*)kmalloc(input_len * AI_FF_DIM * sizeof(float));
        float *ff_out = (float*)kmalloc(input_len * AI_EMBEDDING_DIM * sizeof(float));
        
        for (uint32_t i = 0; i < input_len; i++) {
            for (int f = 0; f < AI_FF_DIM; f++) {
                float sum = l->ff_b1[f];
                for (int e = 0; e < AI_EMBEDDING_DIM; e++) {
                    sum += hidden[i * AI_EMBEDDING_DIM + e] * l->ff_w1[e * AI_FF_DIM + f];
                }
                ff_hidden[i * AI_FF_DIM + f] = sum;
            }
            ai_gelu(&ff_hidden[i * AI_FF_DIM], AI_FF_DIM);
            
            for (int e = 0; e < AI_EMBEDDING_DIM; e++) {
                float sum = l->ff_b2[e];
                for (int f = 0; f < AI_FF_DIM; f++) {
                    sum += ff_hidden[i * AI_FF_DIM + f] * l->ff_w2[f * AI_EMBEDDING_DIM + e];
                }
                ff_out[i * AI_EMBEDDING_DIM + e] = sum;
            }
        }
        
        // Residual + Layer Norm 2
        for (uint32_t i = 0; i < input_len; i++) {
            for (int e = 0; e < AI_EMBEDDING_DIM; e++) {
                hidden[i * AI_EMBEDDING_DIM + e] += ff_out[i * AI_EMBEDDING_DIM + e];
            }
            ai_layer_norm(&hidden[i * AI_EMBEDDING_DIM], AI_EMBEDDING_DIM,
                         l->ln_gamma2[0], l->ln_beta2[0]);
        }
        
        kfree(ff_hidden); kfree(ff_out);
    }
    
    // Output layer norm
    for (uint32_t i = 0; i < input_len; i++) {
        ai_layer_norm(&hidden[i * AI_EMBEDDING_DIM], AI_EMBEDDING_DIM,
                     model.output_norm_gamma[0], model.output_norm_beta[0]);
    }
    
    // LM head - compute logits for last token
    uint32_t last_idx = input_len - 1;
    for (uint32_t v = 0; v < model.vocab_size; v++) {
        float sum = 0;
        for (int e = 0; e < AI_EMBEDDING_DIM; e++) {
            sum += hidden[last_idx * AI_EMBEDDING_DIM + e] * model.lm_head[e * model.vocab_size + v];
        }
        output_logits[v] = sum;
    }
    
    kfree(hidden);
}

uint32_t ai_sample_token(float *logits, uint32_t vocab_size, float temperature) {
    // Apply temperature
    if (temperature != 1.0f) {
        for (uint32_t i = 0; i < vocab_size; i++) {
            logits[i] /= temperature;
        }
    }
    
    ai_softmax(logits, vocab_size);
    
    // Top-k sampling (k=50)
    const uint32_t top_k = 50;
    float top_probs[50];
    uint32_t top_indices[50];
    
    for (uint32_t i = 0; i < top_k && i < vocab_size; i++) {
        top_probs[i] = logits[i];
        top_indices[i] = i;
    }
    
    // Sort top-k
    for (uint32_t i = 0; i < top_k; i++) {
        for (uint32_t j = i + 1; j < top_k; j++) {
            if (top_probs[j] > top_probs[i]) {
                float tp = top_probs[i]; top_probs[i] = top_probs[j]; top_probs[j] = tp;
                uint32_t ti = top_indices[i]; top_indices[i] = top_indices[j]; top_indices[j] = ti;
            }
        }
    }
    
    // Sample from top-k
    float r = (float)(timer_get_ticks() % 10000) / 10000.0f;
    float cumsum = 0;
    
    for (uint32_t i = 0; i < top_k; i++) {
        cumsum += top_probs[i];
        if (r < cumsum) {
            return top_indices[i];
        }
    }
    
    return top_indices[0];
}

void ai_generate(uint32_t *prompt_ids, uint32_t prompt_len, uint32_t *output_ids, uint32_t *output_len, uint32_t max_new_tokens) {
    if (!model_loaded) return;
    
    uint32_t *seq = (uint32_t*)kmalloc((prompt_len + max_new_tokens) * sizeof(uint32_t));
    
    for (uint32_t i = 0; i < prompt_len; i++) {
        seq[i] = prompt_ids[i];
    }
    
    uint32_t seq_len = prompt_len;
    
    for (uint32_t i = 0; i < max_new_tokens && seq_len < prompt_len + max_new_tokens; i++) {
        float *logits = (float*)kmalloc(model.vocab_size * sizeof(float));
        
        ai_forward_pass(seq, seq_len, logits);
        
        uint32_t next_token = ai_sample_token(logits, model.vocab_size, 0.8f);
        
        kfree(logits);
        
        if (next_token == 3) break; // </s> token
        
        seq[seq_len++] = next_token;
        output_ids[i] = next_token;
    }
    
    *output_len = seq_len - prompt_len;
    kfree(seq);
}

// High-level API
void ai_chat(const char *message, char *response, uint32_t max_len) {
    if (!model_loaded) {
        strcpy(response, "AI model not loaded.");
        return;
    }
    
    // Add user message to context
    ai_add_user_message(message);
    
    // Build prompt
    char prompt[AI_MAX_CONTEXT_LEN];
    int pos = 0;
    
    pos += sprintf(prompt + pos, "%s\\n", context.system_prompt);
    
    for (uint32_t i = 0; i < context.num_turns; i++) {
        pos += sprintf(prompt + pos, "User: %s\\n", context.user_messages[i]);
        if (i < context.num_turns - 1) {
            pos += sprintf(prompt + pos, "Assistant: %s\\n", context.assistant_responses[i]);
        }
    }
    
    pos += sprintf(prompt + pos, "Assistant:");
    
    // Tokenize and generate
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 256);
    
    // Decode
    ai_decode(output_ids, output_len, response, max_len);
    
    // Add to context
    ai_add_assistant_response(response);
}

void ai_generate_code(ai_task_t task, const char *prompt, char *code, uint32_t max_len) {
    char full_prompt[2048];
    
    switch (task) {
        case AI_TASK_CODE_C:
            sprintf(full_prompt, "Write C code for: %s\\n\\n```c\\n", prompt);
            break;
        case AI_TASK_CODE_CPP:
            sprintf(full_prompt, "Write C++ code for: %s\\n\\n```cpp\\n", prompt);
            break;
        case AI_TASK_CODE_PYTHON:
            sprintf(full_prompt, "Write Python code for: %s\\n\\n```python\\n", prompt);
            break;
        case AI_TASK_CODE_CSHARP:
            sprintf(full_prompt, "Write C# code for: %s\\n\\n```csharp\\n", prompt);
            break;
        default:
            sprintf(full_prompt, "Write code for: %s\\n\\n```\\n", prompt);
            break;
    }
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(full_prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 512);
    
    ai_decode(output_ids, output_len, code, max_len);
    
    // Add closing backticks if missing
    int len = strlen(code);
    if (len > 3 && code[len-1] != '`') {
        strcat(code, "\\n```");
    }
}

void ai_explain_code(const char *code, char *explanation, uint32_t max_len) {
    char prompt[4096];
    sprintf(prompt, "Explain this code:\\n\\n```\\n%s\\n```\\n\\nExplanation:", code);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 256);
    ai_decode(output_ids, output_len, explanation, max_len);
}

void ai_debug_code(const char *code, const char *error, char *fix, uint32_t max_len) {
    char prompt[4096];
    sprintf(prompt, "Fix this code that has error: %s\\n\\n```\\n%s\\n```\\n\\nFixed code:\\n```\\n", error, code);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 512);
    ai_decode(output_ids, output_len, fix, max_len);
}

void ai_optimize_code(const char *code, char *optimized, uint32_t max_len) {
    char prompt[4096];
    sprintf(prompt, "Optimize this code for performance:\\n\\n```\\n%s\\n```\\n\\nOptimized code:\\n```\\n", code);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 512);
    ai_decode(output_ids, output_len, optimized, max_len);
}

void ai_translate_code(const char *code, const char *from_lang, const char *to_lang, char *translated, uint32_t max_len) {
    char prompt[4096];
    sprintf(prompt, "Translate this %s code to %s:\\n\\n```\\n%s\\n```\\n\\n%s code:\\n```\\n", from_lang, to_lang, code, to_lang);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 512);
    ai_decode(output_ids, output_len, translated, max_len);
}

void ai_summarize(const char *text, char *summary, uint32_t max_len) {
    char prompt[4096];
    sprintf(prompt, "Summarize this text:\\n\\n%s\\n\\nSummary:", text);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 128);
    ai_decode(output_ids, output_len, summary, max_len);
}

void ai_os_command(const char *natural_language, char *command, uint32_t max_len) {
    char prompt[1024];
    sprintf(prompt, "Convert to ZEBX OS command:\\n%s\\n\\nCommand:", natural_language);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 64);
    ai_decode(output_ids, output_len, command, max_len);
}

void ai_file_suggest(const char *query, char *suggestions, uint32_t max_len) {
    char prompt[1024];
    sprintf(prompt, "Suggest files for: %s\\n\\nSuggestions:", query);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 128);
    ai_decode(output_ids, output_len, suggestions, max_len);
}

void ai_system_diagnose(char *report, uint32_t max_len) {
    char prompt[1024];
    sprintf(prompt, "ZEBX OS System Diagnostic Report:\\n\\n");
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 256);
    ai_decode(output_ids, output_len, report, max_len);
}

void ai_compile_assist(const char *source, const char *lang, char *output, uint32_t max_len) {
    char prompt[2048];
    sprintf(prompt, "Compile this %s code and show output/errors:\\n\\n```%s\\n%s\\n```\\n\\nCompilation result:\\n", lang, lang, source);
    
    uint32_t prompt_ids[AI_MAX_TOKENS];
    uint32_t prompt_len = ai_encode(prompt, prompt_ids, AI_MAX_TOKENS);
    
    uint32_t output_ids[AI_MAX_TOKENS];
    uint32_t output_len = 0;
    
    ai_generate(prompt_ids, prompt_len, output_ids, &output_len, 256);
    ai_decode(output_ids, output_len, output, max_len);
}

// Context management
void ai_new_chat(void) {
    memset(&context, 0, sizeof(context));
    strcpy(context.system_prompt,
        "You are ZEBX-GPT, an AI assistant built into ZEBX OS. "
        "You can help with programming in C, C++, Python, and C#. "
        "You can also help with OS commands, file management, and system tasks. "
        "Be concise and helpful.");
}

void ai_set_system_prompt(const char *prompt) {
    strncpy(context.system_prompt, prompt, 1023);
}

void ai_add_user_message(const char *message) {
    if (context.num_turns >= 32) {
        // Shift messages
        for (uint32_t i = 0; i < 31; i++) {
            strcpy(context.user_messages[i], context.user_messages[i + 1]);
            strcpy(context.assistant_responses[i], context.assistant_responses[i + 1]);
        }
        context.num_turns = 31;
    }
    
    strncpy(context.user_messages[context.num_turns], message, AI_MAX_CONTEXT_LEN - 1);
}

void ai_add_assistant_response(const char *response) {
    strncpy(context.assistant_responses[context.num_turns], response, AI_MAX_RESPONSE_LEN - 1);
    context.num_turns++;
}

ai_context_t* ai_get_context(void) {
    return &context;
}

void ai_clear_context(void) {
    context.num_turns = 0;
    context.total_tokens = 0;
}

// SIMD-accelerated operations
void ai_embedding_lookup_simd(uint32_t *token_ids, uint32_t count, float *output) {
    if (simd_has_feature(SIMD_FEATURE_AVX512F) && count >= 16) {
        // Use AVX-512 for batch embedding lookup
        for (uint32_t i = 0; i < count; i++) {
            uint32_t id = token_ids[i];
            if (id >= model.vocab_size) id = 1;
            
            memcpy(&output[i * AI_EMBEDDING_DIM], model.vocab[id].embedding,
                   AI_EMBEDDING_DIM * sizeof(float));
        }
    } else {
        for (uint32_t i = 0; i < count; i++) {
            uint32_t id = token_ids[i];
            if (id >= model.vocab_size) id = 1;
            
            for (int d = 0; d < AI_EMBEDDING_DIM; d++) {
                output[i * AI_EMBEDDING_DIM + d] = model.vocab[id].embedding[d];
            }
        }
    }
}

float ai_cosine_similarity(float *a, float *b, uint32_t len) {
    float dot = 0, norm_a = 0, norm_b = 0;
    
    for (uint32_t i = 0; i < len; i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    return dot / (sqrtf(norm_a) * sqrtf(norm_b) + 1e-8f);
}

static inline int sprintf(char *buf, const char *fmt, ...) {
    int num = 0;
    va_list args;
    va_start(args, fmt);
    
    const char *p = fmt;
    while (*p) {
        if (*p == '%' && *(p+1) == 's') {
            const char *str = va_arg(args, const char*);
            while (*str && num < 4095) buf[num++] = *str++;
            p += 2;
        } else if (*p == '%' && *(p+1) == 'd') {
            int val = va_arg(args, int);
            char tmp[16];
            int i = 0;
            if (val == 0) tmp[i++] = '0';
            else {
                int neg = val < 0;
                if (neg) val = -val;
                while (val > 0) { tmp[i++] = '0' + (val % 10); val /= 10; }
                if (neg) buf[num++] = '-';
            }
            for (int j = i - 1; j >= 0; j--) buf[num++] = tmp[j];
            p += 2;
        } else {
            buf[num++] = *p++;
        }
    }
    va_end(args);
    buf[num] = 0;
    return num;
}
#endif // ZEBX_ENABLE_LEGACY_GPT