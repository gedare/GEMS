
import re
import sys
import os
import shutil

file_pat = re.compile("\"\/p\/multifacet\/projects\/[^\"]*\"")
prefix_pat = re.compile("\/p\/multifacet\/projects\/")

def dump_new_check_file(checkpoint_file, name):

  success = 1

  try:
    f = open(checkpoint_file)

    line_list = f.readlines()
    new_line_list = []


    for line in line_list:
      new_line = prefix_pat.sub("/scratch/multifacet-u3-mirror/", line)
      new_line_list.append(new_line)

    print "writing %d lines into %s \n" % (len(new_line_list), name)
  
    newf = open(name, 'w')
    newf.writelines(new_line_list)
    newf.close()   

    print "created new file %s \n" % (name)
 
    f.close()
  except:
    success = 0
    pass

  return success   
    

def find_multifacet_files(checkpoint_file):

  f = open(checkpoint_file)

  file_list = []
  line_list = f.readlines()

  for line in line_list:
    match = file_pat.search(line)
    if match:
      filename = match.group()
      s = len(filename)
      filename = filename[1:(s-1)]
      file_list.append(filename)

  f.close()
  return file_list



  
def get_mfacet_base(file):

  #foo = re.sub("\/p\/multifacet\/projects\/", "", file)  

  foo = prefix_pat.sub("", file)
  return foo 


def is_copy_needed(afs_file, local_file):

  reason = ""

  if (not os.path.exists(afs_file) ):  
    reason = afs_file + " does not exist"
    return (1, reason)

  if (not os.path.exists(local_file)):
    reason = local_file + " does not exist"
    return (1, reason)

  if ( os.path.getmtime(afs_file) != os.path.getmtime(local_file)):
    reason = local_file + "'s last modification stamp does not match"
    return (1, reason)

  if ( os.path.getsize(afs_file) != os.path.getsize(local_file)):
    reason = local_file + "'s size does not match"
    return (1, reason)

  reason = local_file + " already exists and is up-to-date"

  return (0, reason) 


def prep_mirror_dir(path_prefix):
  # set the umask so that we can create world read/write files and directories
  os.umask(0)

  dir_name = os.path.dirname(path_prefix)
  try:
    os.makedirs(dir_name, 0777)
    print "created directories: %s" % dir_name
  except:
    pass

def do_copy_on_demand(checkpoint_file):

  old_umask = os.umask(0)

  run_from_afs = 0

  # open default .check file and extract list of image files
  files = find_multifacet_files(checkpoint_file)

  if (not len(files)):
    print "ERROR!  copy_on_demand couldn't find anything meaningful in %s\n" % (checkpoint_file)
    return 0 
 
  # update local disk if needed
  for file in files:

    path = os.path.dirname(file)

    mfacet_base = get_mfacet_base(file)
    newpath = os.path.join('/scratch/multifacet-u3-mirror/', mfacet_base)
  
    (copyNeeded, reasonStr) = is_copy_needed(file, newpath)

    if (copyNeeded):
      prep_mirror_dir(newpath)

      print reasonStr, "Copying"

      try:
        shutil.copy2(file, newpath)
        os.chmod(newpath, 0666)
      except(IOError, os.error), why:
        print "Can't copy %s to %s: %s" % (file, newpath, str(why))
        run_from_afs = 1
        break

  # reset umask so that written files aren't world writeable
  os.umask(old_umask)

  # Add the README file
  if (not run_from_afs):
    global read_me_file_text
    read_me_file = "/scratch/multifacet-u3-mirror/README"
    open(read_me_file, "w").write(read_me_file_text)
    try:
      os.chmod(read_me_file, 0666)
    except:
      # file already written by another user
      pass
  
  # if everything ok, see if -local.check exists and create if needed
  if (not run_from_afs):
    new_check_filename = re.sub("\.check", "-local.check", checkpoint_file)
    if (not os.path.exists(new_check_filename)):
      if ( dump_new_check_file(checkpoint_file, new_check_filename) ):
        return 1
      else:
        return 0
  else:
    return 0
   

read_me_file_text = """
The Multifacet group uses this directory as a local file cache for
large input files used in our Simics simulations.  Without caching
these files we've occasionally unintentionally brought AFS to its
knees (affecting the entire department).

If you need more space on this local disk, please contact us at
multifacet@cs.wisc.edu and we will prune some or all of these files.
"""
