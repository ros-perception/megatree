#! /usr/bin/env python
# Neatly prints out a node file.  This needs to be kept in sync with node_file.cpp

import sys
import StringIO

PRINT_BIG_ENDIAN=1
print "Using %s endian" % ("big" if PRINT_BIG_ENDIAN else "small")

def b2s(x):
    if PRINT_BIG_ENDIAN:
        return ''.join(["%02x" % ord(c) for c in reversed(x)])
    return ''.join(["%02x" % ord(c) for c in x])

def childrens(_x):
    x = ord(_x)
    return "(" + ''.join([str((x >> i) & 1) for i in reversed(range(8))]) + ")"

def id2s(id, width=0):
    full = 0
    for i, c in enumerate(id):
        full += ord(c) << (i * 8)
    return "%06o" % full

def dump(f):
    if type(f) == str:
        f = StringIO.StringIO(f)
    file_children = f.read(1)
    print "File children: %s   %s" % (b2s(file_children), childrens(file_children))
    i = 0
    print "         x    y    z    rgb     count             C  ShortId"
    while True:
        x = f.read(2)
        if x == "":
            break
        y = f.read(2)
        z = f.read(2)
        rgb = f.read(3)
        count = f.read(8)
        children = f.read(1)

        short_id = f.read(4)
        
        print "%6d:  %s %s %s %s  %s  %s %s" % \
            (i, b2s(x), b2s(y), b2s(z), b2s(rgb),
             b2s(count), b2s(children), id2s(short_id))
        i += 1

if len(sys.argv) > 1:
    with open(sys.argv[1], 'rb') as f:
        dump(f)
