#ifndef __SPARC64_FREELIST_H
#define __SPARC64_FREELIST_H

#include <rtems/score/chain.h>
#include <rtems/rtems/types.h>
#include <rtems/score/wkspace.h>

#ifdef __cplusplus
extern "C" {
#endif


// FIXME: will this have to be protected for multitasking?
extern Chain_Control sparc64_hwpq_freelist;

void *sparc64_alloc_node(void);
void sparc64_free_node(void *n);
void sparc64_hwpq_allocate_freelist( size_t max_pq_size, size_t node_size );

#ifdef __cplusplus
}
#endif


#endif
