#define _CRT_SECURE_NO_WARNINGS

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
} Tokenizer;

typedef enum TokenType {
    T_invalid,
    T_identifier,
    T_integer,
    T_assign,        // '='
    T_Plus,          // '+'
    T_Minus,         // '-'
    T_Colon,         // ':'
    T_At,            // '@'
    T_Hash,          // '#'
    T_OpenBracket,   // '['
    T_CloseBracket,  // ']'
    T_Comma,         // ','
    
} TokenType;

typedef struct Token {
    TokenType type;
    StringView identifier;
    s32 integer;
} Token;

Tokenizer tokenizer_from(StringView sv) {
    return (Tokenizer) {
	.sv = sv,
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
	return (Token) { .type = T_invalid };    \
    }

Token tokenizer_identifier(Tokenizer *tokenizer) {
    T_RETURN_INVALID_IF_NOTHING
    while (isspace(tokenizer_curr(tokenizer))) {
	tokenizer_advance(tokenizer);
    }
    T_RETURN_INVALID_IF_NOTHING

    int length = 0;
    Token tok = (Token) { .type = T_identifier };
    tok.identifier = (StringView) {
	.data = tokenizer->sv.data
    };

    while (isalnum(tokenizer_curr(tokenizer))) {
	length ++;
	tokenizer_advance(tokenizer);
    }
    tok.identifier.length = length;
    return tok;
}

Token tokenizer_integer(Tokenizer *tokenizer) {
    T_RETURN_INVALID_IF_NOTHING
    while (isspace(tokenizer_curr(tokenizer))) {
	tokenizer_advance(tokenizer);
    }
    T_RETURN_INVALID_IF_NOTHING

    Token tok = (Token) { .type = T_integer };
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
	// support other types like octals and binary?
	else {
	    return (Token){ .type = T_invalid };
	}
    }
    else if (isdigit(tokenizer_curr(tokenizer))) {
	while (isdigit(tokenizer_curr(tokenizer))) {
	    tokenizer_advance(tokenizer);
	}
	sv.length = (usize)(tokenizer->sv.data - sv.data);
	tok.integer = sv_to_int(&sv, "%d");
	return tok;
    }
    else {
	return (Token){ .type = T_invalid };
    }
}

Token tokenizer_get_token(Tokenizer *tokenizer) {
    T_RETURN_INVALID_IF_NOTHING
    while (isspace(tokenizer_curr(tokenizer))) {
	tokenizer_advance(tokenizer);
    }

    if (isdigit(tokenizer_curr(tokenizer))) {
	return tokenizer_integer(tokenizer);
    }
    if (isalpha(tokenizer_curr(tokenizer))) {
	return tokenizer_identifier(tokenizer);
    }
    return (Token){ .type = T_invalid };
}

void tokenizer_print(Token token) {
    switch (token.type) {

    case T_identifier: {
	printf("identifier: "SV_FMT"\n", SV_ARG(token.identifier));
    } break;

    case T_integer: {
	printf("integer: %d\n", token.integer);
    } break;

    case T_invalid: printf("invalid token\n"); break;

    }
}

//-------------------------------------------------------------------------------

int main(int argc, char **argv) {
    (void) argc;
    (void) argv;

    const char* filename = "test.asm";
    const char* data = read_entire_file(filename);

    StringView sv_data = sv_from(data);
    Tokenizer tokenizer = tokenizer_from(sv_data);

    Token tok = tokenizer_get_token(&tokenizer);
    while (tok.type != T_invalid)
    {
	tokenizer_print(tok);
	tok = tokenizer_get_token(&tokenizer);
    }

    free((void*)data);
}
