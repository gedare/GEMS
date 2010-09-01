#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <transaction.h>
#include "btree_c_util.h" 

extern pthread_mutex_t insLock;

void rand_insert(thread_data* t_data, BTree* tree, int id, int rand)
{
  int idx, l_buffer_size;
  char * rand_string;
  /*rand_string = (char *) malloc(12 * sizeof(char));*/
  idx = t_data[id].buffer_idx;
  l_buffer_size = BUFFER_SIZE;
  rand_string = &t_data[id].buffer[idx];
  t_data[id].buffer_idx += 12;

  SIMICS_ASSERT(t_data[id].buffer_idx < l_buffer_size);
  snprintf(rand_string, 12, "%d", rand);
  key_type_t key = str_to_key(rand_string);

  insert(tree, id, key, rand_string);
}

void file_insert(BTree* tree, int id)
{
  key_type_t key;
  char * string, *test_str;
  
  pthread_mutex_lock(&insLock);
  string = (char*) strtok(NULL, " \n");
  pthread_mutex_unlock(&insLock);
  
  if(string != NULL){
    key = str_to_key(string);
    insert(tree, id, key, string);
  } else {
    printf("NULL string.\n");
  }
  
}

void load_text(char* file){
  struct stat file_stats;

  text_file_fd = open(file, O_RDONLY);
  fstat(text_file_fd, &file_stats);
  off_t size = file_stats.st_size;
  file_buffer = (char *) malloc(size * sizeof(char));
  read(text_file_fd, file_buffer, size);

  file_string = (char *) strtok(file_buffer, " \n");
}

key_type_t str_to_key(char* string){
  int i, len;
  key_type_t h;

  len = strlen(string);
  
  h = 0;
  for (i = 0; i < len; i++) {
    h = (31*h + string[i]);
  }
  return h;
}

/* pre-generate random numbers*/
void generate_rands(thread_data* t_data, int num_p){
  int i, j;
  srandom(1000);
  for(j=0; j<num_p; ++j){
    for(i=0; i<NUM_RANDS; ++i){
      t_data[j].rand[i] = random();
    }
  }
}

/* pre-load the tree */
void preload_tree(thread_data* t_data, BTree* tree, int size, int num_p, int use_file){
  int i, id;
  assert(t_data != NULL);
  for(i=0; i<size; ++i){
    id = i%num_p;
    if(use_file)
      file_insert(tree, id);
    else
      rand_insert(t_data, tree, id, random());
  }
}

thread_data* allocate_thread_data(int num_p){
  int i;
  thread_data* t_data;
  
  assert((sizeof(thread_data) % 128) == 0);
  t_data = (thread_data *) memalign(128, num_p * sizeof(thread_data));
  SIMICS_ASSERT(t_data != NULL);
  bzero(t_data,(num_p * sizeof(thread_data)));
  for(i=0; i<num_p; ++i){
    t_data[i].i_count = 0;
    t_data[i].buffer_idx = 0;
  }
  return t_data;
}

void think(){
  int wait_time;
  wait_time = random() % 500;
  
  hrtime_t start = gethrtime();
  while(gethrtime() < start + wait_time);
}
