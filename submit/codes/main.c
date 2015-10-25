//
//  main.cpp
//  myzip
//
//  Created by Inability on 14-5-5.
//  Copyright (c) 2014年 Inability. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "lz77.h"
#include "huffman.h"

#define WINDOWS_NOW 1 // on windows platform or not

#if WINDOWS_NOW
#include <windows.h>
#endif

#define ZIP_OR_UNZIP 0 // 1 for zip and 0 for unzip

#define DIR_STR_LEN 1024

char name[DIR_STR_LEN], tmp_name[DIR_STR_LEN];

const char pattern_str[] = ":\\";

#if WINDOWS_NOW

char is_file(char *name){
	FILE *tmp;
	if (name[0]!='.' && (tmp = fopen(name, "rb"))!=NULL) {
		fclose(tmp);
		return 1;
	}
	return 0;
}

void compress_directory(char *des_name, char *name_start_pos){
	FILE *ou = fopen(des_name, "ab");
	int dir_name_len = -strlen(name_start_pos);
	fwrite(&dir_name_len, sizeof(int), 1, ou);
	fwrite(name_start_pos, sizeof(char), strlen(name_start_pos), ou);
	fclose(ou);
	
	HANDLE f1;
	WIN32_FIND_DATA fData;
	
	int now_len = strlen(name);
	name[now_len] = '\\';
	name[now_len+1] = '*';
	name[now_len+2] = '\0';
	
	f1 = FindFirstFile(name, &fData);
	do {
		if ((fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY && !(fData.cFileName[0]=='.')) {
			//printf("directory: %s\n", fData.cFileName);
			name[now_len+1] = '\0';
			strcat(name, fData.cFileName);
			compress_directory(des_name, name_start_pos);
		}
		else if (fData.cFileName[0]!='.'){
			//printf("file: %s,", fData.cFileName);
			name[now_len+1] = '\0';
			strcat(name, fData.cFileName);
			//printf("\tadd %s...\n", name);
			compress_lz_file(name, name_start_pos, des_name, 1);
		}
	} while (FindNextFile(f1, &fData));
	
	FindClose(f1);
	
	ou = fopen(des_name, "ab");
	int tmp = 0;
	fwrite(&tmp, sizeof(int), 1, ou);
	fclose(ou);
}

#endif

#if !WINDOWS_NOW
void my_own_test();
#endif

char *find_name_start(char *name){
	char *ptr = strstr(name, pattern_str);
	if (ptr) {
		for (ptr = name+strlen(name)-1; ptr>=name && *ptr!='\\'; ptr--);
		ptr ++;
	}
	else
		ptr = name;
	return ptr;
}

int main(int argc, const char * argv[]) {
	
	srand((unsigned)time(0));
	
	//my_own_test();
	
#if WINDOWS_NOW
#if ZIP_OR_UNZIP
	
	int i;
	
	if (argc < 3)
		printf("Illegal Input!\n");
	else if (argc == 3){
		if (is_file(argv[1])) { // compress single file
			printf("cmd: #%s#%s#%s#\n", argv[0], argv[1], argv[2]);
			compress_lz_file(argv[1], find_name_start(argv[1]), argv[2], 0);
		}
		else { // compress single directory
			strcpy(name, argv[1]);
			FILE *tmp = fopen(argv[argc-1], "wb");
			fclose(tmp);
			compress_directory(argv[2],  find_name_start(name));
		}
	}
	else { // multiple files
		FILE *tmp = fopen(argv[argc-1], "wb");
		fclose(tmp);
		for (i=1; i<argc-1; i++)
			if (is_file(argv[i])){
				compress_lz_file(argv[i], find_name_start(argv[i]), argv[argc-1], 1);
			}
	}
	strcpy(name, argv[argc-1]);
	strcat(name, ".tmpswp");
	compress_huffman_file(argv[argc-1], name, 0);
	strcpy(tmp_name, "move \0");
	strcat(tmp_name, name);
	strcat(tmp_name, " ");
	strcat(tmp_name, argv[argc-1]);
	system(tmp_name);
	
#endif

#if !ZIP_OR_UNZIP
	
	if (argc!=2 || !is_file(argv[1])) {
		printf("Illegal Input!\n");
		goto unzip_out;
	}
	
	strcpy(name, argv[argc-1]);
	strcat(name, ".tmpswp");
	decompress_huffman_file(argv[argc-1], name, 0);
	strcpy(tmp_name, "move \0");
	strcat(tmp_name, name);
	strcat(tmp_name, " ");
	strcat(tmp_name, argv[argc-1]);
	system(tmp_name);
	
	int pos = 0, isdir;
	int name_len;
	FILE *in = fopen(argv[1], "rb");
	fseek(in, 0, SEEK_END);
	int end_pos = (int)ftell(in);
	fclose(in);
	while (pos < end_pos) {
		//printf("pos=%d end_pos=%d\n", pos, end_pos);
		
		in = fopen(argv[1], "rb");
		fseek(in, pos, SEEK_SET);
		fread(&name_len, sizeof(int), 1, in);
		if (name_len < 0) {
			isdir = 1;
			name_len = -name_len;
		}
		else if (name_len > 0)
			isdir = 0;
		else if (name_len == 0)
			isdir = -1;
		if (name_len) {
			fread(name, sizeof(char) * (name_len), 1, in);
		}
		name[name_len]='\0';
		
		if (isdir == 1) {
			// directory
			_mkdir(name);
			pos = (int) ftell(in);
			fclose(in);
		}
		else if (isdir == -1){
			// cd ../
			while (name_len>0 && name[name_len-1]!='\\')
				name_len--;
			name[name_len --] = '\0';
			pos = (int) ftell(in);
			fclose(in);
		}
		else {
			// file
			fclose(in);
			decompress_lz_file(argv[1], pos, &pos);
		}
		
	}
	
unzip_out:;
#endif

#endif
	return 0;
}

#if !WINDOWS_NOW

void my_own_test(){
	//compress_lz_file("a.c", "a.c", "lz.c", 0);
	//compress_huffman_file("lz.c", "hlz.c", 0);
	
	//compress_lz_file("InTransit.mp3", "InTransit.mp3", "it.lz", 0);
	//compress_huffman_file("it.lz", "it.lzhf", 0);
	
	compress_huffman_file("test.exe", "test.hf", 0);
	compress_lz_file("test.hf", "test.hf", "test.hflz", 0);
	
}
#endif