import string, os, sys, time, popen2

class RegressionError(Exception):
    def __init__(self, command, output):
        self.command = command
        limit = 10000
        if(len(output) > limit):
          self.output = output[:limit/2] + "\n\n...TRUNCATED...\n\n" + output[-limit/2:]
        else:
          self.output = output
    def __str__(self):
        return "RegressionError:" + `self.command`

g_default_search_path = os.environ["PATH"]

def set_default_search_path(path):
  global g_default_search_path
  assert(path != "")
  g_default_search_path = path
  return

def send_mail(body, from_addr, to_list, subject):
    import smtplib
    out_msg = ("From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n" % (from_addr, string.join(to_list, ", "), subject))
    out_msg = out_msg + body

    server = smtplib.SMTP('smtp.cs.wisc.edu')
#    server.set_debuglevel(1)
    server.sendmail(from_addr, to_list, out_msg)
    server.quit()

def format_time(initial_seconds):
    total_seconds = initial_seconds

    # days
    days = int(total_seconds) / 86400
    total_seconds -= (days * 86400.0)

    # hours
    hours = int(total_seconds) / 3600
    total_seconds -= (hours * 3600.0)

    # minutes
    minutes = int(total_seconds) / 60
    total_seconds -= (minutes * 60.0)

    # seconds
    seconds = total_seconds
    
    # format output
    output = ""
    if initial_seconds >= 86400:
        output += ("%dd" % days)
    if initial_seconds >= 3600:
        output += ("%dh" % hours)
    if initial_seconds >= 60:
        output += ("%dm" % minutes)
    output += ("%.2fs" % seconds)
    return output

def run_command(command_string, input_string="", max_lines=0, verbose=0, echo=1, throw_exception=1):
    assert(g_default_search_path != "")
    os.environ["PATH"] = g_default_search_path
    if echo:
        print "running:", command_string
    obj = popen2.Popen4(command_string)
    output = ""

    obj.tochild.write(input_string)
    obj.tochild.close()
    line = obj.fromchild.readline()
    while (line):
        if verbose == 1:
            print line,
        output += line
        line = obj.fromchild.readline()
    exit_status = obj.wait()

    if(max_lines != 0):
        lines = output.split("\n");
        output = string.join(lines[-max_lines:], "\n")

    if throw_exception and exit_status != 0:
        raise RegressionError(command_string, output)
    return output

def walk(path):
    if os.path.isfile(path):
        return [path]
    elif os.path.isdir(path):
        filelist = []
        for file in os.listdir(path):
            filelist += walk(os.path.join(path, file))
        return filelist
    else:
        pass
