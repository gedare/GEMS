
import sys, re, string

"""
Compares two opal output files, filtering for differences that have to
do with machine timing. As with diff, the program only prints to standard
out if there are differences that are not filtered (e.g. output indicates
something is different, no output indicates things are the same).
"""

accept_diff = [ re.compile(".*Total Elapsed Time.*"),
                re.compile(".*Total Retirement Time.*"),
                re.compile(".*Approximate cycle per sec.*"),
                re.compile(".*Approximate instructions per sec.*"),
                re.compile(".*warning: unimplemented simics API.*"),
                re.compile("^closing file .*"),
                ]

if __name__ == "__main__":
    l = " "
    while l != "":
        l = sys.stdin.readline()
        if len(l) == 0:
            continue
        if l[0] == "<" or l[0] == ">":
            found = 0
            for i in range(len(accept_diff)):
                if accept_diff[i].search( l ):
                    found = 1
            if found == 0:
                print l
        elif l[0] == "-":
            # ignore separators
            pass
        elif l[0] in string.digits:
            # ignore line numbers for now
            pass
        else:
            print l

