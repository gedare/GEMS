
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

typedef struct packed_permission_record{
	uint64 startaddr;
	uint64 permandsize;
} packed_permission_rec;

typedef struct unpacked_permission_record{
	uint64 startaddr;
	uint64 endaddr;
	byte_t perm;
} unpacked_permission_rec;


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
	up.endaddr = up.startaddr + maskBits64(p.permandsize,0,61);
	up.perm = (byte_t) maskBits64(p.permandsize,62,63);
	return up;
}




void LoadContainersFromDecodedAccessListFile(const char * strDecodedAccessListFile);
void LoadContainersChildContainers(const char * FileWithPrefix);


containeropal * globalreference;


containeropal::containeropal(generic_cache_template<generic_cache_block_t> * ic, generic_cache_template<generic_cache_block_t> *dc,
	generic_cache_template<generic_cache_block_t> *pc)
{
	this->l1_inst_cache = ic;
	this->l1_data_cache = dc;
	this->l1_perm_cache = pc;

	
	permissions_p = mySimicsIntSymbolRead("&Permissions");
	permissionsStack_p =mySimicsIntSymbolRead("&Permissions_Stack");
	permissions_size = mySimicsIntSymbolRead("Permissions_size");
	permissionsStack_size = mySimicsIntSymbolRead("Permissions_Stack_size");

	ThreadAdd(0,0);
	container_simicsInit();

	m_stage = IDLE;

	m_loadsPendingValidation = 0;
	m_storesPendingValidation = 0;

	m_pendingWriteRequests = 0;
	m_pendingReadStaticRequests = 0;
	m_pendingReadDynamicRequests = 0;
	m_pendingWriteRequestsPending = 0 ;

	
	m_pendingReadStaticRequestsPending = 0 ;
	m_pendingReadDynamicRequestsPending = 0 ;



	m_stat_numStalledLoads = 0;
	m_stat_numStalledStores = 0;
	m_stat_numStalledCallContainerSwitches = 0;
	m_stat_numStalledReturnContainerSwitches = 0;


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
	
	
	m_wait_list.wl_reset();
	m_allowRetire = TRUE;

	permissionsStack_FP = permissionsStack_p;
	permissionsStack_SP = permissionsStack_p+8;
		
	#ifdef DEBUG_GICA5
		DEBUG_OUT("%s permissions_p=%llx permissionsStack_p=%llx permissions_size=%lld permissionsStack_size=%lld %s\n",
			__PRETTY_FUNCTION__,permissions_p, permissionsStack_p, permissions_size, permissionsStack_size,printStage(m_stage));
	#endif

	m_dynamicPermissionBuffer = NULL;
	m_dynamicPermissionBufferSize = 0;
	m_dynamicContainerRuntimeRecord = NULL;
	m_dynamicContainerRuntimeRecordSize = 0;

	m_debug_memleakNew = 0;
	m_debug_memleakDelete = 0;

	
	globalreference = this;
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


bool containeropal::CacheWrite(pa_t a, memory_inst_t *w){

	bool permissionHit = false;
	bool cacheHit = false;
	
	m_stat_numTotalStores++;

	
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

	//access perm container if it is ready
	if(GetStage() == IDLE){
		permissionHit = true;
		masterWaiter->internalCounter --;
	}
	else{
		permissionHit = false;
		masterWaiter->perm_waiter = new store_waiter_t(masterWaiter);
		m_debug_memleakNew++;
		
		(masterWaiter->perm_waiter)->InsertWaitQueue(m_wait_list);
		m_storesPendingValidation++;
		m_stat_numStalledStores++;
	}

	cacheHit = this->l1_data_cache->Write(a,masterWaiter->data_waiter);
	
	if(cacheHit){
		masterWaiter->internalCounter --;
		m_debug_memleakDelete++;
		delete masterWaiter->data_waiter;
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

bool containeropal::CacheRead(pa_t a, memory_inst_t *w, bool data_request = true,
            bool *primary_bool = NULL){
    
	bool permissionHit = false;
	bool cacheHit = false;
	m_stat_numTotalLoads++;

	pa_t whereToLookForStaticPermissionList = permissions_p;
	//get current container and use the offset from it
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		whereToLookForStaticPermissionList += t.containerObj->opalOffsetLocateContainerInPermissions;
	}

	memOpValidator * masterWaiter = new memOpValidator(whereToLookForStaticPermissionList,(memory_inst_t *)w,this, 0 /*LOAD*/); 
	masterWaiter->data_waiter = new simple_waiter_t(masterWaiter);
	m_debug_memleakNew +=2;

	//access perm container if it is ready
	if(GetStage() == IDLE){
		permissionHit = true;
		masterWaiter->internalCounter --;
	}
	else{
		permissionHit = false;
		masterWaiter->perm_waiter = new load_waiter_t(masterWaiter);
		m_debug_memleakNew ++;
		(masterWaiter->perm_waiter)->InsertWaitQueue(m_wait_list);
		m_stat_numStalledLoads++;
		m_loadsPendingValidation++;
	}
	cacheHit = this->l1_data_cache->Read(a,masterWaiter->data_waiter);

	if(cacheHit){
		masterWaiter->internalCounter --;
		delete masterWaiter->data_waiter;
		m_debug_memleakDelete++;
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
			w->printStage(w->getStage()), w->m_pseq->getLocalCycle(),l1_data_cache->GetName(),a, buf, permissionHit, cacheHit, printStage(GetStage()));
	#endif
	
    return cacheHit && permissionHit;
}

bool containeropal::Retire(dynamic_inst_t *w){
    #ifdef DEBUG_GICA5 
		char buf[128];
  		w->getStaticInst()->printDisassemble(buf);
		DEBUG_OUT("%s %lld %llx %s %s \n",__PRETTY_FUNCTION__,w->m_pseq->getLocalCycle(),w->getPC(),w->printStage(w->getStage()),buf);
	#endif

}


pa_t retAdr;
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
						m_pendingReadDynamicRequestsPending =0;
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
  				if(m_loadsPendingValidation + m_storesPendingValidation == 0 
					&& m_pendingWriteRequests == 0 && m_pendingReadDynamicRequests == 0)
					{
						if (Waiting()) RemoveWaitQueue();
						m_pendingReadStaticRequestsPending =0;
						m_pendingReadDynamicRequestsPending =0;
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
	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s %s\n",__PRETTY_FUNCTION__,callee->name, printStage(m_stage));
	#endif
	m_stat_numTotalCommitedCallContainerSwitches++;
	
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

	assert(permissionsStack_SP <= (permissionsStack_p + permissionsStack_size));

	m_pendingReadStaticRequestsPending = callee->opalSizeOfPermissionLists;
	m_pendingReadStaticRequests = m_pendingReadStaticRequestsPending;

	m_pendingWriteRequestsPending = m_dynamicContainerRuntimeRecordSize;
	m_pendingWriteRequests = m_pendingWriteRequestsPending;
	
	//save FP to stack top : this->permissionsStack_FP
	uint64 newSP = this->permissionsStack_SP + m_pendingWriteRequestsPending * PERMISSION_RECORD_SIZE + 8;
	myMemoryWrite(newSP,this->permissionsStack_FP,8);
	this->permissionsStack_FP = this->permissionsStack_SP;
	this->permissionsStack_SP = newSP;

	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s  this->permissionsStack_SP=%llx this->permissionsStack_FP=%llx m_dynamicContainerRuntimeRecordSize=%d\n",__PRETTY_FUNCTION__, this->permissionsStack_SP, this->permissionsStack_FP, m_dynamicContainerRuntimeRecordSize);
	#endif

}

void containeropal::PushPermissionStackAfter(){

	m_dynamicContainerRuntimeRecordSize = 0;
	m_dynamicContainerRuntimeRecord = NULL;
	transferDynPermBufferToDynContainerRunTimeRecord();
}


void containeropal::transferDynPermBufferToDynContainerRunTimeRecord()
{

	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d m_dynamicContainerRuntimeRecord = ",__PRETTY_FUNCTION__,m_dynamicContainerRuntimeRecordSize);
		printAddressList(m_dynamicContainerRuntimeRecord);
		DEBUG_OUT("m_dynamicPermissionBuffer = ");
		printAddressList(m_dynamicPermissionBuffer);
		DEBUG_OUT("\n");
	#endif

	MergeAddressList(&m_dynamicContainerRuntimeRecord, m_dynamicPermissionBuffer);
	m_dynamicContainerRuntimeRecordSize = addressListSize(m_dynamicContainerRuntimeRecord);
	m_dynamicPermissionBuffer = NULL;
	m_dynamicPermissionBufferSize = 0;

	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d ",__PRETTY_FUNCTION__,m_dynamicContainerRuntimeRecordSize);
		printAddressList(m_dynamicContainerRuntimeRecord);
		DEBUG_OUT("\n");
	#endif
}


void containeropal::Return(container * callee){
	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s %s\n",__PRETTY_FUNCTION__,callee->name,  printStage(m_stage));
	#endif
	assert(callee);

	m_stat_numTotalCommitedReturnContainerSwitches++;

//	if(GetCurrentContainer()){

		PopPermissionStack(callee);
		SetStage(LDDYN);
		Tick();
//	}
}


void containeropal::PopPermissionStack(container * callee){

	assert(permissionsStack_FP >= permissionsStack_p );

	m_pendingReadStaticRequestsPending = callee->opalSizeOfPermissionLists;
	m_pendingReadStaticRequests = m_pendingReadStaticRequestsPending;

	m_dynamicContainerRuntimeRecordSize = 0;
	m_dynamicContainerRuntimeRecord = NULL;

	m_pendingReadDynamicRequestsPending = (permissionsStack_SP - permissionsStack_FP) / PERMISSION_RECORD_SIZE;
    m_pendingReadDynamicRequests = m_pendingReadDynamicRequestsPending;

	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s  this->permissionsStack_SP=%llx this->permissionsStack_FP=%llx m_pendingReadDynamicRequestsPending=%d\n",__PRETTY_FUNCTION__, this->permissionsStack_SP, this->permissionsStack_FP,m_pendingReadDynamicRequestsPending);
	#endif
}

void containeropal::PopPermissionStackAfter(){
	
	uint64 oldFP = myMemoryRead(this->permissionsStack_SP,8);
	this->permissionsStack_SP = this->permissionsStack_FP;
	this->permissionsStack_FP = oldFP;

	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s  this->permissionsStack_SP=%llx this->permissionsStack_FP=%llx m_pendingReadDynamicRequestsPending=%d\n",__PRETTY_FUNCTION__, this->permissionsStack_SP, this->permissionsStack_FP,m_pendingReadDynamicRequestsPending);
	#endif

	transferDynPermBufferToDynContainerRunTimeRecord();
}

container * containeropal::GetCurrentContainer( ){
	container *ret = NULL;
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		ret = t.containerObj;
	}
	return ret;
}

void containeropal::AddDynamicRange(pa_t startAddress, uint64 size, byte_t perm, dynamic_inst_t *w){
	#ifdef DEBUG_GICA8
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %llx %lld %d ",__PRETTY_FUNCTION__, startAddress, size, perm);
		char printbuff[1000];
	#endif
	
	UpdateAddressList( &m_dynamicPermissionBuffer,startAddress, size);
	m_dynamicPermissionBufferSize++;


	#ifdef DEBUG_GICA8
		printAddressList(printbuff,m_dynamicPermissionBuffer);
		DEBUG_OUT(" NEW SIZE = %d \n",addressListSize(m_dynamicPermissionBuffer) );
	#endif
}


void containeropal::LoadStaticPermissionsFromCache(container * c){

	if(!c) {m_pendingReadStaticRequests = 0;m_pendingReadStaticRequests = 0;SetStage(IDLE);return;}
	
	bool permissionHit;
	pa_t whereToLookForStaticPermissionList = permissions_p;
	whereToLookForStaticPermissionList += c->opalOffsetLocateContainerInPermissions;
	whereToLookForStaticPermissionList += (c->opalSizeOfPermissionLists - m_pendingReadStaticRequests) * PERMISSION_RECORD_SIZE; 
	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingReadStaticRequestsPending, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToLookForStaticPermissionList %llx\n",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingReadStaticRequestsPending, whereToLookForStaticPermissionList);
	#endif
	permissionHit = this->l1_perm_cache->Read(whereToLookForStaticPermissionList,this);
	m_pendingReadStaticRequests--;
	
	if(permissionHit) {
		m_pendingReadStaticRequestsPending--;
		if(!m_pendingReadStaticRequestsPending)
			SetStage(IDLE);
	}
	else SetStage(WAITONCACHE);
}

void containeropal::LoadDynamicPermissionsFromCache(container * c){

	if(!c) return;
	
	bool permissionHit;
	pa_t whereToLookForDynamicPermissionList = permissionsStack_SP - m_pendingReadDynamicRequests * PERMISSION_RECORD_SIZE;
	
//	whereToLookForStaticPermissionList += c->opalOffsetLocateContainerInPermissions;
//	whereToLookForStaticPermissionList += (c->opalSizeOfPermissionLists - m_pendingReadRequests) * PERMISSION_RECORD_SIZE; 
	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingReadDynamicRequestsPending, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToLookForDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingReadDynamicRequests, whereToLookForDynamicPermissionList);
	#endif	

	//if(whereToLookForStaticPermissionList >= permissions_p + permissions_size)
	//{
	//	printf("\n %s EXCCEDED PERMISSION MEMORY permissions_p=%llx  permissions_size=%llx whereToLookForStaticPermissionList=%llx  \n",
	//		__PRETTY_FUNCTION__,permissions_p,permissions_size, whereToLookForStaticPermissionList );
	//}
	permissionHit = this->l1_perm_cache->Read(whereToLookForDynamicPermissionList,this);

	packed_permission_rec p;
	p.startaddr = myMemoryRead(whereToLookForDynamicPermissionList,8);
	p.permandsize = myMemoryRead(whereToLookForDynamicPermissionList + 8,8);
	unpacked_permission_rec up = unpack_permrec(p);

	UpdateAddressList(&m_dynamicContainerRuntimeRecord,up.startaddr,up.endaddr-up.startaddr);
	m_dynamicContainerRuntimeRecordSize = addressListSize(m_dynamicContainerRuntimeRecord);
	
	#ifdef DEBUG_GICA6
		DEBUG_OUT(" [%llx %llx] (%d) \n", up.startaddr, up.endaddr,up.perm);
	#endif	
	
	m_pendingReadDynamicRequests--;
	
	
	if(permissionHit) {
		m_pendingReadDynamicRequestsPending--;
		if(!m_pendingReadDynamicRequestsPending)
			Tick();
	}
	else SetStage(WAITONCACHE);
}



void containeropal::SavePermissionsToCache(container * c){

	if(!c) return;
	
	bool permissionHit;
	pa_t whereToSaveDynamicPermissionList = this->permissionsStack_SP - m_pendingWriteRequests * PERMISSION_RECORD_SIZE ;
	
	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingWriteRequestsPending, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToSaveDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingWriteRequestsPending, whereToSaveDynamicPermissionList);
	#endif

	permissionHit = this->l1_perm_cache->Write(whereToSaveDynamicPermissionList,this);
	packed_permission_rec p = create_permrec(m_dynamicContainerRuntimeRecord->startAddress,m_dynamicContainerRuntimeRecord->endAddress,0x3);
	myMemoryWrite(whereToSaveDynamicPermissionList,p.startaddr,8);
	myMemoryWrite(whereToSaveDynamicPermissionList+8,p.permandsize,8);
	#ifdef DEBUG_GICA6
		DEBUG_OUT(" [%llx %llx] (%d) \n", m_dynamicContainerRuntimeRecord->startAddress, m_dynamicContainerRuntimeRecord->endAddress,0x3);
	#endif

	free(m_dynamicContainerRuntimeRecord);
	m_dynamicContainerRuntimeRecordSize--;
	m_dynamicContainerRuntimeRecord = m_dynamicContainerRuntimeRecord->next;
    m_pendingWriteRequests --;

	if(permissionHit) {
		m_pendingWriteRequestsPending--;
		if(!m_pendingWriteRequestsPending)
			Tick();
	}
	else SetStage(WAITONCACHE);
}

void containeropal::Wakeup(  )
{
	#ifdef DEBUG_GICA6 
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s m_pendingWriteRequestsPending=%d m_pendingReadDynamicRequestsPending=%d m_pendingReadStaticRequestsPending = %d\n",__PRETTY_FUNCTION__,printStage(GetStage()), m_pendingWriteRequestsPending, m_pendingReadDynamicRequestsPending, m_pendingReadStaticRequestsPending);
	#endif

	if ( m_pendingWriteRequestsPending ){ 
		m_pendingWriteRequestsPending--;
		SetStage(SAVEDYN);
	}
	else if(m_pendingReadDynamicRequestsPending){
		m_pendingReadDynamicRequestsPending--;
		SetStage(LDDYN);
	}
	else if(m_pendingReadStaticRequestsPending ) {
		m_pendingReadStaticRequestsPending--;
		SetStage(LDSTATIC);
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
			m_stat_StageSTDYN++;
			if(m_pendingWriteRequestsPending > 0)
				SavePermissionsToCache(GetCurrentContainer());
			else{
				
				PushPermissionStackAfter();
				SetStage(LDSTATIC);
				Tick();
			}
			break;
		case LDDYN:
			m_stat_StageLDDYN++;
			if(m_pendingReadDynamicRequestsPending > 0){
				LoadDynamicPermissionsFromCache(GetCurrentContainer());
			}
			else{
				PopPermissionStackAfter();
				SetStage(LDSTATIC);
				Tick();
			}
			break;
		case LDSTATIC:
			m_stat_StageLDSTATIC++;
			if(m_pendingReadStaticRequestsPending > 0){
				LoadStaticPermissionsFromCache(GetCurrentContainer());
			}
			else
				SetStage(IDLE);
			break;
		case IDLE:
			m_stat_StageIDLE++;
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
	FILE * file = fopen(FileWithPrefix,"r");

	if(!file){ exit(printf("\n\n Runtime ERROR : unable to open symbol file %s",FileWithPrefix));}
	
	unsigned long long codeA;
    unsigned long long codeB;
	unsigned long long stackA;
	unsigned long long stackB;
	char junk[7][200];
 	
 	fscanf(file,"data: %llx %llx stack: %llx %llx",&codeA,&codeB,&stackA,&stackB);
	fscanf(file,"%s %s\t%s\t%s\t%s\t%s\t%s",junk[0],junk[1],junk[2],junk[3],junk[4],junk[5],junk[6]);


	#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
 		printf("code: %llx %llx stack: %llx %llx\n",codeA,codeB,stackA,stackB);fflush(stdin);
 		printf("%s %s\t%s\t%s\t%s\t%s\t%s\n",junk[0],junk[1],junk[2],junk[3],junk[4],junk[5],junk[6]);fflush(stdin);
	#endif
	//exit(0);
	md_addr_t offset = 0;
    
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
		newcont->endAddress = addrEnd;
		for(int i=0;i<listLength;i++){
			
			char type;
			unsigned long long rangeStart;
			unsigned long long rangeEnd;
			fscanf(file,"%c[%llx,%llx) ",&type,&rangeStart,&rangeEnd);
			#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
				DEBUG_OUT("%c[%llx,%llx) ",type,rangeStart,rangeEnd);
			#endif
			if(type == 'f'){ ifetch++; consAddressList(rangeStart,rangeEnd,newcont->opalCodeAccessList);  }
			else if (type == 'c') {c++;consAddressList(rangeStart,rangeEnd,newcont->opalStaticDataAccessList); }
			else if (type == 's') {s++;consAddressList(rangeStart,rangeEnd,newcont->opalStackAccessList); }
			else {h++;consAddressList(rangeStart,rangeEnd,newcont->opalHeapAccessList);}
			consAddressList(rangeStart,rangeEnd,newcont->addressAccessList);

			
		}
		newcont->opalSizeOfPermissionLists = c + s; //c for static S for stack
		
		newcont->opalOffsetLocateContainerInPermissions = offset;
		offset += newcont->opalSizeOfPermissionLists * PERMISSION_RECORD_SIZE; 
		
		
		#ifdef DEBUG_GICALoadContainersFromDecodedAccessListFile
			DEBUG_OUT("\n");
		#endif

	}
	fclose(file);

	//container_quickprint();
}





memOpValidator::memOpValidator(){}
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
	#ifdef DEBUG_GICA9x
		DEBUG_OUT("%s %d \n", __PRETTY_FUNCTION__, ((memOpValidator *)parent)->m_containerOpal->m_loadsPendingValidation);
	#endif
	parent->Wakeup();
	((memOpValidator *)parent)->m_containerOpal->m_debug_memleakDelete++;
	delete this;
}

void store_waiter_t::Wakeup( void ){
	((memOpValidator *)parent)->m_containerOpal->m_storesPendingValidation  = ((memOpValidator *)parent)->m_containerOpal->m_storesPendingValidation - 1;
	#ifdef DEBUG_GICA9x
		DEBUG_OUT("%s %d \n", __PRETTY_FUNCTION__, ((memOpValidator *)parent)->m_containerOpal->m_storesPendingValidation);
	#endif
	parent->Wakeup();
	((memOpValidator *)parent)->m_containerOpal->m_debug_memleakDelete++;
	delete this;
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

	m_pseq->getContainerOpal()->AddDynamicRange(m_startaddr, m_size, m_perm,this);

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





