#!/usr/bin/python3
import re

L1_DCACHE   = '/sys/devices/system/cpu/cpu0/cache/index0'
L1_ICACHE   = '/sys/devices/system/cpu/cpu0/cache/index1'
L2_CACHE    = '/sys/devices/system/cpu/cpu0/cache/index2'
L3_CACHE    = '/sys/devices/system/cpu/cpu0/cache/index3'

L1_DCACHE_SIZE          = 0
L1_DCACHE_LINE_SIZE     = 0
L1_DCACHE_ASSOCIATIVITY = 0

L1_ICACHE_SIZE          = 0
L1_ICACHE_LINE_SIZE     = 0
L1_ICACHE_ASSOCIATIVITY = 0

L2_CACHE_SIZE           = 0
L2_CACHE_LINE_SIZE      = 0
L2_CACHE_ASSOCIATIVITY  = 0

L3_CACHE_SIZE           = 0
L3_CACHE_LINE_SIZE      = 0
L3_CACHE_ASSOCIATIVITY  = 0

# check cache types are as assumed s.t. index0,1->L1, index2->L2, index3->L3
with open(f'{L1_DCACHE}/type') as f:
    type = re.search('[^\n]*', f.read()).group(0)
    assert(type.lower() == 'data')
with open(f'{L1_ICACHE}/type') as f:
    type = re.search('[^\n]*', f.read()).group(0)
    assert(type.lower() == 'instruction')
with open(f'{L2_CACHE}/type') as f:
    type = re.search('[^\n]*', f.read()).group(0)
    assert(type.lower() == 'unified')
with open(f'{L3_CACHE}/type') as f:
    type = re.search('[^\n]*', f.read()).group(0)
    assert(type.lower() == 'unified')
with open(f'{L1_DCACHE}/level') as f:
    level = int(re.search('[^\n]+', f.read()).group(0))
    assert(level == 1)
with open(f'{L1_ICACHE}/level') as f:
    level = int(re.search('[^\n]+', f.read()).group(0))
    assert(level == 1)
with open(f'{L2_CACHE}/level') as f:
    level = int(re.search('[^\n]+', f.read()).group(0))
    assert(level == 2)
with open(f'{L3_CACHE}/level') as f:
    level = int(re.search('[^\n]+', f.read()).group(0))
    assert(level == 3)


# L1 Data Cache Info
with open(f'{L1_DCACHE}/size', 'r') as f:
    s = f.read()
    L1_DCACHE_SIZE = int(re.search('[^a-zA-Z\n]*', s).group(0))
with open(f'{L1_DCACHE}/coherency_line_size', 'r') as f:
    s = f.read()
    L1_DCACHE_LINE_SIZE = int(s)
with open(f'{L1_DCACHE}/ways_of_associativity', 'r') as f:
    s = f.read()
    L1_DCACHE_ASSOCIATIVITY = int(s)

# L1 Instruction Cache Info
with open(f'{L1_ICACHE}/size', 'r') as f:
    s = f.read()
    L1_ICACHE_SIZE = int(re.search('[^a-zA-Z\n]*', s).group(0))
with open(f'{L1_ICACHE}/coherency_line_size', 'r') as f:
    s = f.read()
    L1_ICACHE_LINE_SIZE = int(s)
with open(f'{L1_ICACHE}/ways_of_associativity', 'r') as f:
    s = f.read()
    L1_ICACHE_ASSOCIATIVITY = int(s)

# L2 Cache Info
with open(f'{L2_CACHE}/size', 'r') as f:
    s = f.read()
    L2_CACHE_SIZE = int(re.search('[^a-zA-Z\n]*', s).group(0))
with open(f'{L2_CACHE}/coherency_line_size', 'r') as f:
    s = f.read()
    L2_CACHE_LINE_SIZE = int(s)
with open(f'{L2_CACHE}/ways_of_associativity', 'r') as f:
    s = f.read()
    L2_CACHE_ASSOCIATIVITY = int(s)

# L3 Cache Info
with open(f'{L3_CACHE}/size', 'r') as f:
    s = f.read()
    L3_CACHE_SIZE = int(re.search('[^a-zA-Z\n]*', s).group(0))
with open(f'{L3_CACHE}/coherency_line_size', 'r') as f:
    s = f.read()
    L3_CACHE_LINE_SIZE = int(s)
with open(f'{L3_CACHE}/ways_of_associativity', 'r') as f:
    s = f.read()
    L3_CACHE_ASSOCIATIVITY = int(s)

def pinfo(level, type, size, blocksize, associativity):
    print(f'Level {level} {type}Cache Size: {size}KB')
    print(f'Level {level} {type}Cache Line Size: {blocksize}B')
    print(f'Level {level} {type}Cache Associativity: {associativity} Way Assocaitive')
    print()

pinfo(1, "Data ", L1_DCACHE_SIZE, L1_DCACHE_LINE_SIZE, L1_DCACHE_ASSOCIATIVITY)
pinfo(1, "Instruction ", L1_ICACHE_SIZE, L1_ICACHE_LINE_SIZE, L1_ICACHE_ASSOCIATIVITY)
pinfo(2, "", L2_CACHE_SIZE, L2_CACHE_LINE_SIZE, L2_CACHE_ASSOCIATIVITY)
pinfo(3, "", L3_CACHE_SIZE, L3_CACHE_LINE_SIZE, L3_CACHE_ASSOCIATIVITY)

    
