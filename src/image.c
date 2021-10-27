#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>

#include "include/image.h"

int hex2dec(unsigned char hex[4], int digit){
	int res = 0;

	for (int i = digit - 1; i >= 0; i--)
		res = res * 256 + hex[i];

	return res;
}

void complete_header(f_header_t* f_header, i_header_t* i_header, FILE* image){
    unsigned char tmp[4];

    fread(f_header->feature, 2, 1, image);

    fread(&tmp, 4, 1, image);
    f_header->f_size = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    f_header->reserved = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    f_header->offset = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->h_size = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->width = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->height = hex2dec(tmp, 4);

    fread(&tmp, 2, 1, image);
    i_header->plan = hex2dec(tmp, 2);

    fread(&tmp, 2, 1, image);
    i_header->color_depth = hex2dec(tmp, 2);

    fread(&tmp, 4, 1, image);
    i_header->compression = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->i_size = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->hori_res = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->vert_res = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->nb_color = hex2dec(tmp, 4);

    fread(&tmp, 4, 1, image);
    i_header->important_c = hex2dec(tmp, 4);
}

void write_byte(FILE* image, int fd_txt, unsigned char hex, int dec_num, int digit, int bytes_by_pixel, char* dec_char, char* padding){
    fread(&hex, 1, 1, image);
    dec_num = hex2dec(&hex, 1);
    if(0 == dec_num) // log(0) is a bad idea
        digit = 1;
    else
        digit = 1 + log(dec_num)/log(10);

    memset(padding, 0, bytes_by_pixel);
    for(int k = digit; k < bytes_by_pixel; k++)
        strcat(padding, "0");

    sprintf(dec_char, "%s%d", padding, dec_num);
    write(fd_txt, dec_char, 3*sizeof(char));
}

void write_matrix(FILE* image, i_header_t* i_header, int fd_txt){
    unsigned char hex = 0;
    int dec_num = 0;
    int digit = 0;
    int bytes_by_pixel = i_header->color_depth / 8;
    char dec_char[bytes_by_pixel + 1];
    char padding[bytes_by_pixel];
    for (int i = 0; i < i_header->height; i++) {
        for(int j = 0; j < bytes_by_pixel*i_header->width; j++){
            write_byte(image, fd_txt, hex, dec_num, digit, bytes_by_pixel, dec_char, padding);
        }
        for(int k = 0; k < i_header->width % 4; k++) // padding
            write_byte(image, fd_txt, hex, dec_num, digit, bytes_by_pixel, dec_char, padding);
    }
}

void create_image(f_header_t* f_header, i_header_t* i_header, int fd_txt){
    unsigned char f_header_char[FH_SIZE] = {0}; // file header
    unsigned char i_header_char[IH_SIZE] = {0}; // image header

    // convert structures into exploitable headers
    for (int i = 0; i < 4; i++){
        f_header_char[2+i]  = (unsigned char)(f_header->f_size >> 8*i);
        f_header_char[6+i]  = (unsigned char)(f_header->reserved >> 8*i);
        f_header_char[10+i] = (unsigned char)(f_header->offset >> 8*i);

        i_header_char[i]    = (unsigned char)(i_header->h_size >> 8*i);
        i_header_char[4+i]  = (unsigned char)(i_header->width >> 8*i);
        i_header_char[8+i]  = (unsigned char)(i_header->height >> 8*i);
        i_header_char[14+i] = (unsigned char)(i_header->color_depth >> 8*i);
    }
    for (int i = 0; i < 2; i++) {
        f_header_char[i]    = *(f_header->feature +i);

        i_header_char[12+i] = (unsigned char)(i_header->plan >> 8*i);
    }

    // create a new image with headers
    FILE* new_image = fopen("new_img.bmp","wb");
    fwrite(f_header_char, 1, FH_SIZE, new_image);
    fwrite(i_header_char, 1, IH_SIZE, new_image);

    // get back RGB pixels from fd_txt
    int w = i_header->width;
    int h = i_header->height;
    unsigned char* line = (unsigned char*)malloc(3*w);

    char buff[3];
    for(int i=0; i<h; i++){
        memset(line,0,3*w);
        for(int j=0; j<w; j++){
            for (int color = 0; color < 3; color++) { // RGB
                read(fd_txt, buff, 3*sizeof(char));
                line[3*j + color] = atoi(buff);
            }
        }
        fwrite(line, 3, w, new_image);  // write the new line in new_image
        for(int k = 0; k < w % 4; k++){ // padding
            read(fd_txt, buff, 3*sizeof(char));
            fwrite(buff, 1, 1, new_image);
        }
    }

    free(line);
    fclose(new_image);
}
