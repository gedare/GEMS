/*****************************************************************************
 * Microbenchmark to test cache size and set associativity
 * (assumes 4 MB cache that is 4-way set associative
 *
 * Test #1: ./read 16777216 4194304 8
 * Creates a 16 MB buffer, accesses it in 4MB strides, 8 iterations.
 * Should have only 4 misses.
 * 
 * Test #2: ./read 20971520 4194304 8
 * Creates a 20 MB buffer, accesses it in 4MB strides, 8 iterations.
 * Should have 8*5 = 40 misses.  Misses on every access, since we're
 * cycling through 5 addresses that all map to the same 4-way set.
 *
 ****************************************************************************/

/* -- define magic call for breakpoint */

#define RUBY_MAGIC_CALL( service )                              \
  do                                                            \
  {                                                             \
    asm volatile (                                              \
                   "sethi %0, %%g0"                             \
                   : /* -- no outputs */                        \
                   : "g" ( (((service) & 0xff) << 16) | 0 )     \
                 );                                             \
  } while( 0 );
               

const int CACHE_LINE_SIZE = 64;

const int CACHE_SIZE = 1024;    // in bytes
const int CACHE_ASSOCIATIVITY = 4;  // 4-way set associative

#include "stdio.h"
#include "stdlib.h"

//#include "iostream.h"

struct cacheLine
{
  long long lineAddress;
  long lastTouched;
};

int main(int argc, char* argv[])
{

  long NUMBER_OF_SETS = (CACHE_SIZE/CACHE_LINE_SIZE)/CACHE_ASSOCIATIVITY;

  int a;
  long numMisses = 0;

  //  int bufferSizeMegabytes = atoi(argv[1]);
  //  long long bufferSize = bufferSizeMegabytes*1048576;

  long long bufferSize = atoll(argv[1]);

  // cout << "Buffer size = " << bufferSize <<  endl;

  int stride = atoi(argv[2]);
  int numIterations = atoi(argv[3]);

  int oldest;
  long time = 0;
  bool hit;
  long long lineAddress;

  long* readBuffer = (long*)malloc(sizeof(long)*bufferSize);

  //  long readBuffer[bufferSize];
  long long i;

  for(i=0; i<bufferSize; i+= stride){
    readBuffer[i] = 0;
  }

  // initialize cache
  cacheLine cache[NUMBER_OF_SETS][CACHE_ASSOCIATIVITY];
  for(i=0; i< NUMBER_OF_SETS; i++){
    for(int j=0; j < CACHE_ASSOCIATIVITY; j++){
      cache[i][j].lineAddress = -1;	
      cache[i][j].lastTouched = -1;
    }
  }

  // compute number of misses expected
  for(int k=0; k<numIterations; k++){

    for(i=0; i<bufferSize; i += stride){	// for each access (i = byte address)

      //     cout << "Address = " << i << endl;

      time++;
      lineAddress = i/CACHE_LINE_SIZE;

      //    cout << "Line address = " << lineAddress << endl;

      int set = lineAddress % NUMBER_OF_SETS;

      //   cout << "Set = " << set << endl;

      oldest = 0;   // position of oldest block in set
      hit = false;

      for(int j=0; j < CACHE_ASSOCIATIVITY; j++){	// search the appropriate set
	
	if(cache[set][j].lineAddress == lineAddress){
	  cache[set][j].lastTouched = time;
	  hit = true;
	  break;
	} else {
	  if(cache[set][j].lastTouched < cache[set][oldest].lastTouched){
	    oldest = j;
	  }
	}
      }
      if(!hit){
	// find oldest block to replace
	cache[set][oldest].lineAddress = lineAddress;
	cache[set][oldest].lastTouched = time;
	numMisses++;
      }
    }

  }

  int accesses = numIterations*(bufferSize/stride);
  printf("Total number of accesses = %d\n", accesses);
  printf("Expected number of read misses = %d\n", numMisses);

  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );    

  for(int k=0; k<numIterations; k++){
    for(int i=0; i<bufferSize; i += stride){
      readBuffer[i] = i;
      //a = readBuffer[i];
    }
  }

  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );               

  return a;
}
