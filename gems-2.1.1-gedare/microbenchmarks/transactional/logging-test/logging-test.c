#include "logging-test.h"
#include "transaction.h"

#include <assert.h>
#include <stdio.h>

void abort_once(int abortion_group) {
  BEGIN_ESCAPE;
  if (!retries[abortion_group]) {
    retries[abortion_group] = 1;
    END_ESCAPE;
    ABORT_TRANSACTION(0);
  } else {
    profiled[abortion_group] = 1;
    END_ESCAPE;
  }
}

void initialize() {
  printf("initialize BEGIN...\n");
  int i, j;

  version = 0;

  for (i = 0; i < NUM_COMBINATIONS; i++) {
    minterms[i] = 0;
  }

  for (j = 0; j < NUM_VERSIONS; j++) {
    for (i = 0; i < NUM_COMBINATIONS; i++) {
      minterm_versions[j][i] = 0;
    }
  }

  for (i = 0; i < NUM_TESTS; i++) {
    retries[i] = 0;
    profiled[i] = 0;
  }

  // correct output:[1, 1, 2, 1, 2, 2, 3]
  correct_output[0] = 1;
  correct_output[1] = 1;
  correct_output[2] = 2;
  correct_output[3] = 1;
  correct_output[4] = 2;
  correct_output[5] = 2;
  correct_output[6] = 3;

  printf("initialize END\n");
}

void print_usage(char* exe) {
  printf("****************************************************************\n");
  printf("* USAGE\n");
  printf("* %s <bind> \n", exe);
  printf("* <bind> - 0 on cabernet, 1 for LogTM \n");
  printf("****************************************************************\n");
  exit(-1);
}

void take_snapshot(int profile_group) {
  int j;

  BEGIN_ESCAPE {
    if (!retries[profile_group]) {
      for (j = 0; j < NUM_COMBINATIONS; j++) {
        minterm_versions[version][j] = minterms[j];
      }
      version++;
    }
    else{
      // This means we have aborted once already. Check that our checkpoint is equal to current minterms
      // (e.g. rollback occurred correctly)
      for(j=0; j < NUM_COMBINATIONS; ++j){
        if(minterm_versions[profile_group][j] != minterms[j]){
          printf("ERROR, take_snapshot value checks don't match! %d %d\n", minterm_versions[profile_group][j], minterms[j]);
          LOGTM_ASSERT(0);
        }
      }
    }
  }
  END_ESCAPE;
}

int main(int argc, char** argv) {
  int i, j;
  unsigned int bind;

  if (argc < 2) {
    print_usage(argv[0]);
  } else {
    bind = atoi(argv[1]);
  }

  initialize();

  printf("~~~~~~~~ AFTER RUBY BEGIN ~~~~~~~~\n");

  init_transaction_state(1);

  if (bind) {
    tm_bind_to_cabinet(1);
  }

  SIMICS_BREAK_EXECUTION;
  
  set_transaction_registers(0); 

  /*
  threadTransContext[0].transaction_log[0] = 5;
  fprintf(stderr, "log_addr=0x%x (%d), log_val=%d\n", &threadTransContext[0].transaction_log[0], &threadTransContext[0].transaction_log[0], threadTransContext[0].transaction_log[0]);

  for (i = 0; i < 100000; i++);
  */

  BEGIN_CLOSED_TRANSACTION(1) {

    // 1, 2, 6: [0, 0, 0, 0, 0, 0, 0]
    take_snapshot(0); // b

    minterms[3]++;
    minterms[4]++;
    minterms[5]++;
    minterms[6]++;
    
    abort_once(0);
    
    BEGIN_OPEN_TRANSACTION(0) {

      // 3, 4, 7: [0, 0, 0, 1, 1, 1, 1]
      take_snapshot(1); // d
      minterms[1]++;
      minterms[2]++;
      minterms[5]++;
      minterms[6]++;

      abort_once(1);
    } 
    COMMIT_OPEN_TRANSACTION(0);

    // 5, 8: [0, 1, 1, 1, 1, 2, 2]
    take_snapshot(2); // f

    minterms[0]++;
    minterms[2]++;
    minterms[4]++;
    minterms[6]++;

    abort_once(2);

    // 11, 12: [1, 1, 2, 1, 2, 2, 3]
    take_snapshot(3); // g
    abort_once(3);
    // These next statements are actually needed only so that simulator can abort on next memory instruction
    i = 2;
    j = 5;
  } 
  COMMIT_CLOSED_TRANSACTION(1);

  /*
  printf("%d versions produced\n", version);
  assert(version <= NUM_VERSIONS);
  
  printf("ACTUAL VERSIONS\n");
  for (i = 0; i < version; i++) {
    printf("[");
    for (j = 0; j < NUM_COMBINATIONS; j++) {
      printf("%d, ", minterm_versions[i][j]);
    }
    printf("]\n");
  }
  
  printf("\nEXPECTED OUTPUT\n");
  printf("minterms=[1, 1, 2, 1, 2, 2, 3]\n");

  printf("ACTUAL OUTPUT\n");
  printf("minterms=[");
  for (i = 0; i < NUM_COMBINATIONS; i++) {
    printf("%d, ", minterms[i]);
  }
  printf("]\n");
  */

  // check for correctness
  for(i=0; i < NUM_COMBINATIONS; ++i){
    if(minterms[i] != correct_output[i]){
      printf("ERROR, values do not match! %d %d\n", minterms[i], correct_output[i]);
      LOGTM_ASSERT(0);
    }
  }
   
  SIMICS_BREAK_EXECUTION;

  fflush(stdout);

  return 0;
}
