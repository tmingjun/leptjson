#ifndef LEPTJSON_H_
#define LEPTJSON_H_

#include <stddef.h>

/* json数据类型枚举 */
typedef enum {
    LEPT_NULL,
    LEPT_FALSE,
    LEPT_TRUE,
    LEPT_NUMBER,
    LEPT_STRING,
    LEPT_ARRAY,
    LEPT_OBJECT
}lept_type;

/* json树形结构的节点类型 */
typedef struct {
    union{
        struct {
            char * s;   /* string */
            size_t len; /* str len */
        }s;
        double n;       /* number */
    }u;
    lept_type type;     /* json 节点类型 */
}lept_value;

/* 错误返回值类型 */
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,            /* 空白 */
    LEPT_PARSE_INVALID_VALUE,           /* 非法值 */
    LEPT_PARSE_ROOT_NOT_SINGULAR,       /* 非单个值 */
    LEPT_PARSE_NUMBER_TOO_BIG,          /* 数字过大 */
    LEPT_PARSE_MISS_QUOTATION_MARK,     /* 缺少引号 */
    LEPT_PARSE_INVALID_STRING_ESCAPE,   /* 非法转义字符 */
    LEPT_PARSE_INVALID_STRING_CHAR,     /* 非法字符 */
    LEPT_PARSE_INVALID_UNICODE_HEX,     /* 解析hex失败 */
    LEPT_PARSE_INVALID_UNICODE_SURROGATE
};

#define lept_init(v) do { (v)->type = LEPT_NULL;} while(0)
#define lept_set_null(v) lept_free(v)

/* 接口函数 */
int lept_parse(lept_value *v, const char * json);
void lept_free(lept_value* v);
lept_type lept_get_type(const lept_value * v);

int lept_get_boolean(const lept_value* v);
void lept_set_boolean(lept_value* v, int b);

double lept_get_number(const lept_value* v);
void lept_set_number(lept_value* v, double n);

void lept_set_string(lept_value* v, const char* s, size_t len);
size_t lept_get_string_length(const lept_value *v);
const char* lept_get_string(const lept_value *v);

#endif/* LEPTJSON_H__ */