#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#define DEBUG 0

typedef struct PPMImage{
    char format[3];
    int height,width;
    int max;
    int ColorByte;
    //struct PPM_Pixel *data;
    //char *data;
    struct pixel * data;
};

typedef struct PPM_Pixel{
    unsigned int r;
    unsigned int g;
    unsigned int b;
}PPMPixel;

typedef struct pixel{
    char r;
    char g;
    char b;
};

struct PPMImage* getPPM(char *filename);


void flip_img_horizontally(char * file,struct PPMImage* img);


int main(){
    FILE *file;
    struct PPMImage* img;
    //const char* filename = "ppmExample.ppm";
    //const char* filename = "./ppm_example/Iggy.1024.ppm";
    char* filename;
    char* filename_sum;
    int i,j;

    clock_t start,end;
    double result;

    filename = malloc(sizeof(char)*100);
    filename_sum = malloc(sizeof(char)*100);

    printf("enter the file name: ");
    scanf("%s",filename);
    strcat(filename_sum,filename);

    start = clock();
    img = getPPM(filename_sum);
    flip_img_horizontally(filename_sum,img);


    end = clock();
    result = (float) (end-start)/CLOCKS_PER_SEC;

    printf("Elapsed time = %e seconds\n",result);

    return 0;
}


struct PPMImage *getPPM(char *filename){
    char buffer[3];
    int c,rgb_comp_color;
    FILE * fp;
    int i,j;
    //for DEBUG
    int buf;
    int count=0;
    char tmp[500];
    char tmp1;

    struct PPMImage* img = NULL;
    size_t size ;



    fp = fopen(filename,"rb");
    if(!fp ){
        fprintf(stderr,"unable to open file '%s'\n",filename);
        exit(1);
    }

    if(NULL == (img = malloc(sizeof(struct PPMImage)))){
        perror("memory allocation for PPM file failed \n");
        exit(1);
    }
    else{
        printf("Memory alloc succesful\n");
    }


    if(!fgets(buffer,sizeof(buffer),fp)){
        exit(1);
    }

    

    if(buffer[0] != 'P'||buffer[1] != '6'){
        fprintf(stderr,"Invalid image format! \n");
        exit(1);
    }
    else{
        printf("%c %c\n",buffer[0],buffer[1]);
        fscanf(fp,"%c",&tmp1);
        fscanf(fp,"%d %d",&img->width,&img->height);
        printf("%d %d\n",img->width,img->height);
        if(img->width > 1025 || img->width <= 0){
            fprintf(stderr,"has #\n");
            fgets(tmp,499,fp);
            fscanf(fp,"%d %d",&img->width,&img->height);
        }
        fscanf(fp,"%d",&img->max);
        printf("%d %d\n",img->width,img->height);
        printf("%d\n",img->max);
    }


    //check rgb component depth
    if(img->max != 255){
        fprintf(stderr,"'%s' does not have 8-bits components\n",filename);
        exit(1);
    }

    while(fgetc(fp) != '\n');
    //memory alloc for pixel data
    //img->data = (PPMPixel*)malloc(img->width*img->height*sizeof(PPMPixel));



    if(!img){
        fprintf(stderr,"Unable to allocate memory\n");
        exit(1);
    }

    img->ColorByte = (img->max <256 ) ? 1:2;
    size = (size_t)3* img->ColorByte * img->height *img->width;
    img->data = malloc(size);
    //read binary file
    fread(img->data,size,1,fp);


    fclose(fp);


    return img;
}



void flip_img_horizontally(char *filename ,struct PPMImage* img){
    FILE *hori =fopen("hori.ppm","w");
    struct pixel  * pixels = malloc(sizeof(struct pixel)*(img->width)*(img->height));
    int i,j;
    int block_num;
    int row,col; 
    size_t size;

    if(hori == NULL){
        fprintf(stderr,"hori file open failed\n");
        return ;
    }

    fprintf(hori,"P6\n");
    fprintf(hori,"%d %d\n",img->width,img->height);
    fprintf(hori,"%d\n",img->max);


    /*
     1 2 3 4 5 6

     4 5 6 1 2 3
     */
    for(row =0;row<img->height;row++){
        for(col =0;col < img->width/2;col++){
            pixels[row*img->width + col] = img->data[row*img->width + img->width - col -1];
            pixels[row*img->width + img->width - col -1] = img->data[row*img->width + col];
        }
    }

    size = (size_t)3* img->ColorByte * img->height *img->width;

    fwrite(pixels,size,1,hori);
    fclose(hori);


    free(pixels);
}
