
#ifndef __SPARC64_SPLIT_PQ_H
#define __SPARC64_SPLIT_PQ_H


#include <rtems/score/chain.h>
#include <rtems/score/chain.inl>
#include <rtems/score/wkspace.h>

#include <rtems/rtems/types.h>

#include <rtems/score/sparc64.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POINTER_TO_KEY(ptr)   (ptr>>32)
#define POINTER_TO_VALUE(ptr) ((uint32_t)ptr)

//#define GAB_DEBUG
//#define GAB_DEBUG_FILL

#if defined(GAB_DEBUG)
#define GAB_DEBUG_FILL

#endif
#include <rtems/bspIo.h>

typedef struct {
  Chain_Node Node;
  uint32_t key;
  uint32_t val;
  uint32_t heap_index;
} pq_node;

/* heap implementation ... */
#define HEAP_PARENT(i) (i>>1)
#define HEAP_FIRST (1)
#define HEAP_LEFT(i) (i<<1)
#define HEAP_RIGHT(i) (HEAP_LEFT(i)+1)

#define HEAP_NODE_TO_KV(n) ((((uint64_t)n->key) << 32UL) | (uint64_t)n->val)
#define kv_value(kv) ((uint32_t)kv)
#define kv_key(kv)   (kv>>32)

// container-of magic
#define HEAP_NODE_TO_NODE(hn) \
  ((node*)((size_t)hn - ((size_t)(&((node *)0)->data))))

void sparc64_hwpq_heap_initialize( void );
void sparc64_hwpq_heap_allocate( size_t max_size );
void sparc64_hwpq_heap_insert( uint64_t kv );
void sparc64_hwpq_heap_remove( int index );
void sparc64_hwpq_heap_change_key( int index, int new_key );
void sparc64_hwpq_heap_increase_key( int index, int new_key );
void sparc64_hwpq_heap_decrease_key( int index, int new_key );
uint64_t sparc64_hwpq_heap_min( void );
uint64_t sparc64_hwpq_heap_pop_min( void );

#ifdef __cplusplus
}
#endif

#endif
