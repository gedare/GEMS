#define NUM_RANDS 128000
#define BUFFER_SIZE 12*128000

#include "btree_c.h"

typedef struct {
  int id;
  int ready_flag;
  int count;
  int* log;

  int rand[NUM_RANDS];
  char buffer[BUFFER_SIZE];
  int buffer_idx;
  int i_count;
  int pad2[26];
} thread_data;

/* file vars */
int text_file_fd;
char * file_buffer;
char * file_string;
int use_file;

char big_pad[256];
/*int first_time = true;*/

void rand_insert(thread_data* td, BTree * tree, int id, int rand);

void file_insert(BTree * tree, int id);

void load_text(char* file);

key_type_t str_to_key(char* string);

void generate_rands(thread_data* private_data, int num_p);

void preload_tree(thread_data* t_data, BTree* tree, int size, int num_p, int use_file);

thread_data* allocate_thread_data(int num_p);

void think();
