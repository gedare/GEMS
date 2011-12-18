
#ifndef __SPARC64_SPLIT_HEAP_PQ_H
#define __SPARC64_SPLIT_HEAP_PQ_H

#ifdef __cplusplus
extern "C" {
#endif

extern sparc64_spillpq_operations sparc64_splitheappq_ops;

#define SPARC64_SPILLPQ_OPERATIONS (spillpq_ops = &sparc64_splitheappq_ops)


#ifdef __cplusplus
}
#endif

#endif
