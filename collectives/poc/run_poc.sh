#!/usr/bin/env python3
import sys, math, subprocess, urllib.request

nums = []
if (len(sys.argv) == 2):
    nums.append(int(sys.argv[1]))
else:
    start_num = 2
    end_num = 16
    step_num = 1
    if (len(sys.argv) > 2):
        start_num = int(sys.argv[1])
        end_num = int(sys.argv[2])
    if (len(sys.argv) > 3):
        step_num = int(sys.argv[3])
    for i in range(start_num, end_num + 1, step_num):
        nums.append(i)

with open('run_template.sh') as f:
    template = f.read()

def create_node_ring_linkconfig(n):
    conf = ''
    nodes = ['n{:02d}'.format(i) for i in range(n)]
    devices = ['acl{}'.format(2) for i in range(n)]
    for i in range(n):
        conf += '--fpgalink={}:{}:ch0-{}:{}:ch1 '.format(nodes[i-1], devices[i-1], nodes[i], devices[i])
    return conf

for num in nums:
    linkconfig = create_node_ring_linkconfig(num)
    print(linkconfig)
    script = template.format(num, math.ceil(num / 3), linkconfig, urllib.request.pathname2url(linkconfig))
    path = 'build/run_main_n{}.sh'.format(num)
    with open(path, 'w+') as f:
        f.write(script)
    subprocess.run(['sbatch', path])
