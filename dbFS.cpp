#define _FILE_OFFSET_BITS 64
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>

//#include <fstream>
//#include <stdio.h>
#include <ctime>
#include <pthread.h>
#ifndef THREADNUM
#define THREADNUM 16
#endif


//#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>

#ifndef __sun
#define USE_O_DSYNC 0
#define O_LARGEFILE 0
#endif

#ifdef __sun
#define llseek lseek64
#else
#define llseek lseek
#endif

#define offset_t unsigned long
#ifdef WINDOWS
#define fsync _commit
#endif

//typedef __int64 int64_t;

//#endif

#include <iostream>
using namespace std;
using std::cout;
using std::endl;
using std::cerr;

void * randomWrites(void *);
void streamingWrites();
void streamingReads();
void randomBuffer(unsigned long bufSize);

unsigned long writes=7812500; // 64GB worth of 8k blocks
unsigned int writeBlock=8192;

unsigned int streamBlock=8388608; //16777216
unsigned long streamCount=8192; //4096

int64_t offset=96636764160;
  //1099511627776;
//200000000000; ////8589934592;

char *testBuf=NULL;

char dsync=0;

int main(int argc, char* argv[])
{
  pthread_t writer[THREADNUM];
  pthread_attr_t attr;
  
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  
  
  if(argc !=7){
    perror("Wrong number of arguments");
    perror("Try: 8192 100000 8388608 100 96636764160 dsync");
    exit(1);
  }
  writeBlock=atol(argv[1]);
  writes=atol(argv[2]);

  streamBlock=(atol(argv[3]));
  streamCount=(atol(argv[4]));
  offset=(atoll(argv[5]));
  dsync=(!strcmp(argv[6],"dsync"));
  randomBuffer(writeBlock);
  for (int i=0; i<THREADNUM; i++){

    if (pthread_create(&writer[i], &attr, randomWrites, NULL)!=0){
      cerr << "Writer Thread start failed\n";
    }
    //    randomWrites();
  }
  pthread_attr_destroy(&attr);
  void *status;
  double totalWriteSpeed=0;
  for (int i=0; i<THREADNUM; i++){

    if (pthread_join(writer[i], &status)!=0){
      cerr << "Writer Thread join failed\n";
    }
    else{
      totalWriteSpeed+=*(double *)status;
      delete (double *)status;
    }
    //    randomWrites();
  }
  cout <<"Total WPS across all threads: "<<totalWriteSpeed<<endl;

  delete[] testBuf;
  
  
  randomBuffer(streamBlock);
  streamingWrites();
  streamingReads();
  delete[] testBuf;
  
  return 0;
}

void randomBuffer(unsigned long bufSize){
  cout<<"Filling "<<bufSize<<" byte random buffer\n";
  FILE *randFile;
  randFile=fopen("/dev/urandom","r");
  testBuf=new char[bufSize];
  fread(testBuf,1,bufSize,randFile);
  fclose(randFile);


}

void * randomWrites(void *threadId){

  int file0;
  int fileArgs=O_LARGEFILE|O_CREAT|O_WRONLY;
  if( dsync) {
    cout << "DSYNC is on\n";
    fileArgs=fileArgs|O_DSYNC;
  }
  file0=open("./testFile0", fileArgs,0644);
  if(file0==0)  perror("open");

  //cout <<offset<<endl;
  if (llseek(file0,offset-writeBlock, SEEK_SET)){
    // perror("fseek");
  }
  cout<<"\nTesting Random writes\n";
  timeval * tp=new timeval();
  offset_t *randNum=new offset_t[writes];
  for (long i=0; i<writes; i++){
    randNum[i]=-1;
    randNum[i]=(uint64_t(rand())<<48|uint64_t(rand())<<32| uint32_t(rand())<<16 | rand())%(offset-writeBlock);
    randNum[i]= randNum[i] & 0xFFFFFFFFFFFFE000; // put the request on the start of an 8k block    
  }
  if(gettimeofday(tp,NULL))perror("time1");
  for (long i=0; i<writes; i++){
   
    llseek(file0, randNum[i], SEEK_SET);// <<endl;
    if (write(file0, testBuf,writeBlock) !=writeBlock) perror("writing block");
  }
  fsync(file0);
  close(file0);

  long startTime=1000000*(tp->tv_sec)+tp->tv_usec;
  if(gettimeofday(tp,NULL)) 
    perror("time2");
  
  long writeTime=1000000*(tp->tv_sec)+tp->tv_usec-startTime;
  cout<< "Total time: "<<(double)writeTime/1000000<<" seconds\n";
  cout << "Average Write time: "<< (double)writeTime/writes/1000<< "ms\n";
  cout << "Writes per second: "<< (double)writes/((double)writeTime/1000000)<< "\n\n";

  delete[] randNum;
  return  ( new double ((double)writes/((double)writeTime/1000000) ));

}

void streamingWrites(){
  int file0;
  int fileArgs=O_LARGEFILE|O_CREAT|O_WRONLY|O_TRUNC;
  if( dsync) fileArgs=fileArgs|O_DSYNC;
  file0=open("./testFile1", fileArgs,0644);
  if(file0==0)  perror("open");
  cout<<"\nTesting Streaming writes\n";
  timeval * tp=new timeval();
  if(gettimeofday(tp,NULL))perror("time1");
  for (unsigned long i=0; i<streamCount; i++){
    write(file0, testBuf,streamBlock);
  }
  fsync(file0);
  close(file0);  

  long startTime=1000000*(tp->tv_sec)+tp->tv_usec;
  if(gettimeofday(tp,NULL)) 
    perror("time2");

  long writeTime=1000000*(tp->tv_sec)+tp->tv_usec-startTime;
  cout<< "Total time: "<<(double)writeTime/1000000<<" seconds\n";
  cout << "Average Write time: "<< (double)writeTime/streamCount/1000<< "ms\n";
  cout << "Writes per second: "<< (double)streamCount/((double)writeTime/1000000)<< "\n";
  cout << "Throughput: "<<(double)streamCount*(double)streamBlock/(double)writeTime<<"MB/s\n\n";

  //  delete testBuf;
  
  return;
}
void streamingReads(){

  int file0;
  file0=open("./testFile1", O_LARGEFILE|O_RDONLY,0644);
  if(file0==0)  perror("open");
  cout<<"Testing Streaming reads\n";
  timeval * tp=new timeval();
  if(gettimeofday(tp,NULL))perror("time1");
  for (unsigned long i=0; i<streamCount; i++){
    read(file0, testBuf,streamBlock);
  }
  long startTime=1000000*(tp->tv_sec)+tp->tv_usec;
  if(gettimeofday(tp,NULL)) 
    perror("time2");
  
  long writeTime=1000000*(tp->tv_sec)+tp->tv_usec-startTime;
  cout<< "Total time: "<<(double)writeTime/1000000<<" seconds\n";
  cout << "Average Read time: "<< (double)writeTime/streamCount/1000<< "ms\n";
  cout << "Reads per second: "<< (double)streamCount/((double)writeTime/1000000)<< "\n";
  cout << "Throughput: "<<(double)streamCount*(double)streamBlock/(double)writeTime<<"MB/s\n";
  close(file0);
  //  delete testBuf;
  
  return;

}
