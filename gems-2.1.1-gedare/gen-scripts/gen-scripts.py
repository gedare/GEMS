#!/usr/bin/env python
import string, os, sys, workloads, tools, time, config

def mfdate():
  return time.strftime("%b:%d:%Y:%T", time.localtime(time.time()))

# returns a list of lines in the file that matches the pattern
def grep(filename, pattern):
    result = [];
    file = open(filename,'r')
    for line in file.readlines():
        if re.match(pattern, line):
            result.append(string.strip(line))
    return result

################
#  Path Setup
################

## Calculate the CVS root and other absolute paths
cvsroot = os.path.split(os.getcwd())[0]
cvsroot_simics = os.path.join(cvsroot, "simics")
if config.results_dir[0] == '/':
  # absolute results dir
  cvsroot_results = config.results_dir
else:
  # relative results dir
  cvsroot_results = os.path.join(cvsroot, config.results_dir)
cvsroot_results_scripts = os.path.join(cvsroot_results, "scripts")
## Setup directories and AFS permissions
print "Setting afs permissions for simics..."
os.system("fs setacl . net:cs write")
os.system("afs_rseta %s net:cs write" % cvsroot_simics)

if not os.path.exists(cvsroot_results):
    print "Creating %s" % cvsroot_results
    os.mkdir(cvsroot_results)
    os.system("fs setacl %s net:cs write" % cvsroot_results)

if not os.path.exists(cvsroot_results_scripts):
    print "Creating %s" % cvsroot_results_scripts
    os.mkdir(cvsroot_results_scripts)
    os.system("fs setacl %s net:cs read" % cvsroot_results_scripts)

########################
## Condor Requirements
#######################
    
requirement_list = []
requirement_list.append('(Memory > %d)' % config.condor_memory)
requirement_list.append('(Arch == "x86_64")')
#requirement_list.append('((Arch == "x86_64") || (Arch == "INTEL"))')
requirement_list.append('(OpSys == "LINUX")')
if config.condor_memory < 501:
  requirement_list.append('(VirtualMachineID == 1)')
requirement_list.append('( ((ClusterGeneration == 2) && ((PrefersPromethus == TRUE) || (PrefersMultifacet == TRUE))) || (ClusterGeneration==3) || (ClusterGeneration==4) || (ClusterGeneration==5) || (ClusterGeneration==6) || (ClusterGeneration==7) )')
#requirement_list.append('((ClusterGeneration==4) || (ClusterGeneration==5))')
#requirement_list.append('(IsComputeCluster == True)')
requirement_list.append('(FileSystemDomain == "cs.wisc.edu" || FileSystemDomain == ".cs.wisc.edu")')
requirement_list.append('(IsDedicated == True)')
requirement_list.append('(Machine != "c2-090.cs.wisc.edu")')
requirement_list.append('(Machine != "c2-091.cs.wisc.edu")')
requirement_list.append('(Machine != "s0-29.cs.wisc.edu")')
requirement_list.append('(Machine != "s0-30.cs.wisc.edu")')
requirement_list.append('(Machine != "s0-31.cs.wisc.edu")')
requirement_list.append('(Machine != "s0-32.cs.wisc.edu")')
requirement_list.append('(Machine != "clover-01.cs.wisc.edu")')
requirement_list.append('(Machine != "clover-02.cs.wisc.edu")')


####################################################################################
###                 Autogen Workloads
####################################################################################

## Each workload gets a different "condor submit"/"interactive script" file
for (workload_name, transactions, dump_interval, warmup_file) in config.autogen_workload_list:
    if (transactions > 0):
      workload_name = "%s_%d" % (workload_name, transactions)
  
    print workload_name
    
    directory = "%s/%s" % (cvsroot_results, workload_name)
    if not os.path.exists(directory):
        os.mkdir(directory)
    
    ## ######################################################
    ## generate condor submit file
    ## ######################################################
    condor_file = open("%s/%s.condor" % (cvsroot_results_scripts, workload_name), "w")

    ## We run the same executable and go.simics script, but pass
    ## many parameters to the script using environment variables
    condor_file.write('executable = %s/home/sarek/simics\n' % cvsroot_simics)
    condor_file.write('arguments = -echo -verbose -no-log -no-win -x %s/gen-scripts/go.simics\n' % cvsroot)
    condor_file.write('\n')
    
    ## Other condor config
    condor_file.write('rank = TARGET.Mips\n')
    condor_file.write('universe = vanilla\n')
    condor_file.write('notification = Never\n')
    condor_file.write('\n')
    
    ## Don't allow the job to be preempted by other condor jobs.  We
    ## shouldn't use this flag too widely since it, in essence gives
    ## our jobs special priority
    condor_file.write('+IsCommittedJob = TRUE\n')
    condor_file.write('\n')
    
    ## Send the 'quit' command to the jobs as stdin.  If the job
    ## encounters an error, this will force simics to quit.
    condor_file.write('input = %s/gen-scripts/quit.simics\n' % cvsroot)
    condor_file.write('log = %s/condor.log' % cvsroot_results)
    condor_file.write('\n')


    for chips in config.chips_list:
      for procs_per_chip in config.procs_per_chip_list:
        for smt_threads in config.smt_threads_list:
          for target_memory in config.memory_mb_list:
            # create the checkpoint if one doesn't already exist
            pid = os.fork()
            if pid == 0:
              os.execlp("../checkpoints/workload-check-create.sh",
                        "workload-check-create.sh",
                        (chips*smt_threads*procs_per_chip),
                        target_memory,
                        workload_name)
            else:
              waitpid(pid)
          
            for (protocol, protocol_option) in config.protocol_list:
              for opal_config in config.opal_config_file_list:
                print "  Creating %s, %s" % (module, protocol_option)
	        for bandwidth in config.bandwidth_list:
                    condor_file.write("## run: %s module, %s opal, %d chips, %d procs per chip, %d smt threads, %s protocol option, %d bandwidth\n" %
                                      (protocol, opal_config, chips, procs_per_chip, smt_threads, protocol_option, bandwidth))
                    condor_file.write("\n")

                    req = string.join(requirement_list, " && ")

                    condor_file.write("requirements = %s\n" % req)
                    condor_file.write("\n")

                    ## Environment - this is how we pass parameters to simics
                    env_dict = workloads.prepare_env_dictionary(simics = 0)
                    if config.random_seed:
                        workloads.set_var(env_dict, 'RANDOM_SEED', config.random_seed)

                    workloads.set_var(env_dict, 'OPAL_CONFIG_NAME', opal_config[0])
                    workloads.set_var(env_dict, 'OPAL_CONFIG_FILE', opal_config[1])

                    # for the checkpoint, we have to extract the checkpoint storage directory
                    # from the checkpoints configuration file
                    lines = grep("../checkpoints/workload.conf", "WORKLOAD_CHECKPOINT_DIR=")
                    if(len(lines) != 1):
                      print "The workload checkpoint directory could not be determined"
                      os.exit(1)
                    workload_check_dir = lines[0].split("=")[1]

                    if protocol_option:
                        workloads.set_var(env_dict, 'PROTOCOL_OPTION',  protocol_option)
                    if warmup_file:
                      if warmup_file[0] == '/':
                        workloads.set_var(env_dict, 'WARMUP_FILE',"%s" % (warmup_file))
                      else:
                        workloads.set_var(env_dict, 'WARMUP_FILE',"%s-%dp.caches.gz" % (workload_name, (chips*smt_threads*procs_per_chip)))
                    workloads.set_var(env_dict, 'CHECKPOINT_AT_END', config.checkpoint_at_end)
                    workloads.set_var(env_dict, 'GENERATE_TRACE', config.generate_trace)
                    workloads.set_var(env_dict, 'BANDWIDTH', bandwidth)
                    workloads.set_var(env_dict, 'WORKLOAD', workload_name)
                    workloads.set_var(env_dict, 'CHECKPOINT_DIR', workload_check_dir)
                    workloads.set_var(env_dict, 'CHECKPOINT', "%s_warm-%dp-%dmb.check"%( workload_name, (chips*smt_threads*procs_per_chip), target_memory ) )
                    workloads.set_var(env_dict, 'PROTOCOL', protocol)
                    workloads.set_var(env_dict, 'PROCESSORS', chips* procs_per_chip* smt_threads)
                    workloads.set_var(env_dict, 'CHIPS', chips)
                    workloads.set_var(env_dict, 'PROCS_PER_CHIP', procs_per_chip)
                    workloads.set_var(env_dict, "SMT_THREADS", smt_threads)
                    workloads.set_var(env_dict, 'RESULTS_DIR', cvsroot_results)
                    workloads.set_var(env_dict, 'TRANSACTIONS', transactions)
                    workloads.set_var(env_dict, 'DUMP_INTERVAL', dump_interval)
                    workloads.set_var(env_dict, 'CONDORCLUSTER', '$(Cluster)')
                    workloads.set_var(env_dict, 'CONDORPROCESS', '$(Process)')

                    #workloads.set_var(env_dict, 'USE_LOCAL_MIRROR', 'yes')  
                
                    condor_file.write("getenv = False\n")
                    workloads.check_requirements(env_dict)
                    condor_file.write("environment = %s\n" % workloads.get_condor_env_string(env_dict))
                    condor_file.write("\n")
                
                    ## Files and directories
                    condor_file.write('initialdir = %s/home/%s/\n' % (cvsroot_simics, protocol))

                    filename_prefix = "%s/%s" % (cvsroot_results, workloads.get_output_file_name_prefix(env_dict, 1))
                
                    condor_file.write('filename = %s\n' % filename_prefix)
                    condor_file.write('error = $(filename).error\n')
                    condor_file.write('output = $(filename).output\n')
                    condor_file.write("\n")
                
                    ## Run multiple copies of this datapoint with varying
                    ## priority.  This makes sure condor will try to run
                    ## all the 1st data points before running any
                    ## additional runs of a datapoint
                    for num in range(config.condor_num_runs):
                      condor_file.write("priority = %d\n" % (config.condor_base_priority-num))
                      condor_file.write("queue 1\n")
                      condor_file.write("\n")
                
                    ## ######################################################
                    ## generate interactive script
                    ## ######################################################

                    script_filename = workloads.get_script_file_name(env_dict)

                    print "  Making script %s" % script_filename
                    script_file = open("%s/%s" % (cvsroot_results_scripts, script_filename), "w")
                    script_file.write("#!/s/std/bin/bash\n\n")
                    script_file.write("\n# Here is how to make this script interactive:\n")
                    script_file.write("#   1 change the INTERACTIVE define below\n")
                    script_file.write("#   2 change the ../../gen-scripts/go.simics, to comment out the line of quit\n")
                    script_file.write("\n\n")
                    script_file.write("export HOST=`hostname`\n")
                    workloads.set_var(env_dict, 'CONDORCLUSTER', "`echo $HOST | tr -- '-_.' ' ' | tr '[a-zA-Z]' '[0-90-90-90-90-90-9]' | awk '{ print $1; }'`")
                    workloads.set_var(env_dict, 'CONDORPROCESS', "$$")
                    ## set to -- None -- if you do not want to manually setup runs
                    workloads.set_var(env_dict, 'INTERACTIVE', None)

                    script_file.write(workloads.get_shell_setenv_string(env_dict))
                    script_file.write("\n\n")
                    script_file.write("cd %s/simics/home/%s\n\n" % (cvsroot, protocol))
                    script_file.write("./simics -echo -verbose -no-log -no-win -x ../../../condor/gen-scripts/go.simics\n")
                    script_file.write("\n")
                    script_file.close()
                    tools.run_command("chmod +x %s/%s" % (cvsroot_results_scripts, script_filename), echo = 0)
    
    condor_file.close()

####################################################################################
###                   Custom Workloads
####################################################################################

for (workload_name, checkpoint_dir, checkpoint_prefix, transactions, dump_interval, warmup_file) in config.custom_workload_list:
    if (transactions > 0):
      workload_name = "%s_%d" % (workload_name, transactions)
  
    print workload_name
    
    directory = "%s/%s" % (cvsroot_results, workload_name)
    if not os.path.exists(directory):
        os.mkdir(directory)
    
    ## ######################################################
    ## generate condor submit file
    ## ######################################################
    condor_file = open("%s/%s.condor" % (cvsroot_results_scripts, workload_name), "w")

    ## We run the same executable and go.simics script, but pass
    ## many parameters to the script using environment variables
    condor_file.write('executable = %s/home/sarek/simics\n' % cvsroot_simics)
    condor_file.write('arguments = -echo -verbose -no-log -no-win -x %s/gen-scripts/go.simics\n' % cvsroot)
    condor_file.write('\n')
    
    ## Other condor config
    condor_file.write('rank = TARGET.Mips\n')
    condor_file.write('universe = vanilla\n')
    condor_file.write('notification = Never\n')
    condor_file.write('\n')
    
    ## Don't allow the job to be preempted by other condor jobs.  We
    ## shouldn't use this flag too widely since it, in essence gives
    ## our jobs special priority
    condor_file.write('+IsCommittedJob = TRUE\n')
    condor_file.write('\n')
    
    ## Send the 'quit' command to the jobs as stdin.  If the job
    ## encounters an error, this will force simics to quit.
    condor_file.write('input = %s/gen-scripts/quit.simics\n' % cvsroot)
    condor_file.write('log = %s/condor.log' % cvsroot_results)
    condor_file.write('\n')

    
    for (protocol, protocol_option) in config.protocol_list:
      for opal_config in config.opal_config_file_list:
        print "  Creating %s, %s" % (protocol, protocol_option)
        for chips in config.chips_list:
          for procs_per_chip in config.procs_per_chip_list:
            for smt_threads in config.smt_threads_list:
	        for bandwidth in config.bandwidth_list:
                    condor_file.write("## run: %s module, %s opal, %d chips, %d procs per chip, %d smt threads, %s protocol option, %d bandwidth\n" %
                                      (protocol, opal_config, chips, procs_per_chip, smt_threads, protocol_option, bandwidth))
                    condor_file.write("\n")

                    req = string.join(requirement_list, " && ")

                    condor_file.write("requirements = %s\n" % req)
                    condor_file.write("\n")

                    ## Environment - this is how we pass parameters to simics
                    env_dict = workloads.prepare_env_dictionary(simics = 0)
                    if config.random_seed:
                        workloads.set_var(env_dict, 'RANDOM_SEED', config.random_seed)

                    workloads.set_var(env_dict, 'OPAL_CONFIG_NAME', opal_config[0])
                    workloads.set_var(env_dict, 'OPAL_CONFIG_FILE', opal_config[1])

                    if protocol_option:
                        workloads.set_var(env_dict, 'PROTOCOL_OPTION',  protocol_option)
                    if warmup_file:
                        workloads.set_var(env_dict, 'WARMUP_FILE',"%s" % (warmup_file))
                    #workloads.set_var(env_dict, 'CHECKPOINT_AT_END', 'yes')
                    #workloads.set_var(env_dict, 'GENERATE_TRACE', 'yes')
                    workloads.set_var(env_dict, 'BANDWIDTH', bandwidth)
                    workloads.set_var(env_dict, 'WORKLOAD', workload_name)
                    workloads.set_var(env_dict, 'CHECKPOINT_DIR', checkpoint_dir)
                    workloads.set_var(env_dict, 'CHECKPOINT', "%s-%dp.check" % (checkpoint_prefix, (chips*smt_threads*procs_per_chip) ) )
                    workloads.set_var(env_dict, 'PROTOCOL', protocol)
                    workloads.set_var(env_dict, 'PROCESSORS', chips*smt_threads*procs_per_chip)
                    workloads.set_var(env_dict, 'CHIPS', chips)
                    workloads.set_var(env_dict, 'PROCS_PER_CHIP', procs_per_chip)
                    workloads.set_var(env_dict, "SMT_THREADS", smt_threads)
                    workloads.set_var(env_dict, 'RESULTS_DIR', cvsroot_results)
                    workloads.set_var(env_dict, 'TRANSACTIONS', transactions)
                    workloads.set_var(env_dict, 'DUMP_INTERVAL', dump_interval)
                    workloads.set_var(env_dict, 'CONDORCLUSTER', '$(Cluster)')
                    workloads.set_var(env_dict, 'CONDORPROCESS', '$(Process)')

                    #workloads.set_var(env_dict, 'USE_LOCAL_MIRROR', 'yes')  
                
                    condor_file.write("getenv = False\n")
                    workloads.check_requirements(env_dict)
                    condor_file.write("environment = %s\n" % workloads.get_condor_env_string(env_dict))
                    condor_file.write("\n")
                
                    ## Files and directories
                    condor_file.write('initialdir = %s/home/%s/\n' % (cvsroot_simics, protocol))

                    filename_prefix = "%s/%s" % (cvsroot_results, workloads.get_output_file_name_prefix(env_dict, 1))
                
                
                    condor_file.write('filename = %s\n' % filename_prefix)
                    condor_file.write('error = $(filename).error\n')
                    condor_file.write('output = $(filename).output\n')
                    condor_file.write("\n")
                
                    ## Run multiple copies of this datapoint with varying
                    ## priority.  This makes sure condor will try to run
                    ## all the 1st data points before running any
                    ## additional runs of a datapoint
                    for num in range(config.condor_num_runs):
                      condor_file.write("priority = %d\n" % (config.condor_base_priority-num))
                      condor_file.write("queue 1\n")
                      condor_file.write("\n")
                
                    ## ######################################################
                    ## generate interactive script
                    ## ######################################################


                    script_filename = workloads.get_script_file_name(env_dict)

                    print "  Making script %s" % script_filename
                    script_file = open("%s/%s" % (cvsroot_results_scripts, script_filename), "w")
                    script_file.write("#!/s/std/bin/bash\n\n")
                    script_file.write("\n# Here is how to make this script interactive:\n")
                    script_file.write("#   1 change the INTERACTIVE define below\n")
                    script_file.write("#   2 change the ../../gen-scripts/go.simics, to comment out the line of quit\n")
                    script_file.write("\n\n")
                    script_file.write("export HOST=`hostname`\n")
                    workloads.set_var(env_dict, 'CONDORCLUSTER', "`echo $HOST | tr -- '-_.' ' ' | tr '[a-zA-Z]' '[0-90-90-90-90-90-9]' | awk '{ print $1; }'`")
                    workloads.set_var(env_dict, 'CONDORPROCESS', "$$")
                    ## set to -- None -- if you do not want to manually setup runs
                    workloads.set_var(env_dict, 'INTERACTIVE', None)

                    script_file.write(workloads.get_shell_setenv_string(env_dict))
                    script_file.write("\n\n")
                    script_file.write("cd %s/simics/home/%s\n\n" % (cvsroot, protocol))
                    script_file.write("./simics -echo -verbose -no-log -no-win -x ../../../condor/gen-scripts/go.simics\n")
                    script_file.write("\n")
                    script_file.close()
                    tools.run_command("chmod +x %s/%s" % (cvsroot_results_scripts, script_filename), echo = 0)
    
    condor_file.close()

####################################################################################
###                   Transactional Microbenchmarks
####################################################################################

## Each workload gets a different "condor submit"/"interactive script" file
for (benchmark, transactions, dump_interval, arg_prefix, arg_string) in config.microbenchmark_list:
  for lock_type in config.lock_type_list:
    workload_name = "%s-%s" % (benchmark, lock_type)
    #if (transactions > 0):
    #  workload_name = "%s_%d" % (workload_name, transactions)
  
    print workload_name

    directory = "%s/%s-%s" % (cvsroot_results, workload_name, arg_prefix)
      
    if not os.path.exists(directory):
        os.mkdir(directory)
        #os.system("fs setacl %s net:cs write" % directory)
    
    ## ######################################################
    ## generate condor submit file
    ## ######################################################

    condor_file = open("%s/%s-%s.condor" % (cvsroot_results_scripts, workload_name, arg_prefix), "w")

    ## We run the same executable and go.simics script, but pass
    ## many parameters to the script using environment variables
    condor_file.write('executable = %s/home/sarek/simics\n' % cvsroot_simics)
    ## KM -- change script file here
    condor_file.write('arguments = -echo -verbose -no-log -no-win -x %s/gen-scripts/microbench.simics\n' % cvsroot)
    condor_file.write('\n')
    
    ## Other condor config
    condor_file.write('rank = TARGET.Mips\n')
    condor_file.write('universe = vanilla\n')
    condor_file.write('notification = Never\n')
    condor_file.write('\n')
    
    ## Don't allow the job to be preempted by other condor jobs.  We
    ## shouldn't use this flag too widely since it, in essence gives
    ## our jobs special priority
    #condor_file.write('+IsCommittedJob = TRUE\n')
    condor_file.write('\n')
    
    ## Send the 'quit' command to the jobs as stdin.  If the job
    ## encounters an error, this will force simics to quit.
    condor_file.write('input = %s/gen-scripts/quit.simics\n' % cvsroot)
    condor_file.write('log = %s/condor.log' % cvsroot_results)
    condor_file.write('\n')

    for chips in config.chips_list:
      for procs_per_chip in config.procs_per_chip_list:
        for smt_threads in config.smt_threads_list:
 #         for target_memory in config.memory_mb_list:
            # create the checkpoint if one doesn't already exist
#            pid = os.fork()
#            if pid == 0:
#              os.execlp("../checkpoints/naked-check-create.sh",
#                        "naked-check-create.sh",
#                        (chips*smt_threads*procs_per_chip),
#                        (target_memory)
#                        )
#            else:
#              os.waitpid(pid,0)
          for (protocol, protocol_option) in config.protocol_list:
            for opal_config in config.opal_config_file_list:
              for bandwidth in config.bandwidth_list:
                for filter_config in config.filter_config_list:
                  for virtual_filter_config in config.virtual_filter_config_list:
                    for summary_filter_config in config.summary_filter_config_list:
                      for xact_max_depth in config.xact_max_depth_list:
                        if ((lock_type == 'Lock_MCS') and (filter_config != 'Perfect_')):
                          continue              
                        if ((lock_type == 'Lock_MCS') and (protocol_option != 'EagerCD_EagerVM_Base')):
                          continue
                        condor_file.write("## run: %s module, %s opal, %d chips, %d procs per chip, %d smt threads, %s protocol option, %d bandwidth\n" %
                                          (protocol, opal_config, chips, procs_per_chip, smt_threads, protocol_option, bandwidth))
                        condor_file.write("\n")

                        req = string.join(requirement_list, " && ")

                        condor_file.write("requirements = %s\n" % req)
                        condor_file.write("\n")

                        ## Environment - this is how we pass parameters to simics
                        env_dict = workloads.prepare_env_dictionary(simics = 0)
                        if config.random_seed:
                          workloads.set_var(env_dict, 'RANDOM_SEED', config.random_seed)
                        if protocol_option:
                          workloads.set_var(env_dict, 'PROTOCOL_OPTION',  protocol_option)

                        workloads.set_var(env_dict, 'OPAL_CONFIG_NAME', opal_config[0])
                        workloads.set_var(env_dict, 'OPAL_CONFIG_FILE', opal_config[1])

                        #if warmup_file:
                        #    workloads.set_var(env_dict, 'WARMUP_FILE',"%s" % (warmup_file))
                        workloads.set_var(env_dict, 'CHECKPOINT_AT_END', config.checkpoint_at_end)
                        workloads.set_var(env_dict, 'GENERATE_TRACE', config.checkpoint_at_end)
                        workloads.set_var(env_dict, 'BANDWIDTH', bandwidth)
                        workloads.set_var(env_dict, 'WORKLOAD', workload_name)
                        workloads.set_var(env_dict, 'BENCHMARK', benchmark)
                        workloads.set_var(env_dict, 'MBENCH_ARG_STRING', arg_string)
                        workloads.set_var(env_dict, 'MBENCH_ARG_PREFIX', arg_prefix)
                        workloads.set_var(env_dict, 'BATCH_SIZE', config.batch_size)
                        workloads.set_var(env_dict, 'PROTOCOL', protocol)
                        workloads.set_var(env_dict, 'PROCESSORS', chips*procs_per_chip*smt_threads)
                        workloads.set_var(env_dict, 'CHIPS', chips)
                        workloads.set_var(env_dict, 'PROCS_PER_CHIP', procs_per_chip)
                        workloads.set_var(env_dict, "SMT_THREADS", smt_threads)
                        workloads.set_var(env_dict, "READ_WRITE_FILTER", filter_config)
                        workloads.set_var(env_dict, "VIRTUAL_READ_WRITE_FILTER", virtual_filter_config)
                        workloads.set_var(env_dict, "SUMMARY_READ_WRITE_FILTER", summary_filter_config)
                        workloads.set_var(env_dict, 'RESULTS_DIR', cvsroot_results)
                        workloads.set_var(env_dict, 'TRANSACTIONS', transactions)
                        workloads.set_var(env_dict, 'DUMP_INTERVAL', dump_interval)
                        workloads.set_var(env_dict, 'LOCK_TYPE', lock_type)
                        workloads.set_var(env_dict, 'CONDORCLUSTER', '$(Cluster)')
                        workloads.set_var(env_dict, 'CONDORPROCESS', '$(Process)')
                        #workloads.set_var(env_dict, 'USE_LOCAL_MIRROR', 'yes')

                        ## nested transaction specific arguments
                        ## required for deque, slist-early, sortedList
                        workloads.set_var(env_dict, 'MAX_DEPTH', xact_max_depth)
                        workloads.set_var(env_dict, 'MICROBENCH_DIR', config.microbench_dir)

                        if protocol_option == config.ll_base:
                          workloads.set_var(env_dict, 'XACT_EAGER_CD', 0)
                          workloads.set_var(env_dict, 'XACT_LAZY_VM', 1)
                          workloads.set_var(env_dict, 'XACT_NO_BACKOFF', 1)
                        if protocol_option == config.ll_backoff:
                          workloads.set_var(env_dict, 'XACT_EAGER_CD', 0)
                          workloads.set_var(env_dict, 'XACT_LAZY_VM', 1)

                        if (protocol_option == config.el_backoff_pred or protocol_option == config.el_backoff_nopred):
                          workloads.set_var(env_dict, 'XACT_EAGER_CD', 1)
                          workloads.set_var(env_dict, 'XACT_LAZY_VM', 1)
                        if (protocol_option == config.el_timestamp_pred or protocol_option == config.el_timestamp_nopred):
                          workloads.set_var(env_dict, 'XACT_EAGER_CD', 1)
                          workloads.set_var(env_dict, 'XACT_LAZY_VM', 1)
                          workloads.set_var(env_dict, 'XACT_CONFLICT_RES', 'TIMESTAMP')
                          workloads.set_var(env_dict, 'XACT_NO_BACKOFF', 1)
                        if (protocol_option == config.el_cycle):
                          workloads.set_var(env_dict, 'XACT_EAGER_CD', 1)
                          workloads.set_var(env_dict, 'XACT_LAZY_VM', 1)
                          workloads.set_var(env_dict, 'XACT_CONFLICT_RES', 'CYCLE')
                          workloads.set_var(env_dict, 'XACT_NO_BACKOFF', 1)

                        if (protocol_option == config.ee_hybrid_nopred or protocol_option == config.ee_hybrid_pred or protocol_option == config.ee_timestamp or protocol_option == config.ee_magic):
                          workloads.set_var(env_dict, 'XACT_NO_BACKOFF', 1)
                        if protocol_option == config.ee_magic:
                          workloads.set_var(env_dict, 'ENABLE_MAGIC_WAITING', 1)
                        if (protocol_option == config.ee_hybrid_pred or protocol_option == config.ee_hybrid_nopred):
                          workloads.set_var(env_dict, 'XACT_CONFLICT_RES', 'HYBRID')          
                        if (protocol_option == config.ee_timestamp):
                          workloads.set_var(env_dict, 'XACT_CONFLICT_RES', 'TIMESTAMP')          
                        if (protocol_option == config.ee_base_nohwbuf):
                          workloads.set_var(env_dict, 'XACT_LOG_BUFFER_SIZE', 0)          
                        if (protocol_option == config.ee_base_pred or protocol_option == config.ee_hybrid_pred or
                            protocol_option == config.el_backoff_pred or protocol_option == config.el_timestamp_pred):
                          workloads.set_var(env_dict, 'XACT_STORE_PREDICTOR_ENTRIES', 256)
                          workloads.set_var(env_dict, 'XACT_STORE_PREDICTOR_HISTORY', 256)
                          workloads.set_var(env_dict, 'XACT_STORE_PREDICTOR_THRESHOLD', 4)
                        else:
                          workloads.set_var(env_dict, 'XACT_STORE_PREDICTOR_ENTRIES', 0)

                        if (protocol_option == config.logtm_se_virtualization):
                          workloads.set_var(env_dict, 'XACT_ENABLE_VIRTUALIZATION_LOGTM_SE', 1)
                          workloads.set_var(env_dict, 'XACT_ENABLE_VIRTUALIZATION_PURE', 1)

                        if (protocol_option == config.logtm_se_virtualization):
                          workloads.set_var(env_dict, 'XACT_ENABLE_VIRTUALIZATION_LOGTM_VSE', 1)
                          workloads.set_var(env_dict, 'XACT_ENABLE_VIRTUALIZATION_PURE', 1)

                        #workloads.set_var(env_dict, 'XACT_VISUALIZER', 1)

                        condor_file.write("getenv = False\n")
                        condor_file.write("environment = %s\n" % workloads.get_condor_env_string(env_dict))
                        condor_file.write("\n")

                        ## Files and directories
                        condor_file.write('initialdir = %s/home/%s/\n' % (cvsroot_simics, protocol))
                        filename_prefix = "%s/%s" % (cvsroot_results, workloads.get_microbench_output_file_name_prefix(env_dict, 1))

                        condor_file.write('filename = %s\n' % filename_prefix)
                        condor_file.write('error = $(filename).error\n')
                        #condor_file.write('output = $(filename).output\n')
                        condor_file.write("\n")

                        ## Run multiple copies of this datapoint with varying
                        ## priority.  This makes sure condor will try to run
                        ## all the 1st data points before running any
                        ## additional runs of a datapoint
                        for num in range(config.condor_num_runs):
                          condor_file.write("priority = %d\n" % (config.condor_base_priority-num))
                          condor_file.write("queue 1\n")
                        condor_file.write("\n")

                        ## ######################################################
                        ## generate interactive script
                        ## ######################################################

                        script_filename = workloads.get_microbench_script_file_name(env_dict)

                        print "  Making script %s" % script_filename
                        script_file = open("%s/%s" % (cvsroot_results_scripts, script_filename), "w")
                        script_file.write("#!/s/std/bin/bash\n\n")
                        script_file.write("\n# Here is how to make this script interactive:\n")
                        script_file.write("#   1 change the INTERACTIVE define below\n")
                        script_file.write("#   2 change the ../../gen-scripts/go.simics, to comment out the line of quit\n")
                        script_file.write("\n\n")
                        script_file.write("export HOST=`hostname`\n")
                        workloads.set_var(env_dict, 'CONDORCLUSTER', "`echo $HOST | tr -- '-_.' ' ' | tr '[a-zA-Z]' '[0-90-90-90-90-90-9]' | awk '{ print $1; }'`")
                        workloads.set_var(env_dict, 'CONDORPROCESS', "$$")
                        ## set to -- None -- if you do not want to manually setup runs
                        workloads.set_var(env_dict, 'INTERACTIVE', None)

                        script_file.write(workloads.get_shell_setenv_string(env_dict))
                        script_file.write("\n\n")

                        script_file.write("cd %s/simics/home/%s\n\n" % (cvsroot, protocol))
                        #script_file.write("./simics -echo -verbose -no-log -no-win -x ../../../%s/%s/%s.simics\n" % (microbench_dir, benchmark, benchmark))
                        script_file.write("./simics -echo -verbose -no-log -no-win -stall -x ../../../gen-scripts/microbench.simics\n")
                        script_file.write("\n")
                        script_file.close()
                        tools.run_command("chmod +x %s/%s" % (cvsroot_results_scripts, script_filename), echo = 0)
    
    condor_file.close()
