
branchstr = ["fba", "ba", "fbpa", "bpa", "fbn", "fbpn", "bn", "bpn", "bpne", "bpe", "bpg", "bple", "bpge", "bpl", "bpgu", "bpleu", "bpcc", "bpcs", "bppos", "bpneg", "bpvc", "bpvs", "brz", "brlez", "brlz", "brnz", "brgz", "brgez", "fbu", "fbg", "fbug", "fbl", "fbul", "fblg", "fbne", "fbe", "fbue", "fbge", "fbuge", "fble", "fbule", "fbo", "fbpu", "fbpg", "fbpug", "fbpl", "fbpul", "fbplg", "fbpne", "fbpe", "fbpue", "fbpge", "fbpuge", "fbple", "fbpule", "fbpo", "bne", "be", "bg", "ble", "bge", "bl", "bgu", "bleu", "bcc", "bcs", "bpos", "bneg", "bvc", "bvs", "retl"]

#
# parse a disassembly file, find basic blocks & output format in DOT
#
import sys, os, string

class  instruction:
    def __init__( self, address, str ):
        self.address = address
        self.str     = str
        self.opcode  = str[0:13].strip()
        ops = str[13:].split(",")
        for op in ops:
            op.strip()

        self.src     = []
        self.dest    = ""
        self.regs    = []
        self.const   = []
        self.branch  = 0

        op = self.opcode.split(",")[0]
        if op[0] == "b" or op[0:1] == "fb" or op[0] == "r":
            if op in branchstr:
                self.branch = 1
                try:
                    self.target = long(ops[-1], 16)
                except:
                    self.target = None

    def isbranch( self ):
        return self.branch

class  block:
    def __init__(self):
        self.list = []
        self.next = None

    def append( self, inst ):
        self.list.append( inst )
        if inst.isbranch():
            self.next = inst.target

class  function:
    def __init__(self):
        self.amap = {}
        self.blockmap  = {}

    def addinst(self, inst):
        self.amap[inst.address] = inst

    def buildbasicblocks( self ):
        ksort = self.amap.keys()
        ksort.sort()

        targets = []
        for key in ksort:
            inst = self.amap[key]
            if inst.isbranch():
                targets.append( inst.target )

        delaycount = 0
        for key in ksort:
            inst = self.amap[key]
            # make a new basic block (if none exists)
            if delaycount == 0 or inst.address in targets:
                prevblock = block()
                self.blockmap[inst.address] = prevblock
                delaycount = 1000
            # append instruction to basic block
            prevblock.append( inst )
            delaycount = delaycount - 1
            if inst.isbranch():
                delaycount = 1

    def graph(self, filename):
        f = open( filename, 'w' )
        f.write( "digraph map {\n" )

        bsort = self.blockmap.keys()
        bsort.sort()
        f.write( "node [fontname = \"-adobe-courier-medium-r-normal--10-140-75-75-m-90-iso8859-1\" fontsize=10 shape=box];\n" )
        f.write( "rotate=90;\n")
        f.write( "page=\"8.5,11\";\n")
        f.write( "margin=\"0.25\";\n")
        #f.write( "size=\"8.5,11\";\n")

        prevblock = None
        for baddr in bsort:
            block = self.blockmap[baddr]
            f.write( "     a%x [label=\"" % (baddr) )
            for inst in block.list:
                f.write( "%6x %s          \\l" % (inst.address, inst.str) )
            f.write ("\"];\n")
            if prevblock != None:
                f.write( "a%x -> a%x;\n" % (prevblock, baddr) )
            prevblock = baddr
            if block.next != None:
                f.write( "a%x -> a%x;\n" % (baddr, block.next) )

        f.write( "}\n" )
        f.close()
        
    def graphinst(self, filename):
        f = open( filename, 'w' )
        f.write( "digraph map {\n" )
        
        ksort = self.amap.keys()
        ksort.sort()
        for key in ksort:
            inst = self.amap[key]
            f.write( "     a%x [label=\"%s\" shape=box];\n" % (inst.address, inst.str) )
            #print "%10x: " % (inst.address), inst.regs, inst.dest,

        prev = 0
        for key in ksort:
            if prev == 0:
                f.write( "a%x" % self.amap[key].address )
                prev = 1
            else:
                f.write( "-> a%x" % self.amap[key].address )
        f.write(";\n")
        f.write( "}\n" )
        f.close()
        
cfgroot = function()
def readFile( filename ):
    f = open( filename, 'r' )
    l = "init"
    while l != "":
        l = f.readline()
        if l == "":
            break

        # parse address, instruction
        colon   = l.find(":")
        address = long(l[0:colon], 16)
        inst    = l[35:-1]
        inst    = instruction( address, inst )
        cfgroot.addinst( inst )
        
    f.close()

if __name__ == "__main__":
    if len( sys.argv ) <= 1:
        print "usage: dis filename"
        sys.exit(1)

    filename = sys.argv[1]
    readFile( filename )
    cfgroot.buildbasicblocks()
    cfgroot.graph( filename + "-dot" )
