#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t b8;
typedef size_t usize;

#define CNULL '\0'

#define DEBUG printf("%s:%d\n", __FILE__, __LINE__);

//-------------------------------------------------------------------------------

#define SV_FMT "%.*s"
#define SV_ARG(sv) (int)(sv).length, (sv).data

typedef struct StringView {
    const char* data;
    usize length;
} StringView;

StringView sv_from(const char* string) {
    return (StringView) {
	.data = string,
	.length = strlen(string),
    };
}

StringView sv_cut_left(StringView *sv, usize cutlen) {
    cutlen = (cutlen > sv->length) ? sv->length : cutlen;
    sv->length = sv->length - cutlen;
    sv->data = sv->data + cutlen;
    return (StringView) {
	.data = sv->data,
	.length = cutlen,
    };
}

int sv_to_int(StringView *sv, const char* format) {
    char* temp = (char*)calloc(sv->length + 1, sizeof(char));
    memcpy(temp, sv->data, sv->length);
    temp[sv->length] = CNULL;
    int ret = 0;
    sscanf(temp, format, &ret);
    free(temp);
    return ret;
}

//-------------------------------------------------------------------------------

const char* read_entire_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) return NULL;

    fseek(file, 0, SEEK_END);
    usize length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* buffer = (char*)malloc(length + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, 1, length, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

//-------------------------------------------------------------------------------

typedef struct Tokenizer {
    StringView sv;
    usize line;
} Tokenizer;

typedef enum TokenType {
    T_Invalid,

    T_Identifier,
    T_Integer,
    T_Label,
    T_If,
    T_Io8,
    T_Io16,
    T_U8,
    T_U16,

    T_Assign,        // '='
    T_Plus,          // '+'
    T_Minus,         // '-'
    T_Colon,         // ':'
    T_Hash,          // '#'
    T_OpenBracket,   // '['
    T_CloseBracket,  // ']'
    T_Comma,         // ','
    T_Eq,            // ==

    T_UGt,           // '>u' unsigned
    T_ULt,           // '<u' unsigned
    T_UGte,          // '>=u' unsigned
    T_ULte,          // '<=u' unsigned
    
    T_SGt,           // '>' signed
    T_SLt,           // '<' signed
    T_SGte,          // '>=' signed
    T_SLte,          // '<=' signed
} TokenType;

typedef struct Token {
    TokenType type;
    StringView identifier;
    u32 integer;
    usize line;
} Token;

Tokenizer tokenizer_from(StringView sv) {
    return (Tokenizer) {
	.sv = sv,
	.line = 0,
    };
}

char tokenizer_curr(Tokenizer *tokenizer) {
    if (tokenizer->sv.length < 0) return CNULL;
    return tokenizer->sv.data[0];
}

char tokenizer_peek(Tokenizer *tokenizer) {
    if (tokenizer->sv.length < 1) return CNULL;
    return tokenizer->sv.data[1];
}

char tokenizer_advance(Tokenizer *tokenizer) {
    if (tokenizer->sv.length < 1) return CNULL;
    sv_cut_left(&tokenizer->sv, 1);
    return tokenizer->sv.data[0];
}

#define T_RETURN_INVALID_IF_NOTHING              \
    if (tokenizer_curr(tokenizer) == CNULL) {    \
	return (Token) { .type = T_Invalid };    \
    }

Token tokenizer_identifier(Tokenizer *tokenizer) {
    T_RETURN_INVALID_IF_NOTHING
    while (isspace(tokenizer_curr(tokenizer))) {
	if (tokenizer_curr(tokenizer) == '\n') {
	    tokenizer->line += 1;
	}
	tokenizer_advance(tokenizer);
    }
    T_RETURN_INVALID_IF_NOTHING

    int length = 0;
    Token tok = (Token) { .type = T_Identifier, .line = tokenizer->line };
    tok.identifier = (StringView) {
	.data = tokenizer->sv.data
    };

    while (isalnum(tokenizer_curr(tokenizer))) {
	length ++;
	tokenizer_advance(tokenizer);
    }
    tok.identifier.length = length;

    if (length == 2) {
	if (memcmp(tok.identifier.data, "u8", 2) == 0) {
	    tok.type = T_U8;
        } else if (memcmp(tok.identifier.data, "if", 2) == 0) {
	    tok.type = T_If;
	}
    }
    else if (length == 3) {
	if (memcmp(tok.identifier.data, "u16", 3) == 0) {
	    tok.type = T_U16;
	}
	else if (memcmp(tok.identifier.data, "io8", 3) == 0) {
	    tok.type = T_Io8;
	}
    }
    else if (length == 4 && memcmp(tok.identifier.data, "io16", 4) == 0) {
	tok.type = T_Io16;
    }

    return tok;
}

Token tokenizer_integer(Tokenizer *tokenizer) {
    T_RETURN_INVALID_IF_NOTHING
    while (isspace(tokenizer_curr(tokenizer))) {
	if (tokenizer_curr(tokenizer) == '\n') {
	    tokenizer->line += 1;
	}
	tokenizer_advance(tokenizer);
    }
    T_RETURN_INVALID_IF_NOTHING

    Token tok = (Token) { .type = T_Integer, .line = tokenizer->line };
    StringView sv = {
	.data = tokenizer->sv.data,
    };

    if (tokenizer_curr(tokenizer) == '0') {
	// hexadecimal
	if (tokenizer_peek(tokenizer) == 'x') {
	    tokenizer_advance(tokenizer); // skip 0
	    tokenizer_advance(tokenizer); // skip x
	    while (isxdigit(tokenizer_curr(tokenizer))) {
		tokenizer_advance(tokenizer);
	    }
	    sv.length = (usize)(tokenizer->sv.data - sv.data);
	    tok.integer = sv_to_int(&sv, "%x");
	    return tok;
	}
    }
    if (isdigit(tokenizer_curr(tokenizer))) {
	while (isdigit(tokenizer_curr(tokenizer))) {
	    tokenizer_advance(tokenizer);
	}
	sv.length = (usize)(tokenizer->sv.data - sv.data);
	tok.integer = sv_to_int(&sv, "%d");
	return tok;
    }
    return (Token){ .type = T_Invalid };
}

Token tokenizer_get_token(Tokenizer *tokenizer) {
    T_RETURN_INVALID_IF_NOTHING
    while (isspace(tokenizer_curr(tokenizer))) {
	if (tokenizer_curr(tokenizer) == '\n') {
	    tokenizer->line += 1;
	}
	tokenizer_advance(tokenizer);
    }

    // let's skip all the comments
    if (tokenizer_curr(tokenizer) == ';') {
	char current = tokenizer_advance(tokenizer);
	while (current != '\n') {
	    current = tokenizer_advance(tokenizer);
	    T_RETURN_INVALID_IF_NOTHING
        }
        tokenizer->line += 1;
	tokenizer_advance(tokenizer);
    }
    while (isspace(tokenizer_curr(tokenizer))) {
	if (tokenizer_curr(tokenizer) == '\n') {
	    tokenizer->line += 1;
	}
	tokenizer_advance(tokenizer);
    }
    T_RETURN_INVALID_IF_NOTHING

    char first = tokenizer_curr(tokenizer);

    if (first == '+') {
	tokenizer_advance(tokenizer);
        return (Token){ .type = T_Plus, .line = tokenizer->line };
    }
    if (first == '-') {
	tokenizer_advance(tokenizer);
        return (Token){ .type = T_Minus, .line = tokenizer->line };
    }
    if (first == ':') {
	tokenizer_advance(tokenizer);
        return (Token){ .type = T_Colon, .line = tokenizer->line };
    }
    if (first == '#') {
	tokenizer_advance(tokenizer);
        return (Token){ .type = T_Hash, .line = tokenizer->line };
    }
    if (first == '[') {
	tokenizer_advance(tokenizer);
        return (Token){ .type = T_OpenBracket, .line = tokenizer->line };
    }
    if (first == ']') {
	tokenizer_advance(tokenizer);
        return (Token){ .type = T_CloseBracket, .line = tokenizer->line };
    }
    if (first == ',') {
	tokenizer_advance(tokenizer);
        return (Token){ .type = T_Comma, .line = tokenizer->line };
    }

    if (first == '=') {
	tokenizer_advance(tokenizer);
	if (tokenizer_curr(tokenizer) == '=') {
	    tokenizer_advance(tokenizer);
	    return (Token) { .type = T_Eq, .line = tokenizer->line };
	}
	return (Token) { .type = T_Assign, .line = tokenizer->line };
    }

    // >u, >, >=u, >=
    if (first == '>') {
	char second = tokenizer_peek(tokenizer);
	tokenizer_advance(tokenizer);

	// >u
	if (second == 'u') {
	    tokenizer_advance(tokenizer);
	    return (Token) { .type = T_UGt, .line = tokenizer->line };
	}
	// >=, >=u
	if (second == '=') {
	    tokenizer_advance(tokenizer);

	    // >=u
	    if ((tokenizer_curr(tokenizer) == 'u')) {
		tokenizer_advance(tokenizer);
		return (Token) { .type = T_UGte, .line = tokenizer->line };
	    }

	    // >=
	    return (Token) { .type = T_SGte, .line = tokenizer->line };
	}
	// >
	return (Token) { .type = T_SGt, .line = tokenizer->line };
    }

    // <u, <, <=u, <=
    if (first == '<') {
	char second = tokenizer_peek(tokenizer);
	tokenizer_advance(tokenizer);

	// <u
	if (second == 'u') {
	    tokenizer_advance(tokenizer);
	    return (Token) { .type = T_ULt, .line = tokenizer->line };
	}
	// <=, <=u
	if (second == '=') {
	    tokenizer_advance(tokenizer);

	    // <=u
	    if ((tokenizer_curr(tokenizer) == 'u')) {
		tokenizer_advance(tokenizer);
		return (Token) { .type = T_ULte, .line = tokenizer->line };
	    }

	    // <=
	    return (Token) { .type = T_SLte, .line = tokenizer->line };
	}
	// <
	return (Token) { .type = T_SLt, .line = tokenizer->line };
    }

    if (first == '@') {
	Token tok = (Token) { .type = T_Label, .line = tokenizer->line };
	tokenizer_advance(tokenizer);
	Token identifier = tokenizer_identifier(tokenizer);
	tok.identifier = identifier.identifier;
	return tok;
    }

    if (isdigit(first)) {
	return tokenizer_integer(tokenizer);
    }
    if (isalpha(first)) {
	return tokenizer_identifier(tokenizer);
    }

    printf("reached end of all things\n");
    printf("first: %c\n", first);
    printf("curr: %c\n", tokenizer_curr(tokenizer));
    return (Token){ .type = T_Invalid, .line = tokenizer->line };
}

void tokenizer_print(Token token) {
    printf("[%td] ", token.line);

    switch (token.type) {

    case T_Identifier: {
	printf("("SV_FMT")\n", SV_ARG(token.identifier));
    }; break;

    case T_Integer: {
	printf("(%d)\n", token.integer);
    }; break;

    case T_Label: {
	printf("{"SV_FMT"}\n", SV_ARG(token.identifier));
    }; break;

    case T_If: { printf("[if]\n"); }; break;
    case T_Io8: { printf("[io8]\n"); }; break;
    case T_Io16: { printf("[io16]\n"); }; break;
    case T_U8: { printf("[u8]\n"); }; break;
    case T_U16: { printf("[u16]\n"); }; break;
    case T_Assign: { printf("=\n"); }; break;
    case T_Plus: { printf("+\n"); }; break;
    case T_Minus: { printf("-\n"); }; break;
    case T_Colon: { printf(":\n"); }; break;
    case T_Hash: { printf("#\n"); }; break;
    case T_OpenBracket: { printf("[\n"); }; break;
    case T_CloseBracket: { printf("]\n"); }; break;
    case T_Comma: { printf(",\n"); }; break;
    case T_Eq: { printf("==\n"); }; break;
    case T_UGt: { printf(">u\n"); }; break;
    case T_ULt: { printf("<u\n"); }; break;
    case T_UGte: { printf(">=u\n"); }; break;
    case T_ULte: { printf("<=u\n"); }; break;
    case T_SGt: { printf(">\n"); }; break;
    case T_SLt: { printf("<\n"); }; break;
    case T_SGte: { printf(">=\n"); }; break;
    case T_SLte: { printf("<=\n"); }; break;

    case T_Invalid: printf("invalid token\n"); break;

    }
}

//-------------------------------------------------------------------------------

typedef enum Register {
    AX, BX, CX, DX,
    SI, DI, BP, SP,
    AL, AH, BL, BH,
    CL, CH, DL, DH,
    CS, DS, ES, SS,
} Register;

typedef enum OperandType {
    OP_Register,
    OP_Memory,
    OP_Immediate,
} OperandType;

typedef struct Operand {
    OperandType type;
    union {
	s32 regid;
        u32 imm;
        struct {
	    int segment;
	    int base;
	    int index;
	    int disp;
	    int width;
	} mem;
    } data;
} Operand;

typedef enum InstructionKind {
    I_MOV,
    I_MATH,
    I_LABEL,
    I_GOTO,
    I_IF_GOTO,
    I_IO,
    // Directive
} InstructionKind;

typedef enum ALUOp {
    A_ADD,
    A_SUB,
    A_MUL,
    A_DIV,
    A_SHL,
    A_SHR,
} ALUOp;

typedef struct Instruction {
    InstructionKind kind;
    u16 address;
    u8 size;
    union {
	struct {
	    Operand src;
	    Operand dst;
	} mov;

	struct {
	    ALUOp operation;
	    Operand src;
	    Operand dst;
	} alu;

    } instr;
} Instruction;

//-------------------------------------------------------------------------------

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    const char* filename = "test.asm";
    const char* data = read_entire_file(filename);

    StringView sv_data = sv_from(data);
    Tokenizer tokenizer = tokenizer_from(sv_data);

    Token tok = tokenizer_get_token(&tokenizer);
    while (tok.type != T_Invalid)
    {
	tokenizer_print(tok);
	tok = tokenizer_get_token(&tokenizer);
    }

    free((void*)data);
}
