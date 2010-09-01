import time, sim_commands, string, sys, getpass, os
import workloads
import copy_on_demand
from cli import *

### Code for running Ruby/Opal

def load_modules_and_run(fast_forward = 0):
  print "Python version: %s" % sys.version
  ###### Read simulation parameters
  env_dict = workloads.prepare_env_dictionary(simics = 1)
  workloads.check_requirements(env_dict)
  workloads.print_all_variables(env_dict)
  protocol = workloads.get_var(env_dict, "PROTOCOL")
  workload_name = workloads.get_var(env_dict, "WORKLOAD")
  checkpoint_dir = workloads.get_var(env_dict, "CHECKPOINT_DIR")
  checkpoint = workloads.get_var(env_dict, "CHECKPOINT")
  chips = int(workloads.get_var(env_dict, "CHIPS"))
  processors_per_chip = int(workloads.get_var(env_dict, "PROCS_PER_CHIP"))
  smt_threads = int(workloads.get_var(env_dict, "SMT_THREADS"))
  l2_banks = int(workloads.get_var(env_dict, "NUM_L2_BANKS"))
  bandwidth = int(workloads.get_var(env_dict, "BANDWIDTH"))
  results_dir = workloads.get_var(env_dict, "RESULTS_DIR")
  transactions = int(workloads.get_var(env_dict, "TRANSACTIONS"))
  dump_interval = int(workloads.get_var(env_dict, "DUMP_INTERVAL"))
  condor_cluster = int(workloads.get_var(env_dict, "CONDORCLUSTER"))
  condor_process = int(workloads.get_var(env_dict, "CONDORPROCESS"))
  checkpoint_at_end = workloads.get_var(env_dict, "CHECKPOINT_AT_END")
  generate_request_trace = workloads.get_var(env_dict, "GENERATE_TRACE")
  generate_cache_data_dump = workloads.get_var(env_dict, "CACHE_DATA_DUMP")
  protocol_option = workloads.get_var(env_dict, "PROTOCOL_OPTION")
  network_topology = workloads.get_var(env_dict, "NETWORK_TOPOLOGY")
  warmup_file = workloads.get_var(env_dict, "WARMUP_FILE")
  opal_config_file = workloads.get_var(env_dict, "OPAL_CONFIG_FILE")
  opal_config_name = workloads.get_var(env_dict, "OPAL_CONFIG_NAME")
  interactive = workloads.get_var(env_dict, "INTERACTIVE")
  random_seed = workloads.get_var(env_dict, "RANDOM_SEED")
  use_local_mirror = workloads.get_var(env_dict, "USE_LOCAL_MIRROR")
  if random_seed == None:
    random_seed = (condor_cluster << 16) | condor_process
  else:
    random_seed = int(random_seed)

  ###### mandatory parameters
  assert(protocol != None)
  assert(workload_name != None)
  assert(checkpoint != None)
  assert(chips != None)
  assert(processors_per_chip != None)
  assert(smt_threads != None)
  assert(results_dir != None)
  assert(transactions != None)
  assert(dump_interval != None)
  assert(condor_cluster != None)
  assert(condor_process != None)
 
  ruby = None
  if "ruby" in get_module_list():
    ruby = 1 # true
  opal = None
  if "opal" in get_module_list():
    opal = 1 # true
    
  ###### print out local host name to help with troubleshooting
  print "Local host name:", string.strip(os.popen("hostname").readline())
  
  ###### init simics with a checkpoint
  assert(checkpoint[0] == '/')
  run_sim_command('read-configuration "%s/%s"' % (checkpoint_dir, checkpoint))

  if(fast_forward): run_sim_command("continue %d" % fast_forward)
  
  ###### Conserve memory by limiting how much memory each simics image object can use
  #@mfacet.limit_all_images(256)
  
  ###### set simics parameters

  # enable/disable STCs here
  run_sim_command("istc-disable")
  run_sim_command("dstc-disable")
  run_sim_command("stc-status")
  
  # disable breakpoint before we sync all processors
  run_sim_command('magic-break-disable')
  
  # always use 1 for better parallelism
  old_switch_time = conf.sim.cpu_switch_time
  print "Old cpu_switch_time: %d" % conf.sim.cpu_switch_time
  conf.sim.cpu_switch_time = 1
  # 8/28/2003 CM:  stepping or continuing simics before loading opal results
  #                in a major performance loss. See me for more details.
  if not opal:
    run_sim_command("c %d"%(10*old_switch_time)) # sync CPUs after changing switch time
  print "New cpu_switch_time: %d" % conf.sim.cpu_switch_time
  
  # enable breakpoint
  run_sim_command('magic-break-enable')
  
  # enable IFETCH, i-fetch line size is controlled in checkpoint files
  run_sim_command("instruction-fetch-mode instruction-fetch-trace")
  run_sim_command("instruction-fetch-mode")
                  
  filename_prefix = "%s/%s" % (results_dir, workloads.get_output_file_name_prefix(env_dict, 1))
  prep_dir(filename_prefix)
  
  ###### capture xterm output in file
  import mod_xterm_console_commands
  mod_xterm_console_commands.cap_start_cmd(get_console(), filename_prefix + ".xterm")
  
  ###### Load modules and set parameters
  if opal:
    run_sim_command('load-module opal')
  
  if ruby:
    run_sim_command("load-module ruby")
    if network_topology:
      run_sim_command('ruby0.setparam_str g_NETWORK_TOPOLOGY "%s"' % network_topology)
    if protocol_option:
      run_sim_command('ruby0.setparam_str g_MASK_PREDICTOR_CONFIG "%s"' % protocol_option)
    run_sim_command("ruby0.setparam g_PROCS_PER_CHIP %d" % processors_per_chip)
    run_sim_command("ruby0.setparam g_NUM_L2_BANKS %d" % l2_banks)
    run_sim_command("ruby0.setparam g_RANDOM_SEED %d" % random_seed)
    run_sim_command("ruby0.setparam g_endpoint_bandwidth %d" % bandwidth)
    run_sim_command("ruby0.setparam g_NUM_PROCESSORS %d" % (chips*processors_per_chip))
    run_sim_command("ruby0.setparam g_NUM_SMT_THREADS %d" % smt_threads)

    transactional_memory = 0
    if transactional_memory:
      run_sim_command("ruby0.setparam_str REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH true")
      run_sim_command("ruby0.setparam_str PROFILE_EXCEPTIONS true")
      run_sim_command("ruby0.setparam_str PROFILE_XACT true")
      run_sim_command("ruby0.setparam_str XACT_MEMORY true")
      run_sim_command("ruby0.setparam XACT_MAX_DEPTH %s" % xact_max_depth);
      #run_sim_command("ruby0.setparam_str DATA_BLOCK true")
      run_sim_command("ruby0.setparam RETRY_LATENCY 10")
      #run_sim_command("ruby0.setparam RANDOM_SEED 0")
      run_sim_command("ruby0.setparam g_MEMORY_SIZE_BYTES 8589934592")
      run_sim_command("ruby0.setparam g_DEADLOCK_THRESHOLD 20000000")

    run_sim_command("ruby0.init")
    run_sim_command("ruby0.periodic-stats-interval %d" % 1000000)
    run_sim_command('ruby0.periodic-stats-file "%s.periodic" ' % filename_prefix)

    debug = 0
    if debug:
      run_sim_command('ruby0.debug-verb high')
      run_sim_command('ruby0.debug-filter lseagTSN')
      #run_sim_command('ruby0.debug-start-time "500000"')
      run_sim_command('ruby0.debug-start-time "0"')
  
  ###### init opal
  if opal:
    # For debugging with symtable, used by Min's race detector
    #run_sim_command('load-module symtable')
    #run_sim_command('new-symtable mysql')
    # set up the number of SMT Threads per physical processor::
    run_sim_command("opal0.setparam CONFIG_LOGICAL_PER_PHY_PROC %d" % smt_threads)
    # set up the number of physical registers needed, for each physical register type:
    run_sim_command("opal0.setparam CONFIG_IREG_PHYSICAL %d" % (160*smt_threads+64))
    run_sim_command("opal0.setparam CONFIG_FPREG_PHYSICAL %d" % (64*smt_threads+128))
    run_sim_command("opal0.setparam CONFIG_CCREG_PHYSICAL %d" % (5*smt_threads+64))
    if opal_config_file:
      run_sim_command('opal0.init "%s"' % opal_config_file)
    else:
      run_sim_command('opal0.init')
    run_sim_command('opal0.sim-start "%s.opal"' % filename_prefix)
  
  ###### do the following only when it is not interactive simulation
  if not interactive:
    ###### warm up the caches
    if ruby and warmup_file:
      print "Warming caches with %s" % warmup_file
      run_sim_command('ruby0.load-caches "%s/%s"' % (checkpoint_dir, warmup_file))
          
    ###### generate request trace
    if ruby and generate_request_trace:
      trace_filename = "/scratch/%s/traces/%s-%dc-%dp-%dt-%s-%d-%d-%d.trace.gz" % (getpass.getuser(), workload_name, chips, processors_per_chip, smt_threads, protocol, bandwidth, condor_cluster, condor_process)
      prep_dir(trace_filename)
      run_sim_command("ruby0.tracer-output-file %s" % trace_filename)
    
    ###### Run for n transactions or just a short run
    print "Initial memory usage: %s" % memory_usage_str()
    print "Running..."
    if transactions > 0:
      assert(dump_interval > 0)
      setup_run_for_n_transactions(transactions, dump_interval)
      if opal:
        run_sim_command("opal0.sim-step 100000000000")
      else:
        run_sim_command("c")
    else:
      if opal:
        run_sim_command("opal0.sim-step 1000")
      else:
        run_sim_command("c 10000")
    
    ###### dump statistics
    if ruby:
      run_sim_command('ruby0.dump-stats "%s.stats"' % filename_prefix)
      if generate_cache_data_dump:
        run_sim_command('ruby0.dump-cache-data 0 "%s.cache"' % filename_prefix)
        
    if opal:
      run_sim_command('opal0.listparam')
      run_sim_command('opal0.stats')
      run_sim_command('opal0.sim-stop')
    
    if checkpoint_at_end == 'yes':
      mf_write_configuration_cmd("/scratch/%s/%s-%dc-%dp-%dt-%d-caches" % (getpass.getuser(), checkpoint, chips, processors_per_chip, smt_threads, transactions))
    
    # close trace file
    if ruby and generate_request_trace == 'yes':
      run_sim_command('ruby0.tracer-output-file ""')
    
    ###### display resource usage
    print_resource_usage()
    
    ###### close xterm capture file
    get_console().output_file = ""
    
    ###### finishing up
    run_sim_command('quit')
  else:
    print "enter interactive mode..."
    print "Normally, the following command lines will be issued:"
    print ""
    print "  @from mfacet import *"
    print "  @setup_run_for_n_transactions(%d, %d) % (transactions, dump_interval)"
    print ""
    print "  # (optionally)"
    print "  ruby0.load-caches <workload>.warmup.gz"
    print "  ruby0.tracer-output-file <filename>.trace.gz"
    print ""
    print "  # (if ruby only)"
    print "    continue"
    print "    ruby0.dump-stats file.stats"
    print ""
    print "  # (if ruby + opal)"
    print "    opal0.sim-step 100000000000"
    print "    opal0.listparam"
    print "    opal0.stats"
    print "    opal0.sim-stop"
    print ""
    print "  quit"

##############################################################################
# other helper functions
##############################################################################

def run_sim_command(cmd):
  print '### Executing "%s"'%cmd
  try:
    # run() returns None is no error occured
    return run(cmd)
  except:
    run("quit 666")
  return

def prep_dir(path_prefix):
  dir_name = os.path.dirname(path_prefix)
  print "creating directories: %s" % dir_name
  try: 
    os.makedirs(dir_name)
  except:
    pass

def get_module_list():
  global module_list_accumulator
  module_list_accumulator = []
  SIM_for_all_modules(get_module_list_iter)
  return module_list_accumulator

def get_module_list_iter(name, file, user_version, status):
  global module_list_accumulator
  module_list_accumulator.append(name)

### date functions
def mfdate():
  return time.strftime("%b:%d:%Y:%T", time.localtime(time.time()))

def mfdateDash():
  timeStr = time.strftime("%b:%d:%Y:%T", time.localtime(time.time()))
  timeStr = string.replace(timeStr, ":", "-")
  return timeStr

# Figure out which console device to use
def get_console():
  return conf.con0

# setup global variables
__commands = ()
__prompt = ""

# include the wait_for_string from the xterm module
from text_console_common import *

# Issue commands waiting for the prompt in between each command
def console_commands(cmds, prompt):
  global __commands
  __commands = cmds
  global __prompt
  __prompt = prompt
  start_branch(console_branch_internal)

def console_branch_internal():
  for command in __commands:
    get_console().input = command
    wait_for_string(get_console(), __prompt)

# running for N transactions

__start_transaction_counter = 0
__end_transaction_counter = 0
__transaction_limit = 0
__transaction_dump_interval = 0
__start_time = 0
__run_time   = 0

def setup_run_for_n_transactions(num, dump_interval=1):
  global __transaction_limit
  __transaction_limit = num
  global __transaction_dump_interval
  __transaction_dump_interval = dump_interval
  global __start_transaction_counter
  global __end_transaction_counter
  __start_transaction_counter = 0
  __end_transaction_counter = 0
  SIM_hap_add_callback("Core_Magic_Instruction", end_transaction_callback, "end_transaction_magic_call")
  

def setup_run_for_n_cycles(num, dump_interval=1):
  global __start_time
  global __run_time
  global __end_transaction_counter
  global __start_transaction_counter
  global __transaction_dump_interval
  __run_time = num
  __start_time = SIM_cycle_count(SIM_current_processor())
  __end_transaction_counter = 0
  __start_transaction_counter = 0
  __transaction_dump_interval = dump_interval
  print "Setting up run for %d cycles" % __run_time
  SIM_hap_add_callback("Core_Magic_Instruction", end_transaction_callback_time, "end_transaction_magic_call_time")
  
def get_tid():
  # get %l0 register's value
  return SIM_read_register(SIM_current_processor(), 16)

# global variable for xact mem simulations
__start_xact_mem = 0

def register_xact_mem_callback():
  global __start_xact_mem
  __start_xact_mem = 0
  print "Registering xact_mem_callback"
  SIM_hap_add_callback("Core_Magic_Instruction", xact_mem_callback, "transaction_call")
  #SIM_hap_add_callback("Core_Exception", start_trap_callback, "transaction_call")

def start_xact_mem():
  global __start_xact_mem
  print "MFACET: start_xact_mem called"
  __start_xact_mem = 1

def print_cpu_regs(cpu):
  total_procs = 16
  proc_num = SIM_get_proc_no(cpu)

  if(total_procs == 16):
    if(proc_num == 0):
      run_sim_command('cpu0.pregs')
      run_sim_command('print-time cpu0')
    elif(proc_num == 1):
      run_sim_command('cpu1.pregs')
      run_sim_command('print-time cpu1')
    elif(proc_num==2):
      run_sim_command('cpu4.pregs')
      run_sim_command('print-time cpu4')
    elif(proc_num==3):
      run_sim_command('cpu5.pregs')
      run_sim_command('print-time cpu5')
    elif(proc_num==4):
      run_sim_command('cpu8.pregs')
      run_sim_command('print-time cpu8')
    elif(proc_num==5):
      run_sim_command('cpu9.pregs')
      run_sim_command('print-time cpu9')
    elif(proc_num==6):
      run_sim_command('cpu10.pregs')
      run_sim_command('print-time cpu10')
    elif(proc_num == 7):
      run_sim_command('cpu11.pregs')
      run_sim_command('print-time cpu11')
    elif(proc_num==8):
      run_sim_command('cpu12.pregs')
      run_sim_command('print-time cpu12')
    elif(proc_num==9):
      run_sim_command('cpu13.pregs')
      run_sim_command('print-time cpu13')
    elif(proc_num==10):
      run_sim_command('cpu14.pregs')
      run_sim_command('print-time cpu14')
    elif(proc_num==11):
      run_sim_command('cpu15.pregs')
      run_sim_command('print-time cpu15')
    elif(proc_num==12):
      run_sim_command('cpu16.pregs')
      run_sim_command('print-time cpu16')
    elif(proc_num==13):
      run_sim_command('cpu17.pregs')
      run_sim_command('print-time cpu17')
    elif(proc_num==14):
      run_sim_command('cpu18.pregs')
      run_sim_command('print-time cpu18')
    elif(proc_num==15):
      run_sim_command('cpu19.pregs')
      run_sim_command('print-time cpu19')
  elif(total_procs == 32):
     # for 32 procs
     if(proc_num == 0):
       run_sim_command('cpu0.pregs')
       run_sim_command('print-time cpu0')
     elif(proc_num == 1):
       run_sim_command('cpu1.pregs')
       run_sim_command('print-time cpu1')
     elif(proc_num==2):
       run_sim_command('cpu2.pregs')
       run_sim_command('print-time cpu2')
     elif(proc_num==3):
       run_sim_command('cpu3.pregs')
       run_sim_command('print-time cpu3')
     elif(proc_num==4):
       run_sim_command('cpu4.pregs')
       run_sim_command('print-time cpu4')
     elif(proc_num==5):
       run_sim_command('cpu5.pregs')
       run_sim_command('print-time cpu5')
     elif(proc_num==6):
       run_sim_command('cpu6.pregs')
       run_sim_command('print-time cpu6')
     elif(proc_num == 7):
       run_sim_command('cpu7.pregs')
       run_sim_command('print-time cpu7')
     elif(proc_num==8):
       run_sim_command('cpu8.pregs')
       run_sim_command('print-time cpu8')
     elif(proc_num==9):
       run_sim_command('cpu9.pregs')
       run_sim_command('print-time cpu9')
     elif(proc_num==10):
       run_sim_command('cpu10.pregs')
       run_sim_command('print-time cpu10')
     elif(proc_num==11):
       run_sim_command('cpu11.pregs')
       run_sim_command('print-time cpu11')
     elif(proc_num==12):
       run_sim_command('cpu12.pregs')
       run_sim_command('print-time cpu12')
     elif(proc_num==13):
       run_sim_command('cpu13.pregs')
       run_sim_command('print-time cpu13')
     elif(proc_num==14):
       run_sim_command('cpu14.pregs')
       run_sim_command('print-time cpu14')
     elif(proc_num==15):
       run_sim_command('cpu15.pregs')
       run_sim_command('print-time cpu15')
     elif(proc_num == 16):
       run_sim_command('cpu32.pregs')
       run_sim_command('print-time cpu32')
     elif(proc_num==17):
       run_sim_command('cpu33.pregs')
       run_sim_command('print-time cpu33')
     elif(proc_num==18):
       run_sim_command('cpu34.pregs')
       run_sim_command('print-time cpu34')
     elif(proc_num==19):
       run_sim_command('cpu35.pregs')
       run_sim_command('print-time cpu35')
     elif(proc_num==20):
       run_sim_command('cpu36.pregs')
       run_sim_command('print-time cpu36')
     elif(proc_num==21):
       run_sim_command('cpu37.pregs')
       run_sim_command('print-time cpu37')
     elif(proc_num == 22):
       run_sim_command('cpu38.pregs')
       run_sim_command('print-time cpu38')
     elif(proc_num==23):
       run_sim_command('cpu39.pregs')
       run_sim_command('print-time cpu39')
     elif(proc_num==24):
       run_sim_command('cpu40.pregs')
       run_sim_command('print-time cpu40')
     elif(proc_num==25):
       run_sim_command('cpu41.pregs')
       run_sim_command('print-time cpu41')
     elif(proc_num==26):
       run_sim_command('cpu42.pregs')
       run_sim_command('print-time cpu42')
     elif(proc_num==27):
       run_sim_command('cpu43.pregs')
       run_sim_command('print-time cpu43')
     elif(proc_num==28):
       run_sim_command('cpu44.pregs')
       run_sim_command('print-time cpu44')
     elif(proc_num==29):
       run_sim_command('cpu45.pregs')
       run_sim_command('print-time cpu45')
     elif(proc_num==30):
       run_sim_command('cpu46.pregs')
       run_sim_command('print-time cpu46')
     elif(proc_num==31):
       run_sim_command('cpu47.pregs')
       run_sim_command('print-time cpu47')

    
def set_g3_reg(cpu, value):
  global __start_xact_mem
  proc_num = SIM_get_proc_no(cpu)

  print "\n*****set_g3_reg proc = %d SETTING VALUE = %d" % (SIM_get_proc_no(cpu), value)

  #print_cpu_regs(cpu)

  g3_regnum = SIM_get_register_number(cpu, "g3")
  SIM_write_register(cpu, g3_regnum, value);

  #print_cpu_regs(cpu)
  
  #SIM_break_simulation(" ")

def start_trap_callback(desc, cpu, val):
  program_counter = SIM_get_program_counter(cpu)
  print "MFACET: start_trap_callback BEGIN val = %d proc = %d PC = 0x%x" % (val, SIM_get_proc_no(cpu), program_counter)

  if(val == 0x68 and (program_counter == 0xfe4628ec)):
    print_cpu_regs(cpu)
    SIM_break_simulation("exception caught")
    
def xact_mem_callback(desc, cpu, val):
  global __start_xact_mem
  proc_num = SIM_get_proc_no(cpu)
  
  print "MFACET: xact_mem_callback BEGIN val = %d proc = %d PC = 0x%x" % (val, SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))

  #print_cpu_regs(cpu)
  
  if(val == 63):
    m_v9_interface = SIM_get_interface(cpu, SPARC_V9_INTERFACE);
    print "MFACET catching xact_mem_callback proc = %d" % (SIM_get_proc_no(cpu))
    # check if Ruby is loaded, if so, write a 1 to G3 register
    ruby = 0
    g3_regnum = SIM_get_register_number(cpu, "g3")
    # IMPORTANT: proc 0 is OS or JVM related, always use original locking mechanisms
    if ( (__start_xact_mem == 1) and (proc_num > 0)):
      print "RUBY LOADED proc = %d" % (SIM_get_proc_no(cpu))
      #SIM_write_register(cpu, g3_regnum, 1);
      SIM_stacked_post(cpu, set_g3_reg, 1)
    else:
      print "RUBY NOT LOADED proc = %d" % (SIM_get_proc_no(cpu))
      SIM_write_register(cpu, g3_regnum, 0);
      #SIM_stacked_post(cpu, set_g3_reg, 0)
  '''
  elif(val == 13):
    print "MFACET BEGIN CLOSED XACT proc = %d PC = 0x%x" % (SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))
  elif(val == 14):
    print "MFACET COMMIT CLOSED XACT proc = %d PC = 0x%x" % (SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))
  elif(val == 16):
    print "MFACET LOCK OBJECT proc = %d PC = 0x%x" % (SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))
  elif(val == 17):
    print "MFACET UNLOCK_OBJECT proc = %d PC = 0x%x" % (SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))
  elif(val == 18):
    print "MFACET START XACT MEM proc = %d PC=0x%x" % (SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))
    start_xact_mem()
  elif(val == 5):
    print "MFACET TRANSACITON COMPLETED proc = %d PC=0x%x" % (SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))
  elif(val == 31):
    print "CHECKING REGS proc = %d PC=0x%x" % (SIM_get_proc_no(cpu), SIM_get_program_counter(cpu))
    print_cpu_regs(cpu)
  '''

  #print_cpu_regs(cpu)
  
  
def end_transaction_callback_time(desc, cpu, val):
  if (val > 0x10000):
    val = val >> 16
  global __start_time
  global __run_time
  global __start_transaction_counter
  global __end_transaction_counter
  current_time = SIM_cycle_count(SIM_current_processor())
  if ((current_time - __start_time) > __run_time):
    SIM_hap_delete_callback("Core_Magic_Instruction", end_transaction_callback_time, "end_transaction_magic_call_time")
    try:
      opal0 = SIM_get_object( "opal0" )
      SIM_set_attribute( opal0, "break_simulation", 1 )
    except:
        # if opal if not installed, call simics to halt simulation
      SIM_break_simulation("%s: %d End of Run" % (desc, current_time - __start_time))

  if val == 5: # end of trans
    #print "end of transaction of thread %d" % get_tid()
    __end_transaction_counter += 1
    #print_cpu_regs(cpu)
  elif val == 3: # start of trans
    #print "start of transaction of thread %d" % get_tid()
    __start_transaction_counter += 1

  if __transaction_dump_interval != 0 and __end_transaction_counter%__transaction_dump_interval == 0:
    # dump the transaction count
    print "%s: transaction started: %d, transaction completed: %d, transaction_limit: %d, %s" % (desc, __start_transaction_counter, __end_transaction_counter, __transaction_limit, memory_usage_str())
    

def end_transaction_callback(desc, cpu, val):
  if (val > 0x10000):
    val = val >> 16
  global __start_transaction_counter
  global __end_transaction_counter
  if val == 5: # end of trans
    #print "end of transaction of thread %d" % get_tid()
    __end_transaction_counter += 1
    #print_cpu_regs(cpu)
  elif val == 3: # start of trans
    #print "start of transaction of thread %d" % get_tid()
    __start_transaction_counter += 1
  elif val == 4: # break simulation
    return
  elif val >= 6 and val < 15: 
    return
  elif val == 15:  
    print "ASSERT FAILURE at PC %x" % SIM_get_program_counter(SIM_current_processor())
    # dump a .error file
    env_dict = workloads.prepare_env_dictionary(simics = 1)
    results_dir = workloads.get_var(env_dict, "RESULTS_DIR")
    processors = workloads.get_var(env_dict, "PROCESSORS")
    print processors
    error_output_filename = "%s/%s.error" % (results_dir, workloads.get_microbench_output_file_name_prefix(env_dict,0))
    error_output = open(error_output_filename, "w")
    error_output.write("SIMICS ASSERT FAILED at PC %x" % SIM_get_program_counter(SIM_current_processor()))
    error_output.close()

    SIM_hap_delete_callback("Core_Magic_Instruction", end_transaction_callback, "end_transaction_magic_call")
    try:
      opal0 = SIM_get_object( "opal0" )
      SIM_set_attribute( opal0, "break_simulation", 1 )
    except:
      # if opal if not installed, call simics to halt simulation
      SIM_break_simulation("%s: %d transactions reached" % (desc, __end_transaction_counter))
  elif val >= 16 and val <= 62:
    return  
  elif val >= 1024 and val < 7168:
    return    
  else:
    print "%s: Unexpected magic call number %d" % (desc, val)
  # dump sim_cycle and sim_inst out
  if __transaction_dump_interval != 0 and __end_transaction_counter%__transaction_dump_interval == 0:
    # dump the transaction count
    print "%s: transaction started: %d, transaction completed: %d, transaction_limit: %d, %s" % (desc, __start_transaction_counter, __end_transaction_counter, __transaction_limit, memory_usage_str())
    # if the trace module is installed, dump statistics
    try:
      trace0 = SIM_get_object("trace0")
      SIM_set_attribute(trace0, "dump_stats", __end_transaction_counter);
    except:
      pass
  if __end_transaction_counter >= __transaction_limit:
    print "%s: limit reached, unregistering callback" % desc
    SIM_hap_delete_callback("Core_Magic_Instruction", end_transaction_callback, "end_transaction_magic_call")
    # if opal is installed, tell it to halt simulation
    try:
      opal0 = SIM_get_object( "opal0" )
      SIM_set_attribute( opal0, "break_simulation", 1 )
    except:
      # if opal if not installed, call simics to halt simulation
      SIM_break_simulation("%s: %d transactions reached" % (desc, __end_transaction_counter))

# mf_write_configuration command
def mf_write_configuration_cmd(str):
  datestamp = mfdate()
  filename = "%s-%dp-%s" % (str, SIM_number_processors(), datestamp)
  prep_dir(filename)
  sim_commands.write_configuration_cmd(("%s.check" % filename), 0, 1, 1)  # save in standard configuration format, compressed images
  if "ruby" in get_module_list():
    run_sim_command('ruby0.save-caches "%s.caches.gz"' % filename)

new_command("mf-write-configuration", mf_write_configuration_cmd,
            [arg(str_t,"file")],
            type = "multifacet commands",
            short = "mf save configuration",
            doc = """
<b>mf-write-configuration</b> writes a configuration to with the prefex <i>file</i> and appends the number of processors, a timestamp, and '.check' to the prefix to form the full filename
""")

## Manipulate the an image attribute to limit the amount of memory it
## uses.  The limit parameter is in MBs.
files_to_delete = []
def limit_image(image_obj, limit):
  print "Limiting image object %s to %d" % (image_obj.name, limit)
  import copy, tempfile
  
  # Use /scratch if it exists (such as on the c and c2
  # cluster).  Otherwise, use the python default location
  
  if os.access("/scratch/", os.F_OK):
    tempfile.tempdir = "/scratch/"
  
  # Generate a temporary file name
  new_file_name = tempfile.mktemp("-multifacet-" + image_obj.name + ".swap")
  
  # set up the file as a paging/swap file for Simics
  files = image_obj.files_raw     ## read the old list of files
  new_file = copy.deepcopy(files[-1:][0])        ## use the size from the last file in the list
  new_file[0] = new_file_name     ## temporary file name
  new_file[1] = 0                 ## 0 is read/write mode (1 would be read-only)
  files.append(new_file)          ## add this read/write disk to the list of files
  image_obj.files = files
  image_obj.memory_limit = (limit * 1048576) ## set the image limit in MBs
  
  # Remember that we need to delete this file in the end
  if files_to_delete == []:
    SIM_hap_add_callback("Core_At_Exit", delete_files, "delete_files")
  files_to_delete.append(new_file_name)

# delete_files is used to delete the temporary swap files set up by the above function
def delete_files(desc):
  print "Deleting swap files..."
  for filename in files_to_delete:
    print "  %s : size %d bytes" % (filename, os.path.getsize(filename))
    os.remove(filename)
  print "Done."

def limit_all_images(limit):
  for obj in SIM_all_objects():
    if (obj.classname == "image"):
      file_list = obj.files_raw
      for file_param in file_list:
        
        # Only set up a swap file if the size of the image is less
        # than the limit
        
        if file_param[3] > (limit * 1048576):
          limit_image(obj, limit)
          break

## resource usage information
def resource_usage_linux2():
  stat_fields = [
    "pid", 
    "comm", 
    "state", 
    "ppid",
    "pgrp",
    "session",
    "tty",
    "tpgid",
    "flags",
    "minflt",
    "cminflt",
    "majflt",
    "cmajflt",
    "utime",
    "stime",
    "cutime",
    "cstime",
    "counter",
    "priority",
    "timeout",
    "itrealvalue",
    "starttime",
    "vsize",
    "rss",
    "rlim",
    "startcode",
    "endcode",
    "startstack",
    "kstkesp",
    "kstkeip",
    "signal",
    "blocked",
    "sigignore",
    "sigcatch",
    "wchan",
    ]
  
  statm_fields = [
    "size",     # total program size
    "resident", # size of in memory portions
    "shared",   # number of the pages that are shared
    "trs",      # number of pages that are 'code'
    "drs",      # number of pages of data/stack
    "lrs",      # number of pages of library
    "dt",       # number of dirty pages
    ]
  
  mapping = {}
  
  file = open("/proc/self/stat", "r")
  line = file.readline()
  for pair in zip(stat_fields, string.split(line)):
    mapping[pair[0]] = pair[1]
  
  mapping["memory_usage_bytes"] = int(mapping["rss"]) * (4096)
  mapping["memory_usage_kbytes"] = mapping["memory_usage_bytes"]/1024
  mapping["memory_usage_mbytes"] = mapping["memory_usage_kbytes"]/1024
  
  file = open("/proc/self/statm", "r")
  line = file.readline()
  for pair in zip(statm_fields, string.split(line)):
    mapping[pair[0]] = pair[1]
  
  mapping["size_mbytes"] = string.atof(mapping["size"]) * (4096.0/(1024.0*1024.0))
  mapping["resident_mbytes"] = string.atof(mapping["resident"]) * (4096.0/(1024.0*1024.0))
  mapping["resident_ratio"] = string.atof(mapping["resident"])/string.atof(mapping["size"])
  
  return mapping

def print_resource_usage():
  mapping = {}
  if (sys.platform == "linux2"):
    mapping = resource_usage_linux2()
    for key in ["rss", "memory_usage_bytes", "memory_usage_kbytes",
                "memory_usage_mbytes", "pid", "minflt", "majflt", "utime",
                "stime"]:
      print ("%s:" % (key)), mapping[key]
  #elif (sys.platform == "sunos5"):
    #mapping = resource_usage_sunos5()

def memory_usage_str():
  mapping = {}
  if (sys.platform == "linux2"):
    mapping = resource_usage_linux2()
    return "mem_res: %f mem_total: %f mem_ratio: %f" % (mapping["resident_mbytes"], mapping["size_mbytes"], mapping["resident_ratio"])
  #elif (sys.platform == "sunos5"):
    #mapping = resource_usage_sunos5()
    #return "mem_res: %f mem_total: %f mem_ratio: %f" % (mapping["resident_mbytes"], mapping["size_mbytes"], mapping["resident_ratio"])

