#ifndef _CONTAINEROPALH_
#define _CONTAINEROPALH_

#include "mshr.h"
#include "scheduler.h"
#include "confio.h"
#include "pseq.h"
#include "containers/traceFCalls.h"
#include "dynamic.h"




/**
*
*/
class containeropal : public waiter_t{

public:
	
	containeropal(generic_cache_template<generic_cache_block_t> * ic, generic_cache_template<generic_cache_block_t> *dc,
		generic_cache_template<generic_cache_block_t> *pc);
	~containeropal();
	
	bool CacheSameLine(pa_t, pa_t, pseq_t*);
	bool CacheWrite(pa_t , memory_inst_t *);
	//bool CacheWriteAfterPermissionCheck(pa_t a, storeWrapper *w);
	bool CacheRead(pa_t , memory_inst_t *, bool ,  bool *);
	bool Fetch(pa_t , waiter_t *, bool , bool *);
	bool Retire(dynamic_inst_t *w);
	bool AllowRetire(dynamic_inst_t *w);
	bool m_allowRetire;
 
    void Wakeup( void );        

	/// pointer to L1 instruction cache
	generic_cache_template<generic_cache_block_t> *l1_inst_cache;
	/// pointer to L1 data cache
	generic_cache_template<generic_cache_block_t> *l1_data_cache;
	/// L1 permission cache 
	generic_cache_template<generic_cache_block_t> *l1_perm_cache;


	uint64 permissions_p ;
	uint64 permissionsStack_p ;
	uint64 permissions_size ;
	uint64 permissionsStack_size ;

	uint64 permissionsStack_SP ; //stack pointer for the permission cache
	uint64 permissionsStack_FP ; //Frame pointer for the permission cache

	addressList m_dynamicPermissionBuffer; //ALLOW permissions accumulate here between container switches
	

	//Runtime tracking of containers
	void RunTimeContainerTracking(pa_t pc);
	//A new container has been called
	void NewCall(container * callee);
	//A return from a container
	void Return(container * callee);
	//Inside a container
	void InsideCall(container * callee);
	//Add dynamic range - called when ALLOW is issued 
	void AddDynamicRange(pa_t startAddress, uint64 size, byte_t perm,dynamic_inst_t *w);
	
 	

	//FCalls : save dynamic ranges to stack, load static ranges
	//Returns : load static ranges, load dynamic ranges from stack
	enum container_stage_t {
    BUSY,
	WAITONCACHE,
	IDLE
  	};

	int m_pendingWriteRequests;
	int m_pendingReadStaticRequests;
	int m_pendingReadDynamicRequests;


	int m_pendingWriteRequestsPending;
	int m_pendingReadStaticRequestsPending;
	int m_pendingReadDynamicRequestsPending;
	
	
	container_stage_t m_stage; 

	void SetStage(container_stage_t);
	container_stage_t GetStage(){return m_stage;};
	const char * printStage( container_stage_t stage );

	void Tick(); //advance loading the permissions from cache
	container * GetCurrentContainer( );
	void LoadStaticPermissionsFromCache(container * c);
	void LoadDynamicPermissionsFromCache(container * c);
	void SavePermissionsToCache(container * c);
	void PushPermissionStack();
	void PopPermissionStack();

	// load and stores will have to wait if the container list is not loaded.
	// they get notified when everything is ready.
	wait_list_t	m_wait_list; 


	
};




void LoadDecodedAccessListFile(const char *);
void LoadContainerCallListFile(const char *);


class simple_waiter_t: public waiter_t{
	public:
	waiter_t *parent;
	void Wakeup( void ){
		parent->Wakeup();
		delete this;
	}
	simple_waiter_t();
	simple_waiter_t(waiter_t * p){ parent = p; }

};


class storeWrapper: public waiter_t{
	
public:
	storeWrapper();
	storeWrapper(pa_t, store_inst_t *, containeropal *);
	~storeWrapper();

void Wakeup( void );
store_inst_t * getStoreInst();
	int internalCounter;
	
	char dissasembled_instr_forDEBUG_buff[100];
	wait_list_t	m_wait_list;
	simple_waiter_t *perm_waiter;
	simple_waiter_t *data_waiter;
	

private:
	store_inst_t * m_storeInst;	
	containeropal * m_containerOpal;
	pa_t m_addr;
	
};


class pcd_inst_t : public memory_inst_t {

public:
  /** @name Constructor(s) / Destructor */
  //@{
  /** Constructor: uses default constructor */
  pcd_inst_t( static_inst_t *s_inst, 
                int32 window_index,
                pseq_t *pseq,
                abstract_pc_t *at,
                pa_t physicalPC,
                trap_type_t trapgroup,
                uint32 proc
                );
	
  /** Destructor: uses default destructor. */
  virtual ~pcd_inst_t();

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  void Retire( abstract_pc_t *a );
 
  void Execute();

  void Complete();

  static listalloc_t m_myalloc;

    /// issue this memory transaction to the cache
  bool accessCache( void );

  /// The same as accessCache() except used at Retirement time
  bool accessCacheRetirement(void);

  trap_type_t  accessDataMMU(){
    return addressGenerate( OPAL_STORE );
  }

  uint64 m_startaddr;
  uint64 m_size;
  byte_t m_perm;
  
};



#endif  /* _CONTAINEROPALH_ */
