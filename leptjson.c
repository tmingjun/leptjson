#include "leptjson.h"
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>

#define EXPECT(c,ch)  do{ assert(*c->json == (ch)); c->json++; }while(0)

typedef struct {
    const char* json;
}lept_context;

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
    v->n = strtod(c->json, NULL);
    if(errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
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
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

/* 解析接口函数 */
int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
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
    return ret;
}

/* 查看节点值类型 */
lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

/* 获取数值的API */
double lept_get_number(const lept_value * v)
{
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}