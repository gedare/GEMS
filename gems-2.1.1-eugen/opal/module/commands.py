
from cli import *

# list format:
#    COMMAND_NAME  ARGUMENT_LIST  PYTHON_COMMAND  DOCS

opal_command_list = [
### Parameter interfaces
    [ "init", [arg(filename_t(simpath = 1), "param-file", "?", "")], 
      "Read a configuration file." ],
    [ "readparam", [arg(filename_t(simpath = 1), "param-file")], 
      "Just reads a configuration file." ],
    [ "saveparam", [arg(filename_t(), "param-file")], 
      "Saves the current configuration to a file." ],

### Trace interfaces
    [ "trace-start", [arg(str_t, "trace-file")],
      "Start a trace" ],
    [ "trace-stop", [], "Finishes a trace" ],
    [ "take-trace", [arg(int_t, "number of steps")],
      "Trace a number of steps in Simics." ],
    [ "skip-trace", [arg(int_t, "number of steps")],
      "Trace a number of steps (with skipping) in Simics." ],

### Branch tracing interfaces
    [ "branch-trace-start", [arg(str_t, "branch-file-name")],
      "Start a opal branch trace in Simics." ],
    [ "branch-trace-stop", [], "Finishes a branch trace." ],
    [ "branch-trace-take", [arg(int_t, "number of steps")],
      "Branch trace a number of steps in Simics." ],
    [ "branch-trace-inf", [],
      "Branch trace until a magic breakpoint is hit." ],

### Main simulation commands
    [ "sim-start", [arg(str_t, "output-filename")],
      "start a simulation run (opens the logs, etc)" ],
    [ "sim-step", [arg(int_t, "number of steps")],
      "Simulate a number of steps in Simics." ],
    [ "sim-stop", [],
      "stops the simulation (closes the logs, etc)" ],
    [ "sim-warmup", [],
      "Install opal as the memory hierarchy in warm-up mode (untested)." ],
    [ "break_simulation", [],
      "Halt opal's simulation as soon as possible." ],
    [ "sim-inorder-step", [arg(int_t, "number of steps")],
      "Simulate the exection of an inorder processor." ],

### Micro-architectural checkpoints commands
    [ "sim-wr-check", [arg(str_t, "conf-file")],
      "Save the warm micro-architectural state." ],
    [ "sim-rd-check", [arg(str_t, "conf-file")],
      "Reads micro-architectural state from a file." ],

### Misc, Testing, Debugging commands
    [ "install-event-poster", [arg(int_t, "install")],
      "Post a trace-related function to event queue" ]
]

###
### Main loop to create all opal commands
###
for opal_cmd in opal_command_list:

    # create a python command
    def opal_command( obj, arg=1, attr=opal_cmd[0] ):
        SIM_set_attribute( obj, attr, arg )

    new_command( opal_cmd[0], opal_command,
                 opal_cmd[1],
                 alias = "",
                 type = "opal command",
                 short = opal_cmd[2],
                 namespace = "opal",
                 doc = opal_cmd[2] )

# lists opal parameters
def opal_list_param(obj):
    SIM_set_attribute( obj, "param", 1 )

new_command("listparam", opal_list_param,
            [],
            alias = "",
            type  = "hfa commands",
            short = "list configuration parameters",
            namespace = "opal",
            doc = """
List configuration parameters.<br/>
""")

# set opal parameters (integer, and string)
def opal_set_param(obj, name, value):
    SIM_set_attribute( obj, "param", [name, value] )
new_command("setparam", opal_set_param,
            [arg(str_t, "name"), arg(int_t, "value")],
            alias = "",
            type  = "hfa commands",
            short = "list configuration parameters",
            namespace = "opal",
            doc = """
List configuration parameters.<br/>
""")

def opal_set_param_str(obj, name, value):
    SIM_set_attribute( obj, "param", [name, value] )
new_command("setparam_str", opal_set_param_str,
            [arg(str_t, "name"), arg(str_t, "value")],
            alias = "",
            type  = "hfa commands",
            short = "list configuration parameters",
            namespace = "opal",
            doc = """
List configuration parameters.<br/>
""")

#
# step ye olde' simulator
#

def opal_stepper(obj, numsteps, stride):
    SIM_set_attribute( obj, "stepper", [numsteps, stride] )

new_command("step", opal_stepper,
            [arg(int_t, "number of times"), arg(int_t, "number of steps")],
            alias = "",
            type  = "hfa commands",
            short = "Step the simulator n steps [# of times] [# of steps]",
            namespace = "opal",
            doc = """
See short documentation.<br/>
""")

#
# Simulation based interfaces for opal
#


# CYCLE
def opal_sim_cycle(obj):
    SIM_get_attribute( obj, "cycle" )

new_command("cycle", opal_sim_cycle,
            [],
            alias = "",
            type  = "hfa commands",
            short = "returns the current cycle",
            namespace = "opal",
            doc = """
Returns the current cycle of the simulator.<br/>
""")

# STATS
def opal_sim_stats(obj):
    SIM_get_attribute( obj, "sim-stats" )

new_command("stats", opal_sim_stats,
            [],
            alias = "",
            type  = "hfa commands",
            short = "print out statistics for simulation",
            namespace = "opal",
            doc = """
Print out statistics for simulation.<br/>
""")

def opal_sim_inflight(obj):
    SIM_get_attribute( obj, "sim-inflight" )

new_command("inflight", opal_sim_inflight,
            [],
            alias = "",
            type  = "hfa commands",
            short = "print out inflight instructions",
            namespace = "opal",
            doc = """
Print out inflight instructions.<br/>
""")

# DEBUGGING
def opal_dbg_time(obj, procid, starttime):
    SIM_set_attribute( obj, "debugtime", [procid, starttime] )

new_command("debugtime", opal_dbg_time,
            [arg(int_t, "processor id"), arg(int_t, "time to start debugging at")],
            alias = "",
            type  = "hfa commands",
            short = "Start debug statements at a given time",
            namespace = "opal",
            doc = """
Start debug statements at a given time.<br/>
""")

# stall
def opal_stall_time(obj, stalltime):
    SIM_set_attribute( obj, "stall", stalltime )

new_command("stall", opal_stall_time,
            [arg(int_t, "Number of seconds to stall")],
            alias = "",
            type  = "hfa commands",
            short = "Stall for n seconds",
            namespace = "opal",
            doc = """
Stall for n seconds
""")

# toggle mh
def opal_togglemh(obj):
    SIM_set_attribute( obj, "togglemh", 1 )

new_command("togglemh", opal_togglemh,
            [],
            alias = "",
            type  = "hfa commands",
            short = "toggle the memory hier",
            namespace = "opal",
            doc = """
Toggle the memory hier
""")

# install the snoop device
def opal_install_snoop(obj):
    SIM_set_attribute( obj, "install-snoop", 1 )

new_command("install-snoop", opal_install_snoop,
            [],
            alias = "",
            type  = "hfa commands",
            short = "toggle the snoop device",
            namespace = "opal",
            doc = """
Toggles the snoop device interface.
""")

# passes a command to the new 'trace' interface
def opal_subcommand_trace(obj, value1, value2):
    SIM_set_attribute( obj, "mlp-trace", [value1, value2] )

new_command("mlp-trace", opal_subcommand_trace,
            [arg(str_t, "subcommand", "?", ""),
             arg(str_t, "subcommand_args", "?", "")],
            alias = "",
            type  = "",
            short = "Sends a command to opal's MP trace interface.",
            namespace = "opal",
            doc = """
Sends a command to opal's MP trace interface.
""")

# ADD_SIMCOMMAND
