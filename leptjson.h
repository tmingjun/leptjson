#ifndef LEPTJSON_H_
#define LEPTJSON_H_

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
    lept_type type;
}lept_value;

/* 错误返回值类型 */
enum {
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR
};

/* 接口函数 */
int lept_parse(lept_value *v, const char * json);
lept_type lept_get_type(const lept_value * v);


#endif/* LEPTJSON_H__ */