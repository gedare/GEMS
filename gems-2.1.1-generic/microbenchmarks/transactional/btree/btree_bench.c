

#include "btree.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int str_to_int(char* string){
  char buffer[4];
  int len = strlen(string);
  for(int i=0; i<4; ++i){
    if (i<len)
      buffer[i] = string[i];
    else
      buffer[i] = '\0';
  }
  int* tmp = (int *) buffer;
  return *tmp;
}

void insert(BTree* tree, char *string){
  if(string != NULL)
    tree->insert(str_to_int(string), string);
}


void basic_test(BTree * tree){
  insert(tree, "ahead");
  insert(tree, "go");
  insert(tree, "my");
  insert(tree, "day");
  insert(tree, "make");
  insert(tree, "you've");
  insert(tree, "to");
  insert(tree, "yourself,");
  insert(tree, "got");
  insert(tree, "ask");
  tree->print();    
}

void file_test(BTree * tree, int argc, char* argv[]){
  char * buffer;
  struct stat file_stats;
  if(argc != 2 || argv[1] == NULL){
    cout << "usage: btree_bench <input_file>" << endl;
    exit(0);
  }

  int fd = open(argv[1], O_RDONLY);
  fstat(fd, &file_stats);
  off_t size = file_stats.st_size;
  buffer = new char[size];
  read(fd, buffer, size);

 
  char *string = strtok(buffer, " \n");
  while(string != NULL){
    //cout << string;
    string = strtok(NULL, " \n");
    insert(tree, string);
  }

  tree->print();
}

void random_test(BTree * tree, int argc, char* argv[]){
  if(argc != 2 || argv[1] == NULL){
    cout << "usage: btree_bench <num inputs>" << endl;
    exit(0);
  }

  int inserts = atoi(argv[1]);

  for(int i=0; i<inserts; ++i){
    char * rand_string = new char[12];  
    int rand = random();
    sprintf(rand_string, "%d", rand);
    tree->insert(rand, rand_string);
  }
  tree->print();
}

int main(int argc, char* argv[])
{
  
  BTree* tree = new BTree();
 
  //basic_test(tree);
  file_test(tree, argc, argv);
  //random_test(tree, argc, argv);
}
