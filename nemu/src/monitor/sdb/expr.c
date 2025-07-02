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


word_t isa_reg_str2val(const char *s, bool *success);
word_t paddr_read(paddr_t addr, int len);

enum {
  TK_NOTYPE = 256, TK_EQ, TK_NUM, TK_REG, TK_DEREF, TK_HEX, TK_NEQ, TK_AND
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
  {"^==", TK_EQ},        // equal
	{"^!=", TK_NEQ},     // not equal
	{"^&&", TK_AND},     // and
	{"^0x[a-fA-F0-9]+", TK_HEX},   // hex 排在number的前面，避免regex解析错误
	{"[0-9]+", TK_NUM},     // number
	{"^\\$+[a-z0-9]+", TK_REG},	// reg
	  {"\\+", '+'},         // plus
	{"\\-", '-'},					 // sub
	{"\\*", '*'},          // mult
	{"\\/", '/'},          // DIVISION
	{"\\(", '('},         // lparen
	{"\\)", ')'},         // rparen
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

static Token tokens[65536] __attribute__((used)) = {};
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

        switch (rules[i].token_type) {
					case '+':
					case '-':
					case '*':
					case '/':
					case TK_EQ:
					case TK_NEQ:
					case TK_AND:
					case ')':
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					//实现可以省略乘号,在左括号左边的数字
					case '(':
						if(nr_token != 0 && tokens[nr_token-1].type == TK_NUM) tokens[nr_token++].type = '*';
						tokens[nr_token].type = rules[i].token_type;
						nr_token++;
						break;
					//reg,num,hex
				  case TK_REG:			
					case TK_NUM:
					case TK_HEX:
						tokens[nr_token].type = rules[i].token_type;
						strncpy(tokens[nr_token].str,substr_start,substr_len);
						tokens[nr_token].str[substr_len] = '\0';
						nr_token++;
						break;
					//过滤空格
					case TK_NOTYPE: break;
          default:
					 	panic("Bad match token_type"); 
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

//去掉整体表达式的左右括号（如果有的话）
//检查括号是否匹配
bool check_parentheses(int p,int q){
	if(tokens[p].type != '(' || tokens[q].type != ')') return false;
	
	int stk = 0;
	for (int i = p+1; i < q; i++){
		if(tokens[i].type == '(') stk++;
		if(tokens[i].type == ')'){
			if(--stk < 0) return false;
		}
	}

	return stk==0;
}

int eval(int p,int q){
	if(p>q){
		panic("p>q");
	}
	else if(p == q){         //This is a number or a reg  读数字或者从寄存器里读数值
			if(tokens[p].type == TK_REG){
			bool success = true;
			int res = isa_reg_str2val(tokens[p].str,&success); 
			Assert(success,"reg读值不成功");
			return res;
		}
		int val = 0;
		sscanf(tokens[p].str,"%d",&val);
		return val;	
	}
	else if(check_parentheses(p, q) == true){  // 括号匹配检查
		return eval(p+1,q-1);
	}
	else if(p + 1 == q){    // 从内存中读取数值
		if(tokens[p].type == TK_DEREF && tokens[q].type == TK_HEX){
			paddr_t addr = 0;
			sscanf(tokens[q].str, "%x", &addr);
			return paddr_read(addr,4);	
		}
		else{
			printf("%d %d\n", tokens[p].type, tokens[q].type);
			panic("expression wrong");
		}	
	}
	else{
		// 找op，从右往左，如果遇到括号，则忽略括号里面的东西，
		//array  0 AND 1 eq_neq 2 add_sub 3 mul_div
		int array[4]; 
		memset(array, -1, sizeof(array));
		int cnt_lparen = 0;
		for(int i=p;i<=q;i++){
			if(tokens[i].type == '(') cnt_lparen++;
			else if(tokens[i].type == ')') cnt_lparen--;
			if(cnt_lparen == 0){
				if( tokens[i].type == TK_AND ) { array[0] = i; }
				if( tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ ) { array[1] = i; }
				if( tokens[i].type == '+' || tokens[i].type == '-' ) { array[2] = i; }
				if( tokens[i].type == '*' || tokens[i].type == '/' ) { array[3] = i; }
			}		
		}	
		int op = -1;
		for(int i = 0;i<(int)sizeof(array);i++){
			if(array[i] != -1){
				op = array[i];
				break;
			}	
		}
		int val1 = eval(p,op-1);
		int val2 = eval(op+1,q);
		switch(tokens[op].type){
			case TK_AND: return val1 && val2;break;
			case TK_EQ: return val1 == val2;break;
			case TK_NEQ: return val1 != val2;break;
			case '+': return val1+val2;break;
			case '-': return val1-val2;break;
			case '*': return val1*val2;break;
			case '/': return val1/val2;break;
			default: assert(0);
		}
	}
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
	//DEREF (* the address of memory) 
	for(int i = 0;i<nr_token;i++){
		if(tokens[i].type == '*' && (i == 0 || tokens[i-1].type == '(')){
			tokens[i].type = TK_DEREF;
			printf("tokens[%d].type = TK_DEREF\n", i);
		}
	}

	word_t res = (word_t)eval(0,nr_token-1);
  *success = true;
	return res;
}