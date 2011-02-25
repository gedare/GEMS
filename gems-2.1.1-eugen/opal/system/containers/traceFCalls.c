/*by Eugen Leontie*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trace.h"
#include "traceFCalls.h"

#define CONTAINERMAX 6000
#define PRINTBUFFMAX 100000

typedef struct traceData_def traceData;
//External Variables
container containerTable[CONTAINERMAX]; 	//this array contains all containers( functions and is loaded in Loader.c)
int containerSize = 0;				//keps the size of the containerList
int componentTableSize = 1;
int containerInitialized = 0; 	//local flag to track wheater the containers were initialized

//end External Variables

//local variables
//mystack returnAddressStack; 	//this is the main Container stack

md_addr_t return_addr;			//monitors the return addresses ( saved before a function call and stored to stack)

thread_monitor_t *thread_active;
int ignore_due_to_Exception;



struct regs_t *regs;			//this pointer gives access to the simulated register file

char printBuffer[PRINTBUFFMAX];			// use sprintf to print to this buffer and then use myprint for the final print ( full trace )
char printBuffer2[PRINTBUFFMAX];			// use sprintf to print to this buffer and then use myprint for the final print ( second run trace )
char compilerPrintBuffer[PRINTBUFFMAX];	// use sprintf to print to this buffer and then use myprint for the final print ( compiler trace )
char stdoutPrintBuffer[PRINTBUFFMAX];

struct traceData_def{
	char name[100];
	int traceLoadedAddressCount;
	int traceLoadeduniqueChildContainersCalled;
};

traceData traceLoader[CONTAINERMAX];
int traceLoaderSize;

char *fullTracefdFileName = NULL;
FILE *tracefdIn = NULL; //TODO !!
FILE *tracefdOut = NULL;
FILE *compilerInfofd = NULL;
FILE *fullTracefd = NULL;
FILE *overrideOutputfd = NULL;

md_addr_t ld_text_base = 0x0;
md_addr_t ld_text_bound = 0x0;
md_addr_t ld_stack_size = 0x0;
md_addr_t ld_stack_base = 0x0;


conf_object_t *proc = NULL; //Simics Sparc current processor
int o7id; //Simics Sparc registerid for the "i7" register ( %i7 + 8 is the return address)
int o6id;  //sp
int i6id;  //fp


thread_monitor_t* ThreadAdd(uint64 id, uint64 name)
{
	thread_monitor_t* newThread;
	newThread = (thread_monitor_t*) calloc (sizeof(thread_monitor_t),1);
	newThread->thread_id = id;
	newThread->thread_name = name;
	newThread->maxStack = 0;
	newThread->minStack = ULLONG_MAX ;
	newThread->maxFP = 0;
	newThread->minFP = ULLONG_MAX ;

	newThread->container_runtime_stack = stack_create();
	//create a name for the thread trace file
	if(fullTracefdFileName != NULL){
		char fname[500];
		memset(fname,0,500);
		if(id == 0)
			strcpy((char *)fname,fullTracefdFileName);
		else{
			strcpy(fname,fullTracefdFileName);
			strcat(fname,"_");
			char idstr[64];
			itoa(id,idstr);
			strcat(fname,idstr);
			strcat(fname,"_");
			char taskName[5];
			toStringRTEMSTaksName(taskName,name);
			strcat(fname,taskName);
		}
		newThread->traceFD = fopen(fname,"w");
	}
	if(thread_active != NULL){
		newThread->next = thread_active->next;
		thread_active->next = newThread;
	}
	else{
		thread_active = newThread;
		newThread->next = newThread;
	}
	return newThread;
}

thread_monitor_t* ThreadFind(uint64 id)
{
	thread_monitor_t* iterate;
	//ASSERT(thread_active != NULL);
	if(thread_active->thread_id== id) return thread_active;
	iterate = thread_active->next;

	while (iterate != thread_active)
	{
		if(iterate->thread_id == id) return iterate;
		iterate = iterate->next;
	}
	return NULL;
}

void Thread_switch( uint64 id, uint64 name)
{
	thread_monitor_t* newThread;
	newThread = ThreadFind(id);
	if(newThread == thread_active) return;
	if(!newThread) newThread = ThreadAdd(id,name);
	fflush(thread_active->traceFD);
	thread_active = newThread;
    
	printf("\nThread_switch 0x%llx ",thread_active->thread_id);
	printRTEMSTaksName(thread_active->thread_name);
	printf("\n");

	md_addr_t StackArea = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Start.Initial_stack.area");
	uint64 StackSize = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Start.Initial_stack.size");

	ld_stack_base = StackArea;
	ld_stack_size = StackSize;

	ld_text_base = mySimicsIntSymbolRead("start");
	ld_text_bound = mySimicsIntSymbolRead("end");
}


void ThreadInitializeOnStart()
{
	uint64 threadNameNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.name.name_u32");
	uint64 threadIdNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.id");
	Thread_switch( threadIdNew, threadNameNew);
}

void container_simicsInit()
{
	proc =	SIM_current_processor();
	o7id = SIM_get_register_number(proc,"o7");
	
	o6id = SIM_get_register_number(proc,"o6");
	i6id = SIM_get_register_number(proc,"i6");

}

void container_initialize( char * fullTraceFileName)
{
	int i = 0;
	//tracefdOut = stdout;

	if(containerInitialized == 0)
	{
		ignore_due_to_Exception = 0;
		containerInitialized = 1;
		//returnAddressStack = stack_create();
		thread_active = ThreadAdd(0,0);
		//containerTable = (container *)malloc(size * sizeof(container));
		if(tracefdIn){
				traceLoaderSize=0;
				while(!feof(tracefdIn)){
					fscanf(tracefdIn,"%s %d %d",traceLoader[traceLoaderSize].name,&traceLoader[traceLoaderSize].traceLoadedAddressCount,&traceLoader[traceLoaderSize].traceLoadeduniqueChildContainersCalled);
					traceLoaderSize++;
				}
				fclose(tracefdIn);
				while(i<traceLoaderSize){
						//printf("%s %d %d\n",&traceLoader[i].name,traceLoader[i].traceLoadedAddressCount,traceLoader[i].traceLoadeduniqueChildContainersCalled);
						i++;
				}
		}

		if(strlen(fullTraceFileName) == 0)
		{
			fullTracefd = stdout;
		}
		else
		{
			fullTracefd = fopen(fullTraceFileName,"w");
			if(!fullTracefd){
				printf("\n\n Unable to open %s, using stdout \n\n",fullTraceFileName);
				fullTracefd = stdout;
			}
		}


		container_simicsInit();

	}
	else
	{
		exit(printf("\n\n Runtime ERROR ('initializeContainers' called twice"));
	}
}

void container_close()
{
	if(fullTracefd) {
		fclose(fullTracefd);
	}
	containerInitialized = 0;
}


void setFullTraceFile(char *fullTraceFileName)
{
	if(strlen(fullTraceFileName) > 0)
	{
		fullTracefdFileName = fullTraceFileName;
		fullTracefd = fopen(fullTraceFileName,"w");
		if(!fullTracefd){
			printf("\n\n Unable to open %s, using stdout \n\n",fullTraceFileName);
			fullTracefd = stdout;
		}



		thread_monitor_t* th;
		th = ThreadFind(0);
		th->traceFD = fullTracefd;
	}
}

//Obtain a list of symbols
void loadContainersFromSymtable(const char* symFileName)
{
	FILE* symfile ;
	unsigned long long addr;
	char type[4];
	char name[200];
	char linenumber[2000];
	symfile = fopen(symFileName,"r");
	if(!symfile)
	{
		exit(printf("\n\n Runtime ERROR : unable to open symbol file %s",symFileName));
	}
	//printf("\nSymbol file loaded\n");
	while(!feof(symfile)){
		fscanf(symfile,"%llx %s %s\t%s",&addr,type,name,linenumber);
		//printf("%llx %s %s %s\n",addr, type, name,linenumber);
		container * newcont = container_add(addr,name);
		strncpy(newcont->linenumber,linenumber,1000);
	}

	//container_quickprint();

	//get function list from gicasymtable
	//The end address is not in the NM file, but can be found in the simics symtable.functions list
	attr_value_t functions = SIM_get_attribute(SIM_get_object("gicasymtable"),"functions");
	//printf("functions attr type %d\n", functions.kind);
	//printf("functions list lenght %lld\n", functions.u.list.size);
	//FILE * functionsoutFD;
	//functionsoutFD = fopen("listafunctiidinSymtable.txt","w");
	//functionsoutFD = stdout;
	for (int i = 0; i< functions.u.list.size; i++ )
	{
		//printf("functions %d attr type %d\n",i, functions.u.list.vector[i].kind);
		//printf("functions %d list lenght %lld\n",i, functions.u.list.vector[i].u.list.size);
		uint64 startaddr = 0;
		uint64 endaddr = 0 ;
		for(int j=0;j< functions.u.list.vector[i].u.list.size; j++)
		{
			attr_value_t detail = functions.u.list.vector[i].u.list.vector[j];
			//printf("functions %d %d attr type %d\n",i ,j , detail.kind);
			if(detail.kind == 1){
			;
				//fprintf(functionsoutFD,"%s ",detail.u.string);
			}
			else if(detail.kind == 2){
				//fprintf(functionsoutFD,"0x%llx ",detail.u.integer);
				if( j == 2)
					startaddr = detail.u.integer;
				else if (j==3)
					endaddr = detail.u.integer;
			}
		}
		//fprintf(functionsoutFD,"\n");
		container * foundSearch = search(startaddr);
		if(foundSearch)
			foundSearch->endAddress = endaddr;
	}
	//fclose(functionsoutFD);
	//printf("\nloadContainersFromSymtable ended");

	//container_quickprint();
	//exit(0);
}

container* container_add(md_addr_t addr, char * name)
{
	container* newContainer;
	newContainer = (container*) malloc(sizeof(container));
	newContainer->entryAddress = addr;
	newContainer->endAddress = 0;
	newContainer->totalChildContainersCalled = 0;
	newContainer->totalNumberOfReads = 0;
	newContainer->totalNumberOfBytesRead = 0;
	newContainer->totalNumberOfBytesWritten = 0;
	newContainer->totalNumberOfWrites = 0;
	newContainer->totalStackPushes = 0;
	newContainer->totalStackPops = 0;
	newContainer->uniqueChildContainersCalled = 0;
	newContainer->childFunctions = NULL;

	newContainer->instructionFetches = NULL;
	
	newContainer->addressAccessList = NULL;
	newContainer->addressAccessListInstance = NULL;
	newContainer->addressAccessListPenalty = 0;

	newContainer->staticAddressCount = 0;
	newContainer->traceLoadedAddressCount= 0;
	newContainer->traceLoadeduniqueChildContainersCalled = 0;

	newContainer->isCalledWithHeapData = 0;

	newContainer->nonFunction = 0;

	strncpy(newContainer->name,name, 100);

	if(tracefdIn)
	{
		int i = 0;
		while(i<traceLoaderSize){
			if(strcmp(traceLoader[i].name,name) == 0){
				newContainer->traceLoadedAddressCount = traceLoader[i].traceLoadedAddressCount;
				newContainer->traceLoadeduniqueChildContainersCalled = traceLoader[i].traceLoadeduniqueChildContainersCalled;

				break;
			}
			i++;
		}
	}
	if(newContainer->name[0] == '*') {
		newContainer->nonFunction = 1;
		//printf("%s %d\n\n",newContainer->name, newContainer->nonFunction);
	}
	containerTable[containerSize++] = *newContainer;
	//printf("%lld = %llx %s %d %d\n", addr,containerTable[containerSize-1].entryAddress, containerTable[containerSize-1].name, containerTable[containerSize-1].traceLoadedAddressCount, containerTable[containerSize-1].traceLoadeduniqueChildContainersCalled);
	free(newContainer);
	return &containerTable[containerSize-1];
}


struct loadingPenalties container_traceFunctioncall(md_addr_t addr, mem_tp * mem, char displayLineNumbers)
{
	
	
	int i,j=0;
	container * foundSearch;
	mystack returnAddressStack = thread_active->container_runtime_stack;
	struct loadingPenalties loadPenalty;
	loadPenalty.containerStaticListSize = -1;
	loadPenalty.containerDynamicListSize = 0;

	if(ignore_due_to_Exception){ ignore_due_to_Exception = 0; return loadPenalty;}

	cycles_t cycles =  SIM_cycle_count(SIM_current_processor());
	getSP();
	getFP();

	//printf("\n GICA: 0x%llx\n",addr);
	//first verify if the containers were initialized . report exception and quit if not
	if(containerInitialized == 0 || containerSize == 0)
	{
		printf("\n\n Containers not initialized or No symbol table loaded. ex. Call @conf.trace0.set_sym_file = \"file.nm\"\n");
		return loadPenalty;
	}

	//check for function return
	if(!stack_empty(returnAddressStack))
	{
		stackObject t = stack_top(returnAddressStack);
		UpdateAddressList(&( t.containerObj->instructionFetches), addr, 4);

		int returned = 0;
		while(t.returnAddress == addr || t.containerObj->endAddress == addr)
		{
			stack_pop(returnAddressStack);
			t.containerObj->totalStackPops ++;
			updateHeapCalls(t.containerObj,t.containerObj->addressAccessListInstance);
			t.containerObj->addressAccessListPenalty += penaltyAddressList( t.containerObj->addressAccessListInstance);
			sprintf(printBuffer,"%lld\t",cycles);
			myprint(printBuffer);
			for(i = 0; i< returnAddressStack->size; i++) myprint("|\t");
			sprintf(printBuffer,"}\n");
			myprint(printBuffer);

			//for heap acceses we need to update the parent container with that call as well ( heap memory accesses are passed from parent to child)
			if(!stack_empty(returnAddressStack)){
				updateHeapCalls(stack_top(returnAddressStack).containerObj,t.containerObj->addressAccessListInstance);
				loadPenalty.containerStaticListSize = stack_top(returnAddressStack).containerObj->traceLoadedAddressCount + stack_top(returnAddressStack).containerObj->traceLoadeduniqueChildContainersCalled + 3; //all static memory + code + stacksize + timeout
			    loadPenalty.containerDynamicListSize = stack_top(returnAddressStack).containerObj->isCalledWithHeapData;
			}
			t.containerObj->addressAccessListInstance = NULL;

			returned = 1;
			if(!stack_empty(returnAddressStack)) t = stack_top(returnAddressStack);
			else break;
		}
		if(returned) return loadPenalty;
	}

	foundSearch = search(addr);
	if(foundSearch){
		if(ignore_due_to_Exception) {ignore_due_to_Exception = 0;}
		else
			{
				//printf("\n GICA: found 0x%llx 0x%llx %s\n",addr, foundSearch->entryAddress, foundSearch->name);
				//fflush(stdin);
				if(!stack_empty(returnAddressStack)){

					int found = 0;
					stackObject currentContainer = stack_top(returnAddressStack);
					currentContainer.containerObj->totalChildContainersCalled++;
			        llist l = currentContainer.containerObj->childFunctions;
					while(l!= NULL)
					{
						if(l->element.containerObj->entryAddress == addr)
						{
							found = 1;
							break;
						}
						l = l->next;
					}
					if(!found)
					{
						currentContainer.containerObj->uniqueChildContainersCalled ++;
						stackObject t ;
						t.containerObj = foundSearch;
						currentContainer.containerObj->childFunctions = cons(t,currentContainer.containerObj->childFunctions);
					}
				}
				sprintf(printBuffer,"%lld\t",cycles);
				myprint(printBuffer);
				for(j = 0; j< returnAddressStack->size; j++) myprint("|\t");
				if(!foundSearch->nonFunction){
					//sprintf(printBuffer,"%x %s \n",addr,foundSearch->name);
					//sprintf(printBuffer,"0x%llx %s %d %d\n",foundSearch->entryAddress,foundSearch->name,foundSearch->traceLoadedAddressCount,foundSearch->traceLoadeduniqueChildContainersCalled);
					if(displayLineNumbers)
						sprintf(printBuffer,"%llx %s {%s \n",addr, foundSearch->name, foundSearch->linenumber);
					else{
						//sprintf(printBuffer,"%s {\n", foundSearch->name);
						sprintf(printBuffer,"%s ret=%llx {\n", foundSearch->name, getRet());
					}
					myprint(printBuffer);
					//fflush(stdin);
					//simulate loading the access list
					int sizeOfAccessList = foundSearch->traceLoadedAddressCount + foundSearch->traceLoadeduniqueChildContainersCalled + 3; //all static memory + code + stacksize + timeout
					loadPenalty.containerStaticListSize = sizeOfAccessList;
					if(!stack_empty(returnAddressStack))
						loadPenalty.containerDynamicListSize = (-1)*stack_top(returnAddressStack).containerObj->isCalledWithHeapData;

					stackObject t;
					foundSearch->totalStackPushes ++;
					t.containerObj = foundSearch;
					t.returnAddress = getRet();//return_addr;

					stack_push(returnAddressStack, t);
					UpdateAddressList(&( t.containerObj->instructionFetches), addr, 4);

				}
				else
				{
					if(displayLineNumbers)
						sprintf(printBuffer,"%s %s\n", foundSearch->name,foundSearch->linenumber);
					else
						sprintf(printBuffer,"%s\n", foundSearch->name);
					myprint(printBuffer);
				}
				//EXIT function needs to force containers to pop all remaining from stack. Otherwise the Complete access list for main
				/*
				if(strcmp(foundSearch->name,"*halt") == 0)
				{
					while(!stack_empty(returnAddressStack))
					{
						stackObject t = stack_top(returnAddressStack);
						stack_pop(returnAddressStack);
						t.containerObj->totalStackPops ++;
						for(i = 0; i< returnAddressStack->size; i++) myprint("|\t");
						printDecodedAddressList(printBuffer,t.containerObj->addressAccessListInstance);
						myprint("*\n");
						updateGlobalAddressList(t.containerObj);
						updateHeapCalls(t.containerObj,t.containerObj->addressAccessListInstance);
						t.containerObj->addressAccessListPenalty += penaltyAddressList( t.containerObj->addressAccessListInstance);
						for(i = 0; i< returnAddressStack->size; i++) myprint("|\t");
						myprint("*\n");
					}
					exit(printf("FORCE EXIT - please fix"));
				}
				*/
			}
		return loadPenalty;
	}


	//end of search list reached without finding the function, save the address as possible return address
	return_addr = addr+4;
	return loadPenalty;
}

void container_MemoryCall(mem_tp cmd,md_addr_t addr, int nbytes)
{
    
	mystack returnAddressStack = thread_active->container_runtime_stack;
	
	if(containerInitialized == 1 && !stack_empty(returnAddressStack))
	{
		
		stackObject t = stack_top(returnAddressStack);
		//container_dumpRegisters(*regs);
		//if(addr >= 0xf0000000ULL)  
		//	{ printf("%s %s %llxn\n",__PRETTY_FUNCTION__, t.containerObj->name, addr); fflush(stdin);}
		//collect the continuous address accesses
		//this implementation : take care only of the memaccesses that are in sequence.
		//if(strcmp(t.containerObj->name,"crc32file") == 0)
    	//	printf("MemoryCall %d %llx\n",cmd,addr);

		UpdateAddressList(&( t.containerObj->addressAccessList), addr, nbytes);
		UpdateAddressList(&( t.containerObj->addressAccessListInstance), addr, nbytes);


		if(cmd == Read)
		{
			t.containerObj->totalNumberOfReads++;
			t.containerObj->totalNumberOfBytesRead +=nbytes;
			//for(j = 0; j< returnAddressStack->size; j++) myprint("|\t");
			//sprintf(printBuffer,"R %x %x(%d)\n",regs->regs_PC,addr,nbytes);
			//myprint(printBuffer);
		}
		else
		{
			t.containerObj->totalNumberOfBytesWritten +=nbytes;
			t.containerObj->totalNumberOfWrites++;
			//for(j = 0; j< returnAddressStack->size; j++) myprint("|\t");
			//sprintf(printBuffer,"W %x %x(%d)\n",regs->regs_PC,addr,nbytes);
			//myprint(printBuffer);
		}
	}

}

void container_quickprint()
{
	for (int i=0 ; i < containerSize; i++)
	{
		fprintf(stdout,"\n %llx %llx \t %s \n",
						containerTable[i].entryAddress,
						containerTable[i].endAddress,
						containerTable[i].name
		);
	}
}

void container_printMemoryRanges(int bAll )
{
	sprintf(printBuffer,"entryAddress endAddress\tname\tcount\tLIST\n");
	myprint(printBuffer);
	for (int i=0 ; i < containerSize; i++)
	{
		addressList l = containerTable[i].addressAccessList;
		int bUsed = containerTable[i].totalStackPushes > 0;
		if(bAll || bUsed){
			//count (we do not have a size of the list)
			int jcnt = 0;
			addressList jl = l;
			while (jl) {jcnt++;jl=jl->next;}
			
			sprintf(printBuffer,"%llx %llx\t%s\t%d\t",
							containerTable[i].entryAddress,
							containerTable[i].endAddress,
					(containerTable[i].name),
					jcnt);
			myprint(printBuffer);
			
			while(l!=NULL)
			{
				sprintf(printBuffer,"[%llx,%llx) ",l->startAddress, l->endAddress);
				myprint(printBuffer);
				l = l->next;
			}
			sprintf(printBuffer,"\n");
			myprint(printBuffer);
		}
	}
}

void container_printDecodedMemoryRanges(int bAll )
{
	sprintf(printBuffer,"data: %llx %llx stack: %llx %llx \n",ld_text_base,ld_text_bound,ld_stack_base ,ld_stack_base+ ld_stack_size);
	myprint(printBuffer);
	sprintf(printBuffer,"entryAddress endAddress\tname\tcount\tLIST\n");
	myprint(printBuffer);

	
	for (int i=0 ; i < containerSize; i++)
	{
		addressList l = containerTable[i].addressAccessList;
		addressList m = containerTable[i].instructionFetches;
		
		int bUsed = containerTable[i].totalStackPushes > 0;
		if(bAll || bUsed){
			//count (we do not have a size of the list)
			int jcnt = 0;
			addressList jl = l;
			while (jl) {jcnt++;jl=jl->next;}
			jl = m;
			while (jl) {jcnt++;jl=jl->next;}

			
			sprintf(printBuffer,"%llx %llx\t%s\t%d\t",
							containerTable[i].entryAddress,
							containerTable[i].endAddress,
					(containerTable[i].name),
					jcnt);
			myprint(printBuffer);
			printDecodedInstructionFetchList(printBuffer,m);
			myprint(printBuffer);
			printDecodedAddressList(printBuffer,l);
			myprint(printBuffer);
			sprintf(printBuffer,"\n");
			myprint(printBuffer);
		}
	}

}

void container_printSimpleCountAddressAcess(int bAll )
{
	sprintf(printBuffer,"entryAddress endAddress\tname\tcode initialized stack heap\n");
	myprint(printBuffer);
	for (int i=0 ; i < containerSize; i++)
	{
		addressList l = containerTable[i].addressAccessList;
		int bUsed = containerTable[i].totalStackPushes > 0;
		if(bAll || bUsed){
			sprintf(printBuffer,"%llx %llx\t%s\t",
							containerTable[i].entryAddress,
							containerTable[i].endAddress,
					(containerTable[i].name));
			myprint(printBuffer);
			
				printCountMemoryAccesses(printBuffer,l);
				myprint(printBuffer);
				sprintf(printBuffer,"\n");
				myprint(printBuffer);
		}
	}

	
}

void container_printChildFunctionsCalled(int bAll)
{
	sprintf(printBuffer,"entryAddress endAddress\tname\tcount\tLIST\n");
		myprint(printBuffer);

	for (int i=0 ; i < containerSize; i++)
	{
		llist l = containerTable[i].childFunctions;
		if(bAll || l){
			sprintf(printBuffer,"%llx %llx\t%s\t%d\t",
							containerTable[i].entryAddress,
							containerTable[i].endAddress,
					(containerTable[i].name),
					containerTable[i].uniqueChildContainersCalled
					);
			myprint(printBuffer);

			llist l = containerTable[i].childFunctions;
			while(l)
			{
				sprintf(printBuffer,"%llx %s\t",
							l->element.containerObj->entryAddress,
							l->element.containerObj->name);
				myprint(printBuffer);
				l = l->next;
			}
			sprintf(printBuffer,"\n");
			myprint(printBuffer);	
		}
	}
	
}


void printAllStatsFiles(char * fStatsFileBaseName)
{
	char * baseFileName = "\0";

	if(strlen(fStatsFileBaseName) != 0)
	{
		baseFileName = fStatsFileBaseName;
	}
	
	char * fullAddressAccessListFileName = "FullAddressAccessList.txt";
	char * fullDecodedAddressAccessListFileName = "FullDecodedAddressAccessList.txt";
	char * simpleCountAddressAcessFileName = "SimpleCountAddressAccessList.txt";
	char * containerCallListFileName = "ContainerCallList.txt";
	char * containerStatisticsFileName = "ContainerStats.txt";

	FILE * fullAddressAccessListFile;
	FILE * fullDecodedAddressAccessListFile;
	FILE * simpleCountAddressAcessFile;
	FILE * containerCallListFile;
	FILE * containerStatisticsFile;
	
	char *s = (char *)malloc(snprintf(NULL, 0, "%s %s", baseFileName, fullAddressAccessListFileName) + 1);
	sprintf(s, "%s%s", baseFileName, fullAddressAccessListFileName);
    fullAddressAccessListFile = fopen(s,"w");
		if(!fullAddressAccessListFile){
			printf("\n\n Unable to open %s, using stdout \n\n",s);
			fullAddressAccessListFile = stdout;
		}
	free(s);
	s = (char *)malloc(snprintf(NULL, 0, "%s %s", baseFileName, fullDecodedAddressAccessListFileName) + 1);
	sprintf(s, "%s%s", baseFileName, fullDecodedAddressAccessListFileName);
	fullDecodedAddressAccessListFile = fopen(s,"w");
		if(!fullDecodedAddressAccessListFile){
			printf("\n\n Unable to open %s, using stdout \n\n",s);
			fullDecodedAddressAccessListFile = stdout;
		}
	free(s);
	s = (char *)malloc(snprintf(NULL, 0, "%s %s", baseFileName, simpleCountAddressAcessFileName) + 1);
	sprintf(s, "%s%s", baseFileName, simpleCountAddressAcessFileName);
	simpleCountAddressAcessFile = fopen(s,"w");
		if(!simpleCountAddressAcessFile){
			printf("\n\n Unable to open %s, using stdout \n\n",s);
			simpleCountAddressAcessFile = stdout;
		}
	free(s);
	s = (char *)malloc(snprintf(NULL, 0, "%s %s", baseFileName, containerCallListFileName) + 1);
	sprintf(s, "%s%s", baseFileName, containerCallListFileName);
	containerCallListFile = fopen(s,"w");
		if(!containerCallListFile){
			printf("\n\n Unable to open %s, using stdout \n\n",s);
			containerCallListFile = stdout;
		}
	free(s);
	s = (char *)malloc(snprintf(NULL, 0, "%s %s", baseFileName, containerStatisticsFileName) + 1);
	sprintf(s, "%s%s", baseFileName, containerStatisticsFileName);
	containerStatisticsFile = fopen(s,"w");
		if(!containerStatisticsFile){
			printf("\n\n Unable to open %s, using stdout \n\n",s);
			containerStatisticsFile = stdout;
		}
	free(s);


	overrideOutputfd = containerStatisticsFile;
	container_printStatistics(0);
	fflush(overrideOutputfd);
	fclose(overrideOutputfd);
	overrideOutputfd = fullAddressAccessListFile;
	container_printMemoryRanges(0);
	fflush(overrideOutputfd);
	fclose(overrideOutputfd);
	overrideOutputfd = fullDecodedAddressAccessListFile;
	container_printDecodedMemoryRanges(0);
	fflush(overrideOutputfd);
	fclose(overrideOutputfd);
	overrideOutputfd = simpleCountAddressAcessFile;
	container_printSimpleCountAddressAcess(0);
	fflush(overrideOutputfd);
	fclose(overrideOutputfd);
	overrideOutputfd = containerCallListFile;
	container_printChildFunctionsCalled(0);
	fflush(overrideOutputfd);
	fclose(overrideOutputfd);
	overrideOutputfd = NULL;

	printf("Done PRINTING stat files \n"); fflush(stdin);
}


void container_printStatistics (int bAll)
{
	int i;
	int totalFunctionCalls = 0;
	int totalFunctionReturns = 0;
	//mystack returnAddressStack = thread_active->container_runtime_stack;

	//sprintf(printBuffer,"max-stack-size: %d\n",returnAddressStack->maxsize);
	//myprint(printBuffer);
	//sprintf(printBuffer,"container-table-size: %d\n",containerSize);
	//myprint(printBuffer);

	//sprintf(printBuffer,"Container statistics :\n");
	//myprint(printBuffer);
	sprintf(printBuffer,"Address \t Name \t Reads \t BytesRead \t Writes \t BytesWritten \t totalChildContainersCalled \t totalStackPushes \t totalStackPops \t uniqueChildContainersCalled \t SizeOfAccessList\n");
	myprint(printBuffer);
	for ( i=0 ; i < containerSize; i++)
	{
		int bUsed = containerTable[i].totalStackPushes > 0;
		if(bAll || bUsed){
			sprintf(printBuffer,"%llx %llx \t %s \t %d \t %d \t %d \t %d \t %d \t %d \t %d \t %d \t %d \n",
					containerTable[i].entryAddress,
					containerTable[i].endAddress,
					(containerTable[i].name),
					containerTable[i].totalNumberOfReads,
					containerTable[i].totalNumberOfBytesRead,
					containerTable[i].totalNumberOfWrites,
					containerTable[i].totalNumberOfBytesWritten,
					containerTable[i].totalChildContainersCalled,
					containerTable[i].totalStackPushes,
					containerTable[i].totalStackPops,
					containerTable[i].uniqueChildContainersCalled,
					containerTable[i].addressAccessListPenalty
					);
			myprint(printBuffer);
			totalFunctionCalls += containerTable[i].totalStackPushes;
			totalFunctionReturns += containerTable[i].totalStackPops;
		}	
	}
	//sprintf(printBuffer,"totalFunctionCalls: %d\n",totalFunctionCalls);
	//myprint(printBuffer);
	//sprintf(printBuffer,"totalFunctionReturns: %d\n",totalFunctionReturns);
	//myprint(printBuffer);

	//sprintf(printBuffer,"Static Data Access List address name heap staticlist:\n");
	//myprint(printBuffer);
	/*
	for ( i=0 ; i < containerSize; i++)
	{

		if(containerTable[i].totalStackPushes > 0){
			sprintf(printBuffer,"%llx\t%s\t\t\t\t\t%s\t",
					containerTable[i].entryAddress,
					(containerTable[i].name),
					containerTable[i].isCalledWithHeapData ? "true":"false"
					);
			myprint(printBuffer);
			printDecodedAddressList(printBuffer,containerTable[i].staticAddressList);
			sprintf(printBuffer,"\n");
			myprint(printBuffer);

			if(containerTable[i].isCalledWithHeapData){
				sprintf(compilerPrintBuffer,"%s %d\n",
						(containerTable[i].name),
						containerTable[i].isCalledWithHeapData
						);
				myprint(compilerPrintBuffer);
			}

			sprintf(printBuffer2,"%s ",
					containerTable[i].name);
			myprint(printBuffer2);
			//printAddressList(printBuffer2,containerTable[i].staticAddressList);
			addressList l = containerTable[i].staticAddressList;
			while(l!=NULL)
			{
				containerTable[i].staticAddressCount++;
				l = l->next;
			}
			sprintf(printBuffer2,"%d %d\n",containerTable[i].staticAddressCount,containerTable[i].uniqueChildContainersCalled);
			myprint(printBuffer2);
		}
	}
	*/

}

void UpdateAddressList(addressList *list,md_addr_t addr,int nbytes)
{

	/*
	mystack returnAddressStack = thread_active->container_runtime_stack;
	if(containerInitialized == 1 && !stack_empty(returnAddressStack)) {
		stackObject t = stack_top(returnAddressStack);
		
		//if( strcmp(t.containerObj->name,"malloc") == 0){
		  if( addr > 0xf0000000ULL){
		  	
			printf("%s %s start",__PRETTY_FUNCTION__, t.containerObj->name);
			addressList pl = *list;
			while(pl) { printf("(%llx %llx) ",pl->startAddress, pl->endAddress); pl=pl->next;}
			printf("%llx %d\n",addr, nbytes);
		}
	}*/
	

	//parse the list
	//try every range , if the new address is inside , ignore
	//if the addr_nbytes overlaps one range, addit and continue searching . If the address hits another range, merge the 2 ranges

	// if the end is reached, add new range
	int found = 0;
	//int ignore = 0;
	addressList lprevious = NULL;
	addressList l = *list;
	while(l!= NULL)
	{
		md_addr_t start = l->startAddress;
		md_addr_t end = l->endAddress;
		//printf("start = %llx end = %llx addr = %llx addr+nbytes = %llx\n",start, end, addr, addr + nbytes);
		if( start <= addr && end >= addr+nbytes)
		{
			//printf("start <= addr AND end >= addr+nbytes\n");
			if(found == 0)
			{
				found = 1;
				lprevious = l;
				break;
			}
			else
			{
				joinAddress(l, lprevious);
			}
		}
		else if( addr <= start  && addr + nbytes >= start )
		{
			//printf("addr <= start  AND addr + nbytes >= start\n");
			if(found == 1) joinAddress(l, lprevious);
			else
			{
				l->startAddress = addr;
				l->endAddress = ( (addr+nbytes) > l->endAddress) ? addr+nbytes : l->endAddress;
				found = 1;
				lprevious = l;
			}
		}
		else if( addr <= end && addr + nbytes > end )
		{
			//printf("addr <= end AND addr + nbytes > end\n");
			if(found == 1) joinAddress(l, lprevious);
			else
			{
				l->endAddress = addr + nbytes;
				found = 1;
				lprevious = l;
			}
		}
		l = l->next;
	}
	if(!found)
	{
		*list = consAddressList(addr,addr + nbytes,*list);
	}
	
	/*
	if(containerInitialized == 1 && !stack_empty(returnAddressStack)) {
		stackObject t = stack_top(returnAddressStack);
		
		//if(strcmp(t.containerObj->name,"malloc") == 0){
		if( addr > 0xf0000000ULL){
			printf("%s end",__PRETTY_FUNCTION__);
			addressList pl = *list;
			while(pl) { printf("(%llx %llx) ",pl->startAddress, pl->endAddress); pl=pl->next; }
			printf("\n");
		}
	}
	*/
	
}

void joinAddress(addressList future, addressList present)
{
	//printf("%s\n",__PRETTY_FUNCTION__);
	
	present->startAddress = present->startAddress > future->startAddress ? future->startAddress : present->startAddress;
	present->endAddress = present->endAddress < future->startAddress ? future->startAddress : present->endAddress;
	//exit(0);
	//find and delete future
	addressList l = present->next;
	addressList lprev = present;
	while( l != future)
	{
		lprev = l;
		l = l->next;
	}

	lprev->next = l->next;
	free(l);

}

void printAddressList(char * printbuff,addressList l){
	printbuff[0] = 0;
	while(l!=NULL)
	{
		sprintf(printbuff,"[%llx,%llx) ",l->startAddress, l->endAddress);
		myprint(printbuff);
		l = l->next;
	}
}

int addressListSize(addressList l)
{
	int count = 0;
	while(l!=NULL)
	{
		count++;
		l = l->next;
	}
	return count;
}

void printCurrentContainerStack( )
{
	mystack returnAddressStack = thread_active->container_runtime_stack;
	char name[4];
	toStringRTEMSTaksName(name,thread_active->thread_name);
	printf("Thread: %s id:0x%llx\n",
		name,
		thread_active->thread_id
		);
	if(!stack_empty(thread_active->container_runtime_stack)){
		//list start = returnAddressStack->elements;
		llist next = returnAddressStack->elements;
		for(int i=0;i<returnAddressStack->size && next != NULL ;i++)
		{
			printf("%d 0x %llx %s ret = 0x%lld \n",
				i,
				next->element.containerObj->entryAddress,
				next->element.containerObj->name,
				next->element.returnAddress);
			printAddressList(stdoutPrintBuffer,next->element.containerObj->addressAccessListInstance);
			next = next->next;
		}
	}

}

void printDecodedInstructionFetchList(char * printbuff,addressList l)
{
	printbuff[0] = 0;
	while(l!=NULL)
	{
		printbuff += sprintf(printbuff,"%c[%llx,%llx) ",'f',l->startAddress,l->endAddress);
		l = l->next;
	}
}


void printDecodedAddressList(char * printbuff,addressList l)
{
	printbuff[0] = 0;
	while(l!=NULL)
	{
		decodedMemRange md = decodeMemoryRange(l->startAddress, l->endAddress);
		printbuff += sprintf(printbuff,"%c[%llx,%llx) ",md.type,md.base,md.bound);
		l = l->next;
	}
}

void printCountMemoryAccesses(char * printbuff,addressList l)
{
    int c = 0, i = 0, s = 0, h = 0;
	printbuff[0] = 0;
	while(l!=NULL)
	{
		decodedMemRange md = decodeMemoryRange(l->startAddress, l->endAddress);
		switch (md.type)
		{
			case 'c': c++;break;
			case 's': s++;break;
			case 'h': h++;break;
			default : break;
		}
		l = l->next;
	}
	sprintf(printbuff,"%d %d %d %d",c, i, s, h);
}


//Computes how many distinct heap memory access a function makes.
//It takes the maximum number of different accesses that a particular instance makes ( this is a conservative approach)
// I can take the average or mean in the future.
void updateHeapCalls(container* c,addressList l)
{
	int heapAccessesInInstance = 0;
	while(l!=NULL)
	{
		decodedMemRange md = decodeMemoryRange(l->startAddress, l->endAddress);
		if (md.type == 'h') {
			UpdateAddressList( &(c->addressAccessListInstance), l->startAddress,l->endAddress-l->startAddress);
			heapAccessesInInstance ++;
		}
		l = l->next;
	}
	if(c->isCalledWithHeapData < heapAccessesInInstance) c->isCalledWithHeapData = heapAccessesInInstance;
}

decodedMemRange decodeMemoryRange(md_addr_t base, md_addr_t bound)
{
	decodedMemRange ret;
 	ret.base = base;
	ret.bound = bound;
	if( base >= ld_text_base && bound <= ld_text_bound )
		ret.type = 'c';
	else if(base >= ld_stack_base  && bound <= ld_stack_base + ld_stack_size) 
		ret.type = 's'; //stack
	else 
		ret.type = 'h'; //heap

	return ret;
}

int penaltyAddressList( addressList l) //counts the number of ranges in the access list.
{
	int val = 0;
	while(l!=NULL)
	{
		val ++;
		l = l->next;
	}
	return val;
}


void myprint(char * toPrint)
{
	if(overrideOutputfd)
	{
		fprintf(overrideOutputfd,"%s",toPrint);
		return;
	}

	if(toPrint == printBuffer2)
		fprintf(tracefdOut,"%s",toPrint);
	else if(toPrint == compilerPrintBuffer && compilerInfofd)
		fprintf(compilerInfofd,"%s",toPrint);
	else if(fullTracefd){
		fprintf(thread_active->traceFD,"%s",toPrint);
		fflush(thread_active->traceFD);
		//fprintf(stdout,"%s",toPrint);
		//fflush(stdout);
	}
	else
	{
		fprintf(stdout,"%s",toPrint);
		//fflush(stdout);
	}

	//printf("%s",toPrint);
}

void containers_flush()
{
	fflush(stdin);
	fflush(stderr);
	if(fullTracefd)
		fflush(thread_active->traceFD);
	if(compilerInfofd)
		fflush(compilerInfofd);
}

char * funcNameFromAddress(int addr)
{
	int i;
	for ( i=0 ; i < containerSize; i++)
	{
		//function call
		if(addr == containerTable[i].entryAddress)
			return containerTable[i].name;
	}
	return NULL;
}

addressList consAddressList(md_addr_t startAddress, md_addr_t endAddress, addressList l)
{
	addressList temp = (addressList) malloc(sizeof(struct addressCell));
    temp -> startAddress = startAddress;
	temp -> endAddress = endAddress;
    temp -> next = l;
    return temp;
}
addressList freeAddressList(addressList l)
{
	addressList temp = l -> next;
    free(l);
    return temp;
}


llist cons(stackObject element, llist l){
    llist temp = (llist) malloc(sizeof(struct cell));
    temp -> element = element;
    temp -> next = l;
    return temp;
}

llist cdr_and_free(llist l){
    llist temp = l -> next;
    free(l);
    return temp;
  }

mystack stack_create(void)
{
  mystack temp = (mystack) malloc(sizeof(struct mystackStr));
  temp -> elements = NULL;
  temp->size = 0;
  temp->maxsize = 0;
  return temp;
}

void stack_push(mystack s,stackObject element)
{
  fflush(stdin);
  s -> elements = cons(element, s -> elements);
  s->size++;
  if(s->size > s->maxsize) s->maxsize = s->size;
  //printf("stack push %s\n",s->elements->element.container->name);
}

int stack_empty(mystack s)
{
  return s -> elements == NULL;
}

void stack_pop(mystack s)
{
  //assert(!empty(s));
  s -> elements = cdr_and_free(s -> elements);
  s->size--;
}

stackObject stack_top(mystack s)
{
  //assert(!empty(s));
  return s -> elements -> element;
}

int container_comparison(const void * m1, const void * m2){
	//printf("GICA: comparing %llx %llx ? %llx\n", ((container *)m1)->entryAddress, ((container *)m2)->entryAddress, ((container *)m1)->entryAddress - ((container *)m2)->entryAddress);
	return ((container *)m1)->entryAddress - ((container *)m2)->entryAddress;
}

//the container list is sorted by address
container * search(md_addr_t addr)
{
	container key;
	key.entryAddress = addr;
	return (container *)bsearch(&key, containerTable, containerSize,sizeof(container),container_comparison);
}

int container_comparison_debug(const void * m1, const void * m2){
	printf("\t GICA: comparing %llx %llx ? %llx\n", ((container *)m1)->entryAddress, ((container *)m2)->entryAddress, ((container *)m1)->entryAddress - ((container *)m2)->entryAddress);
	return ((container *)m1)->entryAddress - ((container *)m2)->entryAddress;
}

//the container list is sorted by address
container * search_debug(md_addr_t addr)
{
	printf("\t %s %llx\n",__PRETTY_FUNCTION__,addr);
	container key;
	key.entryAddress = addr;
	for(int i=0;i <containerSize; i++)
		printf("\t\t %llx %s\n",containerTable[i].entryAddress,containerTable[i].name);
	return (container *)bsearch(&key, containerTable, containerSize,sizeof(container),container_comparison_debug);
}


//given a PC value, return the container that containes that instruction
int searchUsingInnerAddress(md_addr_t addr)
{
	int i=0;
	for(;i<containerSize;i++)
	{
		container *item = &containerTable[i];
		if(item->entryAddress <= addr && item->endAddress >= addr)
			return i;
	}
	return -1;
}


int checkAlphaNumeric(char x)
{
	if( (x >= 'A' && x <='Z' ) || (x >= 'a' && x <= 'z' ) || (x >= '0' && x <= '9' ))
		return 1;
	return 0;
}

void toStringRTEMSTaksName(char * dest, int _name)
{
	dest[0] = ((_name) >> 24) & 0xff;
	dest[1] = ((_name) >> 16) & 0xff;
	dest[2] = ((_name) >> 8) & 0xff;
	dest[3] = (_name) & 0xff;
	dest[4] = 0;

	if(!checkAlphaNumeric(dest[0])) dest[0] = 0;
	if(!checkAlphaNumeric(dest[1])) dest[1] = 0;
	if(!checkAlphaNumeric(dest[2])) dest[2] = 0;
	if(!checkAlphaNumeric(dest[3])) dest[3] = 0;
}


/* itoa:  convert n to characters in s */
void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

uint64 getSP()
{

	uint64 sp = SIM_read_register(proc,o6id);
	if(sp < thread_active->minStack){
		thread_active->minStack = sp;
		//char name[] = "\0\0\0\0\0";
		//toStringRTEMSTaksName(name,thread_active->thread_name);
		//printf("%s : spmin=0x%llx\n",name,sp);
	}
	if(sp > thread_active->maxStack){
		thread_active->maxStack = sp;
		//char name[] = "\0\0\0\0\0";
		//toStringRTEMSTaksName(name,thread_active->thread_name);
		//printf("%s : spmax=0x%llx\n",name,sp);
	}


	return	sp;
}

uint64 getFP()
{
	uint64 fp = SIM_read_register(proc,i6id);
	if(fp < thread_active->minFP){
		thread_active->minFP = fp;
		//char name[] = "\0\0\0\0\0";
		//toStringRTEMSTaksName(name,thread_active->thread_name);
		//printf("%s : fpmin=0x%llx\n",name,fp);
	}
	if(fp > thread_active->maxFP){
		thread_active->maxFP = fp;
		//char name[] = "\0\0\0\0\0";
		//toStringRTEMSTaksName(name,thread_active->thread_name);
		//printf("%s : fpmax=0x%llx\n",name,fp);
	}
	return	fp;
}

uint64 getRet()
{
	return	SIM_read_register(proc,o7id) + 8;
}


void printThreads()
{
	if(thread_active == NULL ) printf("NO THREAD ADDED\n");
	thread_monitor_t* iterate;
	iterate = thread_active;

	do
	{
		char name[4];
		toStringRTEMSTaksName(name,iterate->thread_name);
		printf("Thread: %s id:%lld spmin:0x%llx spmax:0x%llx fpmin:0x%llx fpmax:0x%llx\n",
			name,
			iterate->thread_id,
			iterate->minStack,
			iterate->maxStack,
			iterate->minFP,
			iterate->maxFP
			);
		iterate = iterate->next;
	}
	while (iterate != thread_active);
}

attr_value_t myeval(char * evalExpr)
{
	//printf("\n GICADEBUG %s \n",evalExpr);
	conf_class_t* symtblclass = SIM_get_class("symtable");
	void * intrface = SIM_get_class_interface(symtblclass,"symtable");
	symtable_interface_t * symIntr = (symtable_interface_t *)intrface;
	conf_object_t *proc;
	proc =	SIM_current_processor();
    //we need the top stack frame
    attr_value_t frames = symIntr->stack_trace(proc,10);
	attr_list_t frameList = frames.u.list;

	return symIntr->eval_sym(proc, evalExpr ,&(frameList.vector[frameList.size-1]), NULL);
}

//lenght in bytes
uint64 myMemoryRead(generic_address_t vaddr, int lenght)
{
	SIM_clear_exception();
	uint64 tt = 0;
	conf_object_t * cpu_mem;

	//printf("\n GICADEBUG myMemoryRead vaddr=%llx length=%d\n",vaddr, lenght);
	//SIM_break_simulation("GICADEBUG break_simulation\n");

	physical_address_t physaddr = SIM_logical_to_physical(SIM_current_processor(),Sim_DI_Data, vaddr);
	cpu_mem = SIM_get_object("phys_mem");
	if(!cpu_mem) exit(printf("\n'phys_mem' is not a valid memory space for this target. Exiting !"));
	for ( generic_address_t addrt = physaddr; addrt < physaddr + lenght; addrt++){
   		uint32 whatdidread =  SIM_read_byte(cpu_mem, addrt);
		tt <<= 8;
		tt |= 0x000000FF & whatdidread;
	}
	return tt;
}

//lenght in bytes
void myMemoryWrite(generic_address_t vaddr,uint64 value, int lenght)
{
	SIM_clear_exception();
	conf_object_t * cpu_mem;

	//printf("\n GICADEBUG myMemoryRead vaddr=%llx length=%d\n",vaddr, lenght);
	//SIM_break_simulation("GICADEBUG break_simulation\n");

	physical_address_t physaddr = SIM_logical_to_physical(SIM_current_processor(),Sim_DI_Data, vaddr);
	cpu_mem = SIM_get_object("phys_mem");
	if(!cpu_mem) exit(printf("\n'phys_mem' is not a valid memory space for this target. Exiting !"));
	for ( generic_address_t addrt = physaddr + lenght - 1; addrt >= physaddr ; addrt--){
   		uint8 toWrite =  0x000000FF & value;
		value >>= 8;
		SIM_write_byte(cpu_mem, addrt, toWrite);
	}
	
}


uint64 mySimicsIntSymbolRead(char * symbol)
{
	uint64 ret = 0;

	attr_value_t symbolStartOld = myeval(symbol);
	//ASSERT(symbolStartOld.kind == 4);
	ret = symbolStartOld.u.list.vector[1].u.integer;
	
	return ret;		
}


void containers_testRandomStuff(int option){

	printf("%s",__PRETTY_FUNCTION__);
	switch(option)
	{
		case 0 : {
				printf("test range merge\n");
				uint64 a,b,c,d;
				a = 0xf000618c;
				b = 0xf0006190;
				c = 0xf0006188;
				d = 0xf000618c;
				
				addressList pl1 = (addressList )malloc(sizeof(struct addressCell));
				addressList pl2 = (addressList )malloc(sizeof(struct addressCell));

						
				pl1->startAddress = c;
				pl1->endAddress = d;
				pl2->startAddress = c;
				pl2->endAddress = d;

				pl1->next = NULL;
				pl2->next = NULL;

				addressList l = pl1;
				while(l){
					printf("(%llx %llx)", l->startAddress, l->endAddress);l = l->next;
				}
				printf("\n");
				UpdateAddressList(&pl1,a,4);
				l = pl1;
				while(l){
					printf("(%llx %llx)", l->startAddress, l->endAddress);l = l->next;
				}
			}
			break;
		default :
			printf("%s nothing to do for this option\n",__PRETTY_FUNCTION__);
			break;
	}
}


