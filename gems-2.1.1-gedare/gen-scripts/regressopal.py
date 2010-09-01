
import os, sys, string, time, re, difflib
import tools, workloads
from tools import RegressionError

# The directories
cvs_directories = [
    "-l multifacet",
    "multifacet/condor",
    "multifacet/protocols",
    "multifacet/ruby",
    "multifacet/scripts",
    "multifacet/slicc",
    "multifacet/common",
    "multifacet/simics",
    "multifacet/opal",
    ]

opal_input_sets = [
    "./config/std-64-L1:64K:2-L2:4M:4.txt /p/multifacet/projects/nobackup1/d/traces/gzip gzip-tr 100000",
    ]

# opal tests have the format:
#     list of setup commands
#     list of commands for running the program
#     comparisions to make after the program has run
#     list of clean-up commands
opal_tests = [
    # tester
    [[],
     ["%s/bin/tester.exec ./config/std-64-L1:64K:2-L2:4M:4.txt /p/multifacet/projects/nobackup1/d/traces/gzip gzip-tr 100000 > output"],
     ["diff output /p/multifacet/projects/opal/regression/golden/gzip.golden | python ../condor/gen-scripts/compareopal.py"],
     ["/bin/rm output"]],

    # makeipage
    [["ln -s /p/multifacet/projects/nobackup1/d/traces/gzip/gzip-tr ."],
     ["%s/bin/makeipage.exec ./ gzip-tr >& output"],
     ["diff imap-gzip-tr-0 /p/multifacet/projects/opal/regression/golden/imap-gzip-tr-0",
      "diff imap-gzip-tr-6213 /p/multifacet/projects/opal/regression/golden/imap-gzip-tr-6213",
      "diff output /p/multifacet/projects/opal/regression/golden/makeipage.golden"],
     ["/bin/rm gzip-tr imap-gzip-tr-0 imap-gzip-tr-6213 output"]],

    # readipage
    [[],
     ["%s/bin/readipage.exec /p/multifacet/projects/nobackup1/d/traces/gzip/ gzip-tr >& output"],
     ["diff output /p/multifacet/projects/opal/regression/golden/readipage.golden"],
     ["/bin/rm output"]],

    # Memscan
    [[],
     ["%s/bin/memscan.exec /p/multifacet/projects/nobackup1/d/traces/read/mem-mread-07 >& output"],
     ["diff output /p/multifacet/projects/opal/regression/golden/memscan.golden"],
     ["/bin/rm output"]],

    # Usd
    [[],
     ["%s/bin/usd.exec < /p/multifacet/projects/opal/regression/inputs/usd.input >& output"],
     ["diff output /p/multifacet/projects/opal/regression/golden/usd.golden"],
     ["/bin/rm output"]],

    # Register tester
    [[],
     ["%s/bin/regtest.exec >& output"],
     ["diff output /p/multifacet/projects/opal/regression/golden/regtest.golden"],
     ["/bin/rm output"]]
    ]

opal_protocols = [
    {"name" : "MOSI_bcast_opt"},
]

message_log_string = ""
error_counter = 0
def log_message(str=""):
    global message_log_string
    print str
    message_log_string += str
    message_log_string += "\n"

def build_opal(protocol):
    os.chdir("opal")
    log_message("Building opal")
    if protocol != "":
        output = tools.run_command("make -j 4 DESTINATION=%s module" %
                                   protocol["name"], max_lines=25)
    else:
        output = tools.run_command("make -j 4 module", max_lines=25)
        
    # build the stand alone tester
    output = tools.run_command("make readipage", max_lines=25)
    output = tools.run_command("make makeipage", max_lines=25)
    output = tools.run_command("make tester", max_lines=25)
    output = tools.run_command("make regtest", max_lines=25)
    output = tools.run_command("make memscan", max_lines=25)
    output = tools.run_command("make usd", max_lines=25)
    os.chdir("..")
    
def opal_clean():
    os.chdir("opal")
    output = tools.run_command("make clean")
    os.chdir("..")

def run_opal_tester():
    os.chdir("opal")    
    # clean up all possible output files before beginning
    os.system("/bin/rm gzip-tr imap-gzip-tr-0 imap-gzip-tr-6213 output")
    for test in opal_tests:
        # set up the test
        for setup in test[0]:
            tools.run_command( setup )
        # run the test
        for cmd in test[1]:
            tools.run_command( cmd % host )
        # check the results
        for check in test[2]:
            output = tools.run_command( check )
            if output != "":
                print "check fails: test ", index, "failed!!!!"
                print output
        # clean up after the test
        for cleanup in test[3]:
            tools.run_command( cleanup )
    os.chdir("..")

def purify_opal():
    log_message("Purify opal: not implemented")

########
def run_opal_simics(checkpoint, workload_name, transactions, protocol={"name" : "None"},
               processors=16, bandwidth=6400,
               condor_process = "1", condor_cluster = "1",
               opal_param_file = "",
               use_ruby=0,
               expected_ruby_cycles=0,
               tolerance=.05
               ):
    name = protocol["name"]
    log_message("Running simics: checkpoint=%s, processors=%s, transactions=%d, protocol: %s" % (checkpoint, processors, transactions, name))
    start_time = time.time()

    # create results directory
    if not os.path.exists("condor/results"):
        os.mkdir("condor/results")
    if not os.path.exists("condor/results/"+workload_name):
        os.mkdir("condor/results/"+workload_name)

    # use the default opal configuration file, unless otherwise specified
    if opal_param_file != "":
        os.environ["OPAL_PARAMFILE"] = "%s" % opal_param_file
    if use_ruby != 0:
        os.environ["USE_RUBY"] = "1"

    os.environ["SIMICS_EXTRA_LIB"] = "./modules"
    os.environ["VTECH_LICENSE_FILE"] = "/p/multifacet/projects/simics/licenses/license.dat"
    #os.environ["SIMICS_HOME"] = "."
    os.environ["WORKLOAD"] = "%s" % workload_name
    os.environ["CHECKPOINT"] = "%s" % checkpoint
    os.environ["MODULE"] = "%s" % name
    os.environ["PROCESSORS"] = "%d" % processors
    os.environ["TRANSACTIONS"] = "%d" % transactions
    os.environ["BANDWIDTH"] = "%d" % bandwidth
    os.environ["CONDORCLUSTER"] = "%s" % condor_cluster
    os.environ["CONDORPROCESS"] = "%s" % condor_process
    
    print os.getcwd()
    if (name == "None"):
        os.chdir("simics/home/template/")
    else:
        os.chdir("simics/home/%s/" % name)
        
    output = tools.run_command("../../x86-linux/bin/simics-sparc-u2 -echo -verbose -no-log -no-win -x ../../../condor/gen-scripts/go.simics", "quit 666\n")
    os.chdir("../../..")

    end_time = time.time()
    minutes = (end_time - start_time)/60.0
    if minutes > 1:
        log_message("  runtime: %f minutes" % minutes)

    if (name != "None"):
        opal_log_filename = "condor/results/%s-%sp-%s-%s-%s-%s.opal" % (workload_name, processors, module, bandwidth, condor_cluster, condor_process)
        if (not os.path.exists(opal_log_filename)):
            raise RegressionError(("Opal log file not present: %s" %
                                   opal_log_filename), output)
        # CM FIX: should add some checking of the output here!!!

# function to checkout the files and expand the symlinks
def checkout_multifacet():
    log_message("Performing CVS checkout")
    # checkout files
    for dir in cvs_directories:
        output = tools.run_command("cvs checkout " + dir)

    # Change dirs to the root of the CVS tree
    os.chdir("multifacet")

    # expand the symlinks
    output = tools.run_command("scripts/cvsmapfs -cw < symlinks.list")
    if output != "":  # any output means an error occurred
        print output
        raise RegressionError("Error expanding symlinks", output)

def main_opal_regression():
    checkout_multifacet()
    global host
    host = string.strip(tools.run_command("scripts/calc_host.sh"))
    log_message("Host type: %s" % host)
    opal_clean()
    build_opal("")
    run_opal_tester()
    for protocol in opal_protocols:
        build_opal( protocol )
        for workload in workloads.regress_list:
            run_opal_simics(workload[0], workload[1], workload[2], protocol=protocol)
    purify_opal()

# main opal regression test
if __name__ == "__main__":
    main_opal_regression()
