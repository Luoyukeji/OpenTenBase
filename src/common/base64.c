/*
 * base64.c
 *      提供无空白字符的 Base64 编码和解码例程。
 *
 * 版权所有 (c) 2001-2017, PostgreSQL 全球开发组
 *
 *
 * 标识
 *      src/common/base64.c
 *
 */

// 根据是否为前端应用程序（如 psql）选择包含不同的头文件
#ifndef FRONTEND
#include "postgres.h" // 后端代码包含的核心头文件
#else
#include "postgres_fe.h" // 前端代码包含的头文件
#endif

#include "common/base64.h" // 包含 Base64 编码和解码所需的公共定义

// BASE64 编码相关的定义和函数

// Base64 编码字符集
static const char _base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64 编码时的查找表，对应字符值到编码索引的映射
static const int8 b64lookup[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
};

/*
 * pg_b64_encode
 *
 * Encode into base64 the given string.  Returns the length of the encoded
 * string.
 * 将给定的二进制字符串编码为 Base64 字符串，并返回编码后字符串的长度
 */
int
pg_b64_encode(const char *src, int len, char *dst)
{
    char       *p;
    const char *s,
               *end = src + len;
    int            pos = 2;
    uint32        buf = 0;

    s = src;
    p = dst;

    while (s < end)
    {
        buf |= (unsigned char) *s << (pos << 3);
        pos--;
        s++;

        /* write it out */
        if (pos < 0)
        {
            *p++ = _base64[(buf >> 18) & 0x3f];
            *p++ = _base64[(buf >> 12) & 0x3f];
            *p++ = _base64[(buf >> 6) & 0x3f];
            *p++ = _base64[buf & 0x3f];

            pos = 2;
            buf = 0;
        }
    }
    if (pos != 2)
    {
        *p++ = _base64[(buf >> 18) & 0x3f];
        *p++ = _base64[(buf >> 12) & 0x3f];
        *p++ = (pos == 0) ? _base64[(buf >> 6) & 0x3f] : '=';
        *p++ = '=';
    }

    return p - dst;
}

/*
 * pg_b64_decode
 *
 * Decode the given base64 string.  Returns the length of the decoded
 * string on success, and -1 in the event of an error.
 *
 * 将给定的 Base64 编码字符串解码为原始的二进制字符串，并在成功时返回解码后字符串
 * 的长度，出错时返回 -1
 */
int
pg_b64_decode(const char *src, int len, char *dst)
{// #lizard forgives
    const char *srcend = src + len,
               *s = src;
    char       *p = dst;
    char        c;
    int            b = 0;
    uint32        buf = 0;
    int            pos = 0,
                end = 0;

    while (s < srcend)
    {
        c = *s++;

        /* Leave if a whitespace is found */
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            return -1;

        if (c == '=')
        {
            /* end sequence */
            if (!end)
            {
                if (pos == 2)
                    end = 1;
                else if (pos == 3)
                    end = 2;
                else
                {
                    /*
                     * Unexpected "=" character found while decoding base64
                     * sequence.
                     */
                    return -1;
                }
            }
            b = 0;
        }
        else
        {
            b = -1;
            if (c > 0 && c < 127)
                b = b64lookup[(unsigned char) c];
            if (b < 0)
            {
                /* invalid symbol found */
                return -1;
            }
        }
        /* add it to buffer */
        buf = (buf << 6) + b;
        pos++;
        if (pos == 4)
        {
            *p++ = (buf >> 16) & 255;
            if (end == 0 || end > 1)
                *p++ = (buf >> 8) & 255;
            if (end == 0 || end > 2)
                *p++ = buf & 255;
            buf = 0;
            pos = 0;
        }
    }

    if (pos != 0)
    {
        /*
         * base64 end sequence is invalid.  Input data is missing padding, is
         * truncated or is otherwise corrupted.
         */
        return -1;
    }

    return p - dst;
}

/*
 * pg_b64_enc_len
 *
 * 根据调用者提供的源字符串长度，返回使用 Base64 编码后的字符串长度。
 * 这对于估计在实际编码之前需要分配多大的缓冲区非常有用。
 */
int
pg_b64_enc_len(int srclen)
{
    /* 3 bytes will be converted to 4 */
    return (srclen + 2) * 4 / 3;
}

/*
 * pg_b64_dec_len
 *
 * 根据调用者提供的源字符串长度，返回 Base64 解码后的字符串长度。
 * 这对于估计在实际解码之前需要分配多大的缓冲区非常有用。
 */
int
pg_b64_dec_len(int srclen)
{
    return (srclen * 3) >> 2;
}
