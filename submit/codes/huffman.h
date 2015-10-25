//
//  huffman.h
//  myzip
//
//  Created by Inability on 14-5-11.
//  Copyright (c) 2014年 Inability. All rights reserved.
//

#ifndef myzip_huffman_h
#define myzip_huffman_h

#include "lz77.h"

void heap_up(int x);
void heap_down(int x);
int heap_pop();
void heap_push(int x);

char get_char(uchar *src, int src_len, int *curbyte, int *curbit, int root, uchar *des);

char compress_huffman_file(char *src, char *des, uchar app);

char decompress_huffman_file(char *src, char *des, int start_pos);

#endif
