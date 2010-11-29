#ifndef _CONTAINEROPALH_
#define _CONTAINEROPALH_

#include "mshr.h"
#include "scheduler.h"
#include "confio.h"
#include "pseq.h"


/**
*
*/
class containeropal : public waiter_t{

public:
	
	containeropal();
	containeropal(generic_cache_template<generic_cache_block_t> * ic, generic_cache_template<generic_cache_block_t> *dc,
		generic_cache_template<generic_cache_block_t> *pc);
	~containeropal();
	
	bool CacheSameLine(pa_t, pa_t, pseq_t*);
	bool CacheWrite(pa_t , memory_inst_t *);
	bool CacheRead(pa_t , memory_inst_t *, bool ,  bool *);
	bool Fetch(pa_t , waiter_t *, bool , bool *);
	bool Retire(dynamic_inst_t *w);
 
    void Wakeup( void );        

private:
	
  /// pointer to L1 instruction cache
  generic_cache_template<generic_cache_block_t> *l1_inst_cache;
  /// pointer to L1 data cache
  generic_cache_template<generic_cache_block_t> *l1_data_cache;
  /// L1 permission cache 
  generic_cache_template<generic_cache_block_t> *l1_perm_cache;

};

#endif  /* _CONTAINEROPALH_ */
