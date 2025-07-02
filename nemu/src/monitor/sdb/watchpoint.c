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

#include "sdb.h"

#define NR_WP 32

typedef struct watchpoint {
  int NO;
  struct watchpoint *next;

  /* TODO: Add more members if necessary */
	char *expr;
	word_t pre_expr_val;
} WP;

static WP wp_pool[NR_WP] = {};           //定义了监视点结构的池wp_pool
static WP *head = NULL, *free_ = NULL;        //head用于组织使用中的监视点结构, free_用于组织空闲的监视点结构

word_t expr(char *e, bool *success);

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
		wp_pool[i].expr = NULL;
		wp_pool[i].pre_expr_val = 0;
  }

  head = NULL;    //没有采用dummy，比较麻烦，需要判断head是否非空
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

void new_wp(char *e){                              //new_wp()从free_链表中返回一个空闲的监视点结构,
	if(free_ == NULL) panic("No free watchpoints available");	

	WP *new_node = free_;
	free_ = free_->next;

	new_node->expr = strdup(e); 	                            //求表达式的值
	bool success = true;
	new_node->pre_expr_val = expr(e,&success);
	if (!success) {
		  panic("Failed to evaluate expression");
	}
	//test
	printf("expr: %s\n", new_node->expr);			

	// 将新节点添加到 head 链表中
	new_node->next = NULL;
	if(head == NULL){
		head = new_node;
	}else{
		WP *tmp = head;
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = new_node;
	}
}

void free_wp(int NO){            //free_wp()将wp归还到free_链表中
	//test
	printf("NO : %d\n", NO);
	// 找到编号为 NO 的监视点
	if (head == NULL) {
		panic("No watchpoints to free");
	}	
	WP *wp = head;
	while(wp != NULL && wp->NO != NO){
		wp = wp->next;
	}
	if (wp == NULL) {
	  panic("Watchpoint not found");
  }
	// 释放 expr 的内存
	if (wp->expr != NULL) {
		free(wp->expr);
		wp->expr = NULL;
  }
	WP *tmp = head;                //从head中去除该wp
	WP *pre = NULL;
	while(tmp != NULL && tmp->NO != wp->NO){
		pre = tmp;
		tmp = tmp->next;
	}
	if(pre == NULL) head = tmp->next;   
	else	pre->next = tmp->next;			
	// 将该监视点归还到 free_ 链表中
	if(free_ == NULL){             //1. free_ = NULL
		free_ = wp;
		wp->next = NULL;
	}else	
	if(free_->NO > wp->NO){ //2.  大  
		wp->next = free_;
		free_ = wp;	
	}else{                 //3.  小
		tmp = free_;
		pre = NULL;
		while( tmp != NULL && tmp->NO < wp->NO){         // 注意遍历中tmp可能会到NULL
			pre = tmp;
			tmp = tmp->next;
		}			
		if(pre != NULL){
			pre->next = wp;
		}else{
			free_ = wp;
		}
		wp->next = tmp;
	}	
}

void traver_trace_diff(){
	//test
//	printf("here traver_trace_diff\n");
	WP *tmp = head;
	if(head == NULL) return;
	while(tmp != NULL){
		bool success = true;
		word_t val = expr(tmp->expr,&success);
		if(val != tmp->pre_expr_val){
			nemu_state.state = NEMU_STOP;
			printf("Watchpoint %d: %s\n", tmp->NO, tmp->expr);
			printf("Old value = %u\n", tmp->pre_expr_val);
			printf("New value = %u\n", val);
			tmp->pre_expr_val = val;
		}			
		tmp = tmp->next;
	}	
}

void watchpoint_display(){
	WP *tmp = head;
	if(head == NULL){ printf("No watchpoint\n"); return; }
	while(tmp != NULL){
		printf("Watchpoint %d : %s value: %u\n", tmp->NO, tmp->expr, tmp->pre_expr_val);
		tmp = tmp->next;
	}	
}
