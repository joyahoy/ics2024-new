/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

static char *buf_start = buf;
static char *buf_end = buf+(sizeof(buf)/sizeof(buf[0]));

static int choose(int n) {
	  return rand() % n;
}

static void gen_space() {
  int size = choose(4);
  if (buf_start < buf_end) {
    int available = buf_end - buf_start;
    int n_writes = snprintf(buf_start, available, "%*s", size, "");
    if (n_writes > 0) {
        int actual = n_writes < available ? n_writes : available - 1;
        buf_start += actual;
    }
  }
}

static void gen_num() {
  int num = choose(INT8_MAX);
  if (buf_start < buf_end) {
    int available = buf_end - buf_start;
    int n_writes = snprintf(buf_start, available, "%d", num);
    if (n_writes > 0) {
        int actual = n_writes < available ? n_writes : available - 1;
        buf_start += actual;
    }
  }
  gen_space();
}

static void gen_char(char c) {
  if (buf_start < buf_end) {
    int available = buf_end - buf_start;
    int n_writes = snprintf(buf_start, available, "%c", c);
    if (n_writes > 0) {
        int actual = n_writes < available ? n_writes : available - 1;
        buf_start += actual;
    }
  }
}

static char* ops[] = {"&&", "==", "!=", "+", "-", "*", "/"};
static void gen_rand_op(){
	int op_index = choose(sizeof(ops)/sizeof(ops[0]));
	char* op = ops[op_index];
	if(buf_start < buf_end) {
		int available = buf_end - buf_start;
		int n_writes = snprintf(buf_start, available, "%s", op);
		if(n_writes > 0) {
			int actual = n_writes < available ? n_writes : available - 1;
			buf_start += actual;
		}
	}	
}
//疑问： 如果递归都是第三种，递归的深度
#define MAX_DEPTH 100
static void gen_rand_expr(int depth) {
	if (depth > MAX_DEPTH) {
    gen_num();
    return;
  }
  switch (choose(3)) {
    case 0: gen_num(); break;
    case 1: gen_char('('); gen_rand_expr(depth + 1); gen_char(')'); break;
    default:
      gen_rand_expr(depth + 1);
      gen_rand_op();
      gen_rand_expr(depth + 1);
      break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
		buf_start = buf; // 重置缓冲区指针
    gen_rand_expr(0);

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);
		// filter div-by-zero expressions
    int ret = system("gcc /tmp/.code.c -Wall -Werror -Wdiv-by-zero -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}