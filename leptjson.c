#include "leptjson.h"
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c,ch)  do{ assert(*c->json == (ch)); c->json++; }while(0)
#define PUTC(c, ch)   do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char * stack;
    size_t size, top;
}lept_context;

static void * lept_context_push(lept_context* c, size_t size) {
    void * ref;
    assert(size > 0);
    if(c->top + size >= c->size) {
        if(c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while(c->top + size >= c->size)
            c->size += c->size >> 1;
        c->stack = (char *)realloc(c->stack, c->size);
    }
    ref = c->stack + c->top;
    c->top += size;
    return ref;
}

static void * lept_context_pop(lept_context *c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

/* 解析空格和空白 */
static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/* 解析字面值 null true false */
static int lept_parse_literal(lept_context *c, lept_value *v, const char* literal, lept_type type)
{
    size_t i;
    EXPECT(c,literal[0]);
    for(i = 0; literal[i+1] != '\0'; i++)
    {
       if(c->json[i] != literal[i+1]) 
            return LEPT_PARSE_INVALID_VALUE;
    }
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

#define STRING_ERROR(ret) do { c->top = head; return ret; } while(0)

static const char* lept_parse_hex4(const char* p, unsigned* u)
{
    int i;
    char ch;
    *u = 0;
    for(i = 0; i < 4; i++)
    {
        ch = p[i];
        *u *= 16;
        if(ch >= '0' && ch <= '9') *u += (ch -'0');
        else if(ch >= 'a' && ch <= 'f') *u += ch - ('a' - 10);
        else if(ch >= 'A' && ch <= 'F') *u += ch - ('A' - 10);
        else 
            return NULL;
    }
    return (p + 4);
}

static void lept_encode_utf8(lept_context* c, unsigned u)
{
    if (u <= 0x7F) 
        PUTC(c, u & 0xFF);
    else if (u <= 0x7FF) {
        PUTC(c, 0xC0 | ((u >> 6) & 0xFF));
        PUTC(c, 0x80 | ( u       & 0x3F));
    }
    else if (u <= 0xFFFF) {
        PUTC(c, 0xE0 | ((u >> 12) & 0xFF));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
    else {
        assert(u <= 0x10FFFF);
        PUTC(c, 0xF0 | ((u >> 18) & 0xFF));
        PUTC(c, 0x80 | ((u >> 12) & 0x3F));
        PUTC(c, 0x80 | ((u >>  6) & 0x3F));
        PUTC(c, 0x80 | ( u        & 0x3F));
    }
}

static int lept_parse_string(lept_context *c, lept_value* v)
{
    size_t head = c->top, len;
    unsigned u, u2;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for(;;)
    {
        char ch = *p++;
        switch(ch) {
            case '\"':            /* 检测到字符串结束引号 */
                len = c->top - head;
                lept_set_string(v,(const char*)lept_context_pop(c,len), len);
                c->json = p;
                return LEPT_PARSE_OK;
            case '\\':             /* 转义字符检测 */
                switch(*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/' : PUTC(c, '/');  break;
                    case 'b' : PUTC(c, '\b'); break;
                    case 'f' : PUTC(c, '\f'); break;
                    case 'n' : PUTC(c, '\n'); break;
                    case 'r' : PUTC(c, '\r'); break;
                    case 't' : PUTC(c, '\t'); break;
                    case 'u' : 
                        if(!(p = lept_parse_hex4(p, &u)))
                        {
                            c->top = head;
                            return LEPT_PARSE_INVALID_UNICODE_HEX;
                        }
                        if (u >= 0xD800 && u <= 0xDBFF) { /* surrogate pair */
                            if (*p++ != '\\')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if (*p++ != 'u')
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            if (!(p = lept_parse_hex4(p, &u2)))
                                STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX);
                            if (u2 < 0xDC00 || u2 > 0xDFFF)
                                 STRING_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE);
                            u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                        }
                        lept_encode_utf8(c, u);
                        break;
                    default:        /* 非法的转义字符 */
                        c->top = head;
                        return LEPT_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            case '\0':              /* 非法异常退出 提前接触到空 */
                c->top = head;
                return LEPT_PARSE_MISS_QUOTATION_MARK;
            default:
                if(ch < 0x20)       /* 非法字符检测 */
                {
                    c->top = head;
                    return LEPT_PARSE_INVALID_STRING_CHAR;
                }
                PUTC(c,ch);
        }
    }
}

/* 解析数字 */
static int lept_parse_number(lept_context*c, lept_value* v)
{
    const char * p = c->json;
    if(*p == '-') p++;
    /* 解析整数部分 */
    if(*p == '0') p++;
    else
    {
        if(*p < '1' || *p > '9')
            return LEPT_PARSE_INVALID_VALUE;
        for(p++; *p >= '0' && *p <= '9'; p++);
    }
    /* 解析小数部分 */
    if (*p == '.')
    {
        p++;
        if (*p < '0' || *p > '9')
            return LEPT_PARSE_INVALID_VALUE;
        for(p++; *p >= '0' && *p <= '9'; p++);
    }
    /* 解析指数部分 */
    if (*p == 'e' || *p == 'E')
    {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (*p < '0' || *p > '9')
            return LEPT_PARSE_INVALID_VALUE;
        for(p++; *p >= '0' && *p <= '9'; p++);
    }
    errno = 0;
    /* 获取值 */
    v->u.n = strtod(c->json, NULL);
    if(errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

/* 解析节点值 */
static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 'n':  return lept_parse_literal(c,v,"null",LEPT_NULL);
        case 'f':  return lept_parse_literal(c,v,"false",LEPT_FALSE);
        case 't':  return lept_parse_literal(c,v,"true",LEPT_TRUE);
        default:   return lept_parse_number(c,v);
        case '"':  return lept_parse_string(c,v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

/* 解析接口函数 */
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0')
        {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void lept_free(lept_value* v) {
    assert(v != NULL);
    if(v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

/* 查看节点值类型 */
lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value* v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value* v, int b) {
    v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

/* 获取数值的API */
double lept_get_number(const lept_value * v)
{
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->u.n;
}

void lept_set_number(lept_value* v, double n)
{
    v->type = LEPT_NUMBER;
    v->u.n = n;
}

void lept_set_string(lept_value* v, const char* s, size_t len)
{
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char *)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}

size_t lept_get_string_length(const lept_value *v)
{
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

const char* lept_get_string(const lept_value* v)
{
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}