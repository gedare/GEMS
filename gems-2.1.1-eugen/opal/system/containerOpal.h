#ifndef _CONTAINEROPALH_
#define _CONTAINEROPALH_

#include "mshr.h"
#include "scheduler.h"
#include "confio.h"
#include "pseq.h"
#include "containers/traceFCalls.h"
#include "dynamic.h"
#include "containerOpalSimpleWaiter.h"


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
	bool AccessCacheThroughContainers(pa_t a, memory_inst_t *w, int operation /* 0=read; 1=write */);
	bool Fetch(pa_t , waiter_t *, bool , bool *);
	bool Retire(dynamic_inst_t *w);
	bool AllowRetire(dynamic_inst_t *w);
	bool CheckMemoryAccess(pa_t addr, uint64 size);
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

	uint64	permissionsContainers_p;
	uint64	permissionsContainers_size;

	uint64 permissionsMulti_p;

	uint64 dynamicPermissionBuffer_size;
	uint64 dynamicPermissionBuffer_p ; 
	uint64 dynamicContainerRuntimeRecord_size ;
	uint64 dynamicContainerRuntimeRecord_p ; 



	addressList m_dynamicPermissionBuffer; //ALLOW permissions accumulate here between container switches
	addressList m_dynamicContainerRuntimeRecord; 	//on call and returns dynamic ranges transfer from the dynamicPermissionBuffer here
													//also these are saved on CALLS, and loaded on Returns
	addressList m_staticContainerRuntimeRecord;
													
	int m_dynamicContainerRuntimeRecordSize;
	int m_dynamicContainerRuntimeRecordAlreadyPushed;
	int	m_dynamicPermissionBufferSize;

	//Runtime tracking of containers
	void RunTimeContainerTracking(pa_t pc, dynamic_inst_t *w);
	//A new container has been called
	void NewCall(container * callee);
	//A return from a container
	void Return(container * callee);
	//Inside a container
	void InsideCall(container * callee);
	//Add dynamic range - called when ALLOW is issued 
	void AddDynamicRange(pa_t startAddress, uint64 size,byte_t multi, byte_t perm,dynamic_inst_t *w);
	void ContextSwitch(pa_t startAddress, dynamic_inst_t *w);
 	void ContextSwitchEnd();

	//FCalls : save dynamic ranges to stack, load static ranges
	//Returns : load static ranges, load dynamic ranges from stack
	enum container_stage_t {
	WAITONCACHE,
	SAVEDYN,
	LDDYN,
	LDSTATIC,
	IDLE,
	LDMULTI, 
	CTXSWITCHSTART,
	CTXSWITCHRESET,
	CTXSWITCHLOADNEW,
	CTXSWITCHEND
  	};

	long long m_pendingWriteRequests;
	int m_pendingReadStaticRequests;
	long long m_pendingReadDynamicRequests;
	int m_pendingReadMultiRequests;
	


	//int m_pendingWriteRequestsPending;
	int m_pendingReadStaticRequestsPending;
	//int m_pendingReadDynamicRequestsPending;
	int m_pendingReadMultiRequestsPending;
	
	//int m_pendingCtxSwitchSaveDyn;
	//int m_pendingCtxSwitchLoadDyn;
	long long m_pendingCtxSwitchLoadContainerList;
	long long m_pendingCtxSwitchLoadContainerListPending;
	long long m_pendingCtxSwitchSaveDynContainerRunTimeRecord;
	long long m_pendingCtxSwitchSaveDynPermBuffer;
	long long m_pendingCtxSwitchLoadDynContainerRunTimeRecord;
	long long m_pendingCtxSwitchLoadDynPermBuffer;
	staticpermssions* m_oldThreadContext;
	staticpermssions* m_newThreadContext;
	
	
	container_stage_t m_stage;
	int m_loadsPendingValidation;
	int m_storesPendingValidation;
	

	void SetStage(container_stage_t);
	container_stage_t GetStage(){return m_stage;};
	const char * printStage( container_stage_t stage );

	void Tick(); //advance loading the permissions from cache
	container * GetCurrentContainer( );
	void LoadStaticPermissionsFromCache(container * c);
	//void LoadDynamicPermissionsFromCache(container * c);
	void LoadPermissionsList(container * c, pa_t location_base, long long * remainingCounter_p, addressList toSave, int * toSaveSize);
	bool CheckPedingWaiters(pa_t addr, uint64 size);
	void LoadMultiPermissionsFromCache(container * c);
	void LoadCtxContainerList(container * c);

	
	//void SavePermissionsToCache(container * c);
	void SavePermissionList(container *c, pa_t location_base, long long * remainingCounter_p, addressList *toSave, int * toSaveSize );
	void SavePermissionToCacheOnIdle(container * c);
	void PushPermissionStack(container * callee);
	void PushPermissionStackAfter();
	void PopPermissionStack(container * callee);
	void PopPermissionStackAfter();
	void transferDynPermBufferToDynContainerRunTimeRecord();
	void PrintStats();

	// load and stores will have to wait if the container list is not loaded.
	// they get notified when everything is ready.
	wait_list_t	m_wait_list; 



	uint64 m_stat_numStalledLoads;
	uint64 m_stat_numTotalLoads;
	
	uint64 m_stat_numStalledStores;
	uint64 m_stat_numTotalStores;

	uint64 m_stat_LoadStoresFoundWithPartialLoadedContainer;
	uint64 m_stat_LoadStoresFoundWithCheckingWaiters;
	
	uint64 m_stat_numStalledCallContainerSwitches;
	uint64 m_stat_numStalledReturnContainerSwitches;

	uint64 m_stat_numTotalCallContainerSwitches;
	uint64 m_stat_numTotalReturnContainerSwitches;

	uint64 m_stat_numTotalCommitedCallContainerSwitches;
	uint64 m_stat_numTotalCommitedReturnContainerSwitches;
	
	//uint64 m_stat_numStalledCyclesLoads;
	//uint64 m_stat_numStalledCyclesStores;
	//uint64 m_stat_numStalledCyclesContainerSwitches;
	//uint64 m_stat_tempLoadStart;
	//uint64 m_stat_tempLoadComplete;

	uint64 m_stat_StageIDLE;
	uint64 m_stat_StageLDDYN;
	uint64 m_stat_StageLDSTATIC;
	uint64 m_stat_StageSTDYN;
	uint64 m_stat_StageWAITONCACHE;
	uint64 m_stat_StageIDLESAVE;
	uint64 m_stat_StageLDMULTI;
	uint64 m_stat_StageCTXSWITCH;
	
	
	int m_debug_memleakNew;
	int m_debug_memleakDelete;

	container* pContainerTable;
};

void LoadDecodedAccessListFile(const char *);
void LoadContainerCallListFile(const char *);


class memOpValidator: public waiter_t{
	
public:
	memOpValidator();
	memOpValidator(pa_t, memory_inst_t *, containeropal *, int type);
	~memOpValidator();

	void Wakeup( void );
	memory_inst_t * getMemoryInst();
	int internalCounter;
	
	char dissasembled_instr_forDEBUG_buff[100];
	wait_list_t	m_wait_list;
	waiter_t *perm_waiter;
	waiter_t *data_waiter;
	containeropal * m_containerOpal;

private:
	memory_inst_t * m_memInst;	
	int m_type; //0-LOAD ; 1-STORE
	pa_t m_addr;
	uint64 size;
	
};

class store_waiter_t: public waiter_t{
 public:
	void Wakeup( void );
	waiter_t *parent;
	store_waiter_t(){};
	store_waiter_t(waiter_t * p,pa_t addr, uint64 sz ){ parent = p;m_addr = addr ;m_size = sz; }
	~store_waiter_t(){};
	pa_t m_addr;
	uint64 m_size;
	bool TryWake( pa_t addr, size_t size );
};

class load_waiter_t: public waiter_t{
	public:
	void Wakeup( void );
	waiter_t *parent;
	load_waiter_t(){};
	load_waiter_t(waiter_t * p,pa_t addr, uint64 sz ){ parent = p;m_addr = addr ;m_size = sz; }
	~load_waiter_t(){};
	pa_t m_addr;
	uint64 m_size;
	bool TryWake( pa_t addr, size_t size );
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
  byte_t m_multi;
  byte_t m_context_switch;
  byte_t m_perm;
  
};



#endif  /* _CONTAINEROPALH_ */
