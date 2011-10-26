
#include "hfa.h"
#include "hfacore.h"

// branch prediction schemes
#include "gshare.h"
#include "agree.h"
#include "yags.h"
#include "igshare.h"
#include "mlpredict.h"

#include "utimer.h"
#include "lsq.h"
#include "ipagemap.h"
#include "tracefile.h"
#include "branchfile.h"
#include "memtrace.h"
#include "fileio.h"
#include "confio.h"
#include "debugio.h"
#include "checkresult.h"
#include "pipestate.h"
#include "rubycache.h"
#include "mf_api.h"
#include "histogram.h"
#include "stopwatch.h"
#include "sysstat.h"
#include "dtlb.h"
#include "writebuffer.h"
#include "power.h"
#include "storeSet.h"
#include "regstate.h"

#include "flow.h"
#include "actor.h"
#include "flatarf.h"
#include "dependence.h"
#include "ptrace.h"
#include "Vector.h"

#include "pseq.h"
#include "containerOpal.h"
#include "containers/trace.h"
#include "containers/traceFCalls.h"

#define PERMISSION_RECORD_SIZE sizeof(packed_permission_rec)


packed_permission_rec create_permrec(uint64 startaddr,uint64 endaddr,byte_t perm)
{
	packed_permission_rec p;
	p.startaddr = startaddr;
	p.permandsize = ((uint64)perm) << 62 | (endaddr - startaddr);
	return p;
}
packed_permission_rec pack_permrec(unpacked_permission_record up)
{
	packed_permission_rec p;
	p.startaddr = up.startaddr;
	p.permandsize = ((uint64)up.perm) << 62 | (up.endaddr - up.startaddr);
	return p;
}

unpacked_permission_rec unpack_permrec(packed_permission_record p)
{
	unpacked_permission_rec up;
	up.startaddr = p.startaddr;
	uint64 size = maskBits64(p.permandsize,0,61);
	//if(size > 1000) {
	//	printf("there must be something wrong in %s startaddr=%llx size=%lld", __PRETTY_FUNCTION__,up.startaddr,size );
	//	fflush(stdin);
	//}
	//assert(size < 1000);
	up.endaddr = up.startaddr + size;
	up.perm = (byte_t) maskBits64(p.permandsize,62,63);
	return up;
}




void LoadContainersFromDecodedAccessListFile(const char * strDecodedAccessListFile);
void LoadContainersChildContainers(const char * FileWithPrefix);


containeropal * globalreference;
pa_t retAdr;

containeropal::containeropal(generic_cache_template<generic_cache_block_t> * ic, generic_cache_template<generic_cache_block_t> *dc,
	generic_cache_template<generic_cache_block_t> *pc)
{
	this->l1_inst_cache = ic;
	this->l1_data_cache = dc;
	this->l1_perm_cache = pc;

	
	//permissions_p = mySimicsIntSymbolRead("&Permissions");
	//permissionsStack_p =mySimicsIntSymbolRead("&Permissions_Stack");
	//permissions_size = mySimicsIntSymbolRead("Permissions_size");
	//permissionsStack_size = mySimicsIntSymbolRead("Permissions_Stack_size");

	permissions_p = NULL;
	permissionsStack_p = NULL;
	permissions_size = 0;
	permissionsStack_size = 0;

	permissionsContainers_p = NULL;
	permissionsContainers_size = 0;

	dynamicPermissionBuffer_size = 0;
	dynamicPermissionBuffer_p = NULL ; 
	dynamicContainerRuntimeRecord_size = 0;
	dynamicContainerRuntimeRecord_p = NULL ; 

	permissionsMulti_p = NULL;
	thread_active = NULL;

	ThreadAdd(0,0);
	container_simicsInit();

//	#ifdef DEBUG_GICA_THREAD_SWITCHING
//		char rtemsname[10];
//		toStringRTEMSTaksName(rtemsname, thread_active->thread_name);
//		DEBUG_OUT("%s %0x%llx %s \n",__PRETTY_FUNCTION__,thread_active->thread_id, rtemsname);
//	#endif

	m_stage = IDLE;

	m_loadsPendingValidation = 0;
	m_storesPendingValidation = 0;

	m_pendingWriteRequests = 0;
	m_pendingReadStaticRequests = 0;
	m_pendingReadDynamicRequests = 0;
	//m_pendingWriteRequestsPending = 0 ;
 	m_pendingReadMultiRequests = 0;
	m_pendingReadMultiRequestsPending = 0; 

	
	m_pendingReadStaticRequestsPending = 0 ;
	//m_pendingReadDynamicRequestsPending = 0 ;

	m_pendingCtxSwitchLoadContainerList = 0;
	m_pendingCtxSwitchLoadContainerListPending = 0;
	m_pendingCtxSwitchSaveDynContainerRunTimeRecord = 0;
	m_pendingCtxSwitchSaveDynPermBuffer = 0;
	m_pendingCtxSwitchLoadDynContainerRunTimeRecord = 0;
	m_pendingCtxSwitchLoadDynPermBuffer = 0;

    m_oldThreadContext = NULL;
	m_newThreadContext = NULL;


	m_stat_numStalledLoads = 0;
	m_stat_numStalledStores = 0;
	m_stat_numStalledCallContainerSwitches = 0;
	m_stat_numStalledReturnContainerSwitches = 0;
	m_stat_LoadStoresFoundWithPartialLoadedContainer = 0;
	m_stat_LoadStoresFoundWithCheckingWaiters = 0;


	m_stat_numTotalLoads = 0;
	m_stat_numTotalStores = 0;
	m_stat_numTotalCallContainerSwitches = 0;
	m_stat_numTotalReturnContainerSwitches = 0;

	m_stat_numTotalCommitedCallContainerSwitches = 0;
	m_stat_numTotalCommitedReturnContainerSwitches = 0;

	m_stat_StageIDLE = 0;
	m_stat_StageLDDYN = 0;
	m_stat_StageLDSTATIC = 0;
	m_stat_StageSTDYN = 0 ;
	m_stat_StageWAITONCACHE = 0;
	m_stat_StageIDLESAVE = 0;
	m_stat_StageLDMULTI = 0;
	m_stat_StageCTXSWITCH = 0;
	
	
	m_wait_list.wl_reset();
	m_allowRetire = TRUE;

	thread_active->permissionsStack_FP = permissionsStack_p;
	thread_active->permissionsStack_SP = permissionsStack_p+8;
		
	#ifdef DEBUG_GICA5
		DEBUG_OUT("%s permissions_p=%llx permissionsStack_p=%llx permissions_size=%lld permissionsStack_size=%lld %s\n",
			__PRETTY_FUNCTION__,permissions_p, permissionsStack_p, permissions_size, permissionsStack_size,printStage(m_stage));
	#endif

	m_dynamicPermissionBuffer = NULL;
	m_dynamicPermissionBufferSize = 0;
	m_dynamicContainerRuntimeRecord = NULL;
	m_dynamicContainerRuntimeRecordSize = 0;
	m_dynamicContainerRuntimeRecordAlreadyPushed = 0;

	m_staticContainerRuntimeRecord = NULL;

	m_debug_memleakNew = 0;
	m_debug_memleakDelete = 0;

	
	globalreference = this;
	pContainerTable = (container *)containerTable;
}

containeropal::~containeropal()
{

}

bool containeropal::CacheSameLine(pa_t pc1, pa_t pc2, pseq_t * w){
	#ifdef DEBUG_GICA4
		DEBUG_OUT("%s %lld %s %llx %llx\n",__PRETTY_FUNCTION__, w->getLocalCycle(),l1_inst_cache->GetName(),pc1, pc2);
	#endif
	return this->l1_inst_cache->same_line(pc1,pc2);
}

bool containeropal::Fetch(pa_t a, waiter_t *w, bool data_request = true,
            bool *primary_bool = NULL)
{
	#ifdef DEBUG_GICA4
			proc_waiter_t *wp = (proc_waiter_t *) w;
			DEBUG_OUT("%s %lld %s %llx\n",__PRETTY_FUNCTION__, wp->m_pseq->getLocalCycle(),l1_inst_cache->GetName(),a);
	#endif
	return this->l1_inst_cache->Read(a,w,data_request,primary_bool);
}

bool containeropal::AccessCacheThroughContainers(pa_t a, memory_inst_t *w, int operation /* 0=read; 1=write */)
{
	bool permissionHit = false;
	bool cacheHit = false;
	
	pa_t whereToLookForStaticPermissionList = permissions_p;
	//get current container and use the offset from it
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		whereToLookForStaticPermissionList += t.containerObj->opalOffsetLocateContainerInPermissions;
	}

	memOpValidator * masterWaiter = new memOpValidator(whereToLookForStaticPermissionList,(memory_inst_t *)w,this, 1/*STORE*/); 
	masterWaiter->data_waiter = new simple_waiter_t(masterWaiter);
	m_debug_memleakNew +=2;

	int * mptr_opsPendingValidations = NULL;
	uint64 * mptr_stat_numStalled = NULL;
	uint64 * m_stat_numTotals = NULL;
	
	if(operation == 1)
	{
		mptr_opsPendingValidations = &m_storesPendingValidation;
		mptr_stat_numStalled = &m_stat_numStalledStores;
		m_stat_numTotals = &m_stat_numTotalStores;
	}
	else
	{
		mptr_opsPendingValidations = &m_loadsPendingValidation;
		mptr_stat_numStalled = &m_stat_numStalledLoads;
		m_stat_numTotals = &m_stat_numTotalLoads;
	}
	
	
	(*m_stat_numTotals)++;
	//access perm container if it is ready
	if(GetStage() == IDLE){
		permissionHit = true;
		masterWaiter->internalCounter --;
	}
	else if( CONTMGR_CHECKPARTIALLOADEDCONTAINER && (permissionHit = CheckMemoryAccess(a, 8)) ){
		masterWaiter->internalCounter --;
		m_stat_LoadStoresFoundWithPartialLoadedContainer ++;
	}
	else{
		permissionHit = false;
		if(operation == 1)
			masterWaiter->perm_waiter = new store_waiter_t(masterWaiter, a, 8);
		else 
			masterWaiter->perm_waiter = new load_waiter_t(masterWaiter, a, 8);

		m_debug_memleakNew++;
		(masterWaiter->perm_waiter)->InsertWaitQueue(m_wait_list);
		(*mptr_stat_numStalled)++;
		(*mptr_opsPendingValidations)++;
	}
	if(operation == 1)
		cacheHit = this->l1_data_cache->Write(a,masterWaiter->data_waiter);
	else
		cacheHit = this->l1_data_cache->Read(a,masterWaiter->data_waiter,true, NULL);
	
	if(cacheHit){
		masterWaiter->internalCounter --;
		m_debug_memleakDelete++;
		delete masterWaiter->data_waiter;
		masterWaiter->data_waiter = NULL;
	}

	if(masterWaiter->internalCounter != 0)
	{
		w->InsertWaitQueue(masterWaiter->m_wait_list);
	}
	else {
		delete masterWaiter;
		m_debug_memleakDelete++;
	}

    #ifdef DEBUG_GICA5
			char buf[128];
			w->getStaticInst()->printDisassemble(buf);
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %s %lld %s %llx %s PERMHIT=%d CACHEHIT=%d %s\n",__PRETTY_FUNCTION__,
				w->printStage(w->getStage()), w->m_pseq->getLocalCycle(),l1_data_cache->GetName(),a, buf, permissionHit, cacheHit,printStage(GetStage()));
			fflush(stdout);
	#endif

	return cacheHit && permissionHit;
}

bool containeropal::CacheWrite(pa_t a, memory_inst_t *w){

	return AccessCacheThroughContainers(a, w, 1 /* 0=read; 1=write */);
}

bool containeropal::CacheRead(pa_t a, memory_inst_t *w, bool data_request = true,
            bool *primary_bool = NULL){
    
	return AccessCacheThroughContainers(a, w, 0 /* 0=read; 1=write */);
}

bool containeropal::Retire(dynamic_inst_t *w){
    #ifdef DEBUG_GICA5 
		char buf[128];
  		w->getStaticInst()->printDisassemble(buf);
		DEBUG_OUT("%s %lld %llx %s %s \n",__PRETTY_FUNCTION__,w->m_pseq->getLocalCycle(),w->getPC(),w->printStage(w->getStage()),buf);
	#endif

}



void containeropal::RunTimeContainerTracking(pa_t pc, dynamic_inst_t *w)
{
	#ifdef DEBUG_GICA5 
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %llx \n",__PRETTY_FUNCTION__,pc);
	#endif

	m_allowRetire = true;
	if(thread_active == NULL){DEBUG_OUT("%s %s \n",__PRETTY_FUNCTION__,"thread_active is NULL");return;}
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(returnAddressStack == NULL){DEBUG_OUT("%s %s \n",__PRETTY_FUNCTION__,"returnAddressStack is NULL");return;}
	int returned = 0;
	//check for function return
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		
		while(t.returnAddress == pc || t.containerObj->endAddress == pc)
		{
			m_stat_numTotalReturnContainerSwitches++;
			//only allow containers to switch if the the container is not busy loading  permssions
  			// or if there are no pending load/stores
  			if(m_loadsPendingValidation + m_storesPendingValidation == 0 
					&& m_pendingWriteRequests && m_pendingReadDynamicRequests == 0 )
					{
						if (Waiting()) RemoveWaitQueue();
						m_pendingReadStaticRequestsPending =0;
						m_pendingReadDynamicRequests =0;
						SetStage(IDLE);
  					}
			if(m_stage == IDLE ){ 
				stack_pop(returnAddressStack);
				returned = 1;
				#ifdef DEBUG_GICA8 
					for(int i = 0; i< returnAddressStack->size; i++) DEBUG_OUT("|\t");
					DEBUG_OUT("}\n");
				#endif
				Return(t.containerObj);
				m_allowRetire = true;
				if(!stack_empty(returnAddressStack)) t = stack_top(returnAddressStack);
				else break;
			}
			else{
				#ifdef DEBUG_GICA7 
					for(int j = 0; j< returnAddressStack->size; j++) DEBUG_OUT("|\t");
					char buf[128];
  					w->getStaticInst()->printDisassemble(buf);
					DEBUG_OUT("DELAY RETIRE %s pending:%d %s\n", printStage(m_stage), m_loadsPendingValidation + m_storesPendingValidation, buf);
				#endif
				m_allowRetire = false;
				m_stat_numStalledReturnContainerSwitches++;
				break;
			}
		}
	}

	if(!returned && m_allowRetire){
		//if it was not a function return , it is either a function call ( push to container stack in this case) or a regular intruction executing (within the container)
		container *foundSearch = search(pc);
		if(foundSearch){
			
			if(!foundSearch->nonFunction){
				m_stat_numTotalCallContainerSwitches++;
				//only allow containers to switch if the the container is not busy loading  permssions
  				// or if there are no pending load/stores
  				if( m_stage != SAVEDYN && m_loadsPendingValidation + m_storesPendingValidation == 0 
					&& m_pendingWriteRequests == 0 && m_pendingReadDynamicRequests == 0)
					{
						if (Waiting()) RemoveWaitQueue();
						m_pendingReadStaticRequestsPending =0;
						m_pendingReadDynamicRequests =0;
						SetStage(IDLE);
  					}
				if(m_stage == IDLE ){ 
					stackObject t;
					t.containerObj = foundSearch;
					t.returnAddress = getRet();//return_addr;
					#ifdef DEBUG_GICA8 
						for(int j = 0; j< returnAddressStack->size; j++) DEBUG_OUT("|\t");
						DEBUG_OUT("%s {\n", foundSearch->name);
					#endif
					stack_push(returnAddressStack, t);
					NewCall(t.containerObj);
					m_allowRetire = true;
				}
				else{
					#ifdef DEBUG_GICA7 
						for(int j = 0; j< returnAddressStack->size; j++) DEBUG_OUT("|\t");
						char buf[128];
  						w->getStaticInst()->printDisassemble(buf);
						DEBUG_OUT("DELAY RETIRE %s pending:%d %s\n", printStage(m_stage), m_loadsPendingValidation + m_storesPendingValidation,buf);
					#endif
					m_stat_numStalledCallContainerSwitches++;
					m_allowRetire = false;
				}
			}
			else{
				#ifdef DEBUG_GICA8 
					DEBUG_OUT("%s\n", foundSearch->name);
				#endif
			}
		}
		else{
			if(!stack_empty(returnAddressStack)){
				stackObject t = stack_top(returnAddressStack);
				InsideCall(t.containerObj);
				if( t.containerObj->entryAddress <= pc && t.containerObj->endAddress >= pc) 
					retAdr = pc + 4;
			}
		}
	}
}

bool containeropal::AllowRetire(dynamic_inst_t *w)
{

	pa_t pc = w->getPC();
	if(w->getTrapType() == Trap_NoTrap) containeropal::RunTimeContainerTracking(pc, w);

	#ifdef DEBUG_GICA5 
		char buf[128];
  		w->getStaticInst()->printDisassemble(buf);
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %lld %llx %s %s ",__PRETTY_FUNCTION__,w->m_pseq->getLocalCycle(),w->getPC(),w->printStage(w->getStage()),buf);
		DEBUG_OUT("getTrapType =%d  m_allowRetire=%s \n",w->getTrapType(),m_allowRetire?"TRUE":"FALSE"); 
		
	#endif
	
	return m_allowRetire;
}

void containeropal::NewCall(container * callee){
	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s %s\n",__PRETTY_FUNCTION__,callee->name, printStage(m_stage));
	#endif
	m_stat_numTotalCommitedCallContainerSwitches++;

	m_staticContainerRuntimeRecord = freeAddressList(m_staticContainerRuntimeRecord);
		
	PushPermissionStack(callee);
	SetStage(SAVEDYN);
	Tick();
}

void containeropal::InsideCall(container * callee){
	#ifdef DEBUG_GICA5 
		DEBUG_OUT("%s INSIDE %s \n",__PRETTY_FUNCTION__,callee->name);
	#endif
}


void containeropal::PushPermissionStack(container * callee){

	assert(thread_active->permissionsStack_SP <= (permissionsStack_p + permissionsStack_size));

	m_pendingReadStaticRequestsPending = callee->opalSizeOfPermissionLists;
	m_pendingReadStaticRequests = m_pendingReadStaticRequestsPending;

	m_pendingWriteRequests = m_dynamicContainerRuntimeRecordSize;
	//m_pendingWriteRequests = m_pendingWriteRequestsPending;
	
	//save FP to stack top : thread_active->permissionsStack_FP
	uint64 newSP = thread_active->permissionsStack_SP + m_pendingWriteRequests * PERMISSION_RECORD_SIZE + 8;
	myMemoryWrite(newSP,thread_active->permissionsStack_FP,8);
	thread_active->permissionsStack_FP = thread_active->permissionsStack_SP;
	thread_active->permissionsStack_SP = newSP;


	//the ranges might have been already saved on IDLE
	m_pendingWriteRequests = m_dynamicContainerRuntimeRecordSize - m_dynamicContainerRuntimeRecordAlreadyPushed;
	//m_pendingWriteRequests = m_pendingWriteRequestsPending;

	#ifdef DEBUG_GICA_PushPermissionStack
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %llx\n",__PRETTY_FUNCTION__, callee->entryAddress);
		//DEBUG_OUT("%s  thread_active->permissionsStack_SP=%llx thread_active->permissionsStack_FP=%llx m_dynamicContainerRuntimeRecordSize=%d\n",__PRETTY_FUNCTION__, thread_active->permissionsStack_SP, thread_active->permissionsStack_FP, m_dynamicContainerRuntimeRecordSize);
	#endif

}

void containeropal::PushPermissionStackAfter(){

	#ifdef DEBUG_GICA_PushPermissionStack_After
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s  thread_active->permissionsStack_SP=%llx thread_active->permissionsStack_FP=%llx m_dynamicContainerRuntimeRecordSize=%d\n",__PRETTY_FUNCTION__, thread_active->permissionsStack_SP, thread_active->permissionsStack_FP, m_dynamicContainerRuntimeRecordSize);
	#endif
	m_dynamicContainerRuntimeRecordSize = 0;
	m_dynamicContainerRuntimeRecord = freeAddressList(m_dynamicContainerRuntimeRecord);
	m_dynamicContainerRuntimeRecord = NULL;
	transferDynPermBufferToDynContainerRunTimeRecord();
}


void containeropal::transferDynPermBufferToDynContainerRunTimeRecord()
{

	#ifdef DEBUG_GICA_DYNAMIC_RANGE
		if(m_dynamicContainerRuntimeRecordSize > 0 || m_dynamicPermissionBufferSize > 0) {
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %d m_dynamicContainerRuntimeRecord = ",__PRETTY_FUNCTION__,m_dynamicContainerRuntimeRecordSize);
			printAddressList(stdoutPrintBuffer,m_dynamicContainerRuntimeRecord);
			DEBUG_OUT("m_dynamicPermissionBuffer = ");
			printAddressList(stdoutPrintBuffer,m_dynamicPermissionBuffer);
			DEBUG_OUT("\n");
		}
	#endif

	MergeAddressList(&m_dynamicContainerRuntimeRecord, m_dynamicPermissionBuffer);
	m_dynamicContainerRuntimeRecordSize = addressListSize(m_dynamicContainerRuntimeRecord);
	freeAddressList(m_dynamicPermissionBuffer);
	m_dynamicPermissionBuffer = NULL;
	m_dynamicPermissionBufferSize = 0;
	
	m_dynamicContainerRuntimeRecordAlreadyPushed = 0;
	

	#ifdef DEBUG_GICA_DYNAMIC_RANGE
		if(m_dynamicContainerRuntimeRecordSize > 0) {
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %d ",__PRETTY_FUNCTION__,m_dynamicContainerRuntimeRecordSize);
			printAddressList(stdoutPrintBuffer,m_dynamicContainerRuntimeRecord);
			DEBUG_OUT("\n");
		}
	#endif
}


void containeropal::Return(container * callee){
	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s %s\n",__PRETTY_FUNCTION__,callee->name,  printStage(m_stage));
	#endif
	assert(callee);

	m_stat_numTotalCommitedReturnContainerSwitches++;

	m_staticContainerRuntimeRecord = freeAddressList(m_staticContainerRuntimeRecord);
	m_dynamicContainerRuntimeRecord = freeAddressList(m_dynamicContainerRuntimeRecord);
	m_dynamicContainerRuntimeRecordSize = 0;
	
//	if(GetCurrentContainer()){

		PopPermissionStack(callee);
		SetStage(LDDYN);
		Tick();
//	}
}


void containeropal::PopPermissionStack(container * callee){

	assert(thread_active->permissionsStack_FP >= permissionsStack_p );

	m_pendingReadStaticRequestsPending = callee->opalSizeOfPermissionLists;
	m_pendingReadStaticRequests = m_pendingReadStaticRequestsPending;

	m_pendingReadDynamicRequests = (thread_active->permissionsStack_SP - thread_active->permissionsStack_FP) / PERMISSION_RECORD_SIZE;
    //m_pendingReadDynamicRequests = m_pendingReadDynamicRequestsPending;

	#ifdef DEBUG_GICA_PopPermissionStack
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		 DEBUG_OUT("%s %llx\n", __PRETTY_FUNCTION__, callee->entryAddress);
		//DEBUG_OUT("%s  thread_active->permissionsStack_SP=%llx thread_active->permissionsStack_FP=%llx m_pendingReadDynamicRequests=%d\n",__PRETTY_FUNCTION__, thread_active->permissionsStack_SP, thread_active->permissionsStack_FP,m_pendingReadDynamicRequests);
	#endif
}

void containeropal::PopPermissionStackAfter(){
	
	uint64 oldFP = myMemoryRead(thread_active->permissionsStack_SP,8);
	thread_active->permissionsStack_SP = thread_active->permissionsStack_FP;
	thread_active->permissionsStack_FP = oldFP;

	#ifdef DEBUG_GICA_PopPermissionStack_After
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s  thread_active->permissionsStack_SP=%llx thread_active->permissionsStack_FP=%llx m_pendingReadDynamicRequests=%d\n",__PRETTY_FUNCTION__, thread_active->permissionsStack_SP, thread_active->permissionsStack_FP,m_pendingReadDynamicRequests);
	#endif

	transferDynPermBufferToDynContainerRunTimeRecord();
}

container * containeropal::GetCurrentContainer( ){
	container *ret = NULL;
	#ifdef DEBUG_GICA_THREAD_SWITCHING
		char rtemsname[10];
		//toStringRTEMSTaksName(rtemsname, thread_active->thread_name);
		DEBUG_OUT("%s 0x%llx 0x%llx\n",__PRETTY_FUNCTION__,thread_active->thread_id, thread_active->thread_name);
	#endif

	if(thread_active->container_runtime_stack == NULL)
		return ret;
	
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		ret = t.containerObj;
	}
	return ret;
}

void containeropal::AddDynamicRange(pa_t startAddress, uint64 size,byte_t multi, byte_t perm, dynamic_inst_t *w){
	#ifdef DEBUG_GICA_DYNAMIC_RANGE_MULTI
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s  %llx %lld  %d %d ",__PRETTY_FUNCTION__,printStage(m_stage), startAddress, size,multi, perm);
	#endif

	if(multi == 1){
		//DEBUG_OUT("IS MULTI %d",multi);
		m_pendingReadMultiRequestsPending = size;
		m_pendingReadMultiRequests = m_pendingReadMultiRequestsPending;
		permissionsMulti_p = startAddress;
		SetStage(LDMULTI);
		Tick();
	}
	else{
		UpdateAddressList( &m_dynamicPermissionBuffer,startAddress, size);
		m_dynamicPermissionBufferSize= addressListSize(m_dynamicPermissionBuffer);
	}


	#ifdef DEBUG_GICA_DYNAMIC_RANGE_MULTI
		printAddressList(stdoutPrintBuffer,m_dynamicPermissionBuffer);
		DEBUG_OUT(" NEW SIZE = %d \n",addressListSize(m_dynamicPermissionBuffer) );
	#endif
}






void containeropal::ContextSwitch(pa_t startAddress, dynamic_inst_t *w){

	assert(m_stage == IDLE );

	//
	//  save, flush , reload
	//
	
	staticpermssions * ctx = (staticpermssions *) startAddress;

	//check staticpermssions structure for the offsets
	permissions_p = myMemoryRead(startAddress+24, 8);
	permissionsStack_p =myMemoryRead(startAddress+40, 8);
	permissions_size = myMemoryRead(startAddress+16, 8);
	permissionsStack_size = myMemoryRead(startAddress+32, 8);
	permissionsContainers_p = myMemoryRead(startAddress+8, 8);
	permissionsContainers_size = myMemoryRead(startAddress+0, 8);
	
	dynamicPermissionBuffer_size = myMemoryRead(startAddress+48, 8);
	dynamicPermissionBuffer_p = myMemoryRead(startAddress+56, 8);
	dynamicContainerRuntimeRecord_size = myMemoryRead(startAddress+64, 8);
	dynamicContainerRuntimeRecord_p = myMemoryRead(startAddress+72, 8);

	m_pendingCtxSwitchLoadContainerListPending = permissionsContainers_size;
	m_pendingCtxSwitchLoadContainerList = m_pendingCtxSwitchLoadContainerListPending;

	m_pendingCtxSwitchSaveDynContainerRunTimeRecord = m_dynamicContainerRuntimeRecordSize;
	m_pendingCtxSwitchSaveDynPermBuffer = dynamicPermissionBuffer_size;

	m_pendingCtxSwitchLoadDynContainerRunTimeRecord = dynamicContainerRuntimeRecord_size;
	m_pendingCtxSwitchLoadDynPermBuffer = dynamicContainerRuntimeRecord_size;
	

	if(thread_active->containerRecord_p ){
		myMemoryWrite( (generic_address_t) (thread_active->containerRecord_p+72), m_pendingCtxSwitchSaveDynContainerRunTimeRecord, 4);
		myMemoryWrite( (generic_address_t) (thread_active->containerRecord_p+56), m_pendingCtxSwitchSaveDynPermBuffer, 4);
	}

	m_oldThreadContext = thread_active->containerRecord_p;
	m_newThreadContext = ctx;

	


	#ifdef DEBUG_GICA_CONTEXTSWITCH
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s  %llx ",__PRETTY_FUNCTION__,printStage(m_stage), startAddress);
		uint64 threadNameNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.name.name_u32");
		uint64 threadIdNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.id");
		char rtemsname[10];
		toStringRTEMSTaksName(rtemsname, threadNameNew);
		DEBUG_OUT("switch to THREAD %s ",rtemsname);
		DEBUG_OUT("permissions_p=%llx permissionsStack_p=%llx permissions_size=%lld permissionsStack_size=%lld ",permissions_p, permissionsStack_p, permissions_size, permissionsStack_size);
		printAddressList(stdoutPrintBuffer,m_dynamicPermissionBuffer);
		DEBUG_OUT(" NEW SIZE = %d \n",addressListSize(m_dynamicPermissionBuffer) );
	#endif

	SetStage(CTXSWITCHSTART);
	Tick();
}

void containeropal::ContextSwitchEnd(){

	uint64 threadNameNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.name.name_u32");
	uint64 threadIdNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.id");
	#ifdef DEBUG_GICA_CONTEXTSWITCH
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %llx %llx ",__PRETTY_FUNCTION__,thread_active->thread_id, thread_active->thread_name);
		DEBUG_OUT("switch to %llx %llx\n",__PRETTY_FUNCTION__,threadIdNew, threadNameNew);
	#endif
	Thread_switch(threadIdNew, threadNameNew);
	#ifdef DEBUG_GICA_CONTEXTSWITCH
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("switched to %llx %llx\n",thread_active->thread_id, thread_active->thread_name);
		
	#endif
		
	if(thread_active->permissionsStack_SP == NULL)
	{
		//a new thread was created, so reset the sp fp at the base of the stack. Othewise previously saved values will be kept. 
		thread_active->permissionsStack_FP = permissionsStack_p;
		thread_active->permissionsStack_SP = permissionsStack_p+8;
	}
	thread_active->containerRecord_p = m_newThreadContext;
}


void containeropal::LoadMultiPermissionsFromCache(container * c){

	if(m_pendingReadMultiRequests <=0 || !c) {m_pendingReadMultiRequests = 0; m_pendingReadMultiRequestsPending = 0;SetStage(IDLE);return;}
	
	bool permissionHit;
	pa_t whereToLookForStaticPermissionList = permissionsMulti_p;
	whereToLookForStaticPermissionList += (m_pendingReadMultiRequests) * PERMISSION_RECORD_SIZE; 

	permissionHit = this->l1_data_cache->Read(whereToLookForStaticPermissionList,this);
	uint64 val1 = myMemoryRead(whereToLookForStaticPermissionList,8);
	uint64 val2 = myMemoryRead(whereToLookForStaticPermissionList+8,8);

	#ifdef DEBUG_GICA_DYNAMIC_RANGE_MULTI
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingReadMultiRequestsPending, c->name);
			DEBUG_OUT("permissionsMulti_p %llx entryAddress %llx m_pendingReadMultiRequests %d whereToLookForStaticPermissionList %llx",permissionsMulti_p, c->entryAddress, m_pendingReadMultiRequests, whereToLookForStaticPermissionList);
			DEBUG_OUT(" %llx %lld \n", val1, val2);
	#endif

	UpdateAddressList( &m_dynamicPermissionBuffer,val1, maskBits64(val2,0,60));
	m_dynamicPermissionBufferSize= addressListSize(m_dynamicPermissionBuffer);
	
	m_pendingReadMultiRequests--;
	
	if(permissionHit) {
		m_pendingReadMultiRequestsPending--;
		if(!m_pendingReadMultiRequestsPending)
			SetStage(IDLE);
	}
	else SetStage(WAITONCACHE);
}


void containeropal::LoadStaticPermissionsFromCache(container * c){

	if(m_pendingReadStaticRequestsPending <=0 || !c) {m_pendingReadStaticRequests = 0;m_pendingReadStaticRequestsPending = 0;SetStage(IDLE);return;}
	
	bool permissionHit;
	pa_t whereToLookForStaticPermissionList = permissions_p;
	whereToLookForStaticPermissionList += c->opalOffsetLocateContainerInPermissions;
	whereToLookForStaticPermissionList += (c->opalSizeOfPermissionLists - m_pendingReadStaticRequests) * PERMISSION_RECORD_SIZE; 

	permissionHit = this->l1_perm_cache->Read(whereToLookForStaticPermissionList,this);
	packed_permission_rec p;
	p.startaddr = myMemoryRead(whereToLookForStaticPermissionList,8);
	p.permandsize = myMemoryRead(whereToLookForStaticPermissionList+8,8);
	unpacked_permission_rec up = unpack_permrec(p);

	#ifdef DEBUG_GICALoadStaticPermissionsFromCache
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingReadStaticRequestsPending, c->name);
			DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToLookForStaticPermissionList ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingReadStaticRequestsPending, whereToLookForStaticPermissionList);
			DEBUG_OUT(" %llx %llx %llx \n", whereToLookForStaticPermissionList, up.startaddr, up.endaddr);
	#endif

	this->m_staticContainerRuntimeRecord = consAddressList(up.startaddr,up.endaddr,this->m_staticContainerRuntimeRecord);
	CheckPedingWaiters(up.startaddr,up.endaddr-up.startaddr);

	m_pendingReadStaticRequests--;
	
	if(permissionHit) {
		m_pendingReadStaticRequestsPending--;
		if(!m_pendingReadStaticRequestsPending)
			SetStage(IDLE);
	}
	else SetStage(WAITONCACHE);
}

/*
void containeropal::LoadDynamicPermissionsFromCache(container * c){

	if(m_pendingReadDynamicRequestsPending <=0 || !c) return;
	
	bool permissionHit;
	pa_t whereToLookForDynamicPermissionList = thread_active->permissionsStack_SP - m_pendingReadDynamicRequests * PERMISSION_RECORD_SIZE;
	

	#ifdef DEBUG_GICA_DYNAMIC_RANGE
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingReadDynamicRequestsPending, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToLookForDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingReadDynamicRequests, whereToLookForDynamicPermissionList);
	#endif	

	permissionHit = this->l1_perm_cache->Read(whereToLookForDynamicPermissionList,this);

	packed_permission_rec p;
	p.startaddr = myMemoryRead(whereToLookForDynamicPermissionList,8);
	p.permandsize = myMemoryRead(whereToLookForDynamicPermissionList + 8,8);
	unpacked_permission_rec up = unpack_permrec(p);

	UpdateAddressList(&m_dynamicContainerRuntimeRecord,up.startaddr,up.endaddr-up.startaddr);
	m_dynamicContainerRuntimeRecordSize = addressListSize(m_dynamicContainerRuntimeRecord);
	CheckPedingWaiters(up.startaddr,up.endaddr-up.startaddr);

	
	#ifdef DEBUG_GICA_DYNAMIC_RANGE
		DEBUG_OUT(" [%llx %llx] (%d) \n", up.startaddr, up.endaddr,up.perm);
	#endif	
	
	m_pendingReadDynamicRequests--;
	
	
	if(permissionHit) {
		m_pendingReadDynamicRequestsPending--;
		//if(!m_pendingReadDynamicRequestsPending)
		//	Tick();
	}
	else SetStage(WAITONCACHE);
}

void containeropal::SavePermissionsToCache(container * c){

	if( m_pendingWriteRequestsPending <= 0 || !c) return;
	
	bool permissionHit;
	pa_t whereToSaveDynamicPermissionList = thread_active->permissionsStack_SP - m_pendingWriteRequests * PERMISSION_RECORD_SIZE ;
	
	#ifdef DEBUG_GICA_DYNAMIC_RANGE
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingWriteRequestsPending, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToSaveDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingWriteRequestsPending, whereToSaveDynamicPermissionList);
	#endif

	permissionHit = this->l1_perm_cache->Write(whereToSaveDynamicPermissionList,this);
	packed_permission_rec p = create_permrec(m_dynamicContainerRuntimeRecord->startAddress,m_dynamicContainerRuntimeRecord->endAddress,0x3);
	myMemoryWrite(whereToSaveDynamicPermissionList,p.startaddr,8);
	myMemoryWrite(whereToSaveDynamicPermissionList+8,p.permandsize,8);
	#ifdef DEBUG_GICA_DYNAMIC_RANGE
		DEBUG_OUT(" [%llx %llx] (%d) \n", m_dynamicContainerRuntimeRecord->startAddress, m_dynamicContainerRuntimeRecord->endAddress,0x3);
	#endif

	addressList temp = m_dynamicContainerRuntimeRecord;
	m_dynamicContainerRuntimeRecordSize--;
	m_dynamicContainerRuntimeRecord = m_dynamicContainerRuntimeRecord->next;
	free(temp);
    m_pendingWriteRequests --;

	if(permissionHit) {
		m_pendingWriteRequestsPending--;
		//if(!m_pendingWriteRequestsPending)
		//	Tick();
	}
	else SetStage(WAITONCACHE);
}
*/

void containeropal::LoadPermissionsList(container * c, pa_t location_base, long long *remainingCounter_p, addressList toSave, int * toSaveSize){

	if( *remainingCounter_p <=0 || !c) return;
	bool permissionHit;
	//pa_t whereToLookForDynamicPermissionList = thread_active->permissionsStack_SP - m_pendingReadDynamicRequests * PERMISSION_RECORD_SIZE;
	pa_t whereToLookForDynamicPermissionList = location_base - *remainingCounter_p * PERMISSION_RECORD_SIZE;

	#ifdef DEBUG_GICA_LoadPermissionsList
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d %llx ",__PRETTY_FUNCTION__, *remainingCounter_p, c->entryAddress);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToLookForDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, *remainingCounter_p, whereToLookForDynamicPermissionList);
	#endif	

	permissionHit = this->l1_perm_cache->Read(whereToLookForDynamicPermissionList,this);

	packed_permission_rec p;
	p.startaddr = myMemoryRead(whereToLookForDynamicPermissionList,8);
	p.permandsize = myMemoryRead(whereToLookForDynamicPermissionList + 8,8);
	unpacked_permission_rec up = unpack_permrec(p);

	UpdateAddressList(&toSave,up.startaddr,up.endaddr-up.startaddr);
	*toSaveSize = addressListSize(toSave);
	CheckPedingWaiters(up.startaddr,up.endaddr-up.startaddr);

	#ifdef DEBUG_GICA_LoadPermissionsList
		DEBUG_OUT(" [%llx %llx] (%d) \n", up.startaddr, up.endaddr,up.perm);
	#endif	

	if(permissionHit) {
		(*remainingCounter_p)--;
	}
	else SetStage(WAITONCACHE);
}


void containeropal::SavePermissionList(container *c, pa_t location_base, long long * remainingCounter_p, addressList *toSave, int * toSaveSize ){
	if( *remainingCounter_p <= 0 || !c) return;
	bool permissionHit;
	pa_t whereToSaveDynamicPermissionList = location_base - *remainingCounter_p * PERMISSION_RECORD_SIZE ;

	#ifdef DEBUG_GICA_SavePermissionsList
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %d %llx ",__PRETTY_FUNCTION__, *remainingCounter_p, c->entryAddress);
			DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToSaveDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, *remainingCounter_p, whereToSaveDynamicPermissionList);
	#endif

	permissionHit = this->l1_perm_cache->Write(whereToSaveDynamicPermissionList,this);
	packed_permission_rec p = create_permrec( (*toSave)->startAddress,(*toSave)->endAddress,0x3);
	myMemoryWrite(whereToSaveDynamicPermissionList,p.startaddr,8);
	myMemoryWrite(whereToSaveDynamicPermissionList+8,p.permandsize,8);

	addressList temp = *toSave;
	(*toSaveSize)--;
	*toSave = (*toSave)->next;
	free(temp);
	if(permissionHit) {
		(*remainingCounter_p)--;
	}
	else SetStage(WAITONCACHE);
	
}

void containeropal::SavePermissionToCacheOnIdle(container * c){
	
	if(m_dynamicContainerRuntimeRecordAlreadyPushed >= m_dynamicContainerRuntimeRecordSize || !c) return;
	
	bool permissionHit;
	int pendingIdleWrites =  ( m_dynamicContainerRuntimeRecordSize - m_dynamicContainerRuntimeRecordAlreadyPushed);
	pa_t whereToSaveDynamicPermissionList = thread_active->permissionsStack_SP + m_dynamicContainerRuntimeRecordSize * PERMISSION_RECORD_SIZE + 8 - pendingIdleWrites * PERMISSION_RECORD_SIZE ;
	
	#ifdef DEBUG_GICA_SavePermissionToCacheOnIdle
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s m_dynamicContainerRuntimeRecordAlreadyPushed = %d %s ",__PRETTY_FUNCTION__, m_dynamicContainerRuntimeRecordAlreadyPushed, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx pendingIdleWrites %d whereToSaveDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, pendingIdleWrites, whereToSaveDynamicPermissionList);
	#endif

	//navigate to the entry that is being saved
	int i = m_dynamicContainerRuntimeRecordAlreadyPushed;
	addressList l = m_dynamicContainerRuntimeRecord;
	addressList lgood = l;
	
	while( i > 0 && l)
	{
		if(l->next) lgood = l->next;
		l = l->next;
		i--;
	}
	permissionHit = this->l1_perm_cache->Write(whereToSaveDynamicPermissionList,this);
	packed_permission_rec p = create_permrec(lgood->startAddress,lgood->endAddress,0x3);
	myMemoryWrite(whereToSaveDynamicPermissionList,p.startaddr,8);
	myMemoryWrite(whereToSaveDynamicPermissionList+8,p.permandsize,8);
	#ifdef DEBUG_GICA_SavePermissionToCacheOnIdle
		DEBUG_OUT(" [%llx %llx] (%d) \n", lgood->startAddress, lgood->endAddress,0x3);
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s size=%d ",__PRETTY_FUNCTION__, m_dynamicContainerRuntimeRecordSize);
		printAddressList(stdoutPrintBuffer,m_dynamicContainerRuntimeRecord);
		DEBUG_OUT(" \n");
	#endif

	if(permissionHit) {
		m_dynamicContainerRuntimeRecordAlreadyPushed++;
	}
	else SetStage(WAITONCACHE);
}

void containeropal::LoadCtxContainerList(container * c){
	if(m_pendingCtxSwitchLoadContainerListPending <=0) {m_pendingCtxSwitchLoadContainerList = 0;m_pendingCtxSwitchLoadContainerListPending = 0;SetStage(IDLE);return;}
	
	bool permissionHit = false;
	pa_t whereToLookForPermissions = permissionsContainers_p;
	whereToLookForPermissions += (permissionsContainers_size - m_pendingCtxSwitchLoadContainerList) * 32; 

	permissionHit = this->l1_perm_cache->Read(whereToLookForPermissions,this);
	
	uint64 startaddr = myMemoryRead(whereToLookForPermissions + 0,8);
	uint64 endaddr = myMemoryRead(whereToLookForPermissions + 8,8);
	uint64 offset = myMemoryRead(whereToLookForPermissions + 16,8);
	uint64 len = myMemoryRead(whereToLookForPermissions + 24,8);

	#ifdef DEBUG_GICA_CONTEXTSWITCH_LoadCtxContainerList
			for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
			DEBUG_OUT("%s %d",__PRETTY_FUNCTION__, m_pendingCtxSwitchLoadContainerListPending);
			DEBUG_OUT(" %llx %llx %llx %lld %lld \n", whereToLookForPermissions, startaddr, endaddr, offset, len);
	#endif

	container * newcont = container_add(startaddr,"noname");
	newcont->endAddress = endaddr;
	newcont->opalOffsetLocateContainerInPermissions = offset * PERMISSION_RECORD_SIZE;
	newcont->opalSizeOfPermissionLists = len;

	m_pendingCtxSwitchLoadContainerList--;
	
	if(permissionHit) {
		m_pendingCtxSwitchLoadContainerListPending--;
//		if(!m_pendingCtxSwitchLoadContainerListPending)
//			SetStage(IDLE);
	}
	else SetStage(WAITONCACHE);
}


void containeropal::Wakeup(  )
{
	#ifdef DEBUG_GICA6 
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s m_pendingWriteRequests=%d m_pendingReadDynamicRequests=%d m_pendingReadStaticRequestsPending = %d m_pendingCtxSwitchLoadContainerListPending = %d\n",__PRETTY_FUNCTION__,printStage(GetStage()), m_pendingWriteRequests, m_pendingReadDynamicRequests, m_pendingReadStaticRequestsPending, m_pendingCtxSwitchLoadContainerListPending);
	#endif

	if(m_pendingCtxSwitchLoadContainerListPending || m_pendingCtxSwitchSaveDynContainerRunTimeRecord || m_pendingCtxSwitchSaveDynPermBuffer)
	{
		if(m_pendingCtxSwitchSaveDynContainerRunTimeRecord)
			m_pendingCtxSwitchSaveDynContainerRunTimeRecord--;
		else if(m_pendingCtxSwitchSaveDynPermBuffer)
			m_pendingCtxSwitchSaveDynPermBuffer--;
		else if (m_pendingCtxSwitchLoadContainerListPending)
			m_pendingCtxSwitchLoadContainerListPending--;
		
		SetStage(CTXSWITCHSTART);
	}
	else if ( m_pendingWriteRequests ){ 
		m_pendingWriteRequests--;
		SetStage(SAVEDYN);
	}
	else if(m_pendingReadDynamicRequests){
		m_pendingReadDynamicRequests--;
		SetStage(LDDYN);
	}
	else if(m_pendingReadStaticRequestsPending ) {
		m_pendingReadStaticRequestsPending--;
		SetStage(LDSTATIC);
	}
	else if(m_pendingReadMultiRequestsPending ) {
		m_pendingReadMultiRequestsPending--;
		SetStage(LDMULTI);
	}else if(CONTMGR_PERM_SAVEBUFFER && m_dynamicContainerRuntimeRecordAlreadyPushed < m_dynamicContainerRuntimeRecordSize){
		m_dynamicContainerRuntimeRecordAlreadyPushed ++;
		SetStage(IDLE);
	}
	else{
		ERROR_OUT("%s inconsistent state in %s",__PRETTY_FUNCTION__);
	}
	
 	Tick();
	
}

void containeropal::Tick(){
	#ifdef DEBUG_GICA5 
		DEBUG_OUT("%s %s \n",__PRETTY_FUNCTION__, printStage(m_stage));
	#endif
	switch(m_stage)
	{
		case SAVEDYN: 
			if(m_pendingWriteRequests > 0){
				m_stat_StageSTDYN++;
				container_stage_t stage = GetStage();
				int pcache2contmgrBusWidthCount = PCACHE_CONTMGR_BUSWIDTH;
				while(stage == SAVEDYN && pcache2contmgrBusWidthCount>0){
					//SavePermissionsToCache(GetCurrentContainer());
					SavePermissionList(GetCurrentContainer(), thread_active->permissionsStack_SP, &m_pendingWriteRequests, &m_dynamicContainerRuntimeRecord, &m_dynamicContainerRuntimeRecordSize);
					stage = GetStage();
					pcache2contmgrBusWidthCount --;
					if(m_pendingWriteRequests == 0)
					{
						PushPermissionStackAfter();
						SetStage(LDSTATIC);
						break;
					}
				}
			}
			else{
				PushPermissionStackAfter();
				SetStage(LDSTATIC);
				Tick();
			}
			break;
		case LDDYN:
			if(m_pendingReadDynamicRequests > 0){
				m_stat_StageLDDYN++;
				container_stage_t stage = GetStage();
				int pcache2contmgrBusWidthCount = PCACHE_CONTMGR_BUSWIDTH;
				while(stage == LDDYN && pcache2contmgrBusWidthCount>0){
					//LoadDynamicPermissionsFromCache(GetCurrentContainer());
					LoadPermissionsList(GetCurrentContainer(), thread_active->permissionsStack_SP, &m_pendingReadDynamicRequests, m_dynamicContainerRuntimeRecord, &m_dynamicContainerRuntimeRecordSize); 
					stage = GetStage();
					pcache2contmgrBusWidthCount --;
					if(m_pendingReadDynamicRequests == 0)
					{
						PopPermissionStackAfter();
						SetStage(LDSTATIC);
						break;
					}
				}
			}
			else{
				PopPermissionStackAfter();
				SetStage(LDSTATIC);
				Tick();
			}
			break;
		case LDSTATIC:
			if(m_pendingReadStaticRequestsPending > 0){
				m_stat_StageLDSTATIC++;
				container_stage_t stage = GetStage();
				int pcache2contmgrBusWidthCount = PCACHE_CONTMGR_BUSWIDTH;
				while(stage == LDSTATIC && pcache2contmgrBusWidthCount>0){
					LoadStaticPermissionsFromCache(GetCurrentContainer());
					stage = GetStage();
					pcache2contmgrBusWidthCount --;
					if(m_pendingReadStaticRequestsPending == 0)
					{
						SetStage(IDLE);
						break;
					}
				}
			}
			else
				SetStage(IDLE);
			break;
		case LDMULTI:
			if(m_pendingReadMultiRequestsPending > 0){
				m_stat_StageLDMULTI++;
				container_stage_t stage = GetStage();
				int pcache2contmgrBusWidthCount = PCACHE_CONTMGR_BUSWIDTH;
				while(stage == LDMULTI && pcache2contmgrBusWidthCount>0){
					LoadMultiPermissionsFromCache(GetCurrentContainer());
					stage = GetStage();
					pcache2contmgrBusWidthCount --;
					if(m_pendingReadMultiRequestsPending == 0)
					{
						SetStage(IDLE);
						break;
					}
				}
			}
			else
				SetStage(IDLE);
			break;
		case CTXSWITCHSTART:
			m_stat_StageCTXSWITCH++;
			if(m_pendingCtxSwitchSaveDynContainerRunTimeRecord > 0){
				SavePermissionList(GetCurrentContainer(),(pa_t) (thread_active->containerRecord_p+72),&m_pendingCtxSwitchSaveDynContainerRunTimeRecord, &m_dynamicContainerRuntimeRecord, &m_dynamicContainerRuntimeRecordSize);
			}
			else if(m_pendingCtxSwitchSaveDynPermBuffer> 0){
				SavePermissionList(GetCurrentContainer(),(pa_t) (thread_active->containerRecord_p+56),&m_pendingCtxSwitchSaveDynPermBuffer, &m_dynamicPermissionBuffer, &m_dynamicPermissionBufferSize);
			}
			else if(m_pendingCtxSwitchLoadDynContainerRunTimeRecord > 0){
				LoadPermissionsList(GetCurrentContainer(), (pa_t)dynamicContainerRuntimeRecord_p ,&m_pendingCtxSwitchLoadDynContainerRunTimeRecord, m_dynamicContainerRuntimeRecord, &m_dynamicContainerRuntimeRecordSize);
			}
			else if(m_pendingCtxSwitchLoadDynPermBuffer > 0){
				LoadPermissionsList(GetCurrentContainer(),(pa_t) dynamicPermissionBuffer_p,&m_pendingCtxSwitchLoadDynPermBuffer , m_dynamicPermissionBuffer, &m_dynamicPermissionBufferSize);
			}
			else if(m_pendingCtxSwitchLoadContainerListPending > 0){
				
				LoadCtxContainerList(GetCurrentContainer());
			}
			else
				SetStage(CTXSWITCHEND);
			break;
		case CTXSWITCHEND:
			ContextSwitchEnd();
			SetStage(IDLE);
			break ;
		case IDLE:
			m_stat_StageIDLE++;
			if( CONTMGR_PERM_SAVEBUFFER && m_dynamicContainerRuntimeRecordAlreadyPushed < m_dynamicContainerRuntimeRecordSize )
			{
				m_stat_StageIDLESAVE ++;
				container_stage_t stage = GetStage();
				int pcache2contmgrBusWidthCount = PCACHE_CONTMGR_BUSWIDTH;
				while(stage == IDLE && m_dynamicContainerRuntimeRecordAlreadyPushed < m_dynamicContainerRuntimeRecordSize && pcache2contmgrBusWidthCount>0){
					SavePermissionToCacheOnIdle(GetCurrentContainer());
					stage = GetStage();
					pcache2contmgrBusWidthCount --;
				}
			}
			//if(!m_wait_list.Empty()) m_wait_list.WakeupChain();
			break;

		case WAITONCACHE:
			m_stat_StageWAITONCACHE++;
			break;
		default:;
	}
}

void containeropal::SetStage(container_stage_t stage)
{
	//DEBUG_OUT("%s OLD=%s NEW=%s\n",__PRETTY_FUNCTION__, printStage(m_stage), printStage(stage) );
	m_stage = stage;
	if(!m_wait_list.Empty() && stage == IDLE )
		m_wait_list.WakeupChain();
}


//iterate the m_staticContainerRuntimeRecord and m_dynamicContainerRuntimeRecord return if found
bool containeropal::CheckMemoryAccess(pa_t addr, uint64 size)
{
	#ifdef DEBUG_GICA6 
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s Searching for %llx %llx ",__PRETTY_FUNCTION__,addr, addr + size);
		addressList l = this->m_staticContainerRuntimeRecord;
		while (l)
		{
			DEBUG_OUT(" s[%llx %llx) ", l->startAddress, l->endAddress);
			l = l->next;
		}
		l = this->m_dynamicContainerRuntimeRecord;
		while (l)
		{
			DEBUG_OUT(" d[%llx %llx) ", l->startAddress, l->endAddress);
			l = l->next;
		}
		
		DEBUG_OUT("\n");
	#endif


	bool found = false;
	if(!(found = FindInAddressList(this->m_staticContainerRuntimeRecord,addr,size)))
		found = FindInAddressList(this->m_dynamicContainerRuntimeRecord,addr,size);

	return found;
}

bool containeropal::CheckPedingWaiters(pa_t addr, uint64 size)
{
	#ifdef DEBUG_GICA9
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s Searching for %llx %lld \n",__PRETTY_FUNCTION__,addr, size);
	#endif

	waiter_t* l;
	
	#ifdef DEBUG_GICA9
		l = m_wait_list.next;
		int cnt = 0;
		while(l){ l=l->next;cnt ++;}
		DEBUG_OUT("%s THERE ARE %d in the waitlist \n",__PRETTY_FUNCTION__,cnt);
	#endif

	l = m_wait_list.next;
	while(l){
		waiter_t *temp = l;
		l= l->next;
		if(temp->TryWake(addr,size)){
			m_stat_LoadStoresFoundWithCheckingWaiters++;
		}
	}	

	#ifdef DEBUG_GICA9
		l = m_wait_list.next;
		cnt = 0;
		while(l){ l=l->next;cnt ++;}
		DEBUG_OUT("%s THERE ARE %d in the waitlist \n",__PRETTY_FUNCTION__,cnt);
	#endif
}


const char *containeropal::printStage( container_stage_t stage )
{
  switch (stage) {
	  	case SAVEDYN:
	    	return ("SAVEDYN");
	  	case LDDYN:
	    	return ("LDDYN");
		case LDSTATIC:
	    	return ("LDSTATIC");
		case IDLE:
			return ("IDLE");
		case CTXSWITCHSTART:
			return ("CTXSWITCHSTART");	
		case CTXSWITCHEND:
			return ("CTXSWITCHEND");
		case WAITONCACHE:
			return ("WAITONCACHE");
		default:
			return ("unknown");
  	}
}


void containeropal::PrintStats(){

	DEBUG_OUT("*** Container Opal stats:\n");
	DEBUG_OUT("numStalledLoads :\t %lld \n", m_stat_numStalledLoads);
	DEBUG_OUT("numTotalLoads :\t %lld \n", m_stat_numTotalLoads);
	
	DEBUG_OUT("numStalledStores :\t %lld \n", m_stat_numStalledStores);
	DEBUG_OUT("numTotalStores :\t %lld \n", m_stat_numTotalStores);

	DEBUG_OUT("LoadStoresFoundWithPartialLoadedContainer :\t %lld \n", m_stat_LoadStoresFoundWithPartialLoadedContainer);
	DEBUG_OUT("LoadStoresFoundWithCheckingWaiters :\t %lld \n", m_stat_LoadStoresFoundWithCheckingWaiters);


	DEBUG_OUT("numStalledCallContainerSwitches :\t %lld \n", m_stat_numStalledCallContainerSwitches);
	DEBUG_OUT("numStalledReturnContainerSwitches :\t %lld \n", m_stat_numStalledReturnContainerSwitches);
	DEBUG_OUT("numTotalCallContainerSwitches :\t %lld \n", m_stat_numTotalCallContainerSwitches);
	DEBUG_OUT("numTotalReturnContainerSwitches :\t %lld \n", m_stat_numTotalReturnContainerSwitches);

	DEBUG_OUT("numTotalCommitedReturnContainerSwitches :\t %lld \n", m_stat_numTotalCommitedReturnContainerSwitches);
	DEBUG_OUT("numTotalCommitedCallContainerSwitches :\t %lld \n", m_stat_numTotalCommitedCallContainerSwitches);

	DEBUG_OUT("m_debug_memleakNew :\t %d \n", m_debug_memleakNew);
	DEBUG_OUT("m_debug_memleakDelete :\t %d \n", m_debug_memleakDelete);

	
	DEBUG_OUT("stat_StageIDLE :\t %lld \n", m_stat_StageIDLE);
	DEBUG_OUT("stat_StageLDDYN :\t %lld \n", m_stat_StageLDDYN);
	DEBUG_OUT("stat_StageSTDYN :\t %lld \n", m_stat_StageSTDYN);
	DEBUG_OUT("stat_StageLDSTATIC :\t %lld \n", m_stat_StageLDSTATIC);
	DEBUG_OUT("stat_StageWAITONCACHE :\t %lld \n", m_stat_StageWAITONCACHE);
	DEBUG_OUT("stat_StageIDLESAVE :\t %lld \n", m_stat_StageIDLESAVE);
	DEBUG_OUT("stat_StageLDMULTI :\t %lld \n", m_stat_StageLDMULTI);
	DEBUG_OUT("m_stat_StageCTXSWITCH :\t %lld \n", m_stat_StageCTXSWITCH);
}



void LoadDecodedAccessListFile(const char * filePrefix)
{
	char *fileName = "FullDecodedAddressAccessList.txt";
	char *FileWithPrefix = NULL;
	int sz = strlen(filePrefix);
	int sz2 = strlen(fileName);
	FileWithPrefix = (char*)malloc( (sz + sz2) * sizeof(char) + 1);

	memcpy(FileWithPrefix, filePrefix, sz);
	memcpy(FileWithPrefix + sz, fileName, sz2);
	FileWithPrefix[sz+sz2] = 0;

	#ifdef DEBUG_GICA4
		DEBUG_OUT("%s => %s",__PRETTY_FUNCTION__, FileWithPrefix);
	#endif
	LoadContainersFromDecodedAccessListFile(FileWithPrefix);

	free(FileWithPrefix);
}

void LoadContainerCallListFile(const char *filePrefix){
	
	char *fileName = "ContainerCallList.txt";
	char *FileWithPrefix = NULL;
	int sz = strlen(filePrefix);
	int sz2 = strlen(fileName);
	FileWithPrefix = (char*)malloc( (sz + sz2) * sizeof(char) + 1);

	memcpy(FileWithPrefix, filePrefix, sz);
	memcpy(FileWithPrefix + sz, fileName, sz2);
	FileWithPrefix[sz+sz2] = 0;

	#ifdef DEBUG_GICA4
		DEBUG_OUT("%s => %s",__PRETTY_FUNCTION__, FileWithPrefix);
	#endif
	LoadContainersChildContainers(FileWithPrefix);

	free(FileWithPrefix);
}

void LoadContainersChildContainers(const char * FileWithPrefix){
	FILE * file = fopen(FileWithPrefix,"r");

	if(!file){ exit(printf("\n\n Runtime ERROR : unable to open symbol file %s",FileWithPrefix));}

	
 	fscanf(file,"entryAddress endAddress\tname\tcount\tLIST");
	//printf("entryAddress endAddress\tname\tcount\tLIST\n");fflush(stdin);
 	

	while(!feof(file)){
		int c = 0,s = 0,h = 0;
		unsigned long long addrStart;
		unsigned long long addrEnd;
		char name[200];
		int listLength;

		fscanf(file,"%llx %llx\t%s\t%d\t",&addrStart,&addrEnd,name,&listLength);
		//printf("%llx %llx\t%s\t%d",addrStart,addrEnd,name,listLength);

		//container * newcont = container_add(addrStart,name);
		container * foundSearch = search(addrStart);
		if(!foundSearch)
			exit(printf("\n\n Runtime ERROR : Container not loaded %s\n",name));
		for(int i=0;i<listLength;i++){
			
			unsigned long long entryAddr;
			char childname[200];
			fscanf(file,"%llx %s\t",&entryAddr,childname);
			//printf("%llx %s\t",entryAddr,childname);
			if(childname[0] != '*'){  //leave out the non-function containers
				container * foundChild = search(entryAddr);
				if(!foundChild) exit(printf("\n\n Runtime ERROR : Container not loaded %s",childname));
				stackObject t;
				t.containerObj = foundChild;
				cons( t,foundSearch->childFunctions);
			}
		}
		
		//printf("\n");

	}
	fclose(file);
}


void LoadContainersFromDecodedAccessListFile(const char * FileWithPrefix)
{
	#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
			DEBUG_OUT("%s \n",__PRETTY_FUNCTION__);
	#endif

	FILE * file = fopen(FileWithPrefix,"r");

	if(!file){ exit(printf("\n\n Runtime ERROR : unable to open symbol file %s",FileWithPrefix));}
	
	unsigned long long codeA;
    unsigned long long codeB;
	unsigned long long stackA;
	unsigned long long stackB;
	char junk[7][200];
	int totalCount;

	fscanf(file,"count: %x",&totalCount);
 	fscanf(file,"data: %llx %llx stack: %llx %llx",&codeA,&codeB,&stackA,&stackB);
	fscanf(file,"%s %s\t%s\t%s\t%s\t%s\t%s",junk[0],junk[1],junk[2],junk[3],junk[4],junk[5],junk[6]);


	#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
		printf("count:%x\n",totalCount);fflush(stdin);
 		printf("code: %llx %llx stack: %llx %llx\n",codeA,codeB,stackA,stackB);fflush(stdin);
 		printf("%s %s\t%s\t%s\t%s\t%s\t%s\n",junk[0],junk[1],junk[2],junk[3],junk[4],junk[5],junk[6]);fflush(stdin);
	#endif
	//exit(0);
	md_addr_t offset = 0;
    int i=0;
	while(!feof(file)){
		int ifetch=0,c = 0,s = 0,h = 0;
		unsigned long long addrStart;
		unsigned long long addrEnd;
		char name[200];
		int listLength;
		int totalHeapCalls;
		int totalStackPushes;
		
		
		
		fscanf(file,"%llx %llx\t%s\t%d\t%d\t%d\t",&addrStart,&addrEnd,name,&totalHeapCalls,&totalStackPushes,&listLength);
		#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
			DEBUG_OUT("%llx %llx\t%s\t%d\t%d\t%d ",addrStart,addrEnd,name,totalHeapCalls,totalStackPushes,listLength);
			//DEBUG_OUT("%d \n",totalHeapCalls/totalStackPushes);
		#endif
		container * newcont = container_add(addrStart,name);
		newcont->opalOffsetLocateContainerInPermissions = offset;
		newcont->endAddress = addrEnd;
		
		unsigned long long minStackStart = ULONG_LONG_MAX; //maxuint64
		unsigned long long maxStackEnd = 0;
		unsigned long long minFetchStart = ULONG_LONG_MAX; //maxuint64
		unsigned long long maxFetchEnd = 0;
		for(int i=0;i<listLength;i++){
			
			char type;
			unsigned long long rangeStart;
			unsigned long long rangeEnd;
			fscanf(file,"%c[%llx,%llx) ",&type,&rangeStart,&rangeEnd);
			#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
				DEBUG_OUT("%c[%llx,%llx) ",type,rangeStart,rangeEnd);
			#endif
			if(type == 'f'){ 
				ifetch++;
				if(minFetchStart > rangeStart) minFetchStart = rangeStart;
				if(maxFetchEnd < rangeEnd) maxFetchEnd = rangeEnd;
				newcont->opalCodeAccessList = consAddressList(rangeStart,rangeEnd,newcont->opalCodeAccessList);  
			}
			else if (type == 'c') {
				c++;
				newcont->opalStaticDataAccessList = consAddressList(rangeStart,rangeEnd,newcont->opalStaticDataAccessList); 
			}
			else if (type == 's') {
				s++;
				if(minStackStart > rangeStart) minStackStart = rangeStart;
				if(maxStackEnd < rangeEnd) maxStackEnd = rangeEnd;
				newcont->opalStackAccessList = consAddressList(rangeStart,rangeEnd,newcont->opalStackAccessList);
			}
			else {
				h++;
				newcont->opalHeapAccessList = consAddressList(rangeStart,rangeEnd,newcont->opalHeapAccessList);
			}
			newcont->addressAccessList = consAddressList(rangeStart,rangeEnd,newcont->addressAccessList);

			//#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
			//	DEBUG_OUT("\n %llx %llx %llx \n",globalreference->permissions_p,globalreference->permissions_p + globalreference->permissions_size, globalreference->permissions_p + newcont->opalOffsetLocateContainerInPermissions + local_write_pointer);
			//#endif
		}


		newcont->opalSizeOfPermissionLists = c + ( (CONTMGR_SINGLESTACKRANGE && minStackStart <= ULONG_LONG_MAX) ?1:s); //c for static S for stack
		newcont->opalSizeOfPermissionLists += minFetchStart <= ULONG_LONG_MAX ? 1 : 0;//add the fetch as well
		offset += newcont->opalSizeOfPermissionLists * PERMISSION_RECORD_SIZE; 

		#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
			DEBUG_OUT("\n at runtime: ");
		#endif
	
		uint64 local_write_pointer = 0;
		//write permissions to permission segment
		addressList ll;


		//PREPARE FETCH PERMISSIONS
		if(CONTMGR_PERM_USEFETCH && minFetchStart <= ULONG_LONG_MAX){
				myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer,minFetchStart,8);
				myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer + 8,maxFetchEnd-minFetchStart,8);
				local_write_pointer += PERMISSION_RECORD_SIZE;
			
			#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
				DEBUG_OUT("f1[%llx,%llx) ",minFetchStart, maxFetchEnd);
			#endif
		}

		//CONTMGR_PERM_STACKFIRST define the order in which these 2 operations happen
		//
		if(!CONTMGR_PERM_STACKFIRST)
			goto globalfirst;

stackfirst:		
		//PREPARE STATIC STACK PERMISSIONS
		if(CONTMGR_SINGLESTACKRANGE)
		{
			if(minStackStart <= ULONG_LONG_MAX){
				myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer,minStackStart,8);
				myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer + 8,maxStackEnd-minStackStart,8);
				local_write_pointer += PERMISSION_RECORD_SIZE;
			
			#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
				DEBUG_OUT("s1[%llx,%llx) ",minStackStart, maxStackEnd);
			#endif
			}
			
		}else{
			ll = newcont->opalStackAccessList;
			while(ll != NULL){
				myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer,ll->startAddress,8);
				myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer + 8,ll->endAddress-ll->startAddress,8);
				local_write_pointer += PERMISSION_RECORD_SIZE;
				#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
					DEBUG_OUT("s[%llx,%llx) ",ll->startAddress, ll->endAddress);
				#endif
				ll = ll->next;
			}
		}
		if(!CONTMGR_PERM_STACKFIRST) goto donereordering;

globalfirst:
		//PREPARE STATIC GLOBAL PERMISSIONS
		ll = newcont->opalStaticDataAccessList;
		while(ll != NULL){
			myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer,ll->startAddress,8);
			myMemoryWrite(globalreference->permissions_p+newcont->opalOffsetLocateContainerInPermissions + local_write_pointer + 8,ll->endAddress-ll->startAddress,8);
			#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
				DEBUG_OUT("c[%llx,%llx) ",ll->startAddress, ll->endAddress);
			#endif
			local_write_pointer += PERMISSION_RECORD_SIZE;
			ll = ll->next;
		}
		if(!CONTMGR_PERM_STACKFIRST) goto stackfirst;

donereordering:
	
		assert(globalreference->permissions_p+ newcont->opalOffsetLocateContainerInPermissions + local_write_pointer <= globalreference->permissions_p + globalreference->permissions_size);
		
		
		#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
			DEBUG_OUT("\n total size = %lld PERMISSION_RECORD_SIZE=%d newcont->opalSizeOfPermissionLists = %lld\n", newcont->opalSizeOfPermissionLists * PERMISSION_RECORD_SIZE,PERMISSION_RECORD_SIZE, newcont->opalSizeOfPermissionLists);
			
			DEBUG_OUT("\n");
		#endif

		i++;
	}
	fclose(file);

	//container_quickprint();
	#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
		for (int i=0 ; i < containerSize; i++)
		{
			fprintf(stdout,"\n %llx %llx\t %s",
							containerTable[i].entryAddress,
							containerTable[i].endAddress,
							containerTable[i].name
			);

			for(int j=0; j< containerTable[i].opalSizeOfPermissionLists;j++){
				uint64 start = myMemoryRead(globalreference->permissions_p + containerTable[i].opalOffsetLocateContainerInPermissions + (2*j)*8 , 8);
				uint64 size =  myMemoryRead(globalreference->permissions_p + containerTable[i].opalOffsetLocateContainerInPermissions + (2*j+1)*8 , 8);
				fprintf(stdout,"[%llx %llx) ",start, start+size);
			}
			fprintf(stdout,"\n");
		}
	#endif
}





memOpValidator::memOpValidator(){

}
memOpValidator::memOpValidator(pa_t a,memory_inst_t * st, containeropal * c, int type)
	{ 
		this->m_addr = a; this->m_memInst = st; this->m_containerOpal = c;
		this->m_type = type;
		internalCounter = 2;
	}
memOpValidator::~memOpValidator(){}

void memOpValidator::Wakeup()
{
	#ifdef DEBUG_GICA9x
		char buf[128];
		dynamic_inst_t * w = this->m_memInst;
		w->getStaticInst()->printDisassemble(buf);
		DEBUG_OUT("%s %s %lld %llx %llx %s \n",__PRETTY_FUNCTION__, w->printStage(w->getStage()),
			w->m_pseq->getLocalCycle(),this->m_memInst->getPC(), this->m_memInst->getPhysicalAddress() , buf);
	#endif

	internalCounter--;

	if(internalCounter==0){
		m_wait_list.WakeupChain();
		this->m_containerOpal->m_debug_memleakDelete++;
		delete this;
	}
}

memory_inst_t * memOpValidator::getMemoryInst(){return this->m_memInst;}

void load_waiter_t::Wakeup( void ){
	((memOpValidator *)parent)->m_containerOpal->m_loadsPendingValidation  = ((memOpValidator *)parent)->m_containerOpal->m_loadsPendingValidation - 1;
	#ifdef DEBUG_GICA9
		DEBUG_OUT("%s %d \n", __PRETTY_FUNCTION__, ((memOpValidator *)parent)->m_containerOpal->m_loadsPendingValidation);
	#endif
	parent->Wakeup();
	((memOpValidator *)parent)->m_containerOpal->m_debug_memleakDelete++;
	delete this;
}

bool load_waiter_t::TryWake( pa_t addr, size_t size ){
	#ifdef DEBUG_GICA9
		DEBUG_OUT("%s [%llx %llx) \n", __PRETTY_FUNCTION__,this->m_addr , this->m_addr + this->m_size);
	#endif
	if(addr <= this->m_addr && addr+size >= this->m_addr + this->m_size){
		RemoveWaitQueue();		
		Wakeup();
		return true;
	}
	return false;	
}

void store_waiter_t::Wakeup( void ){
	((memOpValidator *)parent)->m_containerOpal->m_storesPendingValidation  = ((memOpValidator *)parent)->m_containerOpal->m_storesPendingValidation - 1;
	#ifdef DEBUG_GICA9
		DEBUG_OUT("%s %d \n", __PRETTY_FUNCTION__, ((memOpValidator *)parent)->m_containerOpal->m_storesPendingValidation);
	#endif
	parent->Wakeup();
	((memOpValidator *)parent)->m_containerOpal->m_debug_memleakDelete++;
	delete this;
}

bool store_waiter_t::TryWake( pa_t addr, size_t size ){
	#ifdef DEBUG_GICA9
		DEBUG_OUT("%s [%llx %llx) \n", __PRETTY_FUNCTION__,this->m_addr , this->m_addr + this->m_size);
	#endif
	if(addr <= this->m_addr && addr+size >= this->m_addr + this->m_size){
		RemoveWaitQueue();
		Wakeup();
		return true;
	}
	return false;	
}


void simple_waiter_t::Wakeup( void ){
	parent->Wakeup();
	globalreference->m_debug_memleakDelete++;

	#ifdef DEBUG_GICA9x
		DEBUG_OUT("%s %d \n", __PRETTY_FUNCTION__, globalreference->m_debug_memleakDelete);
	#endif

	delete this;	
}



/*------------------------------------------------------------------------*/
/* pcd_inst                                                             */
/*------------------------------------------------------------------------*/

listalloc_t pcd_inst_t::m_myalloc;


//***************************************************************************
pcd_inst_t::pcd_inst_t( static_inst_t *s_inst, 
                            int32 window_index,
                            pseq_t *pseq,
                            abstract_pc_t *at,
                            pa_t physicalPC,
                            trap_type_t trapgroup,
                            uint32 proc)
  : memory_inst_t( s_inst, window_index, pseq, at, physicalPC, trapgroup, proc) {
}

//***************************************************************************
pcd_inst_t::~pcd_inst_t() {
}


//**************************************************************************
void 
pcd_inst_t::Execute() {
	#ifdef DEBUG_GICA7
			char buf[128];
			dynamic_inst_t * w = this;
			w->getStaticInst()->printDisassemble(buf);
			DEBUG_OUT("%s %s %s \n",__PRETTY_FUNCTION__, w->printStage(w->getStage()), buf);
	#endif

	 Complete();
}



//**************************************************************************
void
pcd_inst_t::Retire( abstract_pc_t *a ) {
	#ifdef DEBUG_GICA7
			char buf[128];
			dynamic_inst_t * w = this;
			w->getStaticInst()->printDisassemble(buf);
			DEBUG_OUT("%s %s %s \n",__PRETTY_FUNCTION__, w->printStage(w->getStage()), buf);
	#endif

#ifdef GICACONTAINER
	if(!m_context_switch)
		m_pseq->getContainerOpal()->AddDynamicRange(m_startaddr, m_size, m_multi, m_perm,this);
	else
		m_pseq->getContainerOpal()->ContextSwitch(m_startaddr, this);
#endif

	memory_inst_t::Retire(a);
}

//**************************************************************************
void 
pcd_inst_t::Complete() {
	#ifdef DEBUG_GICA7
			char buf[128];
			dynamic_inst_t * w = this;
			w->getStaticInst()->printDisassemble(buf);
			DEBUG_OUT("%s %s %s \n",__PRETTY_FUNCTION__, w->printStage(w->getStage()), buf);
	#endif

	memory_inst_t::Execute(); 

	//#ifdef DEBUG_GICA7
		//if(getTrapType()!= Trap_NoTrap)
		//	DEBUG_OUT("TRAP %d",getTrapType());
	//#endif	
}

bool
pcd_inst_t::accessCache( void ) {
	return true;
}

bool
pcd_inst_t::accessCacheRetirement( void ) {

	return true;
}





