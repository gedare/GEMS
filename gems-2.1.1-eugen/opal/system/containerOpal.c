
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


#define PERMOFFSET 0xF0000

containeropal::containeropal(generic_cache_template<generic_cache_block_t> * ic, generic_cache_template<generic_cache_block_t> *dc,
	generic_cache_template<generic_cache_block_t> *pc)
{
	this->l1_inst_cache = ic;
	this->l1_data_cache = dc;
	this->l1_perm_cache = pc;
	containeropal();
}

containeropal::containeropal()
{

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
	#ifdef DEBUG_GICA4
		char buf[128];
  		w->getStaticInst()->printDisassemble(buf);
		DEBUG_OUT("%s %lld %s %llx %s\n",__PRETTY_FUNCTION__, w->m_pseq->getLocalCycle(),l1_data_cache->GetName(),a, buf);
	#endif
	return this->l1_data_cache->Write(a,w);
}

bool containeropal::CacheRead(pa_t a, memory_inst_t *w, bool data_request = true,
            bool *primary_bool = NULL){
    #ifdef DEBUG_GICA4
		char buf[128];
  		w->getStaticInst()->printDisassemble(buf);
		DEBUG_OUT("%s %lld %s %llx %s\n",__PRETTY_FUNCTION__, w->m_pseq->getLocalCycle(),l1_data_cache->GetName(),a, buf);
	#endif
     return this->l1_data_cache->Read(a,w,data_request,primary_bool);
}

bool containeropal::Retire(dynamic_inst_t *w){
    #ifdef DEBUG_GICA4
		char buf[128];
  		w->getStaticInst()->printDisassemble(buf);
		DEBUG_OUT("%s %lld %llx %s %s \n",__PRETTY_FUNCTION__,w->m_pseq->getLocalCycle(),w->getPC(),w->printStage(w->getStage()),buf);
	#endif
}


void containeropal::Wakeup( void )
{
	DEBUG_OUT("%s",__PRETTY_FUNCTION__);
}


