/*
  trace.h

  Copyright 1998-2007 Virtutech AB
  
  The contents herein are Source Code which are a subset of Licensed
  Software pursuant to the terms of the Virtutech Simics Software
  License Agreement (the "Agreement"), and are being distributed under
  the Agreement.  You should have received a copy of the Agreement with
  this Licensed Software; if not, please contact Virtutech for a copy
  of the Agreement prior to using this Licensed Software.
  
  By using this Source Code, you agree to be bound by all of the terms
  of the Agreement, and use of this Source Code is subject to the terms
  the Agreement.
  
  This Source Code and any derivatives thereof are provided on an "as
  is" basis.  Virtutech makes no warranties with respect to the Source
  Code or any derivatives thereof and disclaims all implied warranties,
  including, without limitation, warranties of merchantability and
  fitness for a particular purpose and non-infringement.

*/

#ifndef COMPONENTSTRACE_H
#define COMPONENTSTRACE_H


#include <simics/api.h>
#include <simics/alloc.h>
#include <simics/utils.h>
#include <simics/arch/sparc.h>
#include <simics/arch/x86.h>

#undef HAVE_LIBZ
#if defined(HAVE_LIBZ)
 #include <zlib.h>
 #define GZ_FILE(bt) ((bt)->gz_file)
 #define GZ(x)       (x)
#else
 #define GZ_FILE(bt) NULL
 #define GZ(x)
#endif


typedef enum { 
        TR_Reserved = 0, TR_Data = 1, TR_Instruction = 2, TR_Exception = 3, 
        TR_Execute = 4, TR_Reserved_2, TR_Reserved_3, TR_Reserved_4,
        TR_Reserved_5, TR_Reserved_6, TR_Reserved_7, TR_Reserved_8,
        TR_Reserved_9, TR_Reserved_10, TR_Reserved_11, TR_Reserved_12
} trace_type_t;

typedef enum {
        TA_generic, TA_x86, TA_ia64, TA_v9
} trace_arch_t;

typedef struct {
        unsigned arch:2;
        unsigned trace_type:4;	        /* data, instruction, or exception (trace_type_t) */
        int      cpu_no:16;		/* processor number (0 and up) */
        uint32   size;
        unsigned read_or_write:1;	/* read_or_write_t */

        /* x86 */
        unsigned linear_access:1;       /* if linear access */
        unsigned seg_reg:3;             /* 0-5, with ES=0, CS=1, SS=2, DS=3, FS=4, GS=5 */
        linear_address_t la;
        x86_memory_type_t memory_type;

        /* x86_access_type_t or sparc_access_type_t */
        int access_type;

        /* virtual and physical address of effective address */
        logical_address_t va;
        physical_address_t pa;

        int atomic;

        union {
                uint64 data;            /* if TR_Date */
                uint8  text[16];        /* if TR_Instruction or TR_Execute
                                           (large enough for any target) */
                int exception;		/* if TR_Exception */
        } value;

        cycles_t timestamp;             /* Delta coded */
} trace_entry_t;

/* ADD INTERFACE trace_consume_interface */
typedef struct {
        void (*consume)(conf_object_t *obj, trace_entry_t *entry);
} trace_consume_interface_t;

#define TRACE_CONSUME_INTERFACE "trace_consume"

/* Cached information about a processor. */
typedef struct {
        unsigned va_digits;
        unsigned pa_digits;
        conf_object_t *cpu;
        char name[10];
        tuple_int_string_t (*disassemble_buf)(
                conf_object_t *cpu, generic_address_t address,
                byte_string_t opcode);
} cpu_cache_t;

typedef struct {
        generic_address_t start;
        generic_address_t end;
} interval_t;
typedef VECT(interval_t) interval_list_t;

enum { PHYSICAL, VIRTUAL, NUM_ADDRESS_TYPES };

typedef struct base_trace {
        conf_object_t obj;

        /*
         * Scratch area used for communicating trace event data. This is
         * maintained on a per processor basis.
         */
        trace_entry_t current_entry;
        trace_entry_t last_entry;

        /* Trace to file if file_name is non-NULL. */
        char *file_name;
        FILE *file;
#if defined(HAVE_LIBZ)
        gzFile gz_file;
#endif

        /* Count the events of each type. */
        uint64 exec_count;
        uint64 data_count;
        uint64 exc_count;

        /* 0 for text, 1 for raw */
        int trace_format;

        conf_object_t *consumer;
        trace_consume_interface_t consume_iface;

        /* Cached processor info. */
        cpu_cache_t *cpu;

        /* "Processor info" for non-cpu devices. */
        cpu_cache_t device_cpu;

        /* True if we are currently hooked into the memory hierarchy, false
           otherwise. */
        int memhier_hook;

        int trace_enabled;

        int trace_exceptions;
        int trace_instructions;
        int trace_data;
        int filter_duplicates;

        int print_physical_address;
        int print_virtual_address;
        int print_linear_address;
        int print_access_type;
        int print_memory_type;
        int print_data;

        /* Lists of intervals to trace data accesses for. _stc_ are the same
           lists, butrounded outwards to DSTC block size. */
        interval_list_t data_interval[NUM_ADDRESS_TYPES];
        interval_list_t data_stc_interval[NUM_ADDRESS_TYPES];

        cycles_t last_timestamp;

        /*
         * Function pointer to the trace consumer (if there is one). In future
         * we may want to support multiple trace consumers, especially if we do
         * proper statistical sampling of various characteristics, in which
         * case to reduce unwanted covariance we want separate sampling, with
         * this facility capable of handling overlaps.
         */
        void (*trace_consume)(struct base_trace *bt, trace_entry_t *);

#if defined(TRACE_STATS)
        uint64 instruction_records;
        uint64 data_records;
        uint64 other_records;
#endif   /* TRACE_STATS */

		char fullTraceFileName[200];
		char fTraceSymbolFileName[200];
		char fStatsFileBaseName[200];

		uint64 traceaddress;
		char displayLineNumbers;

		char threadSkip[200];

} base_trace_t;


typedef struct trace_mem_hier_object {
        conf_object_t obj;
        base_trace_t *bt;

        /* For forwarding requests. */
        conf_object_t *timing_model;
        timing_model_interface_t timing_iface;
        conf_object_t *snoop_device;
        timing_model_interface_t snoop_iface;
} trace_mem_hier_object_t;

void TraceStop();
void TraceStart();
void TraceSuspend(base_trace_t *obj);


#endif /* COMPONENTS_H */

