/*
  trace.c

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

/*
  The trace facility will generate a sequence of function calls to a
  user-defined trace consumer. This is done for instruction fetches,
  data accesses, and exceptions. It is implemented by hooking in a
  memory-hierarchy into Simics and installing an hap handler that
  catches exceptions.

  The data passed is encoded in a trace_entry_t data structure and is
  on a "will execute" basis. The trace consumer is presented with a
  sequence of instructions that are about to execute, not that have
  executed. Thus, register contents are those that are in place prior
  to the instruction is executed. Various conditions will
  stop the instruction from executing, primarly exceptions -
  these generate new trace entries. Memory instructions that are
  executed will also generate memory access trace entries. Memory
  instructions that cause a fault will generate a memory access trace
  entry upon correct execution, typically after one or more fault
  handlers have executed.
*/

/*
  <add id="simics generic module short">
  <name>Trace</name><ndx>trace</ndx>
  <ndx>memory traces</ndx><ndx>instruction traces</ndx>
  This module provides an easy way of generating traces from Simics.
  Actions traced are executed instructions, memory accesses and,
  occurred exceptions. Traces will by default be printed as text to the
  terminal but can also be directed to a file in which case a binary
  format is available as well.

  The data presented is on a "will execute" basis. The trace will
  contain a sequence of instructions that are about to execute, not
  that have executed. Thus, register contents are those that are in
  place prior to the instruction is executed. Various conditions will
  stop the instruction from executing, primarly exceptions &mdash;
  these generate new trace entries. Memory instructions that are
  executed will generate memory access trace entries. Memory
  instructions that cause a fault will generate a memory access trace
  entry upon correct execution, typically after one or more fault
  handlers have executed.

  See the <i>trace</i> section of the <i>Commands</i> chapter in the
  <i>Simics Reference Manual</i> for the available trace
  commands. Also look at the documentation for the class base_trace
  that have some attributes that can be set to control what will be
  traced.

  </add> */

#include <simics/api.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>



#include "trace.h"
#include "traceFCalls.h"

extern int ignore_due_to_Exception;



static const char *read_or_write_str[] = { "Read ", "Write"};

const char *seg_regs[] = {"es", "cs", "ss", "ds", "fs", "gs", 0};

static void
external_tracer(base_trace_t *bt, trace_entry_t *ent)
{
        bt->consume_iface.consume(bt->consumer, ent);
}

static void
text_trace_data(base_trace_t *bt, trace_entry_t *ent, char *s)
{
        cpu_cache_t *cc;

        if (ent->cpu_no == -1)
                cc = &bt->device_cpu;
        else
                cc = &bt->cpu[ent->cpu_no];
        bt->data_count++;
        /* vtsprintf works like sprintf but allows the "ll" size modifier
           to be used for long long (64-bit integers) even on Windows,
           which isn't possible with the sprintf in Microsoft's C library. */
        s += vtsprintf(s, "data: [%9llu] %s", bt->data_count, cc->name);

        if (ent->arch == TA_x86 && bt->print_linear_address)
                s += vtsprintf(s, "<l:0x%0*llx> ", cc->va_digits, ent->la);
        if (bt->print_virtual_address) {
                if (ent->arch == TA_x86)
                        if (ent->linear_access) {
                                s += vtsprintf(s, "<linear access> ");
                        } else {
                                s += vtsprintf(s, "<%s:0x%0*llx> ",
                                               seg_regs[ent->seg_reg],
                                               cc->va_digits,
                                               (unsigned long long) ent->va);
                        }
                else
                        s += vtsprintf(s, "<v:0x%0*llx> ", cc->va_digits,
                                       ent->va);
        }
        if (bt->print_physical_address)
                s += vtsprintf(s, "<p:0x%0*llx> ", cc->pa_digits, ent->pa);

        if (ent->arch == TA_v9 && bt->print_access_type) {
                const char *str;
                switch (ent->access_type) {
                case V9_Access_Normal:        str = "Nrml" ; break;
                case V9_Access_Normal_FP:     str = "FP" ; break;
                case V9_Access_Double_FP:     str = "Lddf/Stdf" ; break;
                case V9_Access_Short_FP:      str = "ShortFP" ; break;
                case V9_Access_FSR:           str = "FSR" ; break;
                case V9_Access_Atomic:        str = "Atomic" ; break;
                case V9_Access_Atomic_Load:   str = "AtomicLD" ; break;
                case V9_Access_Prefetch:      str = "Prefech" ; break;
                case V9_Access_Partial_Store: str = "PStore" ; break;
                case V9_Access_Ldd_Std_1:     str = "Ldd/Std1" ; break;
                case V9_Access_Ldd_Std_2:     str = "Ldd/Std2" ; break;
                case V9_Access_Block:         str = "Block" ; break;
                case V9_Access_Internal1:     str = "Intrn1" ; break;
                default:                      str = "Unknwn" ; break;
                }
                s += vtsprintf(s, "%s ", str);
        }

        if (ent->arch == TA_x86 && bt->print_access_type) {
                const char *str = "*er*";
                switch (ent->access_type) {
                case X86_Other: str = "Othr"; break;
                case X86_Vanilla: str = "Vani"; break;
                case X86_Instruction: str = "Inst"; break;
                case X86_Clflush: str = "Clfl"; break;
                case X86_Fpu_Env: str = "Fenv"; break;
                case X86_Fpu_State: str = "Fste"; break;
                case X86_Idt: str = "Idt "; break;
                case X86_Gdt: str = "Gdt "; break;
                case X86_Ldt: str = "Ldt "; break;
                case X86_Task_Segment: str = "Tseg"; break;
                case X86_Task_Switch: str = "Tswi"; break;
                case X86_Far_Call_Parameter: str = "CPar"; break;
                case X86_Stack: str = "Stac"; break;
                case X86_Pml4: str = "Pml4"; break;
                case X86_Pdp: str = "Pdp "; break;
                case X86_Pd: str = "Pd  "; break;
                case X86_Pt: str = "Pt  "; break;
                case X86_Sse: str = "Sse "; break;
                case X86_Fpu: str = "Fpu "; break;
                case X86_Access_Simple: str = "AccS"; break;
                case X86_Microcode_Update: str = "UCP "; break;
                case X86_Non_Temporal: str = "NTmp"; break;
                case X86_Prefetch_3DNow: str = "Pr3 "; break;
                case X86_Prefetchw_3DNow: str = "Prw3"; break;
                case X86_Prefetch_T0: str = "PrT0"; break;
                case X86_Prefetch_T1: str = "PrT1"; break;
                case X86_Prefetch_T2: str = "PrT2"; break;
                case X86_Prefetch_NTA: str = "PrNT"; break;
                case X86_Loadall: str = "Lall"; break;
                case X86_Atomic_Info: str = "AInf"; break;
                case X86_Cmpxchg16b: str = "X16B"; break;
                case X86_Smm_State: str = "SMM "; break;
                }
                s += vtsprintf(s, "%s ", str);

                if (ent->atomic)
                        s += vtsprintf(s, "atomic ");
        }
        if (ent->arch == TA_x86 && bt->print_memory_type) {
                const char *str = "er";
                switch (ent->memory_type) {
                case X86_None:
                        break;
                case X86_Strong_Uncacheable:    /* UC */
                case X86_Uncacheable:           /* UC- */
                        str = "UC";
                        break;
                case X86_Write_Combining:
                        str = "WC";
                        break;
                case X86_Write_Through:
                        str = "WT";
                        break;
                case X86_Write_Back:
                        str = "WB";
                        break;
                case X86_Write_Protected:
                        str = "WP";
                        break;
                }
                s += vtsprintf(s, "%s ", str);
        }

        s += vtsprintf(s, "%s %2d bytes  ",
                       read_or_write_str[ent->read_or_write], ent->size);
        if (bt->print_data)
                s += vtsprintf(s, "0x%llx", ent->value.data);
        strcpy(s, "\n");
}

static void
text_trace_exception(base_trace_t *bt, trace_entry_t *ent, char *s)
{
        cpu_cache_t *cc;

        if (ent->cpu_no == -1)
                cc = &bt->device_cpu;
        else
                cc = &bt->cpu[ent->cpu_no];
        bt->exc_count++;
        s += vtsprintf(s, "exce: [%9llu] %s", bt->exc_count, cc->name);
        s += vtsprintf(s, "exception %3d (%s)\n",
                       ent->value.exception,
                       SIM_get_exception_name(cc->cpu, ent->value.exception));
        SIM_clear_exception();
}

#define EXEC_VA_PREFIX(ent) ((ent)->arch == TA_x86 ? "cs" : "v")

static void
text_trace_instruction(base_trace_t *bt, trace_entry_t *ent, char *s)
{
        tuple_int_string_t ret;
        byte_string_t opcode;
        cpu_cache_t *cc;

        /* CPU number, addresses. */
        cc = &bt->cpu[ent->cpu_no];
        bt->exec_count++;
        s += vtsprintf(s, "inst: [%9llu] %s", bt->exec_count, cc->name);
        if (ent->arch == TA_x86 && bt->print_linear_address)
                s += vtsprintf(s, "<l:0x%0*llx> ", cc->va_digits, ent->la);
        if (bt->print_virtual_address)
                s += vtsprintf(s, "<%s:0x%0*llx> ",
                               EXEC_VA_PREFIX(ent), cc->va_digits, ent->va);
        if (bt->print_physical_address)
                s += vtsprintf(s, "<p:0x%0*llx> ", cc->pa_digits, ent->pa);

        /* Opcode. */
        if (bt->print_data) {
                int i;
                if (ent->arch == TA_x86) {
                        /* x86 has variable length opcodes. Pad with spaces to
                           6 bytes to keep the columns aligned for all but the
                           longest opcodes. */
                        for (i = 0; i < ent->size; i++)
                                s += vtsprintf(s, "%02x ",
                                               (uint32) ent->value.text[i]);
                        for (i = ent->size; i < 6; i++)
                                s += vtsprintf(s, "   ");
                } else {
                        for (i = 0; i < ent->size; i++)
                                s += vtsprintf(s, "%02x",
					     (uint32) ent->value.text[i]);
                        s += vtsprintf(s, " ");
                }
        }

        /* Disassembly. */
        opcode.str = ent->value.text;
        opcode.len = ent->size;
        ret = cc->disassemble_buf(cc->cpu, ent->va, opcode);
        s += vtsprintf(s, "%s", ret.string);

        strcpy(s, "\n");


}

static void
text_tracer(base_trace_t *bt, trace_entry_t *ent)
{
	/* With all options and longest opcodes, the lenght is around 200 */
        char s[8192];

        switch (ent->trace_type) {
        case TR_Data:
                //text_trace_data(bt, ent, s);
				s[0] = 0;
				container_MemoryCall(ent->read_or_write,ent->va,8);
                break;
        case TR_Exception:
                text_trace_exception(bt, ent, s);
                break;
        case TR_Instruction:
                //text_trace_instruction(bt, ent, s);
				s[0] = 0;
				container_traceFunctioncall(ent->va,0,bt->displayLineNumbers);
                break;
        case TR_Reserved:
        default:
                strcpy(s, "*** Trace: unknown trace event type.\n");
                break;
        }

	/* Catch errors that produce unreasonable long lines */
	ASSERT(strlen(s) < 500);

        if (bt->file != NULL) {
                fputs(s, bt->file);
        } else if (GZ_FILE(bt) != NULL) {
                GZ(gzwrite(GZ_FILE(bt), s, strlen(s)));
        } else {
                SIM_write(s, strlen(s));
        }
}

static void
raw_tracer(base_trace_t *bt, trace_entry_t *ent)
{
        if (bt->file != NULL) {
                fwrite(ent, sizeof(trace_entry_t), 1, bt->file);
        } else if (GZ_FILE(bt) != NULL) {
                GZ(gzwrite(GZ_FILE(bt), ent, sizeof(trace_entry_t)));
        }
}

#if defined(TRACE_STATS)
static void
stats_tracer(base_trace_t *bt, trace_entry_t *ent)
{
        if (ent->trace_type == TR_Data)
                bt->data_records++;
        else if (ent->trace_type == TR_Instruction)
                bt->instruction_records++;
        else
                bt->other_records++;
}
#endif   /* TRACE_STATS */


/*****************************************************************/

/*
 * Add your own tracers here.
 */

/*****************************************************************/


/* Return true if the memory operation intersects the data range intervals,
   false otherwise. */
static int
data_range_filter(interval_list_t *ivs, generic_transaction_t *mop)
{
        generic_address_t address[NUM_ADDRESS_TYPES];
        interval_t *iv;
        int i;
        int all_ivs_empty = 1;

        address[PHYSICAL] = mop->physical_address;
        address[VIRTUAL] = mop->logical_address;
        for (i = 0; i < NUM_ADDRESS_TYPES; i++) {
                if (VLEN(ivs[i]) == 0)
                        continue;
                all_ivs_empty = 0; /* we have just seen a nonempty interval
                                      list */
                VFOREACH(ivs[i], iv) {
                        if (address[i] + mop->size - 1 >= iv->start
                            && address[i] <= iv->end) {
                                /* Access intersects data range interval: it
                                   passes the filter. */
                                return 1;
                        }
                }
        }

        /* We get here only by not intersecting any data range interval. This
           is a success if and only if there were no intervals. */
        return all_ivs_empty;
}

static cycles_t
trace_mem_hier_operate(conf_object_t *obj, conf_object_t *space,
                       map_list_t *map, generic_transaction_t *mop)
{
        trace_mem_hier_object_t *tmho = (trace_mem_hier_object_t *)obj;
        base_trace_t *bt = tmho->bt;

        /* If the DSTC line(s) that the memop intersects pass the filter, we
           want to see future transactions. */
        if (data_range_filter(bt->data_stc_interval, mop))
                mop->block_STC = 1;

	/* forward operation on to underlying timing_model */
	return (tmho->timing_model == NULL
                ? 0
                : tmho->timing_iface.operate(tmho->timing_model, space, map, mop));
}

static int
is_duplicate(trace_entry_t *a, trace_entry_t *b)
{
        return (a->arch == b->arch
                && a->trace_type == b->trace_type
                && a->cpu_no == b->cpu_no
                && a->read_or_write == b->read_or_write
                && a->va == b->va
                && a->pa == b->pa);
}


static cycles_t
trace_snoop_operate(conf_object_t *obj, conf_object_t *space,
                    map_list_t *map, generic_transaction_t *mop)
{
        trace_mem_hier_object_t *tmho = (trace_mem_hier_object_t *)obj;
        base_trace_t *bt = tmho->bt;

        if (!SIM_mem_op_is_data(mop) || mop->type == Sim_Trans_Cache
            || !data_range_filter(bt->data_interval, mop))
                goto forward;

        bt->current_entry.trace_type = TR_Data;
        bt->current_entry.read_or_write =
                SIM_mem_op_is_write(mop) ? Sim_RW_Write : Sim_RW_Read;

        bt->current_entry.value.data = 0;
        bt->current_entry.cpu_no = -1;
        if (SIM_mem_op_is_from_cpu(mop)) {
                if (mop->size > 0 && mop->size <= sizeof(uint64))
                        bt->current_entry.value.data
                                = SIM_get_mem_op_value_cpu(mop);
                bt->current_entry.cpu_no = SIM_get_processor_number(mop->ini_ptr);
        }

        bt->current_entry.size          = mop->size;
        bt->current_entry.va            = mop->logical_address;
        bt->current_entry.pa            = mop->physical_address;
        bt->current_entry.atomic        = mop->atomic;

        if (mop->ini_type == Sim_Initiator_CPU_X86) {
                x86_memory_transaction_t *xtrans = (void *)mop;
                bt->current_entry.la            = xtrans->linear_address;
                bt->current_entry.linear_access = xtrans->access_linear;
                bt->current_entry.seg_reg       = xtrans->selector;
                bt->current_entry.access_type   = xtrans->access_type;
                bt->current_entry.memory_type   = xtrans->effective_type;
                bt->current_entry.arch          = TA_x86;
        } else if (mop->ini_type == Sim_Initiator_CPU_IA64) {
                bt->current_entry.arch = TA_ia64;
        } else if (SIM_mem_op_is_from_cpu_arch(mop, Sim_Initiator_CPU_V9)) {
                v9_memory_transaction_t *v9_trans = (void *)mop;
                bt->current_entry.arch = TA_v9;
                bt->current_entry.access_type   = v9_trans->access_type;
        } else {
                bt->current_entry.arch = TA_generic;
        }

        cycles_t t = SIM_cycle_count(mop->ini_ptr);
        bt->current_entry.timestamp = t - bt->last_timestamp;
        bt->last_timestamp = t;

        /* If instruction crosses a page call the trace consumer next time. */
        if (mop->page_cross == 1 && SIM_mem_op_is_instruction(mop))
                goto forward;

        /* Give the memory operation to the trace consumer. */
        if (!bt->filter_duplicates
            || !is_duplicate(&bt->last_entry, &bt->current_entry)) {
                bt->last_entry = bt->current_entry;
                bt->trace_consume(bt, &bt->current_entry);
        }

 forward:
        return (tmho->snoop_device == NULL
                ? 0
                : tmho->snoop_iface.operate(tmho->snoop_device, space, map, mop));
}


/* Determine the architecture implemented by the processor object. */
static trace_arch_t
trace_arch_from_cpu(conf_object_t *cpu)
{
        attr_value_t arch;

        arch = SIM_get_attribute(cpu, "architecture");

        /* x86 or x86-64. */
        if (strncmp(arch.u.string, "x86", 3) == 0)
                return TA_x86;

        /* ia64. */
        if (strcmp(arch.u.string, "ia64") == 0)
                return TA_ia64;

        /* SPARC V9 */
        if (strcmp(arch.u.string, "sparc-v9") == 0)
                return TA_v9;

        /* Any other architecture. */
        return TA_generic;
}


static void
trace_instr_operate(lang_void *data, conf_object_t *cpu,
                    linear_address_t la, logical_address_t va,
                    physical_address_t pa, byte_string_t opcode)
{
        base_trace_t *bt = (base_trace_t *) data;

        bt->current_entry.arch = trace_arch_from_cpu(cpu);
        bt->current_entry.trace_type = TR_Instruction;
        bt->current_entry.cpu_no = SIM_get_processor_number(cpu);
        bt->current_entry.size = opcode.len;
        bt->current_entry.read_or_write = Sim_RW_Read;

        if (bt->current_entry.arch == TA_x86) {
                bt->current_entry.linear_access = 0;
                bt->current_entry.seg_reg = 1; /* cs */
                bt->current_entry.la = la;
                bt->current_entry.memory_type = 0;
        }

        bt->current_entry.va = va;
        bt->current_entry.pa = pa;
        bt->current_entry.atomic = 0;
        bt->current_entry.access_type = 0;
        cycles_t t = SIM_cycle_count(cpu);
        bt->current_entry.timestamp = t - bt->last_timestamp;
        bt->last_timestamp = t;

        memcpy(bt->current_entry.value.text, opcode.str, opcode.len);

        bt->last_entry = bt->current_entry;
        bt->trace_consume(bt, &bt->current_entry);
}


/*************** base class *******************/

/* Call when bt->trace_format or bt->file_name have changed. */
static void
raw_mode_onoff_update(base_trace_t *bt)
{
        /* Make sure we are not writing raw output to the console. */
        if (bt->file_name == NULL)
                bt->trace_format = 0; /* text mode */

        /* Change trace consumer to match trace format. */
        if (bt->trace_format == 0)
                bt->trace_consume = text_tracer;
        else
                bt->trace_consume = raw_tracer;
}

/* Decide if this file name indicates that we should use gz compression (by
   looking for ".gz" suffix). */
static int
is_gz_filename(const char *fname)
{
        int len;

        len = strlen(fname);
        return len > 3 && fname[len - 3] == '.' && fname[len - 2] == 'g'
                && fname[len - 1] == 'z';
}

/* Open or close the output file, as appropriate. Call this whenever file_name
   or trace_enabled has changed. */
static set_error_t
file_onoff_update(base_trace_t *bt)
{
        /* Close regular file, if one is open. */
        if (bt->file != NULL)
                fclose(bt->file);
        bt->file = NULL;

#if defined(HAVE_LIBZ)
        /* Close gz file, if one is open. */
        if (GZ_FILE(bt) != NULL)
                gzclose(GZ_FILE(bt));
        GZ_FILE(bt) = NULL;
#endif

        /* Open file if a file name has been set and the tracer is enabled. */
        if (bt->trace_enabled && bt->file_name != NULL) {
                if (is_gz_filename(bt->file_name)) {
#if defined(HAVE_LIBZ)
                        GZ_FILE(bt) = gzopen(bt->file_name, "a");
#else
                        bt->file_name = NULL;
                        SIM_frontend_exception(SimExc_General,
                                               "gzip compression unavailable");
                        return Sim_Set_Illegal_Value;
#endif
                } else {
                        bt->file = os_fopen(bt->file_name, "a");
                }
                if (GZ_FILE(bt) == NULL && bt->file == NULL) {
                        bt->file_name = NULL;
                        SIM_frontend_exception(SimExc_General,
                                               "Cannot open file");
                        return Sim_Set_Illegal_Value;
                }
        }

        /* Raw output to console is not allowed. */
        raw_mode_onoff_update(bt);

        return Sim_Set_Ok;
}

/* Register or unregister the execution trace callback for all processors,
   depending on the values of trace_enabled and trace_instructions. */
static void
instruction_trace_onoff_update(base_trace_t *bt)
{
        conf_object_t *cpu;
        int i;

        for (i = SIM_number_processors() - 1; i >= 0; i--) {
                cpu = SIM_get_processor(i);
                if (bt->trace_enabled && bt->trace_instructions)
                        VT_register_exec_tracer(cpu, trace_instr_operate, bt);
                else
                        VT_unregister_exec_tracer(cpu, trace_instr_operate, bt);
        }
}

/* Called for each exception, just prior to servicing them. */
static void
catch_exception_hook(lang_void *data, conf_object_t *cpu, integer_t exc)
{
        base_trace_t *bt = (base_trace_t *) data;

        bt->current_entry.trace_type = TR_Exception;
        bt->current_entry.value.exception = exc;
        bt->current_entry.cpu_no = SIM_get_processor_number(cpu);
        cycles_t t = SIM_cycle_count(cpu);
        bt->current_entry.timestamp = t - bt->last_timestamp;
        bt->last_timestamp = t;

        /* Call the trace handler. */
        bt->last_entry = bt->current_entry;
        bt->trace_consume(bt, &bt->current_entry);
}

/* Called at Simics exit. */
static void
at_exit_hook(lang_void *data)
{
        base_trace_t *bt = (base_trace_t *) data;

        /* Close any open files. */
        bt->file_name = NULL;
        file_onoff_update(bt);
}

/* Register or unregister the exception callback, depending on the values of
   trace_enabled and trace_exceptions. */
static void
exception_trace_onoff_update(base_trace_t *bt)
{
        obj_hap_func_t f = (obj_hap_func_t) catch_exception_hook;

        if (bt->trace_enabled && bt->trace_exceptions)
                SIM_hap_add_callback("Core_Exception", f, bt);
        else
                SIM_hap_delete_callback("Core_Exception", f, bt);
}

/* Data structure for get_all_mem_spaces function */
struct memspace_list {
	conf_object_t *space;
	conf_object_t *cpu;
	struct memspace_list *next;
};

static void
memspace_list_add(struct memspace_list **headp, conf_object_t *space,
		  conf_object_t *cpu)
{
	struct memspace_list *p;

	/* check if already in list */
	for (p = *headp; p; p = p->next)
		if (p->space == space) return;

	p = MM_ZALLOC(1, struct memspace_list);
	p->next = *headp;
	p->space = space;
	p->cpu = cpu;
	*headp = p;
}

/* Find all memory-spaces connected to any cpu. Returns a linked list. */
static struct memspace_list *
find_memspaces(void)
{
        conf_object_t *cpu;
	struct memspace_list *spaces = NULL;

        for (cpu = SIM_next_queue(0); cpu != NULL; cpu = SIM_next_queue(cpu)) {
                attr_value_t phys_attr, phys_io;

                phys_attr = SIM_get_attribute(cpu, "physical_memory");
                SIM_clear_exception();

                phys_io = SIM_get_attribute(cpu, "physical_io");
                SIM_clear_exception();

                /* Clocks does not have a physical_memory object. */
                if (phys_attr.kind == Sim_Val_Object)
                        memspace_list_add(&spaces, phys_attr.u.object, cpu);

                /* SPARC has physical-io. */
                if (phys_io.kind == Sim_Val_Object)
			memspace_list_add(&spaces, phys_io.u.object, cpu);
        }
	return spaces;
}

/* Logical xor. True if exactly one of a and b are true, otherwise false. */
#define LXOR(a, b) ((!!(a) ^ !!(b)) != 0)

/* Register or unregister memory transaction snoopers, depending on the values
   of trace_enabled and trace_data. Raise a frontend exception and return a
   suitable error code in case of failure. */
static set_error_t
data_trace_onoff_update(base_trace_t *bt)
{
        struct memspace_list *spaces, *iter;
        attr_value_t attr;
        trace_mem_hier_object_t *tmho;

        /* Assume no error until proven otherwise. */
        const char *err = NULL;
        set_error_t ret = Sim_Set_Ok;

        if (!LXOR(bt->memhier_hook, bt->trace_enabled && bt->trace_data)) {
                /* This means that we are already hooked into the memory
                   hierarchy if and only if we need to be. So, nothing more to
                   do. */
                return ret;
        }

        /* Find all memory-spaces connected to any cpu. */
	spaces = find_memspaces();

        if (!bt->memhier_hook) {
                /* We are not hooked into the memory hierarchy, but we need to
                   be. */

                char buf[128];
                conf_class_t *trace_class;
                int i = 0;

                /* Invalidate any cached memory translations, so that we see
                   all memory operations. */
                SIM_flush_all_caches();

                trace_class = SIM_get_class("trace-mem-hier");

                /* Create mem hiers for every memory-space we will trace on. */
                for (iter = spaces; iter; iter = iter->next) {
                        conf_object_t *space = iter->space;
                        attr_value_t prev_tm, prev_sd;

                        /* Reuse existing object if there is one. */
                        vtsprintf(buf, "trace-mem-hier-%d", i++);
                        tmho = (trace_mem_hier_object_t *) SIM_get_object(buf);
                        if (tmho == NULL) {
                                SIM_clear_exception();
                                /* No, we'll have to create it here. */
                                tmho = (trace_mem_hier_object_t *)
                                        SIM_new_object(trace_class, buf);
                                if (tmho == NULL) {
                                        err = "Cannot create trace object";
                                        ret = Sim_Set_Illegal_Value;
                                        goto finish;
                                }
                        }
                        tmho->bt = bt;
                        tmho->obj.queue = iter->cpu;

			/* Set up forwarding. */
                        tmho->timing_model = NULL;
                        tmho->snoop_device = NULL;
                        memset(&tmho->timing_iface, 0,
                               sizeof(tmho->timing_iface));
                        memset(&tmho->snoop_iface, 0,
                               sizeof(tmho->snoop_iface));
                        prev_tm = SIM_get_attribute(space, "timing_model");
                        if (prev_tm.kind == Sim_Val_Object) {
                                timing_model_interface_t *timing_iface;
                                timing_iface = SIM_get_interface(
                                        prev_tm.u.object,
                                        TIMING_MODEL_INTERFACE);
                                if (timing_iface != NULL) {
                                        tmho->timing_model = prev_tm.u.object;
                                        tmho->timing_iface = *timing_iface;
                                }
                        }
                        prev_sd = SIM_get_attribute(space, "snoop_device");
                        if (prev_sd.kind == Sim_Val_Object) {
                                timing_model_interface_t *snoop_iface;
                                snoop_iface = SIM_get_interface(
                                        prev_sd.u.object,
                                        SNOOP_MEMORY_INTERFACE);
                                if (snoop_iface != NULL) {
                                        tmho->snoop_device = prev_sd.u.object;
                                        tmho->snoop_iface = *snoop_iface;
                                }
                        }

                        /* May have exceptions after
                           SIM_get_[attribute|interface]. We don't care. */
                        SIM_clear_exception();

                        /* Plug in new memory hierarchy. */
                        attr = SIM_make_attr_object(&tmho->obj);
                        SIM_set_attribute(space, "timing_model", &attr);
                        SIM_set_attribute(space, "snoop_device", &attr);
                }
        } else {
                /* We are hooked into the memory hierarchy, but we don't need
                   to be. So splice out the trace_mem_hier_object_t from
                   phys_mem.timing_model and phys_mem.snoop_device. */
                for (iter = spaces; iter; iter = iter->next) {
                        conf_object_t *space = iter->space;

                        attr = SIM_get_attribute(space, "timing_model");
                        tmho = (trace_mem_hier_object_t *) attr.u.object;
                        attr = SIM_make_attr_object(tmho->timing_model);
			SIM_set_attribute(space, "timing_model", &attr);
                        attr = SIM_make_attr_object(tmho->snoop_device);
			SIM_set_attribute(space, "snoop_device", &attr);
                }
        }

        /* If we get here, we changed state. */
        bt->memhier_hook = !bt->memhier_hook;

  finish:
        if (err != NULL)
                SIM_frontend_exception(SimExc_General, err);

	/* free the linked list */
	for (iter = spaces; iter; ) {
		struct memspace_list *next = iter->next;
		MM_FREE(iter);
		iter = next;
	}
        return ret;
}



static set_error_t
set_raw(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;

        bt->trace_format = !!val->u.integer;
        raw_mode_onoff_update(bt);
        if (bt->trace_format != !!val->u.integer) {
                /* Change to raw mode was vetoed. */
                SIM_frontend_exception(SimExc_General,
                                       "Raw output must be written to a file");
                return Sim_Set_Illegal_Value;
        }
        return Sim_Set_Ok;
}


static attr_value_t
get_raw(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->trace_format);
}


static set_error_t
set_consumer(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        trace_consume_interface_t *consume_iface;

        if (val->kind == Sim_Val_Nil) {
                bt->consumer = NULL;
                if (bt->trace_format == 0)
                        bt->trace_consume = text_tracer;
                else
                        bt->trace_consume = raw_tracer;
                return Sim_Set_Ok;
        }

        consume_iface = SIM_get_interface(val->u.object, TRACE_CONSUME_INTERFACE);
	if (!consume_iface)
                return Sim_Set_Interface_Not_Found;

        bt->consume_iface = *consume_iface;
        bt->consumer = val->u.object;
        bt->trace_consume = external_tracer;
        return Sim_Set_Ok;
}


static attr_value_t
get_consumer(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_object(bt->consumer);
}


#if defined(TRACE_STATS)
static set_error_t
set_instruction_records(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->instruction_records = val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_instruction_records(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->instruction_records);
}


static set_error_t
set_data_records(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->data_records = val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_data_records(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->data_records);
}


static set_error_t
set_other_records(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->other_records = val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_other_records(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->other_records);
}
#endif   /* TRACE_STATS */

static set_error_t
set_file(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *) obj;
        set_error_t ret;

        if (bt->file_name != NULL)
                MM_FREE(bt->file_name);

        if (val->kind == Sim_Val_String)
                bt->file_name = MM_STRDUP(val->u.string);
        else
                bt->file_name = NULL;

        /* Try to open/close file. */
        ret = file_onoff_update(bt);
        if (ret != Sim_Set_Ok) {
                /* Failed, set to no file. */
                if (bt->file_name != NULL)
                        MM_FREE(bt->file_name);
                bt->file_name = NULL;
        }

        return ret;
}

static attr_value_t
get_file(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_string(bt->file_name);
}


static set_error_t
set_enabled(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        set_error_t ret;

        if (!!bt->trace_enabled == !!val->u.integer)
                return Sim_Set_Ok; /* value did not change */

		//if(!val->u.integer)
		//	container_close();

        /* Change the enabled state and try to start or stop the data
           tracing. */
        bt->trace_enabled = !!val->u.integer;
        ret = data_trace_onoff_update(bt);
        if (ret == Sim_Set_Ok) {
                /* Success, start or stop the other tracers as well. */
                instruction_trace_onoff_update(bt);
                exception_trace_onoff_update(bt);
                file_onoff_update(bt);
				ThreadInitializeOnStart();
        } else {
                /* Failure, revert the change. */
                bt->trace_enabled = !bt->trace_enabled;
        }



        return ret;
}


static attr_value_t
get_enabled(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->trace_enabled);
}

static set_error_t
set_trace_instructions(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *) obj;

        /* Update if state changed. */
        if (!!bt->trace_instructions != !!val->u.integer) {
                bt->trace_instructions = !!val->u.integer;
                instruction_trace_onoff_update(bt);
        }
        return Sim_Set_Ok;
}


static attr_value_t
get_trace_instructions(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->trace_instructions);
}


static set_error_t
set_trace_data(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *) obj;

        /* Update if state changed. */
        if (!!bt->trace_data != !!val->u.integer) {
                bt->trace_data = !!val->u.integer;
                return data_trace_onoff_update(bt);
        } else {
                return Sim_Set_Ok;
        }
}


static attr_value_t
get_trace_data(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->trace_data);
}


static set_error_t
set_trace_exceptions(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *) obj;

        /* Update if state changed. */
        if (!!bt->trace_exceptions != !!val->u.integer) {
                bt->trace_exceptions = !!val->u.integer;
                exception_trace_onoff_update(bt);
        }
        return Sim_Set_Ok;
}


static attr_value_t
get_trace_exceptions(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->trace_exceptions);
}


static set_error_t
set_filter_duplicates(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->filter_duplicates = !!val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_filter_duplicates(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->filter_duplicates);
}


static set_error_t
set_print_virtual_address(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->print_virtual_address = !!val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_print_virtual_address(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->print_virtual_address);
}


static set_error_t
set_print_physical_address(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->print_physical_address = !!val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_print_physical_address(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->print_physical_address);
}


static set_error_t
set_print_linear_address(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->print_linear_address = !!val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_print_linear_address(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->print_linear_address);
}


static set_error_t
set_print_access_type(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->print_access_type = !!val->u.integer;
        return Sim_Set_Ok;
}

static attr_value_t
get_print_access_type(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->print_access_type);
}


static set_error_t
set_print_memory_type(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->print_memory_type = !!val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_print_memory_type(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->print_memory_type);
}


static set_error_t
set_print_data(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->print_data = !!val->u.integer;
        return Sim_Set_Ok;
}


static attr_value_t
get_print_data(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        return SIM_make_attr_integer(bt->print_data);
}


static attr_value_t
get_base_trace(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        trace_mem_hier_object_t *tmho = (trace_mem_hier_object_t *)obj;
        return SIM_make_attr_object(&tmho->bt->obj);
}


static attr_value_t
get_timing_model(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        trace_mem_hier_object_t *tmho = (trace_mem_hier_object_t *)obj;
        return SIM_make_attr_object(tmho->timing_model);
}


static attr_value_t
get_snoop_device(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        trace_mem_hier_object_t *tmho = (trace_mem_hier_object_t *)obj;
        return SIM_make_attr_object(tmho->snoop_device);
}


/* Create a new interval_t. Round outwards to the specified number of bits. */
static interval_t
create_interval(generic_address_t start, generic_address_t end, int round)
{
        interval_t iv;
        generic_address_t rmask = ~(generic_address_t)((1 << round) - 1);

        iv.start = MIN(start, end) & rmask;
        iv.end = (MAX(start, end) & rmask) + (1 << round) - 1;
        return iv;
}

static set_error_t
set_data_intervals(void *arg, conf_object_t *obj,
                   attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        int address_type = (uintptr_t)arg;
        int i, stc_block;

        /* Find largest DSTC block size. */
        stc_block = 0;
        for (i = 0; i < SIM_number_processors(); i++)
                stc_block = MAX(
                        stc_block,
                        SIM_get_attribute(
                                SIM_get_processor(i),
                                "memory_profiling_granularity_log2")
                        .u.integer);

        VCLEAR(bt->data_interval[address_type]);
        VCLEAR(bt->data_stc_interval[address_type]);
        for (i = 0; i < val->u.list.size; i++) {
                attr_value_t *as = val->u.list.vector[i].u.list.vector;
                VADD(bt->data_interval[address_type], create_interval(
                             as[0].u.integer, as[1].u.integer, 0));
                VADD(bt->data_stc_interval[address_type], create_interval(
                             as[0].u.integer, as[1].u.integer, stc_block));
        }

        SIM_flush_all_caches();
        return Sim_Set_Ok;
}

static attr_value_t
get_data_intervals(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        int address_type = (uintptr_t)arg;
        attr_value_t ret;

        ret = SIM_alloc_attr_list(VLEN(bt->data_interval[address_type]));
        VFORI(bt->data_interval[address_type], i) {
                interval_t *iv = &VGET(bt->data_interval[address_type], i);
                ret.u.list.vector[i] = SIM_make_attr_list(
                        2, SIM_make_attr_integer(iv->start),
                        SIM_make_attr_integer(iv->end));
        }
        return ret;
}



static set_error_t
set_ftrace_file(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        strncpy(bt->fullTraceFileName, val->u.string,200);
		setFullTraceFile(bt->fullTraceFileName);
        return Sim_Set_Ok;
}


static attr_value_t
get_ftrace_filename(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        attr_value_t ret;

		ret = SIM_make_attr_string(bt->fullTraceFileName);
        return ret;
}

static set_error_t
set_threadSkip(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        strncpy(bt->threadSkip, val->u.string,200);
        return Sim_Set_Ok;
}


static attr_value_t
get_threadSkip(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        attr_value_t ret;

		ret = SIM_make_attr_string(bt->threadSkip);
        return ret;
}


static set_error_t
set_ftracesymbol_file(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        strncpy(bt->fTraceSymbolFileName, val->u.string,200);
		loadContainersFromSymtable(val->u.string);
        return Sim_Set_Ok;
}

static attr_value_t
get_ftracesymbol_filename(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        attr_value_t ret;

		ret = SIM_make_attr_string(bt->fTraceSymbolFileName);
        return ret;
}

static set_error_t
set_fstatsbasefilename(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        strncpy(bt->fStatsFileBaseName, val->u.string,200);
        return Sim_Set_Ok;
}


static attr_value_t
get_fstatsbasefilename(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        attr_value_t ret;

		ret = SIM_make_attr_string(bt->fStatsFileBaseName);
        return ret;
}




void bp_callback(lang_void *userdata,
	conf_object_t *obj,
	integer_t exception_number)
{

   // base_trace_t *bt = (base_trace_t *)obj;
	base_trace_t* bt = (base_trace_t*) userdata;
	//enable trace
	//printf("GICA bp_callback reached, enabling the trace 0x%llx.\n",bt->traceaddress);
	//printf("GICA DEBUG bp_callback %s.\n",obj->name);
	TraceStart();
	container_traceFunctioncall(bt->traceaddress,0, bt->displayLineNumbers);

}

int TraceIsRunning()
{
	attr_value_t enabled = SIM_get_attribute(SIM_get_object("trace0"),"enabled");
	return enabled.u.integer ;
}

void TraceStart()
{
	attr_value_t val = SIM_make_attr_integer(1);
	set_error_t result_val =
        SIM_set_attribute(
			    SIM_get_object("trace0"),
				"enabled",
				&val);

    if(result_val != Sim_Set_Ok) printf ("SIM_set_attribute was unsuccessfull trace0.enabled = %lld %d\n",val.u.integer,result_val);
}

void TraceStop()
{
	attr_value_t val = SIM_make_attr_integer(0);
	set_error_t result_val =
        SIM_set_attribute(
			    SIM_get_object("trace0"),
				"enabled",
				&val);

    if(result_val != Sim_Set_Ok) printf ("SIM_set_attribute was unsuccessfull trace0.enabled = %lld %d\n",val.u.integer,result_val);
}

static set_error_t
set_traceaddress(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        bt->traceaddress = val->u.integer;

		//printf("GICA set_traceaddress, before setting breakpoint %llx\n", bt->traceaddress);
		conf_object_t* ctx = SIM_get_object("gicacontext");
		if(ctx == NULL){
			printf("GICA : you need to define a context \"gicacontext\"\n");
		}
		else{
			//set breakpoint and callback here.
			breakpoint_id_t bkpt = SIM_breakpoint(
				SIM_get_object("gicacontext"),//memory object ?
				Sim_Break_Virtual,
				Sim_Access_Execute,
				bt->traceaddress,
				4,
				Sim_Breakpoint_Temporary); //the breakpoint is deleted once it is triggered for the first time

			if(bkpt != -1){
				//now register a callback when the breakpoint is hit
				SIM_hap_add_callback_index(
                "Core_Breakpoint_Memop",
                 (obj_hap_func_t)bp_callback,
                 bt, bkpt);
			}
			else
			{
				printf("GICA set_traceaddress, breakpoint is !!NOT!! set on address %llx\n",bt->traceaddress);
			}
		}




		//once the breakpoint is reached, it will enable tracing
        return Sim_Set_Ok;
}

static attr_value_t
get_traceaddress(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        attr_value_t ret;

		ret = SIM_make_attr_integer(bt->traceaddress);
        return ret;
}

static attr_value_t
get_displayLinenumbers(void *arg, conf_object_t *obj, attr_value_t *idx)
{
        base_trace_t *bt = (base_trace_t *)obj;
        attr_value_t ret;

		ret = SIM_make_attr_integer(bt->displayLineNumbers);
        return ret;
}

static set_error_t
set_displayLinenumbers(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
		base_trace_t *bt = (base_trace_t *)obj;
        bt->displayLineNumbers = val->u.integer;
        return Sim_Set_Ok;
}



/* Cache useful information about each processor. */
static void
cache_cpu_info(base_trace_t *bt)
{
        int num, i;
        attr_value_t r;
        processor_interface_t *ifc;

        /* Cache data for each processor. */
        num = SIM_number_processors();
        bt->cpu = MM_MALLOC(num, cpu_cache_t);
        for (i = 0; i < num; i++) {
                bt->cpu[i].cpu = SIM_get_processor(i);
                r = SIM_get_attribute(bt->cpu[i].cpu, "address-width");
                /* clocks does not have any address-width */
                if (r.kind == Sim_Val_List) {
                        bt->cpu[i].pa_digits = (r.u.list.vector[0].u.integer + 3) >> 2;
                        bt->cpu[i].va_digits = (r.u.list.vector[1].u.integer + 3) >> 2;
                }
                SIM_free_attribute(r);
                ifc = SIM_get_interface(bt->cpu[i].cpu, PROCESSOR_INTERFACE);
                bt->cpu[i].disassemble_buf = ifc->disassemble_buf;
                vtsprintf(bt->cpu[i].name, "CPU %2d ", i);
        }

        /* Invent reasonable values for non-cpu devices. */
        bt->device_cpu.va_digits = 16;
        bt->device_cpu.pa_digits = 16;
        strcpy(bt->device_cpu.name, "Device ");
}

static conf_object_t *
base_trace_new_instance(parse_object_t *pa)
{
        base_trace_t *bt = MM_ZALLOC(1, base_trace_t);
        obj_hap_func_t f = (obj_hap_func_t) at_exit_hook;
        int i;

        SIM_object_constructor(&bt->obj, pa);

        bt->trace_consume = text_tracer; /* text tracing is default */
        bt->trace_instructions = 1;
        bt->trace_data = 1;
        bt->trace_exceptions = 1;
        bt->print_virtual_address = 1;
        bt->print_physical_address = 1;
        bt->print_data = 1;
        bt->print_linear_address = 1;
        bt->print_access_type = 1;
        bt->print_memory_type = 1;
        for (i = 0; i < NUM_ADDRESS_TYPES; i++) {
                VINIT(bt->data_interval[i]);
                VINIT(bt->data_stc_interval[i]);
        }
		bt->displayLineNumbers = 0;

        cache_cpu_info(bt);

        SIM_hap_add_callback("Core_At_Exit", f, bt);

    	container_initialize(bt->fullTraceFileName);

        return &bt->obj;
}




static conf_object_t *
trace_new_instance(parse_object_t *pa)
{
        trace_mem_hier_object_t *tmho = MM_ZALLOC(1, trace_mem_hier_object_t);
        SIM_object_constructor(&tmho->obj, pa);
        return &tmho->obj;
}

void ExceptionCallBack(lang_void *callback_data,
	conf_object_t *trigger_obj,
	integer_t exception_number){
	conf_object_t *proc;
	proc =	SIM_current_processor();

	//printf("HAP HAP : %lld  %s\n",exception_number,SIM_get_exception_name(proc,exception_number));
	if(!stack_empty(thread_active->container_runtime_stack)){

		stackObject t = stack_top(thread_active->container_runtime_stack);
		if(t.containerObj->entryAddress == SIM_get_program_counter(proc)){
			//stack_pop(returnAddressStack);
			ignore_due_to_Exception = 1;
			//printf("HAP AND CONTAINERS : %lld  %s\n",exception_number,SIM_get_exception_name(proc,exception_number));
		}
	}
}



void ThreadMonitor_callback_after(lang_void *userdata,
	conf_object_t *obj,
	integer_t exception_number,
	generic_transaction_t *memop)
{
	uint64 threadNameNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.name.name_u32");
	uint64 threadIdNew = mySimicsIntSymbolRead("_Per_CPU_Information.executing.Object.id");
	
	base_trace_t * bt = (base_trace_t *)SIM_get_object("trace0");
	//printf("threadskip %s\n",bt->threadSkip);
	char threadNameString[20];
	toStringRTEMSTaksName(threadNameString, threadNameNew);
	int suspend = 0;
	char * pch;
	char threadSkip[200];
	strcpy(threadSkip, bt->threadSkip);
	pch = strtok (threadSkip," ");
	while (pch != NULL)
	{
	    //printf ("tok %s\n",pch);
		if( strcmp(pch,"0")==0) {suspend = 1; break;}
		if(strcmp(pch,threadNameString)==0) {suspend = 1; break;}
	    pch = strtok (NULL, " ");
	}

	if(suspend) TraceStop();
	else
	{
		if(!TraceIsRunning()){
			//printf("re-enabling the trace\n");
			TraceStart();
		}
	}

	Thread_switch(threadIdNew,threadNameNew);
	printf("%s => After Thread_switch  %s\n",__PRETTY_FUNCTION__,threadNameString );fflush(stdin);
}


void ThreadMonitor_callback_before(lang_void *userdata,
	conf_object_t *obj,
	integer_t exception_number,
	generic_transaction_t *memop)
{

	uinteger_t memTranVal = SIM_get_mem_op_value_cpu(memop);
	attr_value_t evalThreadPtr = myeval("_Per_CPU_Information.executing");
	uint64 uievalThreadPtr = evalThreadPtr.u.list.vector[1].u.integer;
	
	if(memTranVal != 0 && uievalThreadPtr != 0){
		logical_address_t PC = SIM_get_program_counter(SIM_current_processor());
		breakpoint_id_t bkpt = SIM_breakpoint(
			SIM_get_object("gicacontext"),//memory object ?
			Sim_Break_Virtual,
			Sim_Access_Execute,
			PC+4,
			4,
			Sim_Breakpoint_Temporary);

		if(bkpt != -1){
				//now register a callback when the breakpoint is hit
				SIM_hap_add_callback_index(
                "Core_Breakpoint_Memop",
                 (obj_hap_func_t)ThreadMonitor_callback_after,
                 SIM_get_object("trace0"), bkpt);
			}
			else
			{
				printf("GICA ThreadMonitor_register, breakpoint is !!NOT!! set\n");
			}

		
		printf("%s => reached  %llx\n",__PRETTY_FUNCTION__,uievalThreadPtr );fflush(stdin);
	}

}




void ThreadMonitor_register()
{
	//attr_value_t evalThreadAddress = myeval("&_Per_CPU_Information.executing");
	//ASSERT(evalThreadAddress.kind == 4);
	uint64 breakAddress = mySimicsIntSymbolRead("&_Per_CPU_Information.executing");
	
	breakpoint_id_t bkpt = SIM_breakpoint(
			SIM_get_object("gicacontext"),//memory object ?
			Sim_Break_Virtual,
			Sim_Access_Write,
			//evalThreadAddress.u.list.vector[1].u.integer,
			breakAddress,
			4,
			0);
	if(bkpt != -1){
				//now register a callback when the breakpoint is hit
				SIM_hap_add_callback_index(
                "Core_Breakpoint_Memop",
                 (obj_hap_func_t)ThreadMonitor_callback_before,
                 SIM_get_object("trace0"), bkpt);
			}
			else
			{
				printf("GICA ThreadMonitor_register, breakpoint is !!NOT!! set\n");
			}

   printf("%s => Breakpoint set at address %llx\n",__PRETTY_FUNCTION__, breakAddress);fflush(stdin);

}

static attr_value_t
get_trace_print_stack(void *arg, conf_object_t *obj, attr_value_t *idx){
	printCurrentContainerStack();
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_print_stack(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	printCurrentContainerStack();
	return Sim_Set_Ok;
}

static attr_value_t
get_trace_print_threads(void *arg, conf_object_t *obj, attr_value_t *idx){
	printf("GICA get_trace_print_threads\n");
	printThreads();
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_print_threads(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	printf("GICA get_trace_print_threads\n");
	printThreads();
	return Sim_Set_Ok;
}

static attr_value_t
get_trace_containerstatistics(void *arg, conf_object_t *obj, attr_value_t *idx){
	printf("GICA get_trace_containerstatistics\n");
	container_printStatistics(0);
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_containerstatistics(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	printf("GICA set_trace_containerstatistics\n");
	container_printStatistics(val->u.integer);
	return Sim_Set_Ok;
}

static attr_value_t
get_trace_printAccessList(void *arg, conf_object_t *obj, attr_value_t *idx){
	printf("GICA get_trace_printAccessList\n");
	container_printMemoryRanges(0);
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_printAccessList(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	printf("GICA set_trace_printAccessList\n");
	container_printMemoryRanges(val->u.integer);
	return Sim_Set_Ok;
}


static attr_value_t
get_trace_printDecodedAccessList(void *arg, conf_object_t *obj, attr_value_t *idx){
	container_printDecodedMemoryRanges(0);
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_printDecodedAccessList(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	container_printDecodedMemoryRanges(0);
	return Sim_Set_Ok;
}

static attr_value_t
get_trace_printContainerStats(void *arg, conf_object_t *obj, attr_value_t *idx){
	printAllStatsFiles(((base_trace_t *)obj)->fStatsFileBaseName);
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_printContainerStats(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	printAllStatsFiles(((base_trace_t *)obj)->fStatsFileBaseName);
	return Sim_Set_Ok;
}



static attr_value_t
get_trace_flush(void *arg, conf_object_t *obj, attr_value_t *idx){
	containers_flush();
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_flush(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	containers_flush();
	return Sim_Set_Ok;
}

static attr_value_t
get_trace_TestRandomStuff(void *arg, conf_object_t *obj, attr_value_t *idx){
	return SIM_make_attr_integer(1);
}

static set_error_t
set_trace_TestRandomStuff(void *arg, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
	containers_testRandomStuff(val->u.integer);
	return Sim_Set_Ok;
}



void
init_local(void)
{
        class_data_t base_funcs;
        conf_class_t *base_class;
        class_data_t trace_funcs;
        conf_class_t *trace_class;
        timing_model_interface_t *timing_iface, *snoop_iface;

        /* first the base class */
        memset(&base_funcs, 0, sizeof(class_data_t));
        base_funcs.new_instance = base_trace_new_instance;
        base_funcs.description =
                "The base class for the trace mode. "
                " This module provides an easy way of generating traces from Simics. "
                "Actions traced are executed instructions, memory accesses and, "
                "occurred exceptions. Traces will by default be printed as text to the "
                "terminal but can also be directed to a file in which case a binary "
                "format is available as well. It is also possible to control what will "
                "be traced by setting a few of the provided attributes.";
	base_funcs.kind = Sim_Class_Kind_Session;

        base_class = SIM_register_class("base-trace-mem-hier", &base_funcs);

        SIM_register_typed_attribute(
                base_class, "file",
                get_file, NULL, set_file, NULL,
                Sim_Attr_Session, "s|n", NULL,
                "Name of output file that the trace is written to. If the name"
                " ends in <tt>.gz</tt>, the output will be gzipped.");

        SIM_register_typed_attribute(base_class, "raw-mode",
                                     get_raw, NULL,
                                     set_raw, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 for raw "
                                     "output format, and 0 for ascii. Raw output "
                                     "format is only supported when writing to "
                                     "a file.");

        SIM_register_typed_attribute(base_class, "consumer",
                                     get_consumer, NULL,
                                     set_consumer, NULL,
                                     Sim_Attr_Session,
                                     "o|n", NULL,
                                     "Optional consumer object. Must implement "
                                     TRACE_CONSUME_INTERFACE ".");

        SIM_register_typed_attribute(base_class, "enabled",
                                     get_enabled, NULL,
                                     set_enabled, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "tracing, 0 to disable.");

        SIM_register_typed_attribute(base_class, "trace_instructions",
                                     get_trace_instructions, NULL,
                                     set_trace_instructions, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "instruction tracing, 0 to disable.");

        SIM_register_typed_attribute(base_class, "trace_data",
                                     get_trace_data, NULL,
                                     set_trace_data, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "tracing of data, 0 to disable.");

        SIM_register_typed_attribute(base_class, "trace_exceptions",
                                     get_trace_exceptions, NULL,
                                     set_trace_exceptions, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "tracing of exceptions, 0 to disable.");

        SIM_register_typed_attribute(base_class, "filter_duplicates",
                                     get_filter_duplicates, NULL,
                                     set_filter_duplicates, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to filter "
                                     "out duplicate trace entries. Useful to filter "
                                     "out multiple steps in looping or repeating "
                                     "instructions.");

        SIM_register_typed_attribute(base_class, "print_virtual_address",
                                     get_print_virtual_address, NULL,
                                     set_print_virtual_address, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "printing of the virtual address, 0 to disable.");

        SIM_register_typed_attribute(base_class, "print_physical_address",
                                     get_print_physical_address, NULL,
                                     set_print_physical_address, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "printing of the physical address, 0 to disable.");

        SIM_register_typed_attribute(base_class, "print_linear_address",
                                     get_print_linear_address, NULL,
                                     set_print_linear_address, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "printing of the linear address, 0 to disable.");

        SIM_register_typed_attribute(base_class, "print_access_type",
                                     get_print_access_type, NULL,
                                     set_print_access_type, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "printing of the memory access type, 0 to disable.");

        SIM_register_typed_attribute(base_class, "print_memory_type",
                                     get_print_memory_type, NULL,
                                     set_print_memory_type, 0,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "printing of the linear address, 0 to disable.");

        SIM_register_typed_attribute(base_class, "print_data",
                                     get_print_data, NULL,
                                     set_print_data, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "<tt>1</tt>|<tt>0</tt> Set to 1 to enable "
                                     "printing of data and instruction op codes, "
                                     "0 to disable.");

        SIM_register_typed_attribute(
                base_class, "data_intervals_p",
                get_data_intervals, (void *)(uintptr_t)PHYSICAL,
                set_data_intervals, (void *)(uintptr_t)PHYSICAL,
                Sim_Attr_Session, "[[ii]*]", NULL,
                "List of physical address intervals for data tracing."
                " If no intervals are specified, all addresses are traced.");

        SIM_register_typed_attribute(
                base_class, "data_intervals_v",
                get_data_intervals, (void *)(uintptr_t)VIRTUAL,
                set_data_intervals, (void *)(uintptr_t)VIRTUAL,
                Sim_Attr_Session, "[[ii]*]", NULL,
                "List of virtual address intervals for data tracing."
                " If no intervals are specified, all addresses are traced.");

		SIM_register_typed_attribute(
                base_class, "set_ftrace_file",
                get_ftrace_filename, NULL,
                set_ftrace_file, NULL,
                Sim_Attr_Session, "s", NULL,
                "The file name for the trace dump "
                " If no name is specified, all output is sent to stdout.");

		SIM_register_typed_attribute(
                base_class, "set_ftracesymbol_file",
                get_ftracesymbol_filename, NULL,
                set_ftracesymbol_file, NULL,
                Sim_Attr_Session, "s", NULL,
                "The file name to load the program functions from "
                " This file can be produced by nm. Filter only functions : nm a.out | grep \" T \"");

		SIM_register_typed_attribute(
                base_class, "set_fstatsbasefilename",
                get_fstatsbasefilename, NULL,
                set_fstatsbasefilename, NULL,
                Sim_Attr_Session, "s", NULL,
                "The base name of all stats file that will be output from the simulation"
                " ex. set_fstatsbasefilename \"ouput/\"");

		SIM_register_typed_attribute(
                base_class, "traceaddress",
                get_traceaddress, NULL,
                set_traceaddress, NULL,
                Sim_Attr_Session, "i", NULL,
                "Name of symbol that the trace will monitor."
                " the trace will break on execute at this address ");

		SIM_register_typed_attribute(
                base_class, "displayLinenumbers",
                get_displayLinenumbers, NULL,
                set_displayLinenumbers, NULL,
                Sim_Attr_Session, "i", NULL,
                "If enabled it will display the symbol file and line number in the original source."
                " By default this option is disabled ");


		SIM_register_typed_attribute(
                base_class, "threadSkip",
                get_threadSkip, NULL,
                set_threadSkip, NULL,
                Sim_Attr_Session, "s", NULL,
                "List the name of the RTEMS threads to be skipped by the trace ");

		SIM_register_typed_attribute
			(base_class,"trace_print_stack",
			get_trace_print_stack,	NULL,
			set_trace_print_stack,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"does not set anything, it prints the trace");

		SIM_register_typed_attribute
			(base_class,"trace_print_threads",
			get_trace_print_threads,	NULL,
			set_trace_print_threads,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"does not set anything, it prints the thread and some info about them");


		SIM_register_typed_attribute
			(base_class,"trace_print_containerstatistics",
			get_trace_containerstatistics,	NULL,
			set_trace_containerstatistics,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"does not set anything, it prints container statistics");

		SIM_register_typed_attribute
			(base_class,"trace_print_printAccessList",
			get_trace_printAccessList,	NULL,
			set_trace_printAccessList,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"does not set anything, it prints container memory ranges accessed");

		SIM_register_typed_attribute
			(base_class,"trace_print_printDecodedAccessList",
			get_trace_printDecodedAccessList,	NULL,
			set_trace_printDecodedAccessList,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"does not set anything, it prints container decoded memory ranges accessed");

		SIM_register_typed_attribute
			(base_class,"trace_print_printContainerStats",
			get_trace_printContainerStats,	NULL,
			set_trace_printContainerStats,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"does not set anything, it prints container stats");
			
		
		SIM_register_typed_attribute
			(base_class,"trace_flush",
			get_trace_flush,	NULL,
			set_trace_flush,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"does not set anything, it flushes buffers ");

		SIM_register_typed_attribute
			(base_class,"trace_TestRandomStuff",
			get_trace_TestRandomStuff,	NULL,
			set_trace_TestRandomStuff,	NULL,
			Sim_Attr_Optional,"i",NULL,
			"testing different things");


#if defined(TRACE_STATS)
        SIM_register_typed_attribute(base_class, "instruction_records",
                                     get_instruction_records, NULL,
                                     set_instruction_records, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "Instruction records");

        SIM_register_typed_attribute(base_class, "data_records",
                                     get_data_records, NULL,
                                     set_data_records, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "Data records");

        SIM_register_typed_attribute(base_class, "other_records",
                                     get_other_records, NULL,
                                     set_other_records, NULL,
                                     Sim_Attr_Session,
                                     "i", NULL,
                                     "Other records");
#endif   /* TRACE_STATS */

        /* Register new trace class */
        memset(&trace_funcs, 0, sizeof(class_data_t));
        trace_funcs.new_instance = trace_new_instance;
        trace_funcs.description =
                "This class is defined in the trace module. It is "
                "used by the tracer to listen to memory traffic on "
                "each CPU.";

        trace_class = SIM_register_class("trace-mem-hier", &trace_funcs);

        timing_iface = MM_ZALLOC(1, timing_model_interface_t);
        timing_iface->operate = trace_mem_hier_operate;
        SIM_register_interface(trace_class, "timing-model", timing_iface);

        snoop_iface = MM_ZALLOC(1, timing_model_interface_t);
        snoop_iface->operate = trace_snoop_operate;
        SIM_register_interface(trace_class, SNOOP_MEMORY_INTERFACE, snoop_iface);

        SIM_register_typed_attribute(trace_class, "base_trace_obj",
                                     get_base_trace, NULL,
                                     NULL, NULL,
                                     Sim_Attr_Session,
                                     "o", NULL,
                                     "Base-trace object (read-only)");

        SIM_register_typed_attribute(trace_class, "timing_model",
                                     get_timing_model, NULL,
                                     NULL, NULL,
                                     Sim_Attr_Session,
                                     "o|n", NULL,
                                     "Timing model (read-only)");

        SIM_register_typed_attribute(trace_class, "snoop_device",
                                     get_snoop_device, NULL,
                                     NULL, NULL,
                                     Sim_Attr_Session,
                                     "o|n", NULL,
                                     "Snoop device (read-only)");



		printf("Trace: init_local\n");



		SIM_hap_add_callback("Core_Exception",
							 (obj_hap_func_t)ExceptionCallBack,
							 NULL);

		ThreadMonitor_register();



}

