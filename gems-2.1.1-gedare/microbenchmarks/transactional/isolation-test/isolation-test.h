#include "transaction.h"

/*
 * Running this benchmark on cabernet should cause an endless loop.
 */

/********************
 * GLOBAL VARIABLES *
 ********************/

// shared variables for conflict detection
int a = 42;
double fillera[8];

// shared variables for communication
int comm_signal = 0; // value is the id of the currently "active" thread

// global, but used as private, variables
int num_t1_exes; // verify t1 aborts exactly as often as should

// barrier
//barrier_t bar; 

/*************************
 * FUNCTION DECLARATIONS *
 *************************/

// The main function for each of the two threads
// Thread 0 uses even numbered transaction id's, and thread 1 uses odd ones
// Transaction id's generally increase as the program progresses
void thread0();
void thread1();

// the function where each thread begins
void* thread_main(void* id);
