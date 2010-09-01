#include "isolation-test.h"
#include "transaction.h"
#include <assert.h>

int num_threads;

void thread0() {
  int local = 1;
  int iter = 0;
  printf("thread0\n");

  for(iter = 0; iter < 1000; ++iter){
    BEGIN_CLOSED_TRANSACTION(0) {
      local = 1;
        BEGIN_OPEN_TRANSACTION(0) 
        comm_signal = 0;
        local = comm_signal;
        COMMIT_OPEN_TRANSACTION(0);
    }
    COMMIT_CLOSED_TRANSACTION(0);
    LOGTM_ASSERT(local == 0);
  }
  //printf("thread0 local=%d\n", local);

  /*
  BEGIN_CLOSED_TRANSACTION(0) {
    local = a; // read a
    
    BEGIN_OPEN_TRANSACTION(0) {
      comm_signal = 1;
    }
    COMMIT_OPEN_TRANSACTION(0);
 
    while (comm_signal) {
      // busy wait, holding a in read set
    }
  }
  COMMIT_CLOSED_TRANSACTION(0);
  printf("Thread0 local_exp=42 local_act=%d\n", local);
  */
}

void thread1() {
  int local = 0;
  int aborted = 0;
  int local_signal;
  int iter = 0;
  num_t1_exes = 0;

  printf("thread1\n");

  for(iter =0; iter < 1000; ++iter){
    BEGIN_CLOSED_TRANSACTION(1) {
      local = 0;
      BEGIN_OPEN_TRANSACTION(1) {
        comm_signal = 1;
        local = comm_signal;
      }
      COMMIT_OPEN_TRANSACTION(1);
    }
    COMMIT_CLOSED_TRANSACTION(1);
    LOGTM_ASSERT(local == 1);
  }
  //printf("thread1 local=%d\n", local);

  /*
  BEGIN_CLOSED_TRANSACTION(1) {

    while (!comm_signal) {
      // "busy wait"
    }

    BEGIN_OPEN_TRANSACTION(1) {
      num_t1_exes++;
    }
    COMMIT_OPEN_TRANSACTION(1);

    // verify reading a is OK
    local = a;
    local++;

    if (!aborted) { // haven't already done this test
      BEGIN_OPEN_TRANSACTION(3) {
        aborted = 1;
      }
      COMMIT_OPEN_TRANSACTION(3);

      // verify writing a is not OK
      a = local;
    } else {

      BEGIN_OPEN_TRANSACTION(5) {
        comm_signal = 0;
      }
      COMMIT_OPEN_TRANSACTION(5);
    }
  }
  COMMIT_CLOSED_TRANSACTION(1);
  printf("Thread1 local_exp=43 local_act=%d\n", local);
  */
}

void* thread_main(void* id) {
  int myid = (int) id;
  int local_sense = 0; // for sense-reversing barrier

  tm_bind_to_cabinet(myid + 1);
  Barrier_breaking(&local_sense, myid, num_threads);
  set_transaction_registers(myid);

  //BEGIN_WORKLOAD_TRANSACTION;
  if (myid == 0) {
    thread0();
  } else {
    thread1();
  }
  //END_WORKLOAD_TRANSACTION;
}

int main(int argc, char** argv) {
  // variables
  int i;
  pthread_t* threads;
  pthread_attr_t attr;
  num_threads = 2;
  

  init_transaction_state(num_threads);

  //thread0();
  // start the threads in the thread_main routine
  threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
  assert(!pthread_attr_init(&attr));
  assert(!pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM));
  for (i = 0; i < num_threads; i++) {
    pthread_create(&threads[i], &attr, thread_main, (void*) i);
  }

  // as each thread completes, merge with it
  for (i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  SIMICS_BREAK_EXECUTION;

  // verify functionality
  /*
  printf("a_exp=42 a_act=%d\n", a);
  printf("num_t1_exes_exp=2 num_t1_exes_act=%d\n", num_t1_exes);
  */

  // clean up
  //barrier_destroy(&bar);
  pthread_attr_destroy(&attr);
  free(threads);

  return 0;
}
