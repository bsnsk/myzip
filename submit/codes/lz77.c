//
//  lz77.c
//  myzip
//
//  Created by Inability on 14-5-5.
//  Copyright (c) 2014年 Inability. All rights reserved.
//

#include "lz77.h"

int window_size;
int curbyte;
int curbit;

int NO_USE_BAD, NO_USE_GOOD;

inline int reverse(int n){
	union {
		int _int;
		uchar _uchar[4];
	}tmp;
	uchar _swp;
	tmp._int = n;
	_swp = tmp._uchar[0]; tmp._uchar[0]=tmp._uchar[3]; tmp._uchar[3]=_swp;
	_swp = tmp._uchar[1]; tmp._uchar[1]=tmp._uchar[2]; tmp._uchar[2]=_swp;
	return tmp._int;
}

int lower_log2(int n){ 
	int re=0;
	while ((1<<(re+1))<=n)
		re++;
	return re;
}

int upper_log2(int n){
	int re=0;
	while ((1<<re)<=n)
		re++;
	return re;
}

inline void move_forward(int *bytes, int *bits, int num){
	num += (*bits);
	(*bytes) += (num>>3);
	(*bits) = (num&7);
}

inline void copy_bits_in_byte(uchar *des, int des_pos, uchar *src, int src_pos, int bits){
	int t = (*src) & ((1<<(8-src_pos))-1);
	if (8-src_pos < bits)
		t = (t<<(bits-8+src_pos)) |   (src[1]>>(16-bits-src_pos)) ;
	else
		t = ( t >> (8-src_pos-bits) );
	if (8-des_pos >= bits)
		(*des) = ( (*des) & (~((1<<(8-des_pos))-1)) ) ^ (t << (8-des_pos-bits));
	else {
		(*des) = ( (*des) & (~((1<<(8-des_pos))-1)) ) ^ (t >> (bits+des_pos-8));
		des[1] = ( t & ((1<<(bits+des_pos-8))-1) ) << (16-bits-des_pos);
	}
}

void copy_bits(uchar *des, int des_pos, uchar *src, int src_pos, int bits){
	int left_bits, cur_bits;
	int src_bytes=0, src_bits=src_pos;
	int des_bytes=0, des_bits=des_pos;
	while (bits) {
		left_bits = calc_min(bits, 8-des_bits);
		cur_bits = calc_min(left_bits, 8-src_bits);
		
		copy_bits_in_byte(des + des_bytes, des_bits, src + src_bytes, src_bits, cur_bits);
		if (left_bits > cur_bits) {
			src_bytes ++;
			src_bits = 0;
			move_forward(&des_bytes, &des_bits, cur_bits);
			copy_bits_in_byte(des + des_bytes, des_bits, src + src_bytes, src_bits, left_bits - cur_bits);
			move_forward(&src_bytes, &src_bits, left_bits - cur_bits);
			move_forward(&des_bytes, &des_bits, left_bits - cur_bits);
		}
		else {
			move_forward(&des_bytes, &des_bits, cur_bits);
			move_forward(&src_bytes, &src_bits, cur_bits);
		}
		bits -= left_bits;
	}
}

void print_code(uchar *des, int code, int bits, uchar gamma_code){
	unsigned tmp;
	if (gamma_code) {
		int q = lower_log2(code); // code has q+1 bits
		//if (2*q+1>5)	NO_USE_BAD++;
		//else if (2*q+1<5) NO_USE_GOOD ++;
		if (q>0) {
			tmp=0;
			copy_bits(des + curbyte, curbit, (uchar*)&tmp, 0, q);
			move_forward(&curbyte, &curbit, q);
		}
		tmp = 0xffffffff;
		copy_bits(des + curbyte, curbit, (uchar*)&tmp, 0, 1);
		move_forward(&curbyte, &curbit, 1);
		if (q>0) {
			tmp = reverse(code);
			copy_bits(des + curbyte, curbit, ((uchar*)&tmp) + (32-q)/8, (32-q)&7, q);
			move_forward(&curbyte, &curbit, q);
		}
	}
	else {
		tmp = reverse(code);
		copy_bits(des + curbyte, curbit, ((uchar*)&tmp) + (32-bits)/8, (32-bits)&7, bits);
		move_forward(&curbyte, &curbit, bits);
	}
}

// 1 -- src is less than key
// 2 -- src is greater than key
// 0 -- the same
uchar less(uchar *src, int len, uchar *key, int *most_match) {
	int i;
	for (i=0; i<len && key[i]; i++)
		if (src[i] != key[i]) {
			if (most_match)
				*most_match = i;
			return (src[i] < key[i]) ? 1 : 2;
		}
	if (most_match)
		*most_match = i;
	if (i==max_match_len || (i==len && !key[i]))
		return 0;
	return key[i] ? 1 : 2;
}

inline void right_rotate(struct tree_node ** x){
	struct tree_node *t = (*x)->left;
	(*x)->left = t->right;
	t->right = *x;
	*x = t;
}

inline void left_rotate(struct tree_node ** x){
	struct tree_node *t = (*x)->right;
	(*x)->right = t->left;
	t->left = *x;
	*x = t;
}

void index_insert_at_node(struct tree_node **root, uchar *src, int start_pos){
	if (!(*root)) {
		*root = (struct tree_node *) malloc(sizeof(struct tree_node));
		(*root)->left = (*root)->right = 0;
		(*root)->off = start_pos;
		(*root)->ran = rand()%256;
		memset((*root)->key, 0, sizeof((*root)->key));
		memcpy((*root)->key, src+start_pos, calc_min(max_match_len, window_size-start_pos) * sizeof(uchar));
		return;
	}
	uchar t = less(src+start_pos, calc_min(max_match_len, window_size-start_pos), (*root)->key, 0);
	if (t==1) {
		index_insert_at_node(&((*root)->left), src, start_pos);
		if ((*root)->left->ran > (*root)->ran)
			right_rotate(root);
	}
	else if (t==2){
		index_insert_at_node(&((*root)->right), src, start_pos);
		if ((*root)->right->ran > (*root)->ran)
			left_rotate(root);
	}
}

void index_insert(uchar *src, int start_pos){
	index_insert_at_node(&root, src, start_pos);
}

void index_clear_node(struct tree_node ** node){
	if ((*node)->left)
		index_clear_node(&((*node)->left));
	if ((*node)->right)
		index_clear_node(&((*node)->right));
	free(*node);
	*node=0;
}

void index_clear(){
	if (root)
		index_clear_node(&root);
}

int gamma_decode(uchar *des, int *curbyte, int *curbit){
	int flag, tmp;
	int q = -1;
	do {
		q++;
		flag = ( des[*curbyte] >> (7-*curbit) )&1;
		move_forward(curbyte, curbit, 1);
	} while (!flag); // q zeros
	if (q>0) {
		tmp = 0;
		copy_bits(((uchar*)&tmp) + (32-q)/8, (32-q)&7, des + *curbyte, *curbit, q);
		move_forward(curbyte, curbit, q);
		tmp = reverse(tmp);
		tmp += (1<<q);
	}
	else
		tmp = 1;
	return tmp;
}

int decode(uchar *des, int *curbyte, int *curbit, int bits){
	int tmp = 0;
	copy_bits(((uchar*)&tmp) + (32-bits)/8, (32-bits)&7, des + *curbyte, *curbit, bits);
	move_forward(curbyte, curbit, bits);
	return reverse(tmp);
}

uchar seek_phase(uchar *src, int src_len, int start_pos, int *off, int *len){
	if (!root)
		return 0;
	struct tree_node * p = root;
	int now=0, till_now=0, t;
	for (; p; ) {
		t = less(src+start_pos, calc_min(max_match_len, src_len-start_pos), p->key, &now);
		if (now > till_now) {
			till_now = now;
			*len = now;
			*off = p->off;
		}
		if (t==1)
			p=p->left;
		else if (t==2)
			p=p->right;
		else break;
	}
	return till_now >= min_match_len;
}

void window_slide(uchar *src, int n){
	int i;
	for (i=0; i<n; i++) {
		window_size ++;
		if (window_size >= min_match_len)
			index_insert(src, window_size - min_match_len);
	}
}

void compress_lz_str(uchar *src, int src_len, uchar *des, int *des_len){
	assert(src_len <= max_window_size);
	curbyte = 0;
	curbit = 0;
	window_size = 0;
	int off, len, i;
	
	for (i=0; i<src_len; ) {
		if (seek_phase(src, src_len, i, &off, &len)) {
			// 1(1bit) + len(gamma) + offset(16bit)
			print_code(des, 1, 1, 0);
			print_code(des, len, 0, 1);
			print_code(des, off, upper_log2(window_size), 0);
			
			window_slide(src, len);
			i += len;
			
		}
		else {
			// 0(1bit) + uchar(8bit)
			print_code(des, 0, 1, 0);
			print_code(des, src[i], 8, 0);
			
			window_slide(src, 1);
			i++;
		}
		assert(curbyte>=0);
	}
	*des_len = curbyte + (curbit>0?1:0);
}

char compress_lz_file(char *src, char *src_name, char *des, uchar app){
	
	fprintf(stderr, "\ncompressing %s to %s (lz77)...\n", src, des);

	//NO_USE_BAD = NO_USE_GOOD = 0;
	
	FILE *in = fopen(src, "rb");
	FILE *ou;
	if (app)
		ou = fopen(des, "ab");
	else
		ou = fopen(des, "wb");
	if (!in || !ou) {
		fprintf(stderr, "file error!\n");
		return 0;
	}
	
	fseek(in, 0, SEEK_END);
	int last = (int)ftell(in);
	fseek(in, 0, SEEK_SET);
	
	uchar * buff = (uchar *) malloc(sizeof(uchar) * max_window_size);
	uchar * des_buff = (uchar *) malloc(sizeof(uchar) * (max_window_size *2));
	
	int flag, cur, tmp;
	
	// print file_name
	tmp = (int)strlen(src_name);
	fwrite(&tmp, sizeof(int), 1, ou);
	fwrite(src_name, strlen(src_name) * sizeof(char), 1, ou);
	fwrite(&last, sizeof(int), 1, ou);
	
	for (; last > 0; last -= cur) {
		cur = calc_min(max_window_size, last);
		fread(buff, cur, 1, in);
		flag = (cur == max_window_size ? 0 : cur);
		fwrite(&flag, sizeof(int), 1, ou); // original length
		
		//fprintf(stderr, "original length: %d\n", cur);
		
		root = 0;
		compress_lz_str(buff, cur, des_buff, &flag);
		index_clear();
		fwrite(&flag, sizeof(int), 1, ou); // compressed length
		//fprintf(stderr, "\tcompressed length: %d\n", flag);
		assert(flag>=0);
		
		fwrite(des_buff, sizeof(uchar), flag, ou);
	}
	
	//fprintf(stderr, "gamma good/bad : %d / %d\n", NO_USE_GOOD, NO_USE_BAD);
	
	free(buff);
	free(des_buff);
	
	fclose(in);
	fclose(ou);
	return 1;
}

// src -- receive original data
// des -- compressed data
char decompress_lz_str(uchar *src, int src_len, uchar *des){
	
	int i;
	curbyte = 0;
	curbit = 0;
	window_size = 0;
	
	if (src_len > max_window_size)
		return 0;
	
	for (i=0; i<src_len; ) {
		uchar flag = ( des[curbyte] >> (7-curbit) ) & 1;
		move_forward(&curbyte, &curbit, 1);
		if (!flag) { // [0(1bit) +] single uchar
			copy_bits(src + i, 0, des + curbyte, curbit, 8);
			move_forward(&curbyte, &curbit, 8);
			window_size ++;
			i++;
		}
		else {
			// [1(1bit) +] len(gamma) + offset(16bit)
			int off, len, j;
			
			// len
			//len = decode(des, &curbyte, &curbit, upper_log2(window_size));
			len = gamma_decode(des, &curbyte, &curbit);
			
			// off
			//off = gamma_decode(des, &curbyte, &curbit) -1;
			off = decode(des, &curbyte, &curbit, upper_log2(window_size));
			
			for (j=0; j<len; j++)
				src[i+j] = src[off+j];
			window_size += len;
			i += len;
		}
		
		assert(window_size <= max_window_size);
	}
	
	return 1;
}

char decompress_lz_file(char *src, int start_pos, int *end_pos){
	
	fprintf(stderr, "\ndecompressing (lz77)...\n");
	
	char output_file_name[256];
	int output_file_name_len;
	
	FILE *in = fopen(src, "rb");
	FILE *ou;
	if (!in) {
		fprintf(stderr, "file error!\n");
		return 0;
	}
	
	int last, cur, flag1, flag2;
	
	fseek(in, start_pos, SEEK_SET);
	
	fread(&output_file_name_len, sizeof(int), 1, in);
	fread(output_file_name, output_file_name_len * sizeof(char), 1, in);
	output_file_name[output_file_name_len] = '\0';
	ou = fopen(output_file_name, "wb");
	
	fread(&last, sizeof(int), 1, in);
	
	uchar * buff = (uchar *) malloc(sizeof(uchar) * max_window_size);
	uchar * des_buff = (uchar *) malloc(sizeof(uchar) * (max_window_size *2));
	
	while (last > 0) {
		fread(&flag1, sizeof(int), 1, in); // original length
		fread(&flag2, sizeof(int), 1, in); // compressed length
		
		//printf("flag 1,2 = %d,%d\n", flag1, flag2);
		
		assert(flag1>=0 && flag1<max_window_size);
		assert(flag2>0 && flag2 < max_window_size*2);
		
		cur = (flag1 ? flag1 : max_window_size);
		last -= cur;
		
		//fprintf(stderr, "\toriginal length: %d\ncompressed length: %d\n", cur, flag2);
		
		fread(des_buff, sizeof(uchar), flag2, in);
		if (!decompress_lz_str(buff, cur, des_buff)) {
			fprintf(stderr, "decompress error!\n");
			goto func_end;
		}
		fwrite(buff, cur, 1, ou);
	}
	
func_end:
	
	free(buff);
	free(des_buff);
	
	*end_pos = (int)ftell(in);
	
	fclose(in);
	fclose(ou);
	return 1;
}
