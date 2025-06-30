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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_ADD, TK_SUB, TK_MUL, TK_DIV, TK_LPAREN, TK_RPAREN
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"==", TK_EQ},        // equal
  {"[0-9]+", TK_NUM},     // number
  {"\\+", TK_ADD},         // plus
  {"\\-", TK_SUB},         // sub
  {"\\*", TK_MUL},         // multi
  {"\\/", TK_DIV},         // div
  {"\\(", TK_LPAREN},      // lparen
  {"\\)", TK_RPAREN},      // rparen
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        //默认空格不算token, 其他都加入tokens数组，所以default让nr_token ++
        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;

          case TK_ADD:
          case TK_SUB:
          case TK_MUL:
          case TK_DIV:
          case TK_LPAREN:
          case TK_RPAREN:
            tokens[nr_token].type = rules[i].token_type;

          case TK_NUM:
            tokens[nr_token].type = rules[i].token_type;
            if(substr_len > 31) panic("The number is too big!\n");
            strncpy(tokens[nr_token].str, e + position - substr_len, substr_len);  
            tokens[nr_token].str[substr_len] = '\0';

          default: nr_token ++;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

static bool check_parentheses(int p, int q) {
  int cnt_l = 0;
  //检查表达式是否正确
  //从左往右遍历， 记录左括号的数量， 遇到右括号减
  for(int i = p; i <= q ; i ++) {
    if(tokens[i].type == TK_RPAREN) {
      if(cnt_l == 0) {
        panic("parentheses is wrong\n");
      }
      cnt_l--;
    }else if(tokens[i].type == TK_LPAREN) {
      cnt_l++;
    }
  }
  if(cnt_l != 0) return false;
  
  if(tokens[p].type == TK_LPAREN && tokens[q].type == TK_RPAREN)
    return true;

  return false;
}

static int eval(int p, int q) {
  if (p > q) {
    //Bad expression
    panic("expr is wrong\n");
  }else if (p == q) {
    // Must be number
    assert(tokens[p].type == TK_NUM);
    int res = atoi(tokens[p].str);
    return res;
  }else if (check_parentheses(p, q) == true) {
    // 成立的话，就是去掉左右的括号，并且表达式内的括号正确
    return eval(p+1, q-1);
  }else {
    //主运算符
    //上面check确保了括号的正确
    int op_type = 0;
    int cnt_r = 0;
    int op_addorsub = -1, op_mulordiv = -1;
    for(int i = q; i >= p ; i --) {
      if(tokens[i].type == TK_RPAREN) {
        cnt_r ++;
        continue;
      }
      if(tokens[i].type == TK_LPAREN) {
        assert(cnt_r != 0);
        cnt_r --;
        continue;
      }
      
      //此时不在括号里
      if(cnt_r == 0) {
        if(tokens[i].type == TK_ADD || tokens[i].type == TK_SUB) {
          op_addorsub = i;
          op_type = tokens[i].type;
          break;
        }
        if(tokens[i].type == TK_MUL || tokens[i].type == TK_DIV) {
          if(op_mulordiv != -1) continue;
          op_mulordiv = i;
          op_type = tokens[i].type;
        }
      }
    }
    assert(op_addorsub != -1 || op_mulordiv != -1);
    int idx;
    if(op_addorsub != -1) {
      idx = op_addorsub;
    }else {
      idx = op_mulordiv;
    }
    int val1 = eval(p, idx -1);
    int val2 = eval(idx+1, q);
    switch(op_type) {
      case TK_ADD :
        return val1 + val2; break;
      case TK_SUB :
        return val1 - val2; break;
      case TK_MUL :
        return val1 * val2; break;
      case TK_DIV :
        return val1 / val2; break;
      default : assert(0);
    }
  }
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  //TODO();
  word_t ret = eval(0, nr_token-1);
  *success = true;

  return ret;
}
