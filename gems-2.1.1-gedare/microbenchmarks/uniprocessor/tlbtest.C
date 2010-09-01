/*****************************************************************************
 * Microbenchmark to test the number of TLB misses incurred when
 * accessing a 16 MB block of memory, n times
 *
 * A command line parameter gives the number of times to run the benchmark.
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

typedef int int32;

int main(int argc, char* argv[])
{

  if ( argc != 2 ) {
    printf("usage: %s number-of-trials\n", argv[0]);
    exit(1);
  }

  int bufferSize = 16 * 262144;
  int32 * readBuffer = (int32*)malloc(sizeof(int32)*bufferSize);
  int numTrials = atoi( argv[1] );
  int total  = 0;
  int stride = 4096;

  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );    

  for (int i = 0; i < numTrials; i++) {

    for (int j = 0; j < bufferSize / stride; j++) {
      total = total + readBuffer[j * stride] + 1;
      readBuffer[j * stride] = total;
    }

  }

  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );               

  printf("Total = %d\n", total);
  return 0;
}
