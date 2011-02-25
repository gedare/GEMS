
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

	m_pendingWriteRequests = 0;
	m_pendingReadStaticRequests = 0;
	m_pendingReadDynamicRequests = 0;
	m_pendingWriteRequestsPending = 0 ;
	m_pendingReadStaticRequestsPending = 0 ;
	m_pendingReadDynamicRequestsPending = 0 ;
	
	
	m_wait_list.wl_reset();
	m_allowRetire = TRUE;

	permissionsStack_FP = permissionsStack_p;
	permissionsStack_SP = permissionsStack_p;
		
	#ifdef DEBUG_GICA5
		DEBUG_OUT("%s permissions_p=%llx permissionsStack_p=%llx permissions_size=%lld permissionsStack_size=%lld %s\n",
			__PRETTY_FUNCTION__,permissions_p, permissionsStack_p, permissions_size, permissionsStack_size,printStage(m_stage));
	#endif


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

	
	pa_t whereToLookForStaticPermissionList = permissions_p;
	//get current container and use the offset from it
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		whereToLookForStaticPermissionList += t.containerObj->opalOffsetLocateContainerInPermissions;
	}

	storeWrapper * masterWaiter = new storeWrapper(whereToLookForStaticPermissionList,(store_inst_t *)w,this); 
	masterWaiter->perm_waiter = new simple_waiter_t(masterWaiter);
	masterWaiter->data_waiter = new simple_waiter_t(masterWaiter);
	
	//storeWrapper * permWaiter2 = new storeWrapper(a,(store_inst_t *)w,this); 
	//w->getStaticInst()->printDisassemble(masterWaiter->dissasembled_instr_forDEBUG_buff);

	//access both data cache and perm cache.
	//permissionHit =  this->l1_perm_cache->Read(whereToLookForStaticPermissionList,masterWaiter->perm_waiter);

	//access perm container if it is ready
	if(GetStage() == IDLE)
		permissionHit = true;
	else{
		permissionHit = false;
		(masterWaiter->perm_waiter)->InsertWaitQueue(m_wait_list);
	}

	cacheHit = this->l1_data_cache->Write(a,masterWaiter->data_waiter);
	
	if(!permissionHit || !cacheHit) w->InsertWaitQueue(masterWaiter->m_wait_list);

	if(permissionHit){
		masterWaiter->internalCounter --;
		delete masterWaiter->perm_waiter;
	}

	if(cacheHit){
		masterWaiter->internalCounter --;
		delete masterWaiter->data_waiter;
	}

	if(!masterWaiter->data_waiter && !masterWaiter->perm_waiter) delete masterWaiter;

    #ifdef DEBUG_GICA5
			char buf[128];
			w->getStaticInst()->printDisassemble(buf);
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

	pa_t whereToLookForStaticPermissionList = permissions_p;
	//get current container and use the offset from it
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		whereToLookForStaticPermissionList += t.containerObj->opalOffsetLocateContainerInPermissions;
	}

	storeWrapper * masterWaiter = new storeWrapper(whereToLookForStaticPermissionList,(store_inst_t *)w,this); 
	masterWaiter->perm_waiter = new simple_waiter_t(masterWaiter);
	masterWaiter->data_waiter = new simple_waiter_t(masterWaiter);

	//permissionHit = this->l1_perm_cache->Read(whereToLookForStaticPermissionList,masterWaiter->perm_waiter);
	//access perm container if it is ready
	if(GetStage() == IDLE)
		permissionHit = true;
	else{
		permissionHit = false;
		(masterWaiter->perm_waiter)->InsertWaitQueue(m_wait_list);
		
	}
	cacheHit = this->l1_data_cache->Read(a,masterWaiter->data_waiter);

	if(!permissionHit || !cacheHit) w->InsertWaitQueue(masterWaiter->m_wait_list);

	if(permissionHit){
		masterWaiter->internalCounter --;
		delete masterWaiter->perm_waiter;
	}

	if(cacheHit){
		masterWaiter->internalCounter --;
		delete masterWaiter->data_waiter;
	}

	if(!masterWaiter->data_waiter && !masterWaiter->perm_waiter) delete masterWaiter;
	
	#ifdef DEBUG_GICA5 
		char buf[128];
  		w->getStaticInst()->printDisassemble(buf);
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
void containeropal::RunTimeContainerTracking(pa_t pc)
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
			if(m_stage == IDLE ){ 
				stack_pop(returnAddressStack);
				returned = 1;
				#ifdef DEBUG_GICA6 
					for(int i = 0; i< returnAddressStack->size; i++) DEBUG_OUT("|\t");
					DEBUG_OUT("}\n");
				#endif
				Return(t.containerObj);
				m_allowRetire = true;
				if(!stack_empty(returnAddressStack)) t = stack_top(returnAddressStack);
				else break;
			}
			else{
				m_allowRetire = false;
				break;
			}
		}
	}

	if(!returned && m_allowRetire){
		//if it was not a function return , it is either a function call ( push to container stack in this case) or a regular intruction executing (within the container)
		container *foundSearch = search(pc);
		if(foundSearch){
			
			if(!foundSearch->nonFunction){
				//only allow containers to switch if the the container is not busy loading  permssions
				// TODO : do this only if there are no pending load/stores
				// otherwise cancel current loading process, and go on
				if(m_stage == IDLE ){ 
					stackObject t;
					t.containerObj = foundSearch;
					t.returnAddress = getRet();//return_addr;
					#ifdef DEBUG_GICA6 
						for(int j = 0; j< returnAddressStack->size; j++) DEBUG_OUT("|\t");
						DEBUG_OUT("%s {\n", foundSearch->name);
					#endif
					stack_push(returnAddressStack, t);
					NewCall(t.containerObj);
					m_allowRetire = true;
				}
				else{
					m_allowRetire = false;
				}
			}
			else{
				#ifdef DEBUG_GICA6 
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
	if(w->getTrapType() == Trap_NoTrap) containeropal::RunTimeContainerTracking(pc);

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
	#ifdef DEBUG_GICA5
		DEBUG_OUT("%s NEW CALL %s\n",__PRETTY_FUNCTION__,callee->name);
	#endif
	m_pendingReadStaticRequestsPending = callee->opalSizeOfPermissionLists;
	m_pendingReadStaticRequests = m_pendingReadStaticRequestsPending;
	PushPermissionStack();
	SetStage(BUSY);
	Tick();
}

void containeropal::InsideCall(container * callee){
	#ifdef DEBUG_GICA5 
		DEBUG_OUT("%s INSIDE %s \n",__PRETTY_FUNCTION__,callee->name);
	#endif
}


void containeropal::Return(container * callee){
	#ifdef DEBUG_GICA5
		DEBUG_OUT("%s RETURN FROM %s \n",__PRETTY_FUNCTION__,callee->name);
	#endif
	assert(callee);
	if(GetCurrentContainer()){
		m_pendingReadStaticRequestsPending = callee->opalSizeOfPermissionLists;
		m_pendingReadStaticRequests = m_pendingReadStaticRequestsPending;
		PopPermissionStack();
		SetStage(BUSY);
		Tick();
	}
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
	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %llx %lld %d ",__PRETTY_FUNCTION__, startAddress, size, perm);
		char printbuff[1000];
	#endif
	
	UpdateAddressList( &m_dynamicPermissionBuffer,startAddress, size);
	m_pendingWriteRequestsPending++;

	#ifdef DEBUG_GICA6
		printAddressList(printbuff,m_dynamicPermissionBuffer);
		DEBUG_OUT(" NEW SIZE = %d \n",addressListSize(m_dynamicPermissionBuffer) );
	#endif
}


void containeropal::LoadStaticPermissionsFromCache(container * c){

	if(!c) return;
	
	bool permissionHit;
	pa_t whereToLookForStaticPermissionList = permissions_p;
	whereToLookForStaticPermissionList += c->opalOffsetLocateContainerInPermissions;
	whereToLookForStaticPermissionList += (c->opalSizeOfPermissionLists - m_pendingReadStaticRequests) * PERMISSION_RECORD_SIZE; 
	#ifdef DEBUG_GICA5
		DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingReadStaticRequestsPending, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToLookForStaticPermissionList %llx\n",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingReadStaticRequestsPending, whereToLookForStaticPermissionList);
	#endif
	permissionHit = this->l1_perm_cache->Read(whereToLookForStaticPermissionList,this);
	m_pendingReadStaticRequests--;
	
	if(permissionHit) {
		m_pendingReadStaticRequestsPending--;
		if(!m_pendingWriteRequestsPending && !m_pendingReadStaticRequestsPending && !m_pendingReadDynamicRequestsPending)
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

	#ifdef DEBUG_GICA6
		DEBUG_OUT(" [%llx %llx] (%d) \n", up.startaddr, up.endaddr,up.perm);
	#endif	
	
	m_pendingReadDynamicRequests--;
	
	if(permissionHit) {
		m_pendingReadDynamicRequestsPending--;
		if(!m_pendingWriteRequestsPending && !m_pendingReadStaticRequestsPending && !m_pendingReadDynamicRequestsPending)
			SetStage(IDLE);
	}
	else SetStage(WAITONCACHE);
}

void containeropal::PushPermissionStack(){

	assert(permissionsStack_SP <= (permissionsStack_p + permissionsStack_size));
	//save FP to stack top : this->permissionsStack_FP
	myMemoryWrite(this->permissionsStack_SP,this->permissionsStack_FP,8);
	this->permissionsStack_FP = this->permissionsStack_SP;
	m_pendingWriteRequests = m_pendingWriteRequestsPending;
	this->permissionsStack_SP += m_pendingWriteRequestsPending * PERMISSION_RECORD_SIZE + 8;
	
	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s  this->permissionsStack_SP=%llx this->permissionsStack_FP=%llx \n",__PRETTY_FUNCTION__, this->permissionsStack_SP, this->permissionsStack_FP);
	#endif

}
void containeropal::PopPermissionStack(){

	assert(permissionsStack_FP >= permissionsStack_p );
    uint64 oldFP = myMemoryRead(this->permissionsStack_FP,8);
	this->permissionsStack_SP = this->permissionsStack_FP;
	this->permissionsStack_FP = oldFP;
	m_pendingReadDynamicRequestsPending = (permissionsStack_SP - permissionsStack_FP) / PERMISSION_RECORD_SIZE;
    m_pendingReadDynamicRequests = m_pendingReadDynamicRequestsPending;

	#ifdef DEBUG_GICA6
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s  this->permissionsStack_SP=%llx this->permissionsStack_FP=%llx \n",__PRETTY_FUNCTION__, this->permissionsStack_SP, this->permissionsStack_FP);
	#endif
}


void containeropal::SavePermissionsToCache(container * c){

	if(!c) return;
	
	bool permissionHit;
	pa_t whereToSaveDynamicPermissionList = this->permissionsStack_SP - m_pendingWriteRequests * PERMISSION_RECORD_SIZE ;
	
	#ifdef DEBUG_GICA6
		DEBUG_OUT("%s %d %s ",__PRETTY_FUNCTION__, m_pendingWriteRequestsPending, c->name);
		DEBUG_OUT("permissions_p %llx entryAddress %llx opalOffsetLocateContainerInPermissions %llx m_pendingReadRequests %d whereToSaveDynamicPermissionList %llx ",permissions_p, c->entryAddress, c->opalOffsetLocateContainerInPermissions, m_pendingWriteRequestsPending, whereToSaveDynamicPermissionList);
	#endif

	permissionHit = this->l1_perm_cache->Write(whereToSaveDynamicPermissionList,this);
	packed_permission_rec p = create_permrec(m_dynamicPermissionBuffer->startAddress,m_dynamicPermissionBuffer->endAddress,0x3);
	myMemoryWrite(whereToSaveDynamicPermissionList,p.startaddr,8);
	myMemoryWrite(whereToSaveDynamicPermissionList+8,p.permandsize,8);
	DEBUG_OUT(" [%llx %llx] (%d) \n", m_dynamicPermissionBuffer->startAddress, m_dynamicPermissionBuffer->endAddress,0x3);
	free(m_dynamicPermissionBuffer);
	m_dynamicPermissionBuffer = m_dynamicPermissionBuffer->next;
    m_pendingWriteRequests --;
	
	if(permissionHit) {
		m_pendingWriteRequestsPending--;
		if(!m_pendingWriteRequestsPending && !m_pendingReadStaticRequestsPending && !m_pendingReadDynamicRequestsPending)
			SetStage(IDLE);
	}
	else SetStage(WAITONCACHE);
}

void containeropal::Wakeup(  )
{
	#ifdef DEBUG_GICA6 
		for(int j = 0; j< thread_active->container_runtime_stack->size; j++) DEBUG_OUT("|\t");
		DEBUG_OUT("%s %s m_pendingWriteRequestsPending=%d m_pendingReadStaticRequestsPending=%d m_pendingReadDynamicRequestsPending = %d\n",__PRETTY_FUNCTION__,printStage(GetStage()), m_pendingWriteRequestsPending, m_pendingReadDynamicRequestsPending, m_pendingReadDynamicRequestsPending);
	#endif

	if ( m_pendingWriteRequestsPending ) m_pendingWriteRequestsPending--;
	else if(m_pendingReadDynamicRequestsPending) m_pendingReadDynamicRequestsPending--;
	else if(m_pendingReadStaticRequestsPending ) m_pendingReadStaticRequestsPending--;
	
	
	SetStage(BUSY);
	if(!m_pendingWriteRequestsPending && !m_pendingReadStaticRequestsPending && !m_pendingReadDynamicRequestsPending)
			SetStage(IDLE);
	else Tick();
	
}


void containeropal::Tick(){
	#ifdef DEBUG_GICA5 
		DEBUG_OUT("%s %s \n",__PRETTY_FUNCTION__, printStage(m_stage));
	#endif
	switch(m_stage)
	{
		case BUSY: 
			if(m_pendingWriteRequestsPending > 0)
				SavePermissionsToCache(GetCurrentContainer());
			else if(m_pendingReadDynamicRequestsPending > 0)
				LoadDynamicPermissionsFromCache(GetCurrentContainer());
			else if(m_pendingReadStaticRequestsPending > 0)
				LoadStaticPermissionsFromCache(GetCurrentContainer());
			else
				SetStage(IDLE);
			break;
		case IDLE:
			//if(!m_wait_list.Empty()) m_wait_list.WakeupChain();
			break;
		case WAITONCACHE:;
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
	  	case BUSY:
	    	return ("BUSY");
		case IDLE:
			return ("IDLE");
		case WAITONCACHE:
			return ("WAITONCACHE");
		default:
			return ("unknown");
  	}
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
			if(childname[0] != '*'){
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
	char junk[5][200];
 	
 	fscanf(file,"data: %llx %llx stack: %llx %llx",&codeA,&codeB,&stackA,&stackB);
 	//printf("code: %llx %llx stack: %llx %llx\n",codeA,codeB,stackA,stackB);fflush(stdin);
 	fscanf(file,"%s %s\%s\t%s\t%s",junk[0],junk[1],junk[2],junk[3],junk[4]);
 	//printf("%s %s\%s\t%s\t%s\n",junk[0],junk[1],junk[2],junk[3],junk[4]);fflush(stdin);

	md_addr_t offset = 0;
    
	while(!feof(file)){
		int ifetch=0,c = 0,s = 0,h = 0;
		unsigned long long addrStart;
		unsigned long long addrEnd;
		char name[200];
		int listLength;
		fscanf(file,"%llx %llx\t%s\t%d\t",&addrStart,&addrEnd,name,&listLength);
		#ifdef DEBUG_GICA5
			DEBUG_OUT("%llx %llx\t%s\t%d ",addrStart,addrEnd,name,listLength);
		#endif
		container * newcont = container_add(addrStart,name);
		newcont->endAddress = addrEnd;
		for(int i=0;i<listLength;i++){
			
			char type;
			unsigned long long rangeStart;
			unsigned long long rangeEnd;
			fscanf(file,"%c[%llx,%llx) ",&type,&rangeStart,&rangeEnd);
			#ifdef DEBUG_GICA5
				DEBUG_OUT("%c[%llx,%llx) ",type,rangeStart,rangeEnd);
			#endif
			if(type == 'f'){ ifetch++; consAddressList(rangeStart,rangeEnd,newcont->opalCodeAccessList);  }
			else if (type == 'c') {c++;consAddressList(rangeStart,rangeEnd,newcont->opalStaticDataAccessList); }
			else if (type == 's') {s++;consAddressList(rangeStart,rangeEnd,newcont->opalStackAccessList); }
			else {h++;consAddressList(rangeStart,rangeEnd,newcont->opalHeapAccessList);}
			consAddressList(rangeStart,rangeEnd,newcont->addressAccessList);

			
		}
		//newcont->opalSizeOfPermissionLists = ifetch+c+s; //ignore heap
		newcont->opalSizeOfPermissionLists = ifetch + c + s; //one for fetches and 1 for stack
		newcont->opalOffsetLocateContainerInPermissions = offset;
		offset += newcont->opalSizeOfPermissionLists * PERMISSION_RECORD_SIZE; 
		
		
		#ifdef DEBUG_GICA5
			DEBUG_OUT("\n");
		#endif

	}
	fclose(file);

	//container_quickprint();
}





storeWrapper::storeWrapper(){}
storeWrapper::storeWrapper(pa_t a,store_inst_t * st, containeropal * c)
	{ 
		this->m_addr = a; this->m_storeInst = st; this->m_containerOpal = c;
		internalCounter = 2;
	}
storeWrapper::~storeWrapper(){}

void storeWrapper::Wakeup()
{
	#ifdef DEBUG_GICA2STORE
		char buf[128];
		dynamic_inst_t * w = this->m_storeInst;
		w->getStaticInst()->printDisassemble(buf);
		DEBUG_OUT("%s %s %lld %llx %llx %s \n",__PRETTY_FUNCTION__, w->printStage(w->getStage()),
			w->m_pseq->getLocalCycle(),this->m_storeInst->getPC(), this->m_storeInst->getPhysicalAddress() , buf);
	#endif

	internalCounter--;
	if(internalCounter==0){
		m_wait_list.WakeupChain();
		delete this;
	}
}
store_inst_t * storeWrapper::getStoreInst(){return this->m_storeInst;}


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





