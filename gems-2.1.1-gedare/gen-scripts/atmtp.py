# Copyright (C) 2007 Sun Microsystems, Inc.  All rights reserved.
# U.S. Government Rights - Commercial software.  Government users are
# subject to the Sun Microsystems, Inc. standard license agreement and
# applicable provisions of the FAR and its supplements.  Use is
# subject to license terms.  This distribution may include materials
# developed by third parties.Sun, Sun Microsystems and the Sun logo
# are trademarks or registered trademarks of Sun Microsystems, Inc. in
# the U.S. and other countries.  All SPARC trademarks are used under
# license and are trademarks or registered trademarks of SPARC
# International, Inc. in the U.S.  and other countries.
#
# ----------------------------------------------------------------------
#
# This file is part of the Adaptive Transactional Memory Test Platform
# (ATMTP) developed and maintained by Kevin Moore and Dan Nussbaum of
# the Scalable Synchronization Research Group at Sun Microsystems
# Laboratories (http://research.sun.com/scalable/).  For information
# about ATMTP, see the GEMS website: http://www.cs.wisc.edu/gems/.
#
# Please send email to atmtp-interest@sun.com with feedback,
# questions, or to request future announcements about ATMTP.
#
# ----------------------------------------------------------------------
#
# ATMTP is distributed as part of the GEMS software toolset and is
# available for use and modification under the terms of version 2 of
# the GNU General Public License.  The GNU General Public License is
# contained in the file $GEMS/LICENSE.
#
# Multifacet GEMS is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# Multifacet GEMS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the Multifacet GEMS; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
# USA
#
# ----------------------------------------------------------------------

import time, workloads, sim_commands, string, sys, getpass, os
import mod_xterm_console_commands
from cli import *
import mfacet

config_map = {}
config_map[1] = 6  #  1p
config_map[2] = 7  #  2p
config_map[3] = 8  #  3p
config_map[4] = 9  #  4p
config_map[8] = 10 #  6p
config_map[8] = 11 #  8p
config_map[8] = 12 # 12p
config_map[8] = 13 # 16p
config_map[8] = 14 # 24p
config_map[8] = 15 # 32p
config_map[8] = 16 # 48p
config_map[8] = 17 # 64p

def start_atmtp(debug=0):
  ###### set simics parameters
  # enable/disable STCs here
  mfacet.run_sim_command("istc-disable")
  mfacet.run_sim_command("dstc-disable")
  mfacet.run_sim_command("stc-status")
  
  # disable breakpoint before we sync all processors
  mfacet.run_sim_command('magic-break-disable')
  
  # always use 1 for better parallelism
  old_switch_time = conf.sim.cpu_switch_time
  print "Old cpu_switch_time: %d" % conf.sim.cpu_switch_time
  mfacet.run_sim_command("cpu-switch-time")
  conf.sim.cpu_switch_time = 1
  if (SIM_number_processors() == 1):
    delay_time = 1
  else:
    delay_time = old_switch_time
  mfacet.run_sim_command("c %d"%(delay_time)) # sync CPUs after changing switch time
  mfacet.run_sim_command("cpu-switch-time")
  print "New cpu_switch_time: %d" % conf.sim.cpu_switch_time
  
  # enable breakpoint
  mfacet.run_sim_command('magic-break-enable')

  ###### Conserve memory by limiting how much memory each simics image object can use
  #mfacet.limit_all_images(256)

  # IFETCH
  # setting instruction-fetch-mode to
  # instruction-fetch-trace slows down simulation too
  # much to be worth the added accuracy that it
  # affords.
  #
  # to enable IFETCH, uncomment the lines below:
  # (i-fetch line size is controlled in checkpoint files)
  #
  #- print "Old instruction-fetch-mode: "
  #- mfacet.run_sim_command("instruction-fetch-mode")
  #- mfacet.run_sim_command("instruction-fetch-mode instruction-fetch-trace")
  #- print "New instruction-fetch-mode: "
  #- mfacet.run_sim_command("instruction-fetch-mode")

  ###### Load Ruby and set parameters
  mfacet.run_sim_command("load-module ruby")
  mfacet.run_sim_command("ruby0.setparam_str REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH true")
  mfacet.run_sim_command("ruby0.setparam_str PROFILE_EXCEPTIONS false")
  mfacet.run_sim_command("ruby0.setparam_str PROFILE_XACT false")
  mfacet.run_sim_command("ruby0.setparam_str PROFILE_NONXACT false")
  mfacet.run_sim_command("ruby0.setparam_str XACT_MEMORY true")
  mfacet.run_sim_command("ruby0.setparam g_DEADLOCK_THRESHOLD 20000000")
  mfacet.run_sim_command("ruby0.setparam g_MEMORY_SIZE_BYTES 8589934592")
  mfacet.run_sim_command("ruby0.setparam g_RANDOM_SEED %d" % 0)
  mfacet.run_sim_command("ruby0.setparam g_PAGE_SIZE_BYTES %d" % 8192)

# memory control
#  mfacet.run_sim_command("ruby0.setparam MEM_BUS_CYCLE_MULTIPLIER %d" % 1)
#  mfacet.run_sim_command("ruby0.setparam BANKS_PER_RANK %d" % 1)
#  mfacet.run_sim_command("ruby0.setparam RANKS_PER_DIMM %d" % 1)
#  mfacet.run_sim_command("ruby0.setparam DIMMS_PER_CHANNEL %d" % 1)
#  mfacet.run_sim_command("ruby0.setparam BANK_QUEUE_SIZE %d" % 100)
  
  mfacet.run_sim_command("ruby0.setparam_str ATMTP_ENABLED true")
  mfacet.run_sim_command("ruby0.setparam_str ATMTP_ABORT_ON_NON_XACT_INST true")
  mfacet.run_sim_command("ruby0.setparam_str ATMTP_ALLOW_SAVE_RESTORE_IN_XACT false")
  mfacet.run_sim_command("ruby0.setparam ATMTP_XACT_MAX_STORES 32")
  mfacet.run_sim_command("ruby0.setparam ATMTP_DEBUG_LEVEL 1")
  mfacet.run_sim_command("ruby0.setparam_str XACT_DEBUG false")
  mfacet.run_sim_command("ruby0.setparam XACT_DEBUG_LEVEL %d" % 1)
  # Need to set this for CMPs!
  mfacet.run_sim_command("ruby0.setparam g_PROCS_PER_CHIP %d" % SIM_number_processors())
  mfacet.run_sim_command("ruby0.setparam g_NUM_SMT_THREADS %d" % 1)
  # Number of outstanding requests from each sequencer object (default is 16)
  mfacet.run_sim_command("ruby0.setparam g_SEQUENCER_OUTSTANDING_REQUESTS %d" % 16)

  # Comment this out for production/faster runs
  mfacet.run_sim_command("ruby0.setparam_str XACT_ISOLATION_CHECK true")

  # Turn off store-predictor
  mfacet.run_sim_command("ruby0.setparam XACT_STORE_PREDICTOR_ENTRIES 0");
  mfacet.run_sim_command("ruby0.setparam XACT_STORE_PREDICTOR_HISTORY 0");
  mfacet.run_sim_command("ruby0.setparam XACT_STORE_PREDICTOR_THRESHOLD 0");
  
  # set multipliers here
  mfacet.run_sim_command("ruby0.setparam SIMICS_RUBY_MULTIPLIER %d" % 1)
  mfacet.run_sim_command("ruby0.setparam OPAL_RUBY_MULTIPLIER %d" % 1)
  
  # 32 kB 4-way L1 cache
  mfacet.run_sim_command("ruby0.setparam L1_CACHE_ASSOC %d" % 4)
  mfacet.run_sim_command("ruby0.setparam L1_CACHE_NUM_SETS_BITS %d" % 7)
  mfacet.run_sim_command("ruby0.setparam SEQUENCER_TO_CONTROLLER_LATENCY %d" % 1) # L1 hit time

  # 8 MB 8-way L2 cache
  mfacet.run_sim_command("ruby0.setparam L2_CACHE_ASSOC %d" % 8)
  mfacet.run_sim_command("ruby0.setparam L2_CACHE_NUM_SETS_BITS %d" % 14)
  mfacet.run_sim_command("ruby0.setparam L2_RESPONSE_LATENCY %d" % 20)
  mfacet.run_sim_command("ruby0.setparam L2_TAG_LATENCY %d" % 6)
  mfacet.run_sim_command("ruby0.setparam L2_REQUEST_LATENCY %d" % 15)

  # Main memory
  mfacet.run_sim_command("ruby0.setparam MEMORY_RESPONSE_LATENCY_MINUS_2 %d" % 448)
  
  # interconnection network parameters
  mfacet.run_sim_command("ruby0.setparam NETWORK_LINK_LATENCY %d" % 14)

  # network topology
  #
  # Use PT_TO_PT so we can vary the number of processors easily.
  #
  mfacet.run_sim_command("ruby0.setparam_str g_NETWORK_TOPOLOGY PT_TO_PT")

  mfacet.run_sim_command("ruby0.setparam RECYCLE_LATENCY %d" % 1)
  mfacet.run_sim_command("ruby0.setparam NUMBER_OF_VIRTUAL_NETWORKS %d" % 5)

  if (SIM_number_processors() <= 16):
    mfacet.run_sim_command("ruby0.setparam g_NUM_MEMORIES %d" % 8)
  else:
    mfacet.run_sim_command("ruby0.setparam g_NUM_MEMORIES %d" % 16)

  mfacet.run_sim_command("ruby0.setparam L2CACHE_TRANSITIONS_PER_RUBY_CYCLE %d" % 1000)
  mfacet.run_sim_command("ruby0.setparam DIRECTORY_TRANSITIONS_PER_RUBY_CYCLE %d" % 1000)
  mfacet.run_sim_command("ruby0.setparam XACT_LOG_BUFFER_SIZE %d" % 1024)

  ## Read/Write Filter options
  # Physical signatures
  mfacet.run_sim_command("ruby0.setparam_str PERFECT_FILTER true")
  
  # Virtual signatures
  mfacet.run_sim_command("ruby0.setparam_str PERFECT_VIRTUAL_FILTER true")
  
  # Summary signatures
  mfacet.run_sim_command("ruby0.setparam_str PERFECT_SUMMARY_FILTER true")
  
  mfacet.run_sim_command("ruby0.setparam_str XACT_VISUALIZER false")
  mfacet.run_sim_command("ruby0.setparam_str XACT_LAZY_VM true")
  mfacet.run_sim_command("ruby0.setparam_str XACT_EAGER_CD true")

  mfacet.run_sim_command("ruby0.setparam_str XACT_NO_BACKOFF true")
  mfacet.run_sim_command("ruby0.setparam_str ENABLE_MAGIC_WAITING false")
  mfacet.run_sim_command("ruby0.setparam_str XACT_ENABLE_VIRTUALIZATION_LOGTM_SE false")
  mfacet.run_sim_command("ruby0.setparam_str ENABLE_WATCHPOINT false")
  mfacet.run_sim_command("ruby0.setparam XACT_FIRST_ACCESS_COST %d" % 0)
  mfacet.run_sim_command("ruby0.setparam XACT_FIRST_PAGE_ACCESS_COST %d" % 0)
              
  mfacet.run_sim_command("ruby0.init")

  if debug > 0:
    mfacet.run_sim_command('ruby0.debug-verb high')
    mfacet.run_sim_command('ruby0.debug-filter lsNqST')
    mfacet.run_sim_command('ruby0.debug-start-time "%d"' % debug)
    mfacet.run_sim_command('ruby0.debug-output-file /scratch/ruby')
