//
//  lz77.h
//  myzip
//
//  Created by Inability on 14-5-5.
//  Copyright (c) 2014年 Inability. All rights reserved.
//

#ifndef myzip_lz77_h
#define myzip_lz77_h

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include <time.h>

#define max_window_size 65536
#define min_match_len 3
#define max_match_len 20

#define max_child 256

#define calc_min(a,b) (((a)<(b))?(a):(b))
#define calc_max(a,b) (((a)>(b))?(a):(b))

#define swap2(a,b,c) {(c)=(a); (a)=(b); (b)=(c);}

typedef unsigned char uchar;

int reverse(int n);

int upper_log2(int n);
int lower_log2(int n);

void copy_bits_in_byte(uchar *des, int des_pos, uchar *src, int src_pos, int bits);
void copy_bits(uchar *des, int des_pos, uchar *src, int src_pos, int bits);

struct tree_node {
	uchar key[max_match_len];
	struct tree_node *left, *right;
	int off;
	uchar ran;
} *root;


uchar less(uchar *src, int len, uchar *key, int *most_match);

void index_clear_node(struct tree_node ** node);
void index_clear();


void left_rotate(struct tree_node ** x);
void right_rotate(struct tree_node ** x);

void index_insert_at_node(struct tree_node **root, uchar *src, int start_pos);
void index_insert(uchar *src, int start_pos);
void index_copy(uchar *des, uchar *src, int start_pos, int len);
uchar seek_phase(uchar *src, int src_len, int start_pos, int *off, int *len);

void window_slide(uchar *src, int n);

void move_forward(int *bytes, int *bits, int num);

void print_code(uchar *des, int code, int bits, uchar gamma_code);
int gamma_decode(uchar *des, int *curbyte, int *curbit);
int decode(uchar *des, int *curbyte, int *curbit, int bits);

void compress_lz_str(uchar *src, int src_len, uchar *des, int *des_len);
char compress_lz_file(char *src, char *src_name, char *des, uchar app);

char decompress_lz_str(uchar *src, int src_len, uchar *des);
char decompress_lz_file(char *src, int start_pos, int *end_pos);

#endif
