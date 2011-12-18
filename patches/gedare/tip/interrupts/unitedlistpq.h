
#ifndef __SPARC64_UNITED_LIST_PQ_H
#define __SPARC64_UNITED_LIST_PQ_H

#include "spillpq.h"

typedef struct {
  Chain_Node Node;
  uint32_t key;
  uint32_t val;
} pq_node;

#endif
