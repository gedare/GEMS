from mod_xterm_console_commands import wait_for_string

def build_dynamic_web():
	hostpath = "/host" + os.getcwd() + "/../../../opal/benchmark/cache/"
	lines = (
		 "mount /host\n",
		 "cp " + hostpath + "ub.bin /space\n",
		 "umount /host\n",
		 "cd /space\n",
		 "chmod +x ub.bin\n",
		 "nice --20 ./ub.bin &\n",
		 "sleep 5\n", # will until cpu is occupied
		 "magic_call break\n"
		 )

	if SIM_number_processors() == 1 or SIM_number_processors() == 2:
		for i in lines:
			wait_for_string(conf.xterm_s6a, "#")
			conf.xterm_s6a.input = i
	else:
		for i in lines:
			wait_for_string(conf.xterm_s0a, "#")
			conf.xterm_s0a.input = i

start_branch(build_dynamic_web)

