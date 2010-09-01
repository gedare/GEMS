#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <transaction.h>
#include <errno.h>
#include <string.h>
#include "btree_c.h"
#include "btree_c_util.h"

thread_data* private_data;

int num_p;

pthread_mutex_t idLock;
pthread_mutex_t bufferLock;
pthread_mutex_t insLock;

int g_id;
void * tree;

int iterations;
int insert_pct;
int ready;

void *SlaveStart()
{
  int i, id, result;
  int rand, pct;
  int local_sense = 0;

  pthread_mutex_lock(&idLock);
  id = g_id;
  g_id++;
  pthread_mutex_unlock(&idLock);
  
  //printf("Starting thread %d\n", id);
  
  private_data[id].id = id;
  private_data[id].ready_flag = 1;
  
  tm_bind_to_cabinet(id+1);
  Barrier_breaking(&local_sense, id, num_p);
  set_transaction_registers(id);
  
  /* Main Loop */
  for(i=0; i<iterations; ++i){   
    pct = ((private_data[id].rand[i%NUM_RANDS]) % 100);
    rand = private_data[id].rand[(i+7)%NUM_RANDS];
    /*think();*/
    BEGIN_WORKLOAD_TRANSACTION
    if(pct < insert_pct){
      // insert
      if(use_file)
        file_insert(tree, id);
      else
        rand_insert(private_data, tree, id, rand);
    } else {
      lookup(tree, id, rand);
    }
    END_WORKLOAD_TRANSACTION
    private_data[id].i_count++;
  }
  
  Barrier_breaking(&local_sense, id, num_p);
}

int main(int argc, char * argv[])
{
  int i, j, test_total, all_ready, total_count, preload_size;
  pthread_t* threads;
  pthread_attr_t pthread_custom_attr;
  
  use_file = 0;
  if(argc < 5 || argc > 6){
    printf("usage: btree <num_processors> <preloads> <iterations> <insert_pct> [text file]\n");
    exit(1);
  }
  
  num_p = atoi(argv[1]);
  preload_size = atoi(argv[2]);
  iterations = atoi(argv[3])/num_p;
  insert_pct = atoi(argv[4]);
  if(argc == 6){
    use_file = 1;
    load_text(argv[5]);
    printf("Loading from file:  %s\n", argv[5]);
  } else {
    printf("Using random strings.\n");
  }
  printf("Tree Struct Size: %d\n", sizeof(BTree));
  printf("Node Size: %d\n", NODE_NUM_PAIRS);
  printf("Preloads:  %d\n", preload_size);
  printf("Insert Pct:  %d\n", insert_pct);
  
  tree = BTree_create();

  pthread_mutex_init(&idLock, NULL);
  pthread_mutex_init(&bufferLock, NULL);
  pthread_mutex_init(&insLock, NULL);
  g_id = 0;
  
  /* Allocate Thread Data */
  private_data = allocate_thread_data(num_p);
  fprintf(stderr, "done allocating\n");

 /* Pre-Load the Tree */
  printf("\nstarting preload...");
  preload_tree(private_data, tree, preload_size, num_p, use_file);
  printf("done preloading\n\n");

  /* Pre-Generate Random Numbers */
  generate_rands(private_data, num_p);
  printf("done randomizing\n");

  init_transaction_state(num_p);

  /* create threads */
  pthread_attr_init(&pthread_custom_attr);
 	threads = (pthread_t *)malloc(num_p*sizeof(pthread_t));
  for(i=0; i<num_p; ++i){
    int err = pthread_create(&threads[i], &pthread_custom_attr, SlaveStart, NULL);
    if (err)
      printf("THREAD CREATION FAILED %d %s \n", err, strerror(errno));
  }

  for(i=0; i<num_p; ++i){
    pthread_join(threads[i], NULL);
  }
  
  int total_i = 0;
  for(i=0; i<num_p; ++i){
    total_i += private_data[i].i_count;
  }
  fprintf(stderr, "%d iterations completed.\n", total_i);
}
