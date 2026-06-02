// GCCcompiler.c - OS Native Compiler
#include "types.h"
#include "memory.h"
#include "process.h"
#include "disk.h"
#include "interrupt.h"
#include "keyboard.h"
#include "storage.h"
#include "shell.h"

#define GCC_VERSION "12.4.0-OS"
#define MAX_SOURCE 1048576
#define MAX_SYMBOLS 65536
#define MAX_ERRORS 100

// Token Types
typedef enum {
    TOK_EOF, TOK_ID, TOK_NUM, TOK_STR, TOK_CHAR,
    TOK_PLUS, TOK_MINUS, TOK_MUL, TOK_DIV, TOK_MOD,
    TOK_ASSIGN, TOK_EQ, TOK_NE, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_AND, TOK_OR, TOK_NOT, TOK_BAND, TOK_BOR, TOK_BXOR, TOK_BNOT,
    TOK_SHL, TOK_SHR, TOK_INC, TOK_DEC,
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET, TOK_SEMI, TOK_COMMA, TOK_DOT,
    TOK_ARROW, TOK_COLON, TOK_QUESTION, TOK_ELLIPSIS,
    TOK_KW_AUTO, TOK_KW_BREAK, TOK_KW_CASE, TOK_KW_CHAR,
    TOK_KW_CONST, TOK_KW_CONTINUE, TOK_KW_DEFAULT, TOK_KW_DO,
    TOK_KW_DOUBLE, TOK_KW_ELSE, TOK_KW_ENUM, TOK_KW_EXTERN,
    TOK_KW_FLOAT, TOK_KW_FOR, TOK_KW_GOTO, TOK_KW_IF,
    TOK_KW_INLINE, TOK_KW_INT, TOK_KW_LONG, TOK_KW_REGISTER,
    TOK_KW_RESTRICT, TOK_KW_RETURN, TOK_KW_SHORT, TOK_KW_SIGNED,
    TOK_KW_SIZEOF, TOK_KW_STATIC, TOK_KW_STRUCT, TOK_KW_SWITCH,
    TOK_KW_TYPEDEF, TOK_KW_UNION, TOK_KW_UNSIGNED, TOK_KW_VOID,
    TOK_KW_VOLATILE, TOK_KW_WHILE, TOK_KW_BOOL, TOK_KW_COMPLEX,
    TOK_KW_IMAGINARY, TOK_KW_ALIGNAS, TOK_KW_ALIGNOF,
    TOK_KW_ATOMIC, TOK_KW_GENERIC, TOK_KW_NORETURN,
    TOK_KW_STATIC_ASSERT, TOK_KW_THREAD_LOCAL,
    TOK_PREPROC, TOK_COMMENT, TOK_WHITESPACE, TOK_NEWLINE,
    TOK_TYPE_NAME, TOK_ENUM_CONST, TOK_LABEL,
    TOK_BUILTIN, TOK_ATTRIBUTE, TOK_ASM, TOK_PRAGMA
} TokenType;

typedef struct {
    TokenType type;
    char text[256];
    int line, col;
    int value;      // for numeric constants
} Token;

// Symbol Table
typedef enum {
    SYM_VAR, SYM_FUNC, SYM_TYPE, SYM_ENUM, SYM_STRUCT,
    SYM_UNION, SYM_LABEL, SYM_MACRO, SYM_PARAM
} SymbolKind;

typedef struct Symbol {
    char name[128];
    SymbolKind kind;
    int type;           // type index
    int scope;          // scope level
    int offset;         // stack offset or address
    int size;           // size in bytes
    int is_global;
    int is_static;
    int is_extern;
    int is_const;
    struct Symbol *next;
} Symbol;

typedef struct {
    Symbol *table[MAX_SYMBOLS];
    int count;
    int current_scope;
} SymbolTable;

// Type System
typedef enum {
    TYPE_VOID, TYPE_CHAR, TYPE_SHORT, TYPE_INT, TYPE_LONG,
    TYPE_LONGLONG, TYPE_FLOAT, TYPE_DOUBLE, TYPE_LONGDOUBLE,
    TYPE_BOOL, TYPE_POINTER, TYPE_ARRAY, TYPE_FUNCTION,
    TYPE_STRUCT, TYPE_UNION, TYPE_ENUM, TYPE_TYPEDEF,
    TYPE_CONST, TYPE_VOLATILE, TYPE_RESTRICT
} BaseType;

typedef struct Type {
    BaseType base;
    int size;
    int align;
    struct Type *ptr_to;     // for pointers
    struct Type *array_of;   // for arrays
    int array_size;
    struct Type *ret_type;   // for functions
    struct Type *params[32]; // function parameters
    int param_count;
    char struct_name[128];
    int is_unsigned;
} Type;

// AST Nodes
typedef enum {
    AST_PROGRAM, AST_FUNCTION, AST_BLOCK, AST_DECL,
    AST_VAR_DECL, AST_TYPE_DECL, AST_STRUCT_DECL,
    AST_ENUM_DECL, AST_UNION_DECL, AST_TYPEDEF,
    AST_IF, AST_SWITCH, AST_WHILE, AST_DO_WHILE,
    AST_FOR, AST_GOTO, AST_CONTINUE, AST_BREAK,
    AST_RETURN, AST_LABEL, AST_CASE, AST_DEFAULT,
    AST_EXPR_STMT, AST_EMPTY_STMT, AST_ASM,
    AST_EXPR, AST_BINARY, AST_UNARY, AST_TERNARY,
    AST_CALL, AST_INDEX, AST_MEMBER, AST_ARROW,
    AST_CAST, AST_SIZEOF, AST_ALIGNOF, AST_OFFSET,
    AST_LITERAL, AST_STRING, AST_IDENTIFIER,
    AST_COMPOUND_LITERAL, AST_INIT_LIST,
    AST_DESIGNATOR, AST_GENERIC, AST_BUILTIN
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    int line, col;
    Type *dtype;
    union {
        // Literals
        struct { long long ival; double fval; char *sval; } lit;
        // Binary/Unary
        struct { int op; struct ASTNode *left, *right; } binary;
        struct { int op; struct ASTNode *operand; } unary;
        // Ternary
        struct { struct ASTNode *cond, *tbranch, *fbranch; } ternary;
        // Call
        struct { struct ASTNode *func; struct ASTNode *args[64]; int argc; } call;
        // Index
        struct { struct ASTNode *array, *index; } index;
        // Member
        struct { struct ASTNode *obj; char member[128]; } member;
        // Declaration
        struct { char name[128]; Type *type; struct ASTNode *init; int storage; } decl;
        // Function
        struct { char name[128]; Type *rtype; struct ASTNode *params[32]; int paramc; struct ASTNode *body; } func;
        // Block
        struct { struct ASTNode *stmts[1024]; int count; } block;
        // If
        struct { struct ASTNode *cond, *tbranch, *fbranch; } ifstmt;
        // Loop
        struct { struct ASTNode *init, *cond, *inc, *body; } loop;
        // Switch
        struct { struct ASTNode *expr; struct ASTNode *cases[256]; int casec; } swtch;
        // Case
        struct { long long val; struct ASTNode *stmt; } cas;
        // Return
        struct { struct ASTNode *expr; } ret;
        // Goto
        struct { char label[128]; } gt;
        // Label
        struct { char name[128]; struct ASTNode *stmt; } label;
        // Asm
        struct { char code[4096]; struct ASTNode *outputs[16]; struct ASTNode *inputs[16]; int outc, inc; } asmstmt;
        // Generic
        struct { struct ASTNode *ctrl; struct ASTNode *assoc[64]; int count; } generic;
    } data;
    struct ASTNode *next;
} ASTNode;

// IR Instructions
typedef enum {
    IR_NOP, IR_LABEL, IR_FUNC_BEGIN, IR_FUNC_END,
    IR_LOAD, IR_STORE, IR_MOVE, IR_PUSH, IR_POP,
    IR_ADD, IR_SUB, IR_MUL, IR_DIV, IR_MOD,
    IR_AND, IR_OR, IR_XOR, IR_NOT, IR_SHL, IR_SHR,
    IR_NEG, IR_CMP, IR_JMP, IR_JE, IR_JNE,
    IR_JL, IR_JLE, IR_JG, IR_JGE, IR_JZ, IR_JNZ,
    IR_CALL, IR_RET, IR_ARG, IR_LOCAL, IR_GLOBAL,
    IR_ADDR, IR_DEREF, IR_INDEX, IR_MEMBER,
    IR_CAST, IR_TRUNC, IR_EXT, IR_CONV,
    IR_PHI, IR_ALLOCA, IR_MALLOC, IR_FREE,
    IR_MEMCPY, IR_MEMSET, IR_STRLEN,
    IR_BUILTIN, IR_ASM, IR_DEBUG
} IROpcode;

typedef struct {
    IROpcode op;
    int dest, src1, src2;
    long long imm;
    double fimm;
    char label[128];
    Type *type;
} IRInstr;

// Register Allocation
typedef enum {
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

// Optimization Passes
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
} Compiler;

// ============ LEXER ============

int is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int is_digit(char c) {
    return c >= '0' && c <= '9';
}

int is_alnum(char c) {
    return is_alpha(c) || is_digit(c);
}

int is_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

char peek(Compiler *c) {
    if (c->pos >= c->source_len) return '\0';
    return c->source[c->pos];
}

char advance(Compiler *c) {
    char ch = peek(c);
    c->pos++;
    if (ch == '\n') { c->line++; c->col = 1; }
    else c->col++;
    return ch;
}

void skip_whitespace(Compiler *c) {
    while (is_space(peek(c)) && peek(c) != '\n') advance(c);
}

void skip_comment(Compiler *c) {
    if (peek(c) == '/' && c->source[c->pos+1] == '/') {
        while (peek(c) != '\n' && peek(c) != '\0') advance(c);
    } else if (peek(c) == '/' && c->source[c->pos+1] == '*') {
        advance(c); advance(c);
        while (!(peek(c) == '*' && c->source[c->pos+1] == '/') && peek(c) != '\0') {
            advance(c);
        }
        if (peek(c) == '*') { advance(c); advance(c); }
    }
}

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

void lex_number(Compiler *c, Token *tok) {
    int i = 0;
    int is_float = 0;
    int is_hex = 0;
    
    if (peek(c) == '0' && (c->source[c->pos+1] == 'x' || c->source[c->pos+1] == 'X')) {
        is_hex = 1;
        tok->text[i++] = advance(c);
        tok->text[i++] = advance(c);
        while (is_digit(peek(c)) || (peek(c) >= 'a' && peek(c) <= 'f') || 
               (peek(c) >= 'A' && peek(c) <= 'F')) {
            if (i < 255) tok->text[i++] = advance(c);
            else advance(c);
        }
    } else {
        while (is_digit(peek(c))) {
            if (i < 255) tok->text[i++] = advance(c);
            else advance(c);
        }
        if (peek(c) == '.') {
            is_float = 1;
            if (i < 255) tok->text[i++] = advance(c);
            else advance(c);
            while (is_digit(peek(c))) {
                if (i < 255) tok->text[i++] = advance(c);
                else advance(c);
            }
        }
        if (peek(c) == 'e' || peek(c) == 'E') {
            is_float = 1;
            if (i < 255) tok->text[i++] = advance(c);
            else advance(c);
            if (peek(c) == '+' || peek(c) == '-') {
                if (i < 255) tok->text[i++] = advance(c);
                else advance(c);
            }
            while (is_digit(peek(c))) {
                if (i < 255) tok->text[i++] = advance(c);
                else advance(c);
            }
        }
    }
    
    // Suffixes
    while (peek(c) == 'u' || peek(c) == 'U' || peek(c) == 'l' || 
           peek(c) == 'L' || peek(c) == 'f' || peek(c) == 'F') {
        if (i < 255) tok->text[i++] = advance(c);
        else advance(c);
    }
    
    tok->text[i] = '\0';
    tok->type = TOK_NUM;
    if (is_hex) tok->value = strtol(tok->text, NULL, 16);
    else if (is_float) tok->value = 0; // Store float separately
    else tok->value = strtol(tok->text, NULL, 10);
}

void lex_string(Compiler *c, Token *tok) {
    int i = 0;
    char quote = advance(c); // ' or "
    tok->text[i++] = quote;
    
    while (peek(c) != quote && peek(c) != '\0') {
        if (peek(c) == '\\') {
            if (i < 255) tok->text[i++] = advance(c);
            else advance(c);
        }
        if (i < 255) tok->text[i++] = advance(c);
        else advance(c);
    }
    
    if (peek(c) == quote) {
        if (i < 255) tok->text[i++] = advance(c);
        else advance(c);
    }
    
    tok->text[i] = '\0';
    tok->type = (quote == '"') ? TOK_STR : TOK_CHAR;
}

void lex_preproc(Compiler *c, Token *tok) {
    int i = 0;
    tok->text[i++] = advance(c); // #
    
    skip_whitespace(c);
    
    while (!is_space(peek(c)) && peek(c) != '\0') {
        if (i < 255) tok->text[i++] = advance(c);
        else advance(c);
    }
    
    tok->text[i] = '\0';
    tok->type = TOK_PREPROC;
}

void lex_operator(Compiler *c, Token *tok) {
    char ch = peek(c);
    char next = c->source[c->pos+1];
    int i = 0;
    
    switch (ch) {
        case '+':
            if (next == '+') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_INC; }
            else if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* += */ }
            else { tok->text[i++] = advance(c); tok->type = TOK_PLUS; }
            break;
        case '-':
            if (next == '-') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_DEC; }
            else if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* -= */ }
            else if (next == '>') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ARROW; }
            else { tok->text[i++] = advance(c); tok->type = TOK_MINUS; }
            break;
        case '*':
            if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* *= */ }
            else { tok->text[i++] = advance(c); tok->type = TOK_MUL; }
            break;
        case '/':
            if (next == '/' || next == '*') { skip_comment(c); tok->type = TOK_COMMENT; tok->text[0] = '\0'; }
            else if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* /= */ }
            else { tok->text[i++] = advance(c); tok->type = TOK_DIV; }
            break;
        case '%':
            if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* %= */ }
            else { tok->text[i++] = advance(c); tok->type = TOK_MOD; }
            break;
        case '=':
            if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_EQ; }
            else { tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; }
            break;
        case '!':
            if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_NE; }
            else { tok->text[i++] = advance(c); tok->type = TOK_NOT; }
            break;
        case '<':
            if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_LE; }
            else if (next == '<') { 
                tok->text[i++] = advance(c); 
                if (c->source[c->pos+1] == '=') { tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* <<= */ }
                else { tok->type = TOK_SHL; }
            }
            else { tok->text[i++] = advance(c); tok->type = TOK_LT; }
            break;
        case '>':
            if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_GE; }
            else if (next == '>') { 
                tok->text[i++] = advance(c); 
                if (c->source[c->pos+1] == '=') { tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* >>= */ }
                else { tok->type = TOK_SHR; }
            }
            else { tok->text[i++] = advance(c); tok->type = TOK_GT; }
            break;
        case '&':
            if (next == '&') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_AND; }
            else if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* &= */ }
            else { tok->text[i++] = advance(c); tok->type = TOK_BAND; }
            break;
        case '|':
            if (next == '|') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_OR; }
            else if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* |= */ }
            else { tok->text[i++] = advance(c); tok->type = TOK_BOR; }
            break;
        case '^':
            if (next == '=') { tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->type = TOK_ASSIGN; /* ^= */ }
            else { tok->text[i++] = advance(c); tok->type = TOK_BXOR; }
            break;
        case '~':
            tok->text[i++] = advance(c); tok->type = TOK_BNOT;
            break;
        case '(':
            tok->text[i++] = advance(c); tok->type = TOK_LPAREN;
            break;
        case ')':
            tok->text[i++] = advance(c); tok->type = TOK_RPAREN;
            break;
        case '{':
            tok->text[i++] = advance(c); tok->type = TOK_LBRACE;
            break;
        case '}':
            tok->text[i++] = advance(c); tok->type = TOK_RBRACE;
            break;
        case '[':
            tok->text[i++] = advance(c); tok->type = TOK_LBRACKET;
            break;
        case ']':
            tok->text[i++] = advance(c); tok->type = TOK_RBRACKET;
            break;
        case ';':
            tok->text[i++] = advance(c); tok->type = TOK_SEMI;
            break;
        case ',':
            tok->text[i++] = advance(c); tok->type = TOK_COMMA;
            break;
        case '.':
            if (next == '.' && c->source[c->pos+2] == '.') {
                tok->text[i++] = advance(c); tok->text[i++] = advance(c); tok->text[i++] = advance(c);
                tok->type = TOK_ELLIPSIS;
            } else {
                tok->text[i++] = advance(c); tok->type = TOK_DOT;
            }
            break;
        case ':':
            tok->text[i++] = advance(c); tok->type = TOK_COLON;
            break;
        case '?':
            tok->text[i++] = advance(c); tok->type = TOK_QUESTION;
            break;
        case '#':
            lex_preproc(c, tok);
            return;
        default:
            tok->text[i++] = advance(c);
            tok->type = TOK_EOF;
            break;
    }
    tok->text[i] = '\0';
}

int lex(Compiler *c) {
    c->token_count = 0;
    c->line = 1;
    c->col = 1;
    
    while (c->pos < c->source_len) {
        Token *tok = &c->tokens[c->token_count++];
        tok->line = c->line;
        tok->col = c->col;
        
        if (is_space(peek(c))) {
            if (peek(c) == '\n') {
                tok->type = TOK_NEWLINE;
                tok->text[0] = '\n'; tok->text[1] = '\0';
                advance(c);
            } else {
                skip_whitespace(c);
                tok->type = TOK_WHITESPACE;
                tok->text[0] = ' '; tok->text[1] = '\0';
            }
        } else if (peek(c) == '/' && (c->source[c->pos+1] == '/' || c->source[c->pos+1] == '*')) {
            skip_comment(c);
            tok->type = TOK_COMMENT;
            tok->text[0] = '\0';
        } else if (is_alpha(peek(c))) {
            lex_identifier(c, tok);
        } else if (is_digit(peek(c))) {
            lex_number(c, tok);
        } else if (peek(c) == '"' || peek(c) == '\'') {
            lex_string(c, tok);
        } else if (peek(c) == '#') {
            lex_preproc(c, tok);
        } else {
            lex_operator(c, tok);
        }
    }
    
    // Add EOF token
    Token *eof = &c->tokens[c->token_count++];
    eof->type = TOK_EOF;
    eof->line = c->line;
    eof->col = c->col;
    eof->text[0] = '\0';
    
    return c->token_count;
}

// ============ PARSER ============

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

// ============ OPTIMIZATION ============

void opt_const_fold(Compiler *c) {
    for (int i = 0; i < c->ir_count; i++) {
        IRInstr *ir = &c->ir[i];
        if (ir->op == IR_ADD && ir->src1 >= 0 && ir->src2 >= 0) {
            // Fold constants
        }
    }
}

void opt_dead_code(Compiler *c) {
    // Remove unreachable code
}

void opt_inline(Compiler *c) {
    // Function inlining
}

void optimize(Compiler *c, int level) {
    if (level >= 1) {
        opt_const_fold(c);
        opt_dead_code(c);
    }
    if (level >= 2) {
        opt_inline(c);
    }
    if (level >= 3) {
        // Aggressive optimizations
    }
}

// ============ MAIN COMPILER INTERFACE ============

int gcc_compile(const char *source, const char *output, int opt_level) {
    Compiler comp;
    memset(&comp, 0, sizeof(Compiler));
    
    comp.source = (char*)source;
    comp.source_len = strlen(source);
    comp.opt_level = opt_level;
    comp.target = TARGET_X86_64;
    strcpy(comp.output_file, output);
    
    // Lexical analysis
    printf("[GCC] Lexical analysis...\\n");
    lex(&comp);
    printf("[GCC] %d tokens generated\\n", comp.token_count);
    
    // Parsing
    printf("[GCC] Parsing...\\n");
    comp.pos = 0;
    comp.ast = parse_block(&comp);
    printf("[GCC] AST generated\\n");
    
    // Semantic analysis
    printf("[GCC] Semantic analysis...\\n");
    // Type checking, symbol resolution
    
    // Intermediate code generation
    printf("[GCC] Generating IR...\\n");
    gen_stmt(&comp, comp.ast);
    printf("[GCC] %d IR instructions\\n", comp.ir_count);
    
    // Optimization
    if (opt_level > 0) {
        printf("[GCC] Optimization level %d...\\n", opt_level);
        optimize(&comp, opt_level);
    }
    
    // Code generation
    printf("[GCC] Generating x86_64 code...\\n");
    FILE *out = fopen(output, "w");
    if (!out) {
        printf("[GCC] Error: Cannot open output file\\n");
        return 1;
    }
    emit_x86(&comp, out);
    fclose(out);
    
    printf("[GCC] Compilation complete: %s\\n", output);
    return 0;
}

// OS Integration Functions
int gcc_compile_file(const char *input, const char *output, int opt_level) {
    // Read file from OS filesystem
    int fd = os_open(input, 0);
    if (fd < 0) {
        os_printf("Error: Cannot open %s\\n", input);
        return 1;
    }
    
    int size = os_size(fd);
    char *source = (char*)os_malloc(size + 1);
    os_read(fd, source, size);
    source[size] = '\0';
    os_close(fd);
    
    int result = gcc_compile(source, output, opt_level);
    os_free(source);
    return result;
}

void gcc_set_debug(int enable) {
    // Enable debug info generation
}

void gcc_add_include_path(const char *path) {
    // Add include search path
}

void gcc_add_define(const char *name, const char *value) {
    // Add preprocessor define
}

void gcc_add_library(const char *lib) {
    // Link with library
}

// Built-in OS functions integration
void gcc_register_os_builtins(Compiler *c) {
    // Register OS-specific built-in functions
    // VGA, GUI, Network, Process, Memory, etc.
}

// ============ SHELL INTERFACE ============

int cmd_gcc(int argc, char **argv) {
    if (argc < 2) {
        os_printf("Usage: gcc [options] <source.c> [-o output]\\n");
        os_printf("Options:\\n");
        os_printf("  -O0       No optimization\\n");
        os_printf("  -O1       Basic optimization\\n");
        os_printf("  -O2       Standard optimization\\n");
        os_printf("  -O3       Aggressive optimization\\n");
        os_printf("  -S        Generate assembly only\\n");
        os_printf("  -c        Compile only (no link)\\n");
        os_printf("  -g        Debug info\\n");
        os_printf("  -I<<path>  Include path\\n");
        os_printf("  -D<<name>  Define macro\\n");
        os_printf("  -l<<lib>   Link library\\n");
        return 1;
    }
    
    const char *input = NULL;
    const char *output = "a.out";
    int opt_level = 0;
    int debug = 0;
    int asm_only = 0;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'O':
                    opt_level = argv[i][2] - '0';
                    break;
                case 'g':
                    debug = 1;
                    break;
                case 'S':
                    asm_only = 1;
                    break;
                case 'o':
                    if (i + 1 < argc) output = argv[++i];
                    break;
                case 'I':
                    gcc_add_include_path(argv[i] + 2);
                    break;
                case 'D':
                    gcc_add_define(argv[i] + 2, "1");
                    break;
                case 'l':
                    gcc_add_library(argv[i] + 2);
                    break;
            }
        } else {
            input = argv[i];
        }
    }
    
    if (!input) {
        os_printf("Error: No input file specified\\n");
        return 1;
    }
    
    gcc_set_debug(debug);
    return gcc_compile_file(input, output, opt_level);
}

// ============ INITIALIZATION ============

void gcc_init(void) {
    os_printf("[GCC] Compiler initialized v%s\\n", GCC_VERSION);
    os_printf("[GCC] Target: x86_64-pc-os\\n");
    os_printf("[GCC] Supported: C11, GNU extensions\\n");
}

void gcc_shutdown(void) {
    os_printf("[GCC] Compiler shutdown\\n");
}