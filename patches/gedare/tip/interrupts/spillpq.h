
#ifndef __SPARC64_PQ_H
#define __SPARC64_PQ_H


#include <rtems/score/chain.h>
#include <rtems/rtems/types.h>

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

extern void sparc64_spillpq_allocate( size_t max_pq_size );
extern void sparc64_spillpq_initialize( int size );
extern void sparc64_spillpq_insert( uint64_t p );
extern uint64_t sparc64_spillpq_first( void );
extern uint64_t sparc64_spillpq_pop( void );

#ifdef __cplusplus
}
#endif

#endif
