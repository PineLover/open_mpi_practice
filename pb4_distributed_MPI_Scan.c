#include<stdio.h>
#include<mpi.h>
#include<stdlib.h>

//random numbers size
#define DEBUG 0
//for writing test document
#define TEST 1
#define P2PCommunication 0
#define randsize 100000
#define randmax 100000000

void get_stdt(int*st,int*dt,int numprocs,int rank){
    int leftover = randsize - numprocs*(randsize/numprocs);

    //0~leftover-1
    if(rank < leftover){
        *st = (randsize/numprocs+1)*rank;
        *dt = *st + (randsize/numprocs+1);
    }
    else{
        *st = (randsize/numprocs+1)*(leftover) + (randsize/numprocs)*(rank-leftover);
        *dt = *st + (randsize/numprocs);
    }

    return ;
}

int main(int argc,char * argv[]){
    //number of procs, rank, namelen
    int numprocs, rank,namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    //array of rand nums
    int arr[randsize];
    int prefix_sum[randsize];
    int random =0 ;
    //input buf, output buf
    //current process's sum of st~dt ,and return sum from prev process
    int inbuf=0,outbuf=0;
    int cur_proc_prefixSum=0;
    //start of index, end of index;
    int st,dt;
    //for loop
    int i;
    //for TEST time
    double start_time,finish_time;
    //for mpi_recv status
    MPI_Status status;

    //make rand array
    for(i=0;i<randsize;i++){
        random = rand()%randmax;
        arr[i] = random;
    }
    #if DEBUG
    //set 1~randsize to arr for test
    for(i=0;i<randsize;i++){
        arr[i] = i;
    }
    #endif

    #if TEST
    start_time = MPI_Wtime();
    #endif

    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Get_processor_name(processor_name,&namelen);
   
    //getting start index and destination index; 
    get_stdt(&st,&dt,numprocs,rank);
    //calculating inbuf,outbuf.
    //inbuf : sum of st~dt-1
    for(i=st;i<dt;i++){
        cur_proc_prefixSum += arr[i];
    }
    inbuf = cur_proc_prefixSum;
    
    #if P2PCommunication
    //0~7, 8
    if( 1){
        if(rank == 0){
            MPI_Send(&inbuf,1,MPI_INTEGER,rank+1,0,MPI_COMM_WORLD);
        }
        else{
            MPI_Recv(&outbuf,1,MPI_INTEGER,rank-1,0,MPI_COMM_WORLD,&status);
            inbuf = cur_proc_prefixSum + outbuf;
            if(rank +1 < numprocs){
            MPI_Send(&inbuf,1,MPI_INTEGER,rank+1,0,MPI_COMM_WORLD);
            }
        }

    }
    #else
    //MPI_Scan
    MPI_Scan(&inbuf,&outbuf,1,MPI_INTEGER,MPI_SUM,MPI_COMM_WORLD); 
    #endif

    #if P2PCommunication
    prefix_sum[st] = outbuf + arr[st];
    for(i = st+1;i<dt;i++){
        prefix_sum[i] = prefix_sum[i-1] + arr[i];
    }


    #else
    //use outbuf to get prefix_sum
    //outbuf : sum of 0~st-1
    if(rank != 0)
        prefix_sum[st] = outbuf-cur_proc_prefixSum+arr[st];
    else{
        prefix_sum[st] = 0;
    }

    //get prefix_sum from st to dt
    for(i= st+1;i<dt;i++){
        prefix_sum[i] = prefix_sum[i-1] + arr[i];
    }
    #endif
    #if DEBUG
    //for test
    for(i=st;i<dt;i++){
        printf("rank:%d, arr[%d]:%d, prefix[%d]:%d\n",rank,i,arr[i],i,prefix_sum[i]);
    }
    #endif


    #if TEST
    finish_time = MPI_Wtime();
    printf("Proc %d > Elapsed time = %e seconds\n",rank,finish_time-start_time);
    #endif

    //finalize MPI
    MPI_Finalize();
}


