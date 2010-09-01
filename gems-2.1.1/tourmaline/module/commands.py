# Based HEAVILY on Ruby's commands.py
# Commands are not currently implemented by the Tourmaline module

from cli import *

# list format:
#    COMMAND_NAME  ARGUMENT_LIST  PYTHON_COMMAND  DOCS

tourmaline_command_list = [
### Parameter interfaces
    [ "init", [arg(str_t, "param-file", "?", "")], 
      "Read a configuration file." ],
#    [ "readparam", [arg(str_t, "param-file", "?", "")], 
#      "Just reads a configuration file." ],
#    [ "saveparam", [arg(str_t, "param-file", "?", "")], 
#      "Saves the current configuration to a file." ],
]

###
### Main loop to create tourmaline commands
###
for tourmaline_cmd in tourmaline_command_list:

    # create a python command
    def tourmaline_command( obj, arg=1, attr=tourmaline_cmd[0] ):
        SIM_set_attribute( obj, attr, arg )

    new_command( tourmaline_cmd[0], tourmaline_command,
                 tourmaline_cmd[1],
                 alias = "",
                 type = "tourmaline command",
                 short = tourmaline_cmd[2],
                 namespace = "tourmaline",
                 doc = tourmaline_cmd[2] )

#
# parameter commands
#

# lists tourmaline parameters
def tourmaline_list_param(obj):
    SIM_set_attribute( obj, "param", 1 )

new_command("listparam", tourmaline_list_param,
            [],
            alias = "",
            type  = "commands",
            short = "list configuration parameters",
            namespace = "tourmaline",
            doc = """
List configuration parameters.<br/>
""")

# set tourmaline parameters (integer, and string)
def tourmaline_set_param(obj, name, value):
    SIM_set_attribute( obj, "param", [name, value] )
new_command("setparam", tourmaline_set_param,
            [arg(str_t, "name"), arg(int_t, "value")],
            alias = "",
            type  = "commands",
            short = "list configuration parameters",
            namespace = "tourmaline",
            doc = """
List configuration parameters.<br/>
""")

def tourmaline_set_param_str(obj, name, value):
    SIM_set_attribute( obj, "param", [name, value] )
new_command("setparam_str", tourmaline_set_param_str,
            [arg(str_t, "name"), arg(str_t, "value")],
            alias = "",
            type  = "commands",
            short = "list configuration parameters",
            namespace = "tourmaline",
            doc = """
List configuration parameters.<br/>
""")

#
# statistics commands
#

def tourmaline_dump_stats(obj, filename):
    SIM_set_attribute(obj, "dump-stats", filename)
       
new_command("dump-stats", tourmaline_dump_stats,
            args = [arg(str_t, "filename", "?", "")],
            type = "multifacet commands",
            namespace = "tourmaline",
            short = "dumps the collected statistics",
            doc = """
<b>dump-stats</b> dumps the collected statistics from the instrumentation module
""")

def tourmaline_clear_stats(obj):
    SIM_set_attribute(obj, "clear-stats", 0)
       
new_command("clear-stats", tourmaline_clear_stats,
            args = [],
            type = "multifacet commands",
            namespace = "tourmaline",
            short = "clears the collected statistics",
            doc = """
<b>clear-stats</b> clears the collected statistics from the instrumentation module
""")

