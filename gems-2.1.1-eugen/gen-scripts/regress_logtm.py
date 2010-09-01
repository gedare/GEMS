#!/s/std/bin/env /s/std/bin/python
import os, sys, string, time, re, getpass, glob

import tools, workloads
from tools import RegressionError

########### Configuration ###########

# The Directories
core_directories = [
    "condor",
    "protocols",
    "ruby",
    "scripts",
    "slicc",
    "common",
    "opal",
    "jgraph",
    #"template",
    ]

no_tab_directories = [
    "protocols",
    "ruby",
    "common",
    "slicc",
    "opal",
    #"template",
    "jgraph",
    "condor/gen-scripts"
    ]

protocols = [
    # Base CMP protocol - EE base
    {"name" : "MESI_CMP_filter_directory",
     "procs_per_chip" : 16,
     "eager_cd" : 1,
     "eager_vm" : 1,
     "magic_waiting": 0,
     "no_backoff" : 0
     },
    
    # EE magic nopred
    {"name" : "MESI_CMP_filter_directory",
     "procs_per_chip" : 16,
     "eager_cd" : 1,
     "eager_vm" : 1,
     "magic_waiting": 1,
     "no_backoff" : 1,
     "modification" : [("config/rubyconfig.defaults",
                        "XACT_NO_BACKOFF: false",
                        "XACT_NO_BACKOFF: true"),
                       ("config/rubyconfig.defaults",
                        "ENABLE_MAGIC_WAITING: false",
                        "ENABLE_MAGIC_WAITING: true"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_ENTRIES: 256",
                        "XACT_STORE_PREDICTOR_ENTRIES: 0"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_HISTORY: 256",
                        "XACT_STORE_PREDICTOR_HISTORY: 0")
                       ]
     },
    
    # EE hybrid nopred
    {"name" : "MESI_CMP_filter_directory",
     "procs_per_chip" : 16,
     "eager_cd" : 1,
     "eager_vm" : 1,
     "magic_waiting": 0,
     "no_backoff" : 1,
     "modification" : [("config/rubyconfig.defaults",
                        "XACT_NO_BACKOFF: false",
                        "XACT_NO_BACKOFF: true"),
                       ("config/rubyconfig.defaults",
                        "XACT_CONFLICT_RES: BASE",
                        "XACT_CONFLICT_RES: HYBRID"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_ENTRIES: 256",
                        "XACT_STORE_PREDICTOR_ENTRIES: 0"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_HISTORY: 256",
                        "XACT_STORE_PREDICTOR_HISTORY: 0")
                       ]
     },

    # EL Timestamp nopred
    {"name" : "MESI_CMP_filter_directory",
     "procs_per_chip" : 16,
     "eager_cd" : 1,
     "eager_vm" : 0,
     "magic_waiting": 0,
     "no_backoff" : 1,
     "modification" : [ ("config/rubyconfig.defaults",
                        "XACT_MEMORY: false",
                        "XACT_MEMORY: true"),
                        ("config/rubyconfig.defaults",
                        "XACT_LAZY_VM: false",
                        "XACT_LAZY_VM: true"),
                       ("config/rubyconfig.defaults",
                        "XACT_NO_BACKOFF: false",
                        "XACT_NO_BACKOFF: true"),
                       ("config/rubyconfig.defaults",
                        "XACT_CONFLICT_RES: BASE",
                        "XACT_CONFLICT_RES: TIMESTAMP"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_ENTRIES: 256",
                        "XACT_STORE_PREDICTOR_ENTRIES: 0"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_HISTORY: 256",
                        "XACT_STORE_PREDICTOR_HISTORY: 0")
                       ]
     },

    # LL Backoff 
    {"name" : "MESI_CMP_filter_directory",
     "procs_per_chip" : 16,
     "eager_cd" : 0,
     "eager_vm" : 0,
     "magic_waiting": 0,
     "no_backoff" : 0,
     "modification" : [ ("config/rubyconfig.defaults",
                        "XACT_MEMORY: false",
                        "XACT_MEMORY: true"),
                        ("config/rubyconfig.defaults",
                        "XACT_EAGER_CD: true",
                        "XACT_EAGER_CD: false"),
                        ("config/rubyconfig.defaults",
                        "XACT_LAZY_VM: false",
                        "XACT_LAZY_VM: true"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_ENTRIES: 256",
                        "XACT_STORE_PREDICTOR_ENTRIES: 0"),
                       ("config/rubyconfig.defaults",
                        "XACT_STORE_PREDICTOR_HISTORY: 256",
                        "XACT_STORE_PREDICTOR_HISTORY: 0")
                       ]
     },

    ]

# opal tests have the format:
#     list of setup commands
#     list of commands for running the program
#     comparisions to make after the program has run
#     list of clean-up commands
opal_tests = [
    # tester
    [[],
     ["%s/bin/tester.exec ./config/std-64-L1:64K:2-L2:4M:4.txt /p/multifacet/projects/regress/opal_traces/gzip gzip-trace 100000 > output"],
     ["diff output regression/golden_results/gzip.golden | python ../condor/gen-scripts/compareopal.py"],
     ["/bin/rm output"]],
    
    # Usd
    [[],
     ["%s/bin/usd.exec < regression/inputs/usd.input >& output"],
     ["/s/std/bin/tail -20 output >& output.20",
      "diff regression/golden_results/usd.tail.20 output.20"],
     ["/bin/rm output output.20"]],
    ]

## These are tests for Opal's SMT capabilities
opal_smt_tests = [
    # 4-way SMT
    {"name" : "4-Thread SMT, Multiple SMT chips", "smt_threads" : 4},
    
    # 16-way SMT
    {"name" : "16-Thread single SMT chip", "smt_threads" : 16},
    
    ]

# watchdog timer interval (in hours)
g_timer_hours = 14
g_timer = None

# do you need random seed?
g_random_seed = "random"
#g_random_seed = "1"

###########################################

# lock file path
g_locked = 0
g_lockfile_path = "%s/%s"%(os.path.dirname(os.path.abspath(sys.argv[0])),
                           "regression_logtm.running")
message_log_string = ""
error_counter = 0

def log_message(str="", newline=True):
    global message_log_string
    if newline:
        str += "\n"
    message_log_string += str
    sys.stdout.write(str)
    sys.stdout.flush()
    
def log_error(str=""):
    global error_counter
    log_message(str)
    error_counter += 1
    
def log_heading(str=""):
    log_message()
    log_message(str)
    log_message("-"*len(str))
    log_message()

def send_message():
    global message_log_string
    global error_counter
    if (error_counter > 0):
        subj = "[LogTM-Source] Regression Tester: %d TESTS FAILED" % error_counter
    else:
        subj = "[LogTM-Source] Regression Tester: SUCCESS"
    if os.environ.has_key("REGRESS") and os.environ["REGRESS"] == "true":
        recipients = ["lyen@cs.wisc.edu, bobba@cs.wisc.edu, drh5@cs.wisc.edu, hvolos@cs.wisc.edu"]
        #recipients = ["lyen@cs.wisc.edu"]
    else:
        recipients = [getpass.getuser()]
    tools.send_mail(message_log_string, "LogTM Regression Tester <lyen@cs.wisc.edu>", recipients, subj)
    print "Mail sent"

# function to checkout the files
def clone_multifacet():

    # get the mfacet_code repository
    if not os.path.exists("BitKeeper"):
        log_message("Performing BitKeeper Clone of mfacet_code")
        output = tools.run_command("bk clone /p/multifacet/projects/xact_memory/multifacet_release .")

    # Update the code base with the bk root directory
    if 0 and os.environ.has_key("CHECKOUT") and os.environ["CHECKOUT"] == "true":
        output = tools.run_command("bk pull")

    for dir in core_directories:
        # ensure all core directories are there
        if not os.path.exists(dir):
            raise RegressionError("Error find all core code directories", output)

    # get the mfacet_simics_sup repository and export simics_source repository
    if not os.path.exists("simics/BitKeeper"):
        log_message("Performing BitKeeper Clone of mfacet_simics and BitKeeper Export of simics_source")
        output = tools.run_command("bk clone /p/multifacet/bkroot/mfacet_simics_sup simics")
        output = tools.run_command("bk export -r+ -w /p/multifacet/bkroot/simics_source simics")
    
    # Change dirs to the root of the multifacet code tree
    os.chdir("simics")

    # Update the simics base with the bk root directory
    if 0 and os.environ.has_key("CHECKOUT") and os.environ["CHECKOUT"] == "true":
        output = tools.run_command("bk pull")

    # Move back up to the top level
    os.chdir("..")

def check_for_tabs():
    for path in no_tab_directories:
        log_message("Searching in %s..." % path)
        for filename in tools.walk(path):
            if re.compile(".*\.(c|C|h|H|l|y|cpp|sm|slicc)$").match(filename) and not re.search("SCCS", filename):
                content = open(filename, "r").read()
                count = string.count(content, "\t")
                if count > 0:
                    log_error("  WARNING: Tabs found in %s: %d" % (filename, count)) 

def build_slicc(host):
    log_message("Building slicc")
    # build slicc
    os.chdir("slicc")
    output = tools.run_command("make clean")
    output = tools.run_command("make -j 4", max_lines=50)
    os.chdir("..")

# parse a purify output file and verify there are no memory errors or leaks
def check_purify(filename):
    if not os.path.exists(filename):
        raise RegressionError("Purify output file not present: %s" % filename, "")
    
    access_errors = -1
    total_occurrences = -1
    bytes_leaked = -1
    contents = open(filename, "r").read()
    for line in string.split(contents, "\n"):
        elements = string.split(line)
        if re.search("access errors", line):
            access_errors = int(elements[1])
            total_occurrences = int(elements[4])
        elif re.search("bytes leaked", line):
            bytes_leaked = int(elements[1])
    
    if access_errors != 0 or total_occurrences != 0:
        raise RegressionError("Purify: access errors detected in log %s" % filename, contents)
    
    if bytes_leaked != 0:
        raise RegressionError("Purify: memory leak detected in log %s" % filename, contents)
    
    log_message("  Purify reports no error")

g_valgrind_output_dict = {}
g_valgrind_output_idx = 0
# parse the tester output and verify there are no memory errors or leaks
def check_valgrind(contents, valgrind):
    global g_valgrind_output_dict
    global g_valgrind_output_idx
    # start to parse the output
    lines = string.split(contents, "\n")
    
    if valgrind == 0:
        # only check for the "Success" message from the tester
        last_line = string.strip(lines[-2])
        if re.search("^Success:", last_line):
            return 1
        else:
            return 0
    else:
        # check both tester and valgrind outputs
        success = 0
        access_errors = -1
        total_occurrences = -1
        bytes_leaked = -1
        valgrind_output = ""
        for i in range(len(lines)):
            line = lines[i]
            # sample line follows:
            # ==32352== ERROR SUMMARY: 501 errors from 25 contexts (suppressed: 929 from 8)
            if re.search("ERROR SUMMARY:", line):
                elements = string.split(line)
                access_errors = int(elements[6])
                total_occurrences = int(elements[3])
                # sample line follows:
                # ==32352==    definitely lost: 19265 bytes in 635 blocks.
            elif re.search("definitely lost:", line):
                elements = string.split(line)
                bytes_leaked = int(elements[3])
            elif re.search("are definitely lost", line):
                assert(bytes_leaked != 0)
                # get five lines of the output
                for line_no in range(5):
                    valgrind_output += "    "
                    tokens = lines[i+line_no].split("==");
                    valgrind_output += tokens[2]
                    valgrind_output += "\n"
            elif re.search("^Success:", line):
                if success == 1:
                    raise RegressionError("Valgrind: \"^Success:\" appears more than once in output", contents)
                success = 1
        if access_errors != 0 or total_occurrences != 0:
            raise RegressionError("Valgrind: access errors detected in output", contents)
        elif bytes_leaked != 0:
            #raise RegressionError("Valgrind: memory leak detected in output", contents)
            # report each type of leak once
            if not g_valgrind_output_dict.has_key(valgrind_output):
                g_valgrind_output_dict[valgrind_output] = g_valgrind_output_idx
                g_valgrind_output_idx = g_valgrind_output_idx + 1
                log_message("  Valgrind reports leaks: Type %d" %
                            g_valgrind_output_dict[valgrind_output])
                log_message(valgrind_output)
            else:
                log_message("  Valgrind reports leaks: Type %d" %
                            g_valgrind_output_dict[valgrind_output])
        else:
          log_message("  Valgrind reports no errors")
        return success
    # impossible here
    assert(0)
    return 0

def ruby_clean(protocol):
    os.chdir("ruby")
    tools.run_command("make clean")
    os.chdir("..")
    output = tools.run_command("/bin/rm -rf simics/home/%s" % (protocol["name"]))

def ruby_clean_slicc(protocol):
    os.chdir("ruby")
    tools.run_command("make clean_slicc")
    os.chdir("..")
    output = tools.run_command("/bin/rm -rf simics/home/%s" % (protocol["name"]))

def build_ruby(protocol):
    name = protocol["name"]
    log_message("Building: %s" % (name))
    # build ruby tester/module
    os.chdir("ruby")
#    tools.run_command("make clean_slicc")

    # before making any modifications, backup all files that will be modified
    for (filename_exp, re_to_replace, replacement) in protocol.get("modification", []):
        for filename in glob.glob(filename_exp):
            if os.access(filename, os.W_OK) != 1:
                output = tools.run_command("bk edit %s"%filename)
            input_file = open(filename, "r")
            file_contents = input_file.read()
            # backup the file
            backup_file = open(filename+".bak", "w")
            backup_file.write(file_contents)
            backup_file.close()
    
    # perform any replacements on the code
    for (filename_exp, re_to_replace, replacement) in protocol.get("modification", []):
        for filename in glob.glob(filename_exp):
            input_file = open(filename, "r")
            file_contents = input_file.read()
            # modify the file
            (new_file_contents, number_of_subs) = re.subn(re_to_replace, replacement, file_contents)
            if number_of_subs == 0:
                raise RegressionError("Modification specification error: no replacement performed for '%s' in '%s'" % (re_to_replace, filename), "")
            output_file = open(filename, "w")
            output_file.write(new_file_contents)
            output_file.close()
        
    # build
    if protocol.has_key("no_html"):
        command = "make -j 4 PROTOCOL=%s DESTINATION=%s NO_HTML=yes" % (name, name)
    else:
        command = "make -j 4 PROTOCOL=%s DESTINATION=%s" % (name, name)
    output = tools.run_command(command, max_lines=50)
    
    # restore any modification
    for (filename_exp, re_to_replace, replacement) in protocol.get("modification", []):
        for filename in glob.glob(filename_exp):
            # read the backup file
            backup_file = open(filename+".bak", "r")
            if not backup_file:
                raise RegressionError("Modification specification error: Where is the backup file for file %s?"%filename)
            file_contents = backup_file.read()
            backup_file.close()
            # write the original file
            input_file = open(filename, "w")
            input_file.write(file_contents)
            input_file.close()
    
    os.chdir("..")

def build_opal(protocol):
    os.chdir("opal")
    log_message("Building opal")
    tools.run_command("make clean")
    # opal module must be build before building stand-alone executables
    if protocol != "":
        output = tools.run_command("make -j 4 DESTINATION=%s module" %
                                   protocol["name"], max_lines=25)
    else:
        raise RegressionError("Opal build failed: ", "build without DESTINATION being set")
    # build the stand alone tester
    output = tools.run_command("make tester", max_lines=25)
    output = tools.run_command("make usd", max_lines=25)        
    os.chdir("..")
    
def opal_clean(protocol, remove_module=0):
    os.chdir("opal")
    tools.run_command("make clean")
    if remove_module:
        output = tools.run_command("make removemodule DESTINATION=%s" %
                                   protocol["name"])
    os.chdir("..")

def build_opal_smt(smt_test, protocol):
    name = smt_test["name"]
    protocol_name = protocol["name"]
    log_message("Building SMT test: %s" % (name))
    # build ruby tester/module
    os.chdir("opal")

    # before making any modifications, backup all files that will be modified
    for (filename_exp, re_to_replace, replacement) in smt_test.get("modification", []):
        for filename in glob.glob(filename_exp):
            if os.access(filename, os.W_OK) != 1:
                output = tools.run_command("bk edit %s"%filename)
            input_file = open(filename, "r")
            file_contents = input_file.read()
            # backup the file
            backup_file = open(filename+".bak", "w")
            backup_file.write(file_contents)
            backup_file.close()
    
    # perform any replacements on the code
    for (filename_exp, re_to_replace, replacement) in smt_test.get("modification", []):
        for filename in glob.glob(filename_exp):
            input_file = open(filename, "r")
            file_contents = input_file.read()
            # modify the file
            (new_file_contents, number_of_subs) = re.subn(re_to_replace, replacement, file_contents)
            if number_of_subs == 0:
                raise RegressionError("Modification specification error: no replacement performed for '%s' in '%s'" % (re_to_replace, filename), "")
            output_file = open(filename, "w")
            output_file.write(new_file_contents)
            output_file.close()
        
    # build Opal with changed configfile
    command = "make -j 4 DESTINATION=%s" % protocol_name
    output = tools.run_command(command, max_lines=50)
    
    # restore any modification
    for (filename_exp, re_to_replace, replacement) in smt_test.get("modification", []):
        for filename in glob.glob(filename_exp):
            # read the backup file
            backup_file = open(filename+".bak", "r")
            if not backup_file:
                raise RegressionError("Modification specification error: Where is the backup file for file %s?"%filename)
            file_contents = backup_file.read()
            backup_file.close()
            # write the original file
            input_file = open(filename, "w")
            input_file.write(file_contents)
            input_file.close()
    
    os.chdir("..")
    
def run_opal_tester():
    os.chdir("opal")    
    # clean up all possible output files before beginning
    tools.run_command("/bin/rm -f gzip-trace imap-gzip-trace-0 imap-gzip-trace-6213 output")
    for test in opal_tests:
        # set up the test
        for setup in test[0]:
            tools.run_command(setup)
        # run the test
        for cmd in test[1]:
            tools.run_command(cmd % host)
        # check the results
        for check in test[2]:
            output = tools.run_command(check, max_lines=50)
            if output != "":
                log_message("    check fails: %s" % check)
                log_message("    testing: %s" % test[1])
                raise RegressionError("Opal tester failed: ", output)
        # clean up after the test
        for cleanup in test[3]:
            tools.run_command(cleanup)
    os.chdir("..")

def run_tester(host, protocol, length=10, processors=16, purify=0):
    os.chdir("ruby")
    name = protocol["name"]
    if protocol.has_key("specific_processor_count"):
        processors = protocol.get("specific_processor_count", 1)
    procs_per_chip = protocol.get("procs_per_chip", 1)
    if procs_per_chip > processors:
        procs_per_chip = 1
    l2_cache_banks = protocol.get("specific_cache_count", processors)
    
    # the '-r random' parameter make the tester pick a new seed each night
    command = "%s/generated/%s/bin/tester.exec -r %s -l %d -p %d -a %d -e %d " % (host, name, g_random_seed, length, processors, procs_per_chip, l2_cache_banks)
    if purify == 0:
        log_message("Running tester:   %s for %d with %d processors and %d procs_per_chip" % (name, length, processors, procs_per_chip))
    else:
        log_message("Running valgrind: %s for %d with %d processors and %d procs_per_chip" % (name, length, processors, procs_per_chip))
        command = "/s/valgrind-2.2.0/bin/valgrind --tool=memcheck -v --leak-check=yes " + command

    output = tools.run_command(command)
    result = check_valgrind(output, purify)
    if result != 1:
        raise RegressionError("Tester error: 'Success' message not displayed", output)


    # log random seed and ruby_cycles
    lines = string.split(output, "\n")
    for i in lines:
        if re.search("g_RANDOM_SEED", i):
            tokens = i.split()
            log_message("  Random seed: %d"%int(tokens[1]))
        if re.search("Ruby_cycle", i):
            log_message("  %s" % i)
    
    os.chdir("..")

def get_random_checkpoint():
    import random
    id = int(len(workloads.all_checkpoints) * random.random())
    return workloads.all_checkpoints[0]

def run_simics(checkpoint, workload_name, 
               transactions,
               protocol={"name" : "test"},
               processors=16,
               smt_threads=1,
               phy_signature="Perfect_",
               virtual_signature="Perfect_",
               summary_signature="Perfect_",
               xact_max_depth="1",
               microbenchmark = 0,
               mbench_arg_prefix="",
               mbench_arg_string="",
               bench_arg = 0,
               condor_process = os.getpid(), condor_cluster = os.getppid(),
               expected_ruby_cycles=0, tolerance=.05,
               check_opal=0, check_ruby=0,
               ):
    # use SPARC V8 directory
    name = protocol["name"]
    output = tools.run_command("scripts/prepare_simics_home.sh simics/home/%s"%name)
    if protocol.has_key("specific_processor_count"):
        processors = protocol.get("specific_processor_count", 1)
    procs_per_chip = protocol.get("procs_per_chip", 1)
    l2_cache_banks = protocol.get("specific_cache_count", (processors/smt_threads))
    eager_cd = protocol.get("eager_cd", 1)
    eager_vm = protocol.get("eager_vm", 1)
    magic_waiting = protocol.get("magic_waiting", 0)
    no_backoff = protocol.get("no_backoff", 0)
    
    bandwidth = protocol.get("bandwidth", 6400)
    if procs_per_chip > processors:
        procs_per_chip = 1
        
    log_message("Running simics: checkpoint=%s, processors=%s, smt_threads=%s, procs_per_chip=%d transactions=%d, protocol: %s" % (checkpoint, processors, smt_threads, procs_per_chip, transactions, name))

    # prepare environment variables for running simics
    env_dict = workloads.prepare_env_dictionary(simics = 0)
    workloads.set_var(env_dict, "RESULTS_DIR", "../../../results")
    workloads.set_var(env_dict, "WORKLOAD", workload_name)
    workloads.set_var(env_dict, "CHECKPOINT", checkpoint)
    workloads.set_var(env_dict, "CHECKPOINT_DIR", " ")
    workloads.set_var(env_dict, "PROTOCOL", name)
    workloads.set_var(env_dict, "PROCESSORS", processors)
    workloads.set_var(env_dict, "CHIPS", 1)
    workloads.set_var(env_dict, "SMT_THREADS", smt_threads)
    workloads.set_var(env_dict, "PROCS_PER_CHIP", procs_per_chip)
    workloads.set_var(env_dict, "NUM_L2_BANKS", l2_cache_banks)
    workloads.set_var(env_dict, "TRANSACTIONS", transactions)
    workloads.set_var(env_dict, "BANDWIDTH", bandwidth)
    if(g_random_seed != "random"):
      workloads.set_var(env_dict, "RANDOM_SEED", g_random_seed)
    workloads.set_var(env_dict, "CONDORCLUSTER", condor_cluster)
    workloads.set_var(env_dict, "CONDORPROCESS", condor_process)

    # Transactional Memory variables
    workloads.set_var(env_dict, "MICROBENCH_DIR", "microbenchmarks/transactional")
    workloads.set_var(env_dict, "BENCHMARK", workload_name)
    workloads.set_var(env_dict, "MAX_DEPTH", xact_max_depth)
    workloads.set_var(env_dict, 'LOCK_TYPE', "TM")
    workloads.set_var(env_dict, 'READ_WRITE_FILTER', phy_signature)        
    workloads.set_var(env_dict, 'VIRTUAL_READ_WRITE_FILTER', virtual_signature)
    workloads.set_var(env_dict, 'SUMMARY_READ_WRITE_FILTER', summary_signature)
    workloads.set_var(env_dict, "XACT_EAGER_CD", eager_cd)
    if(eager_vm == 0):
        workloads.set_var(env_dict, "XACT_LAZY_VM", 1)
    else:
        workloads.set_var(env_dict, "XACT_LAZY_VM", 0)
    workloads.set_var(env_dict, "ENABLE_MAGIC_WAITING", magic_waiting)
    workloads.set_var(env_dict, "XACT_NO_BACKOFF", no_backoff)
    
    # set per-microbenchmark specific variables
    #if ((workload_name == "compensation") or (workload_name == "commit-action")
    #      or (workload_name == "isolation-test") or (workload_name == "logging-test")
    #      or (workload_name == "partial-rollback")):
    #    workloads.set_var(env_dict, "MICROBENCH_DIR", "microbenchmarks/transactional/test")
    workloads.set_var(env_dict, 'MBENCH_ARG_PREFIX', mbench_arg_prefix)        
    workloads.set_var(env_dict, 'MBENCH_ARG_STRING', mbench_arg_string)
        
    workloads.update_system_env(env_dict)

    # create results directory
    output = tools.run_command("/bin/rm -rf results")
    
    mbench_arg_prefix = workloads.get_var(env_dict, 'MBENCH_ARG_PREFIX')
    os.mkdir("results")
    if(microbenchmark == 1):
        print "CREATING DIRECTORY results/%s-%s" % (workload_name,mbench_arg_prefix)
        os.mkdir("results/%s-%s" %(workload_name,mbench_arg_prefix))
    else:
        os.mkdir("results/"+workload_name)
    print "WORKLOAD NAME %s" % workload_name

    #'''
    os.chdir("simics/home/%s/" % name)

    # run the microbenchmark script if needed
    if( microbenchmark == 1):
        output = tools.run_command("./simics -echo -verbose -no-log -no-win -x ../../../gen-scripts/microbench.simics", "quit 666\n", verbose=1, max_lines=0)
    else:
        output = tools.run_command("./simics -echo -verbose -no-log -no-win -x ../../../gen-scripts/go.simics", "quit 666\n", verbose=1)
    #tools.run_command("./simics -echo -verbose -no-log -x ../../../gen-scripts/go.simics", "quit 666\n")
    os.chdir("../../..")
    
    # dump simics output
    if(microbenchmark == 0):
        simics_output_filename = "results/%s.output" % workloads.get_output_file_name_prefix(env_dict)
    else:
        simics_output_filename = "results/%s.output" % workloads.get_microbench_output_file_name_prefix(env_dict,0)
    simics_output = open(simics_output_filename, "w")
    simics_output.write(output)
    simics_output.close()
    
    if check_ruby == 1 and name != "template":
        if( microbenchmark == 0):
            ruby_stats_filename = "results/%s.stats" % workloads.get_output_file_name_prefix(env_dict)
            error_output_filename = "condor/results/%s.error" % (workloads.get_output_file_name_prefix(env_dict, 0))
        else:
            ruby_stats_filename = "results/%s.stats" % workloads.get_microbench_output_file_name_prefix(env_dict,0)
            error_output_filename = "condor/results/%s.error" % (workloads.get_microbench_output_file_name_prefix(env_dict, 0))
        if (not os.path.exists(ruby_stats_filename)):
            raise RegressionError("Ruby stats output file not present: %s" % ruby_stats_filename, output)

        # Check for error file, indicating a SIMICS_ASSERT() failure
        if(os.path.exists(error_output_filename)):
            print "SIMICS ASSERT error!"
            raise RegressionError("SIMICS_ASSERT error file found: %s" % error_output_filename, output)

        # get random seed
        simics_output = open(simics_output_filename, "r")
        for line in simics_output.readlines():
            if re.search("g_RANDOM_SEED", line):
                tokens = line.split()
                log_message("  Random seed: %d"%int(tokens[4][:-1]))
        # get ruby cycle
        ruby_stats = open(ruby_stats_filename, "r")
        ruby_cycles = 0
        for line in ruby_stats.readlines():
            line_elements = string.split(line)
            if len(line_elements) > 1 and line_elements[0] == "Ruby_cycles:":
                ruby_cycles = int(line_elements[1])
        if (ruby_cycles == 0):
            raise RegressionError("Ruby_cycles not found from the output file: %s" % ruby_stats_filename, output)
        else:
            log_message("  Ruby_cycles: %d"%ruby_cycles)
        if (expected_ruby_cycles != 0):
            percent_diff = 1.0*ruby_cycles/expected_ruby_cycles
            if percent_diff < (1.0-tolerance) or percent_diff > (1.0 + tolerance):
                log_message("  Checking ruby_cycles - ratio is %f: OUT OF RANGE" % percent_diff)
                log_error("ERROR: Ruby_cycles not within tolerances.  expected %d, actual %d" % (expected_ruby_cycles, ruby_cycles))
            else:
                log_message("  Checking ruby_cycles - ratio is %f: OK" % percent_diff)

    if check_opal == 1:
        opal_log_filename = "results/%s.opal" % workloads.get_output_file_name_prefix(env_dict)
        if (not os.path.exists(opal_log_filename)):
            raise RegressionError(("Opal log file not present: %s" %
                                   opal_log_filename), output)
        # check opal correct rate!
        else:
            opal_log = open(opal_log_filename)
            processor_total_instructions = 1001 # > 1000
            processor_correct_rate = 98 # < 99
            for line in opal_log.readlines():
                tokens = line.split()
                # remember the correct rate
                if(len(tokens) == 5 and tokens[1] == "Percent" and tokens[2] == "correct"):
                    processor_correct_rate = float(tokens[4])
                # remember the processor's commit instruction number
                if(len(tokens) == 6 and tokens[1] == "Total" and tokens[2] == "number" and tokens[3] == "of" and tokens [4] == "instructions"):
                    processor_total_instructions = int(tokens[5])
                    # check the correct rate here since the total instruction
                    # number comes last during the scan of the output file
                    if(processor_correct_rate < 99 and processor_total_instructions > 1000):
                        raise RegressionError(("Opal correct rate too low (%f%% of %d instructions)!" % (processor_correct_rate, processor_total_instructions)), output)
    #'''

def timer_abort():
    global g_lockfile_path
    global g_timer
    log_error("Error: Watchdog timer expired");
    # unlock the directory
    if (g_locked == 1):
        tools.run_command("rm -f %s" % g_lockfile_path)
    send_message()
    import signal
    os.kill(os.getpid(), signal.SIGKILL)

# function factory to time the function's execution
g_start_time = time.time()
def make_timed_function(function):
    def time_function_call(*param_list, **param_map):
        global g_start_time
        start_time = time.time()
        to_return = function(*param_list, **param_map)
        total_runtime = tools.format_time(time.time() - g_start_time)
        runtime = tools.format_time(time.time() - start_time)
        log_message("  runtime: %s, total runtime: %s" % (runtime, total_runtime))
        return to_return
    return time_function_call

# "wrap" these functions with timer code
clone_multifacet = make_timed_function(clone_multifacet)
build_slicc = make_timed_function(build_slicc)
build_ruby = make_timed_function(build_ruby)
build_opal = make_timed_function(build_opal)
build_opal_smt = make_timed_function(build_opal_smt)
run_tester = make_timed_function(run_tester)
run_simics = make_timed_function(run_simics)

########### Main ###########

try:

    # lock the directory
    if(os.path.exists(g_lockfile_path)):
      raise RegressionError("Another regression tester is running?", "")
    else:
      g_locked = 1
      tools.run_command("touch %s" % g_lockfile_path)
    
    # watchdog timer
    from threading import Timer
    g_timer = Timer(60*60*g_timer_hours, timer_abort)
    g_timer.start()
    
    log_message("Regression tester started at %s" % time.asctime())
    
    # setup the default search path for tools module
    tools.set_default_search_path(os.environ["PATH"])
    
    import socket
    log_message("Running on host %s" % socket.getfqdn())

    '''
    log_heading("BitKeeper Clone")
    clone_multifacet()
    
    # find out what host we're on (x86-linux, etc.)
    host = string.strip(tools.run_command("scripts/calc_host.sh"))
    log_message("Host type: %s" % host)

    log_heading("Tab check")
    #check_for_tabs()

    log_heading("Simics checkpoints")
    for workload in workloads.all_checkpoints:
        for processors in workload[2]:
            run_simics(workload[0], workload[1], 0, processors=processors)
    
    log_heading("SLICC")
    build_slicc(host)
    
    log_heading("Ruby testers/modules")
    for protocol in protocols:
        # clean up previous protocol
        if protocol.has_key("clean_all"):
            ruby_clean(protocol)
        else:
            # the protocol tester is much faster if we don't have to rebuild ruby between
            # each protocol.  Just clean_slicc unless absolutely necessary
            #ruby_clean_slicc(protocol)
            ruby_clean(protocol)

        # build the protocol
        build_ruby(protocol)

        # length == 5000 is about 30 seconds
        run_tester(host, protocol, length=5000, processors=16, purify=0)

        # length == 100 is about 30 seconds? (for valgrind)
        if host == "x86-linux":
            run_tester(host, protocol, length=100, processors=16, purify=1)
            
        # run simics for just a bit to make sure it loads
        workload = get_random_checkpoint()
        run_simics(workload[0], workload[1], 0, protocol=protocol, check_ruby=1)
    '''

    #log_heading("Ruby full runs")
    # rotates through the protocol list - every day a different protocol chosen 
    # protocol = protocols[time.gmtime().tm_yday%(len(protocols))]
    #ruby_clean(protocol)
    #build_ruby(protocol)

    # All TM systems should run through all micro-benchmarks
    log_heading("Transactional Runs")
    for protocol in protocols:
        ruby_clean(protocol)
        build_ruby(protocol)
        for workload in workloads.transactional_runs:
            run_simics(workload[0], workload[1], workload[2],
                       processors = 16, xact_max_depth=1, microbenchmark=1, mbench_arg_prefix=workload[6], mbench_arg_string=workload[7], bench_arg=1, protocol=protocol, check_ruby=1)

    eager_protocols = protocols[0:3]
    # Only Eager VM systems should run these microbenchmarks
    log_heading("Eager VM Runs")
    for protocol in eager_protocols:
        ruby_clean(protocol)
        build_ruby(protocol)
        for workload in workloads.eagervm_transactional_runs:
            run_simics(workload[0], workload[1], workload[2],
                       processors = 16, xact_max_depth=1, microbenchmark=1, mbench_arg_prefix=workload[6], mbench_arg_string=workload[7], bench_arg=1, protocol=protocol, check_ruby=1)

    # Every day, run a different STAMP workloads with the different TM systems
    stamp_workload = workloads.stamp_runs[time.gmtime().tm_yday%(len(workloads.stamp_runs))]
    #print stamp_workload
    log_heading("STAMP Runs")
    for protocol in protocols:
        ruby_clean(protocol)
        build_ruby(protocol)
        # With Perfect physical signatures
        run_simics(stamp_workload[0], stamp_workload[1], stamp_workload[2],
                   processors = 16, xact_max_depth=1, microbenchmark=1, mbench_arg_prefix=stamp_workload[6], mbench_arg_string=stamp_workload[7], bench_arg=1, protocol=protocol, check_ruby=1)
        # With small physical signatures
        #run_simics(stamp_workload[0], stamp_workload[1], stamp_workload[2],
        #           processors = 16, xact_max_depth=1, microbenchmark=1, mbench_arg_prefix=stamp_workload[6], mbench_arg_string=stamp_workload[7], bench_arg=1, protocol=protocol, check_ruby=1, phy_signature="H3_64_2_Parallel")
        # With large physical signatures
        #run_simics(stamp_workload[0], stamp_workload[1], stamp_workload[2],
        #           processors = 16, xact_max_depth=1, microbenchmark=1, mbench_arg_prefix=stamp_workload[6], mbench_arg_string=stamp_workload[7], bench_arg=1, protocol=protocol, check_ruby=1, phy_signature="H3_2048_2_Parallel")

    # clean_slicc no longer a valid target
    # ruby_clean_slicc(protocol)
    
    '''
    log_heading("Opal test")
    # build opal, run tester & one short run w/ ruby (transactions == 1)
    workload = workloads.transactional_runs[1]
    protocol = protocols[0]
    build_opal(protocol)
    run_opal_tester()
    # currently run simics just so it loads ... 100 Ts take too long
    run_simics(workload[0], workload[1], 0, protocol=protocol,
                check_opal=1)
    # opal with ruby, run for a little bit
    build_ruby(protocol)
    run_simics(workload[0], workload[1], 1, protocol=protocol,
               check_ruby=1, check_opal=1)
    # run Opal with SMT configurations, with Ruby for a little bit
    #for smt_test in opal_smt_tests:
        # build_opal_smt(smt_test, protocol)
   #     if smt_test.has_key("smt_threads"):
   #         num_smt_threads = smt_test.get("smt_threads", 1)
   #     run_simics(workload[0], workload[1], 1, smt_threads=num_smt_threads, protocol=protocol,
   #            check_ruby=1, check_opal=1)
    # clean up so later runs don't suffer opal's slowdown
    opal_clean(protocol, remove_module=1)
    '''

    log_message()
    log_message("Regression tester completed at %s" % time.asctime())
    if (g_timer):
        g_timer.cancel()
    # unlock the directory
    if (g_locked == 1):
        tools.run_command("rm -f %s" % g_lockfile_path)
    send_message()
except tools.RegressionError, e:
    log_message()
    log_error("Error: %s" % e)
    log_message()
    log_message("-------------")
    log_message()
    log_message("%s" % e.output)
    if (g_timer):
        g_timer.cancel()
    # unlock the directory
    if (g_locked == 1):
        tools.run_command("rm -f %s" % g_lockfile_path)
    send_message()
except:
    print sys.exc_info()[0]
    log_message()
    log_error("  Unexpected error: %s" % sys.exc_info()[0])
    if (g_timer):
        g_timer.cancel()
    # unlock the directory
    if (g_locked == 1):
        tools.run_command("rm -f %s" % g_lockfile_path)
    send_message()
    raise
