#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t cnt = 0;
  for(const char* p = s; *p != '\0'; p ++) {
    cnt ++;
  }
  return cnt;
}

char *strcpy(char *dst, const char *src) {
  char *d = dst;
  while ((*d++ = *src++));
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t i;
  for (i = 0; i < n && src[i]; i++)
    dst[i] = src[i];
  for (; i < n; i++)
    dst[i] = '\0';
  return dst;
}

char *strcat(char *dst, const char *src) {
  char *d = dst;
  while(*d) d++;  // 找到dst结尾
  while((*d++ = *src++)); // 复制src
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while(*s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if(n == 0) return 0;
  while(--n && *s1 && *s2 && *s1 == *s2) {
    s1++;
    s2++;
  }
  return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void *memset(void *s, int c, size_t n) {
  char* dst = s;
  for(size_t i = 0; i < n; i ++) {
    dst[i] = (unsigned char) c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  char* d = dst;
  const char* s = src;

  if(d == s) {
    return dst; 
  }

  if(s < d && d < s + n) {
    s += n;
    d += n;
    while(n --) {
      *--d = *--s;
    }
  }else {
    while(n --) {
      *d++ = *s++;
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  char* dst = (char*) out;
  const char* src = (const char*) in;
  for(size_t i = 0; i < n; i ++) {
    dst[i] = src[i];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return (p1[i] > p2[i]) ? 1 : -1;
    }
  }
  return 0;
}

#endif
