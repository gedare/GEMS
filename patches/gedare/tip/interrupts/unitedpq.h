
#ifndef __SPARC64_UNITED_PQ_H
#define __SPARC64_UNITED_PQ_H

#include "spillpq.h"

typedef struct {
  Chain_Node Node;
  uint32_t priority;
  uint32_t pointer;
} pq_node;

#endif
