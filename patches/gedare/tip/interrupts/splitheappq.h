
#ifndef __SPARC64_SPLIT_HEAP_PQ_H
#define __SPARC64_SPLIT_HEAP_PQ_H

#include "spill.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  Chain_Node Node;
  uint32_t key;
  uint32_t val;
  uint32_t heap_index;
} pq_node;

#ifdef __cplusplus
}
#endif

#endif
