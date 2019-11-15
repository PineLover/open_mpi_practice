#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mpi.h>
#include<malloc.h>
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

void get_stdt(int *st, int *dt,int numprocs,int rank,int row_size){
    int leftover = row_size - numprocs*(row_size / numprocs);
    if(rank < leftover){
        *st = (row_size/numprocs+1)* rank;
        *dt = *st + (row_size/numprocs+1);
    }else{
        *st = (row_size/numprocs +1 )*(leftover) + (row_size/numprocs)*(rank-leftover);
        *dt = *st + (row_size/numprocs);
    }
    return ;
}



struct PPMImage* getPPM(const char *filename);


void flip_img_horizontally(const char * file,struct PPMImage* img);
void flip_img_horizontally_MPI(const char * file,struct PPMImage* img);


int main(int argc,char * argv[]){
    FILE *file;
    struct PPMImage* img;
    const char* filename = "./ppm_example/Iggy.1024.ppm";
    int numprocs,rank,namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    int row,col;
    FILE * hori;
    struct pixel * pixels;
    struct pixel * rbuf;
    int* recvcnts;
    int* displs;
    int st,dt;
    int i,j;

    //mpi_scatterv
    int* sendcnts;
    char* scatter_recvbuf;
    char* to_send;
    int imgWidth,imgHeight;
    

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Get_processor_name(processor_name,&namelen);

    recvcnts = malloc(sizeof(int)*numprocs);
    displs = malloc(sizeof(int)*numprocs);
    sendcnts = malloc(sizeof(int)*numprocs);

    if(rank == 0){
        //only rank 0 opens the file
        img = getPPM(filename);
        fprintf(stderr,"\nsizeof buffer: %d\n",(img->width)*(img->height)*3);

        imgWidth = img->width;
        imgHeight = img->height;
    }
    if(rank == 0){
       /* 
        imgData = malloc(sizeof(char)*imgHeight);
        for(i=0;i<imgHeight;i++){
            imgData[i] = malloc(sizeof(char)*3*imgWidth);
        }
        */
        rbuf = malloc(sizeof(struct pixel)*((imgWidth)*(imgHeight)));
        if( rbuf == NULL){
            fprintf(stderr,"rank 0 alloc failed\n");
        }
    }
        
    MPI_Bcast(&imgWidth,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&imgHeight,1,MPI_INT,0,MPI_COMM_WORLD);

    
    fprintf(stderr,"imgWidth : %d, imgHeight : %d\n",imgWidth,imgHeight);

    //pixels = malloc(sizeof(struct pixel)*((imgWidth)*(imgHeight)));
    //used by gatherv
    //displacement index place
    //this was the problem scatterv needed values for each processes
    for(i =0 ;i< numprocs;i++){
        get_stdt(&st,&dt,numprocs,i,imgHeight);
        //each process calculate starting at row num of st , each row have pixels*3bytes.
        displs[i] = st* imgWidth*3;
        //each process gets : (dt-st) * images row of pixels * each pixel 3bytes.
        recvcnts[i] = (dt-st)*imgWidth * 3;    
    }
    get_stdt(&st,&dt,numprocs,rank,imgHeight);



    if(rank ==0){
        for(j=0;j<numprocs;j++){
            fprintf(stderr,"numprocs:%d, displs[%d]:%d, recvcnts[%d]:%d\n",numprocs,j,displs[j],j,recvcnts[j]);
        }
    }

    //img->data에 있는 데이터를 mpi_scatterv하자
    //sendcnts를 설정한다-> recvcnts랑 동일한 것 같다.
    //displs 은 scatter 와 gatherv와 같은듯.
    
    //각 프로세스 마다. scatter_recvbuf를 설정한다.
    // each process has 3bytes * rows of pixels * (dt-st) rows for each process
    if( NULL==( scatter_recvbuf = malloc(sizeof(struct pixel) * ((imgWidth)*(dt-st+1))))){
        fprintf(stderr,"rank:%d malloc failed\n",rank);
    }
    else{
        fprintf(stderr,"rank:%d malloc succeed\n",rank);
    }
   
    if( NULL ==(   to_send = malloc(sizeof(struct pixel)*((imgWidth)*(dt-st+1))))){
        fprintf(stderr,"rank:%d mallock failed\n",rank);
    }
    else{
        fprintf(stderr,"rank:%d malloc succeed\n",rank);
    }

    MPI_Scatterv(img->data,recvcnts,displs,MPI_CHAR,scatter_recvbuf, (dt-st)*imgWidth*3  ,MPI_CHAR,0,MPI_COMM_WORLD);
    
    //각 프로세스는scatter_recvbuf로 img->data에서 나누어진 데이터가 들어온다.
    //st ~ dt-1 , dt not included
    for(row =0 ; row < dt-st;row++){
        for(col =0;col<imgWidth/2 ; col=col+1){
            //swaps by pixels struct size
            to_send[row*3*imgWidth+col*3] = scatter_recvbuf[row*3*imgWidth+ 3*imgWidth - col*3 -3];
            to_send[row*3*imgWidth + 3*imgWidth - col*3 -3] = scatter_recvbuf[row*3*imgWidth + col*3];

            to_send[row*3*imgWidth+col*3+1] = scatter_recvbuf[row*3*imgWidth+ 3*imgWidth - col*3 -2];
            to_send[row*3*imgWidth + 3*imgWidth - col*3 -2] = scatter_recvbuf[row*3*imgWidth + col*3+1];

            to_send[row*3*imgWidth+col*3+2] = scatter_recvbuf[row*3*imgWidth+ 3*imgWidth - col*3 -1];
            to_send[row*3*imgWidth + 3*imgWidth - col*3 -1] = scatter_recvbuf[row*3*imgWidth + col*3+2];
        }
    }

    /*
    if(rank == 7)
    {
        fprintf(testfp,"P6\n");
        fprintf(testfp,"%d %d\n",img->width,img->height);
        fprintf(testfp,"%d\n",img->max);
        fwrite(pixels, (size_t)3*img->ColorByte *img->height * img->width, 1, testfp);
    }
    */
    //what displs should be?
    //전송 버퍼 시작 주소, 전송 데이터 갯수, 데이터 타입 
    //송신 버퍼 시작 주소, 각 프로세스 별송신 갯수,각 프로세스별 데이터 받는 시작 주소,데이터 타입
    //집합 프로세스, 통신 집합
    //fprintf(stderr,"rank: %d, st: %d, dt:%d\n",rank,st,dt);
    fprintf(stderr,"rank: %d, displs[%d]: %d, recvcnts[%d]: %d\n",rank,rank,displs[rank],rank,recvcnts[rank]);
    fprintf(stderr,"rank:%d\nstarting pos:%d ,sending %d bytes \n\n",rank,st*imgWidth*3,imgWidth*(dt-st)*3);


    //each process has to_send
    MPI_Gatherv(to_send,recvcnts[rank],MPI_CHAR,rbuf,recvcnts,displs,MPI_CHAR,0,MPI_COMM_WORLD);
    
    //ranks 0 makes file 
    if(rank == 0){
         hori = fopen("hori_mpi.ppm","wb");
        if(hori == NULL){
            fprintf(stderr,"hori file open failed\n");
            return 0;
        }
        else{
            fprintf(stderr,"file opened\n");
        }
        //gather images and make a file
        fprintf(hori,"P6\n");
        fprintf(hori,"%d %d\n",img->width,img->height);
        fprintf(hori,"%d\n",img->max);
        
        fwrite(rbuf, (size_t)3*img->ColorByte *img->height * img->width, 1, hori);

        fprintf(stderr,"file close\n");
        fclose(hori);
    }

    fprintf(stderr,"rank: %d finished\n",rank);

    MPI_Finalize();
    return 0;
}


struct PPMImage *getPPM(const char *filename){
    char buffer[3];
    int c,rgb_comp_color;
    FILE * fp;
    int i,j;
    //for DEBUG
    int buf;
    int count=0;

    struct PPMImage* img = NULL;
    size_t size ;

    /*
     *
     * problem due to open same resources
     */
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
     //   printf("Memory alloc succesful\n");
        ;
    }


    #if DEBUG
    while(fscanf(fp,"%d",&buf)){
        printf("%d ",buf);
    }

    #endif

    if(!fgets(buffer,sizeof(buffer),fp)){
        exit(1);
    }

    

    if(buffer[0] != 'P'||buffer[1] != '6'){
        fprintf(stderr,"Invalid image format! \n");
        exit(1);
    }
    else{
      //  printf("File format is correct \n");
        fscanf(fp,"%d %d",&img->width,&img->height);
        fscanf(fp,"%d",&img->max);
 //       printf("%d %d\n",img->width,img->height);
 //       printf("%d\n",img->max);
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

