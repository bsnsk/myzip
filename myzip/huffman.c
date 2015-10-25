//
//  huffman.c
//  myzip
//
//  Created by Inability on 14-5-11.
//  Copyright (c) 2014年 Inability. All rights reserved.
//

#include "huffman.h"

#define F(x) ((x)>>1)
#define L(x) ((x)<<1)
#define R(x) (((x)<<1)+1)

int cnt_chars[600];

int heap_place[600], heap_array[600], heap_len;

int huffman_lchild[600], huffman_rchild[600];

int huffman_len[256];

uchar huffman_code[256][32];

void heap_up(int x){
	int tmp;
	for (; x>1 && cnt_chars[heap_array[F(x)]] > cnt_chars[heap_array[x]]; x>>=1) {
		swap2(heap_array[F(x)], heap_array[x], tmp);
		swap2(heap_place[heap_array[F(x)]], heap_place[heap_array[x]], tmp);
	}
}

void heap_down(int x){
	int mi, tmp;
	for (; L(x)<=heap_len; x=mi) {
		mi = (L(x)==heap_len) ? L(x) : (cnt_chars[heap_array[L(x)]] < cnt_chars[heap_array[R(x)]] ? L(x) : R(x));
		if (cnt_chars[heap_array[mi]] >= cnt_chars[heap_array[x]])
			break;
		swap2(heap_array[mi], heap_array[x], tmp);
		swap2(heap_place[heap_array[mi]], heap_place[heap_array[x]], tmp);
	}
}

int heap_pop(){
	assert(heap_len > 0);
	int re = heap_array[1];
	heap_array[1] = heap_array[heap_len];
	heap_place[heap_array[1]] = 1;
	heap_len --;
	heap_down(1);
	return re;
}

void heap_push(int x){
	heap_array[++heap_len]=x;
	heap_place[x]=heap_len;
	heap_up(heap_len);
}

void build_huffman_code(int root, int depth, uchar* cur_code){
	if (huffman_lchild[root]==-1 && huffman_rchild[root]==-1) {
		huffman_len[root] = depth;
		memcpy(huffman_code[root], cur_code, sizeof(huffman_code[root]));
		return;
	}
	assert(huffman_lchild[root]!=-1 && huffman_rchild[root]!=-1);
	unsigned tmp = 0;
	copy_bits_in_byte(cur_code + depth/8, depth&7, (uchar*)&tmp, 0, 1);
	build_huffman_code(huffman_lchild[root], depth+1, cur_code);
	tmp = 0xffffffff;
	copy_bits_in_byte(cur_code + depth/8, depth&7, (uchar*)&tmp, 0, 1);
	build_huffman_code(huffman_rchild[root], depth+1, cur_code);
}

char compress_huffman_file(char *src, char *des, uchar app){
	
	fprintf(stderr, "\ncompressing %s to %s (Huffman) ...\n", src, des);
	
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
	int input_file_length = (int)ftell(in);
	fseek(in, 0, SEEK_SET);
	
	fprintf(stderr, "total length: %d\n", input_file_length);
	
	int i, j;
	
	memset(cnt_chars, 0, sizeof(cnt_chars));
	for (i=0; i<input_file_length; i++){
		cnt_chars[j=fgetc(in)]++;
		//fprintf(stderr, "char %d\n", j);
	}
	
	memset(huffman_code, 0, sizeof(huffman_code));
	memset(huffman_len, 0, sizeof(huffman_len));
	memset(huffman_lchild, -1, sizeof huffman_lchild);
	memset(huffman_rchild, -1, sizeof huffman_rchild);
	heap_len = 0;
	for (i=0; i<256; i++) {
		if (cnt_chars[i]>0)
			heap_push(i);
	}
	int tot = 256, p, q;
	while ( heap_len>1 ) {
		p = heap_pop();
		q = heap_pop();
		huffman_lchild[tot] = p;
		huffman_rchild[tot] = q;
		cnt_chars[tot] = cnt_chars[p] + cnt_chars[q];
		heap_push(tot++);
	}
	assert(tot <= 256*2);
	
	uchar tmp_uchar[32];
	memset(tmp_uchar, 0, sizeof(tmp_uchar));
	build_huffman_code(heap_pop(), 0, tmp_uchar);
	
	// length
	for (i=0; i<256; i++)
		fwrite(huffman_len+i, sizeof(int), 1, ou);
	
	/*
	for (i=1; i<=256; i++)
		for (j=0; j<256; j++)
			if (huffman_len[j]==i){
				int k;
				fprintf(stderr, "huffman %d length %d with cnt_chars %d: ", j-128, i, cnt_chars[j]);
				for (k=0; k<i; k++)
					fprintf(stderr, "%d", (huffman_code[j][k/8] >> (7-(k&7))&1));
				fprintf(stderr, "\n");
			}
	*/
	
	uchar * buff = (uchar *) malloc(sizeof(uchar) * max_window_size);
	
	// dict
	int cur_byte=0, cur_bit=0;
	for (i=0; i<256; i++)
		if (huffman_len[i]>0){
			copy_bits(buff + cur_byte, cur_bit, huffman_code[i], 0, huffman_len[i]);
			move_forward(&cur_byte, &cur_bit, huffman_len[i]);
		}
	
	fwrite(buff, sizeof(uchar), cur_byte + (cur_bit>0?1:0), ou);
	
	fprintf(stderr, "after dict pos: %d\n", (int)ftell(ou));
	
	// code
	cur_byte = 0, cur_bit = 0;
	fseek(in, 0, SEEK_SET);
	for (j=0; j<input_file_length; j++) {
		i = fgetc(in);
		if (cur_byte + (cur_bit+huffman_len[i]+7)/8 > max_window_size) {
			fwrite(buff, sizeof(uchar), cur_byte, ou);
			if (cur_bit)
				buff[0] = buff[cur_byte];
			cur_byte = 0;
		}
		copy_bits(buff + cur_byte, cur_bit, huffman_code[i], 0, huffman_len[i]);
		move_forward(&cur_byte, &cur_bit, huffman_len[i]);
	}
	if (cur_byte || cur_bit)
		fwrite(buff, sizeof(uchar), cur_byte + (cur_bit?1:0), ou);
	
	uchar zeros = (cur_bit ? 8-cur_bit : 0);
	fwrite(&zeros, sizeof(char), 1, ou);
	
	free(buff);
	
	fclose(in);
	fclose(ou);
	return 1;

}

// 1 -- successful
// 0 -- unsuccessful
char get_char(uchar *src, int src_len, int *curbyte, int *curbit, int root, uchar *des){
	int p = root, bit, cby = *curbyte, cbi = *curbit;
	while (huffman_lchild[p]!=-1 || huffman_rchild[p]!=-1) {
		bit = (src[*curbyte] >> (7-*curbit))&1;
		p = (!bit ? huffman_lchild[p] : huffman_rchild[p]);
		if (++ (*curbit) == 8){
			++ (*curbyte);
			*curbit = 0;
			if (*curbyte == src_len) {
				*curbyte = cby;
				*curbit = cbi;
				return 0;
			}
		}
	}
	*des = p;
	return 1;
}

char decompress_huffman_file(char *src, char *des, int start_pos){
	
	fprintf(stderr, "\ndecompressing %s to %s (Huffman) ...\n", src, des);
	
	FILE *in = fopen(src, "rb");
	FILE *ou = fopen(des, "wb");
	if (!in || !ou) {
		fprintf(stderr, "file error!\n");
		return 0;
	}
	
	memset(huffman_code, 0, sizeof(huffman_code));
	memset(huffman_len, 0, sizeof(huffman_len));
	memset(huffman_lchild, -1, sizeof huffman_lchild);
	memset(huffman_rchild, -1, sizeof huffman_rchild);

	fseek(in, start_pos, SEEK_SET);
	
	int totlen=0, i, k;
	for (i=0; i<256; i++){
		fread(huffman_len+i, sizeof(int), 1, in);
		totlen += huffman_len[i];
	}
	
	uchar * buff = (uchar *) malloc(sizeof(uchar) * max_window_size);
	
	fread(buff, sizeof(uchar), (totlen+7)/8, in);
	
	int cur_byte=0, cur_bit=0;
	for (i=0; i<256; i++)
		if (huffman_len[i] > 0){
			copy_bits(huffman_code[i], 0, buff + cur_byte, cur_bit, huffman_len[i]);
			move_forward(&cur_byte, &cur_bit, huffman_len[i]);
		}
	
	int tot = 256, root = tot++, p, v;
	for (i=0; i<256; i++)
		if (huffman_len[i]>0)
			for (k=0, p=root; k<huffman_len[i]; k++) {
				v = ((huffman_code[i][k/8] >> (7-(k&7))) &1);
				if (!v) {
					if (huffman_lchild[p]==-1)
						huffman_lchild[p] = (k+1<huffman_len[i] ? tot++ : i);
					p = huffman_lchild[p];
				}
				else {
					if (huffman_rchild[p]==-1)
						huffman_rchild[p] = (k+1<huffman_len[i] ? tot++ : i);
					p = huffman_rchild[p];
				}
			}
	
	int now_pos = (int)ftell(in);
	fseek(in, -1, SEEK_END);
	int last = (int)ftell(in) - now_pos, cur_len, pre_len;
	uchar zeros;
	fread(&zeros, sizeof(char), 1, in);
	//fprintf(stderr, "zeros = %d\n", zeros);
	fseek(in, now_pos, SEEK_SET);
	
	fprintf(stderr, "D after dict pos: %d\n", (int)ftell(in));
	
	cur_byte = cur_bit = pre_len = 0;
	uchar _uchar;
	for (; last+pre_len>0; ) {
		cur_len = calc_min(max_window_size-pre_len, last);
		//fprintf(stderr, "cur_len = %d\n", cur_len);
		fread(buff+pre_len, sizeof(uchar), cur_len, in);
		while (get_char(buff, cur_len, &cur_byte, &cur_bit, root, &_uchar) ){
			if ( cur_byte - pre_len + 1 == last && cur_bit+zeros == 8) {
				break;
			}
			fwrite(&_uchar, sizeof(uchar), 1, ou);
		}
		pre_len += cur_len - cur_byte;
		for (i=0; i<pre_len; i++)
			buff[i] = buff[cur_byte+i];
		cur_byte = 0;
		last -= cur_len;
		if (last+pre_len==1 && cur_bit+zeros==8)
			break;
	}
	
	free(buff);
	
	fclose(in);
	fclose(ou);
	return 1;
}
