from cli import *

# list format:
#    COMMAND_NAME  ARGUMENT_LIST  PYTHON_COMMAND  DOCS

ruby_command_list = [
### Parameter interfaces
    [ "init", [arg(filename_t(simpath = 1), "param-file", "?", "")], 
      "Read a configuration file." ],
    [ "readparam", [arg(filename_t(simpath = 1), "param-file")], 
      "Just reads a configuration file." ],
    [ "saveparam", [arg(filename_t(), "param-file")], 
      "Saves the current configuration to a file." ],
]

###
### Main loop to create ruby commands
###
for ruby_cmd in ruby_command_list:

    # create a python command
    def ruby_command( obj, arg=1, attr=ruby_cmd[0] ):
        SIM_set_attribute( obj, attr, arg )

    new_command( ruby_cmd[0], ruby_command,
                 ruby_cmd[1],
                 alias = "",
                 type = "ruby command",
                 short = ruby_cmd[2],
                 namespace = "ruby",
                 doc = ruby_cmd[2] )

#
# parameter commands
#

# lists ruby parameters
def ruby_list_param(obj):
    SIM_set_attribute( obj, "param", 1 )

new_command("listparam", ruby_list_param,
            [],
            alias = "",
            type  = "hfa commands",
            short = "list configuration parameters",
            namespace = "ruby",
            doc = """
List configuration parameters.<br/>
""")

# set ruby parameters (integer, and string)
def ruby_set_param(obj, name, value):
    SIM_set_attribute( obj, "param", [name, value] )
new_command("setparam", ruby_set_param,
            [arg(str_t, "name"), arg(int_t, "value")],
            alias = "",
            type  = "hfa commands",
            short = "list configuration parameters",
            namespace = "ruby",
            doc = """
List configuration parameters.<br/>
""")

def ruby_set_param_str(obj, name, value):
    SIM_set_attribute( obj, "param", [name, value] )
new_command("setparam_str", ruby_set_param_str,
            [arg(str_t, "name"), arg(str_t, "value")],
            alias = "",
            type  = "hfa commands",
            short = "list configuration parameters",
            namespace = "ruby",
            doc = """
List configuration parameters.<br/>
""")

#
# statistics commands
#

def ruby_dump_stats(obj, filename):
    SIM_set_attribute(obj, "dump-stats", filename)
       
new_command("dump-stats", ruby_dump_stats,
            args = [arg(str_t, "filename", "?", "")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "dumps the collected statistics",
            doc = """
<b>dump-stats</b> dumps the collected statistics from the instrumentation module
""")

def ruby_dump_short_stats(obj, filename):
    SIM_set_attribute(obj, "dump-short-stats", filename)
       
new_command("dump-short-stats", ruby_dump_short_stats,
            args = [arg(str_t, "filename", "?", "")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "dumps a summary of the collected statistics",
            doc = """
<b>dump-short-stats</b> dumps some of the most common statistics from the instrumentation module
""")


def ruby_periodic_stats_file(obj, filename):
    SIM_set_attribute(obj, "periodic-stats-file", filename)
       
new_command("periodic-stats-file", ruby_periodic_stats_file,
            args = [arg(str_t, "filename", "?", "")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sets the filename to which stats are dumped periodically",
            doc = """
<b>periodic-stats-file</b> sets the filename to which stats are dumped periodically
""")

def ruby_periodic_stats_interval(obj, interval):
    SIM_set_attribute(obj, "periodic-stats-interval", interval)
       
new_command("periodic-stats-interval", ruby_periodic_stats_interval,
            args = [arg(int_t, "interval", "?", "")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sets the interval at which stats are dumped periodically",
            doc = """
<b>periodic-stats-interval</b> sets the interval at which stats are dumped periodically
""")

def ruby_clear_stats(obj):
    SIM_set_attribute(obj, "clear-stats", 0)
       
new_command("clear-stats", ruby_clear_stats,
            args = [],
            type = "multifacet commands",
            namespace = "ruby",
            short = "clears the collected statistics",
            doc = """
<b>clear-stats</b> clears the collected statistics from the instrumentation module
""")

def ruby_abort_all(obj):
    SIM_set_attribute(obj, "abort-all", 0)

new_command("abort-all", ruby_abort_all,
            args = [],
            type = "multifacet commands",
            namespace = "ruby",
            short = "aborts all running transactions",
            doc = """
<b>abort-all</b> aborts all running transactions
""")


def ruby_system_recovery(obj):
    SIM_set_attribute(obj, "system-recovery", 0)
       
new_command("system-recovery", ruby_system_recovery,
            args = [],
            type = "multifacet commands",
            namespace = "ruby",
            short = "recovers the system",
            doc = """
<b>system-recovery</b> recovers the system
""")

def ruby_debug_verb(obj, new_verbosity):
    SIM_set_attribute(obj, "debug-verb", new_verbosity)

new_command("debug-verb", ruby_debug_verb,
            args = [arg(str_t, "new_verbosity_str")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sets the debug verbosity",
            doc = """
<b>debug-verb</b> sets the debug verbosity
""")

def ruby_debug_filter(obj, new_debug_filter):
    SIM_set_attribute(obj, "debug-filter", new_debug_filter)

new_command("debug-filter", ruby_debug_filter,
            args = [arg(str_t, "new_debug_str")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sets the debug filter",
            doc = """
<b>debug-filter</b> sets the debug filter based on args: all, none, or specific char. defined in Debug.C
""")

def ruby_debug_output_file (obj, new_filename):
    SIM_set_attribute(obj, "debug-output-file", new_filename)

new_command("debug-output-file", ruby_debug_output_file,
            args = [arg(str_t, "new_filename")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sends debugging output to a file",
            doc = """
<b>output-file</b> sends normal Ruby debugging output to a file.
Anything sent to cerr (i.e. failed assertions, warnings, errors)
will still appear in the Simics window, as well as in the file.
When dump-stats is called, the file is automatically closed. 
""")

def ruby_debug_start_time (obj, new_start_time):
    SIM_set_attribute(obj, "debug-start-time", new_start_time)

new_command("debug-start-time", ruby_debug_start_time,
            args = [arg(str_t, "new_start_time")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sets start time for debugging",
            doc = """
<b>output-file</b> sets start time for debugging.
""")

def ruby_set_checkpoint_interval(obj, interval):
    SIM_set_attribute(obj, "set-checkpoint-interval", interval)

new_command("set-checkpoint-interval", ruby_set_checkpoint_interval,
            args = [arg(int_t, "interval")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sets the checkpoint interval",
            doc = """
<b>set-checkpoint-interval</b> sets the checkpoint interval
""")

def ruby_load_caches(obj, filename):
    SIM_set_attribute(obj, "load-caches", filename)

new_command("load-caches", ruby_load_caches,
            args = [arg(str_t, "filename")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "loads the caches from file",
            doc = """
<b>load-caches</b> loads the caches from file
""")

def ruby_save_caches(obj, filename):
    SIM_set_attribute(obj, "save-caches", filename)

new_command("save-caches", ruby_save_caches,
            args = [arg(str_t, "filename")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "saves the caches to file",
            doc = """
<b>save-caches</b> saves the caches to file
""")

def ruby_dump_cache(obj, cpuNumber):
    SIM_set_attribute(obj, "dump-cache", cpuNumber)

new_command("dump-cache", ruby_dump_cache,
            args = [arg(int_t, "cpuNumber")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "dumps contents of specified cache",
            doc = """
<b>dump-cache</b> dumps contents of specified cache
""")

def ruby_dump_cache_data(obj, cpuNumber, filename):
    SIM_set_attribute(obj, "dump-cache-data", [cpuNumber, filename])

new_command("dump-cache-data", ruby_dump_cache_data,
            args = [arg(int_t, "cpuNumber"), arg(str_t, "Filename")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "dumps data contents of specified cache",
            doc = """
<b>dump-cache</b> dumps data contents of specified cache
""")

def ruby_tracer_output_file (obj, new_filename):
    SIM_set_attribute(obj, "tracer-output-file", new_filename)

new_command("tracer-output-file", ruby_tracer_output_file,
            args = [arg(str_t, "new_filename")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "sends perfect mask trace to a file",
            doc = """
<b>output-file</b> Sends a request trace to the output file.  When ruby exits, the file is automatically closed. 
""")

def ruby_xact_visualizer_file(obj, filename):
    SIM_set_attribute(obj, "xact-visualizer-file", filename)
       
new_command("xact-visualizer-file", ruby_xact_visualizer_file,
            args = [arg(str_t, "filename", "?", "")],
            type = "multifacet commands",
            namespace = "ruby",
            short = "Sets the Xact Visualizer output file",
            doc = """
<b>xact-visualizer-file</b> Sets the Xact Visualizer output file
""")

