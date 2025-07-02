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

#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
word_t expr(char *e,bool *success);

void test_expr(){
	FILE* fp = fopen("/home/sxy/ics2024/nemu/tools/gen-expr/input", "r");
	if(fp == NULL) perror("test expr error!");
	char *e = NULL;
	size_t len = 0;
	ssize_t read = 0;
	bool success = true;
	while(true){
		read = getline(&e, &len, fp);	
		if(read == -1)  {perror("getline failed"); exit(0);} 
		char *arg1 = strtok(e," ");
		word_t correct_res = 0;
		sscanf(arg1, "%u", &correct_res);

		char *arg2 = strtok(NULL,"\n");
		//test
		printf("\033[33marg1: %s arg2: %s\033[0m\n", arg1, arg2);

		word_t res = expr(arg2,&success);
		assert(success);
		if(res != correct_res){
			printf("\033[31m%s  \033[0m",arg2);
			printf("\033[31mcorrect_res: %u res: %u\033[0m\n", correct_res, res);
			assert(0);
		}else
			printf("\033[34m%s  correct_res: %u res: %u\033[0m\n",arg2, correct_res, res); 
	}
	fclose(fp);
	if(e) free(e);
}



int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif
	//test_expr();
  /* Start engine. */
  engine_start();

  return is_exit_status_bad();
}