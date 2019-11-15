#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<mpi.h>
#include<malloc.h>
#define DEBUG 0
#define TEST 1
#define FILEIN 1

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



struct PPMImage* getPPM(char *filename);


void flip_img_horizontally(const char * file,struct PPMImage* img);
void flip_img_horizontally_MPI(const char * file,struct PPMImage* img);


int main(int argc,char * argv[]){
    FILE *file;
    struct PPMImage* img;
    char *filename;
    char *filename_sum;
    char *hori_filename;
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

    //time TEST
    double start_time,finish_time;

    
 
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Get_processor_name(processor_name,&namelen);


    #if TEST
    if(rank ==0){
        fprintf(stderr,"num of procs : %d\n",numprocs);
        start_time = MPI_Wtime();
    }
    #endif


    //why should i malloc in open_mpi
    recvcnts = malloc(sizeof(int)*numprocs);
    displs = malloc(sizeof(int)*numprocs);
    sendcnts = malloc(sizeof(int)*numprocs);
    filename = malloc(sizeof(char)*100);
    filename_sum= malloc(sizeof(char)*100);

    if(rank == 0){
        #if FILEIN
        fprintf(stderr,"enter the file name:");
        //strcpy(filename_sum,"./ppm_example/large/");
        strcpy(filename_sum,"");
        scanf("%s",filename);
        strcat(filename_sum,filename);
        fprintf(stderr,"entered : %s\n",filename_sum);
        #endif
        //only rank 0 opens the file
        img = getPPM(filename_sum);

        imgWidth = img->width;
        imgHeight = img->height;
        fprintf(stderr,"imgWidth: %d, imgHeight: %d\n",imgWidth,imgHeight);
    }
    if(rank == 0){
        rbuf = malloc(sizeof(struct pixel)*((imgWidth)*(imgHeight)));
        if( rbuf == NULL){
            fprintf(stderr,"rank 0 alloc failed\n");
        }
    }
        
    MPI_Bcast(&imgWidth,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&imgHeight,1,MPI_INT,0,MPI_COMM_WORLD);


    for(i =0 ;i< numprocs;i++){
        get_stdt(&st,&dt,numprocs,i,imgHeight);
        //each process calculate starting at row num of st , each row have pixels*3bytes.
        displs[i] = st* imgWidth*3;
        //each process gets : (dt-st) * images row of pixels * each pixel 3bytes.
        recvcnts[i] = (dt-st)*imgWidth * 3;    
    }
    get_stdt(&st,&dt,numprocs,rank,imgHeight);

    if( NULL==( scatter_recvbuf = malloc(sizeof(struct pixel) * ((imgWidth)*(dt-st+1))))){
        fprintf(stderr,"rank:%d malloc failed\n",rank);
    }
    else{
    ;//    fprintf(stderr,"rank:%d malloc succeed\n",rank);
    }
   
    if( NULL ==(   to_send = malloc(sizeof(struct pixel)*((imgWidth)*(dt-st+1))))){
        fprintf(stderr,"rank:%d mallock failed\n",rank);
    }
    else{
    ;//    fprintf(stderr,"rank:%d malloc succeed\n",rank);
    }

    MPI_Scatterv(img->data,recvcnts,displs,MPI_CHAR,scatter_recvbuf, (dt-st)*imgWidth*3  ,MPI_CHAR,0,MPI_COMM_WORLD);
    
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

    //each process has to_send
    MPI_Gatherv(to_send,recvcnts[rank],MPI_CHAR,rbuf,recvcnts,displs,MPI_CHAR,0,MPI_COMM_WORLD);
    
    //ranks 0 makes file 
    if(rank == 0){
         hori_filename = malloc(sizeof(char)*100);
         strcpy(hori_filename,"./hori_");
         strcat(hori_filename,filename);
         fprintf(stderr,"hori_filename: %s\n",hori_filename);
        hori =fopen(hori_filename,"wb");
//         hori = fopen("hori_mpi.ppm","wb");
        if(hori == NULL){
            fprintf(stderr,"hori file open failed\n");
            return 0;
        }
        else{
            fprintf(stderr,"file opened\n");
        }
        fprintf(hori,"P6\n");
        fprintf(hori,"%d %d\n",img->width,img->height);
        fprintf(hori,"%d\n",img->max);

        fwrite(rbuf, (size_t)3*img->ColorByte *img->height * img->width, 1, hori);

        fclose(hori);

        #if TEST
        finish_time = MPI_Wtime();
        printf("Elapsed time = %e seconds\n",finish_time-start_time);
        #endif
    }
    fprintf(stderr,"%d has finished\n",rank);

    MPI_Finalize();
    return 0;
}


struct PPMImage *getPPM( char *filename){
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

    /*
     *
     * problem due to open same resources
     */
    fp = fopen(filename,"rb");
    if(!fp ){
        fprintf(stderr,"unable to open file '%s'\n",filename);
        exit(1);
    }
    else{
        fprintf(stderr,"file: %s  opened\n",filename);
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
        fscanf(fp,"%c",&tmp1);
        fscanf(fp,"%d %d",&img->width,&img->height);
        if( img->width >1025 || img->width < 0){
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




    img->ColorByte = (img->max <256 ) ? 1:2;
    size = (size_t)3* img->ColorByte * img->height *img->width;
    img->data = malloc(size);

    if(img->data == NULL){
        fprintf(stderr,"memory alloc failed\n");
    }
    
    //read binary file
    fread(img->data,size,1,fp);


    fclose(fp);


    return img;
}

