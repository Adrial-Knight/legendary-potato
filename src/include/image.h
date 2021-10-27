// bonus: sending .bmp images

#if !defined(__IMAGE__H__)
    #define __IMAGE__H__

    #define FH_SIZE 14
    #define IH_SIZE 40

    typedef struct file_header{
    	char feature[2];
    	int f_size;
        int reserved;
    	int offset;
    }f_header_t;

    typedef struct image_header{
        int h_size;
        int width;
        int height;
        int plan;
        int color_depth;
        int compression;
        int i_size;
        int hori_res;
        int vert_res;
        int nb_color;
        int important_c;
    }i_header_t;

    int hex2dec(unsigned char hex[4], int digit);
    void complete_header(f_header_t* f_header, i_header_t* i_header, FILE* image);
    void write_byte(FILE* image, int fd_txt, unsigned char hex, int dec_num, int digit, int bytes_by_pixel, char* dec_char, char* padding);
    void write_matrix(FILE* image, i_header_t* i_header, int fd_txt);
    void create_image(f_header_t* f_header, i_header_t* i_header, int fd_txt);

#endif
