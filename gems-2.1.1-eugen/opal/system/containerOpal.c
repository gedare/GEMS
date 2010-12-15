
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



#define PERMOFFSET 0xF0000


void LoadContainersFromDecodedAccessListFile(const char * strDecodedAccessListFile);
void LoadContainersChildContainers(const char * FileWithPrefix);





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
			exit(printf("\n\n Runtime ERROR : Container not loaded %s",name));
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
	unsigned long long initA;
	unsigned long long initB;
	unsigned long long stackA;
	unsigned long long stackB;
	char junk[5][200];
 	
 	fscanf(file,"code: %llx %llx initialized: %llx %llx stack: %llx %llx",&codeA,&codeB,&initA,&initB,&stackA,&stackB);
 	//printf("code: %llx %llx initialized: %llx %llx stack: %llx %llx\n",codeA,codeB,initA,initB,stackA,stackB);fflush(stdin);
 	fscanf(file,"%s %s\%s\t%s\t%s",junk[0],junk[1],junk[2],junk[3],junk[4]);
 	//printf("%s %s\%s\t%s\t%s\n",junk[0],junk[1],junk[2],junk[3],junk[4]);fflush(stdin);
    
	while(!feof(file)){
		int c = 0,s = 0,h = 0;
		unsigned long long addrStart;
		unsigned long long addrEnd;
		char name[200];
		int listLength;
		fscanf(file,"%llx %llx\t%s\t%d\t",&addrStart,&addrEnd,name,&listLength);
		//printf("%llx %llx\t%s\t%d",addrStart,addrEnd,name,listLength);
		container * newcont = container_add(addrStart,name);
		newcont->endAddress = addrEnd;
		for(int i=0;i<listLength;i++){
			
			char type;
			unsigned long long rangeStart;
			unsigned long long rangeEnd;
			fscanf(file,"%c[%llx,%llx) ",&type,&rangeStart,&rangeEnd);
			//printf("%c[%llx,%llx) ",type,rangeStart,rangeEnd);
			if(type == 'c'){ c++; consAddressList(rangeStart,rangeEnd,newcont->opalCodeAccessList);  }
			else if (type == 's') {s++;consAddressList(rangeStart,rangeEnd,newcont->opalStackAccessList); }
			else {h++;consAddressList(rangeStart,rangeEnd,newcont->opalHeapAccessList);}
			consAddressList(rangeStart,rangeEnd,newcont->addressAccessList);
		}
		
		//printf("\n");

	}
	fclose(file);

	//container_quickprint();
}





