import os
import sys
from random import *
import subprocess

def write_inputs(name, s1, s2):
    if not os.path.exists('inputs/' + name):
        try:
            os.mkdir('inputs/' + name)
        except FileExistsError:
            pass
    try:
        f1 = open('inputs/' + name + '/p0.txt', 'w')
        f1.write(s1)
        f1.close()
    except FileExistsError:
        pass
    try:
        f2 = open('inputs/' + name + '/p1.txt', 'w')
        f2.write(s2)
        f2.close()
    except FileExistsError:
        pass

def run(name, party, address):
    if os.path.exists('stats/' + name + '-p' + str(party) + '.txt'):
        print('=== Skipping ' + name + ' ===')
        return
    print('=== Running ' + name + ' ===')
    with open('inputs/' + name + '/p' + str(party) + '.txt', 'r') as f:
        subprocess.run(['./build/bin/msf', '-r', str(party), '-a', address, '-c', '8', '-f', 'stats/' + name + '-p' + str(party) + '.txt'], stdin=f, text=True)

def run_tsp(name, party, address):
    if os.path.exists('stats/' + name + '-1-p' + str(party) + '.txt'):
        print('=== Skipping ' + name + ' ===')
        return
    print('=== Running ' + name + ' ===')
    with open('inputs/tsp/' + name + '/p' + str(party) + '.txt', 'r') as f:
        subprocess.run(['./build/bin/msf', '-r', str(party), '-a', address, '-c', '8', '-f', 'stats/' + name + '-1-p' + str(party) + '.txt'], stdin=f, text=True)

def run_genmts(count, party, address):
    if os.path.exists('stats/genmt-' + str(count) + '-p' + str(party) + '.txt'):
        print('=== Skipping ' + str(count) + ' MTs ===')
        return
    if os.path.exists('pre_comp_client.dump'):
        try:
            os.remove('pre_comp_client.dump')
        except FileNotFoundError:
            pass
    if os.path.exists('pre_comp_server.dump'):
        try:
            os.remove('pre_comp_server.dump')
        except FileNotFoundError:
            pass
    print('=== Running ' + str(count) + ' MTs ===')
    subprocess.run(['./build/bin/msf', '-r', str(party), '-a', address, '-c', '8', '-f', 'stats/genmt-' + str(count) + '-p' + str(party) + '.txt', '-t', 'genots', '-n', str(count)], text=True)

def run_connectivity(size, number, party, address):
    if os.path.exists('stats/connctivity-' + str(size) + '-' + str(number) + '-p' + str(party) + '.txt'):
        print('=== Skipping ' + str(size) + ' x' + str(number) + ' Connectivity ===')
        return
    print('=== Running ' + str(size) + ' x' + str(number) + ' Connectivity ===')
    subprocess.run(['./build/bin/msf', '-r', str(party), '-a', address, '-c', '8', '-f', 'stats/connctivity-' + str(size) + '-' + str(number) + '-p' + str(party) + '.txt', '-t', 'connectivity', '-k', str(size), '-n', str(number)], text=True)

def run_subgraph(size, number, party, address):
    if os.path.exists('stats/subgraph-' + str(size) + '-' + str(number) + '-p' + str(party) + '.txt'):
        print('=== Skipping ' + str(size) + ' x' + str(number) + ' Subgraph ===')
        return
    print('=== Running ' + str(size) + ' x' + str(number) + ' Subgraph ===')
    subprocess.run(['./build/bin/msf', '-r', str(party), '-a', address, '-c', '8', '-f', 'stats/subgraph-' + str(size) + '-' + str(number) + '-p' + str(party) + '.txt', '-t', 'subgraph', '-k', str(size), '-n', str(number)], text=True)

def generate_unique(name,n,m,t):
    name = 'unique-' + str(n) + '-' + str(m) + '-' + str(t)
    seed(name)

    m1 = m // 2
    m2 = (m+1) // 2
    e = []
    for i in range(m):
        x = randrange(n)
        y = randrange(n)
        while x == y:
            x = randrange(n)
            y = randrange(n)
        e.append((x,y,i))
    shuffle(e)

    res1 = str(n) + " " + str(m1) + "\n"
    for i in range(m1):
        res1 += str(e[i][0]) + " " + str(e[i][1]) + " " + str(e[i][2]) + "\n"
    res2 = str(n) + " " + str(m2) + "\n"
    for i in range(m1,m):
        res2 += str(e[i][0]) + " " + str(e[i][1]) + " " + str(e[i][2]) + "\n"
    write_inputs(name, res1, res2)

def run_unique(n,m,t,party,address):
    name = 'unique-' + str(n) + '-' + str(m) + '-' + str(t)
    generate_unique(name,n,m,t)
    run(name, party, address)

def generate_bounded(name,n,m,w,t):
    seed(name)

    maxWeight = max(1, int(m * w))

    m1 = m // 2
    m2 = (m+1) // 2
    es = set()
    while len(es) < m:
        x = randrange(n)
        y = randrange(n)
        while x == y:
            x = randrange(n)
            y = randrange(n)
        w = randrange(maxWeight)
        es.add((x,y,w))
    e = list(es)
    shuffle(e)

    res1 = str(n) + " " + str(m1) + "\n"
    for i in range(m1):
        res1 += str(e[i][0]) + " " + str(e[i][1]) + " " + str(e[i][2]) + "\n"
    res2 = str(n) + " " + str(m2) + "\n"
    for i in range(m1,m):
        res2 += str(e[i][0]) + " " + str(e[i][1]) + " " + str(e[i][2]) + "\n"
    write_inputs(name, res1, res2)

def run_bounded(n,m,w,t,party,address):
    name = 'bounded-' + str(n) + '-' + str(m) + '-' + str(w) + '-' + str(t)
    generate_bounded(name,n,m,w,t)
    run(name, party, address)

if len(sys.argv) < 4:
    print('Usage: python3 run_msf.py [Party, i.e., 0 or 1] [Address: i.e., 127.0.0.1 or the ip address of the other party] [Operation: msf/mt/connectivity/subgraph]')
    sys.exit(0)

party = int(sys.argv[1])
address = sys.argv[2]
op = sys.argv[3]

if not os.path.exists('inputs'):
    os.mkdir('inputs')
if not os.path.exists('stats'):
    os.mkdir('stats')

if op == 'msf':
    if len(sys.argv) < 5:
        print('Usage:       python3 run_msf.py [Party] [Address] msf [N] [M] [W: float or "unique"] [T]')
        print('                  This will run the MSF protocol for N vertices, M random edges with edge weights that are either unique or chosen randomly between 1 and W*M, and T is a seed')
        print('Alternative: python3 run_msf.py [Party] [Address] msf all')
        print("                  This will run tests for a variety of N's, M's, W's, and T's")
        print('Alternative: python3 run_msf.py [Party] [Address] msf tsp')
        print("                  This will run tests for a variety of TSP graphs")
        sys.exit(0)
    if sys.argv[4] == 'all':
        N = [10,20,50,100,200,500,1000,2000,5000,10000,20000,50000,100000,200000]
        M = [3,6]
        T = [1,2,3]
        W = [1.0,0.5,0.2,0.1,0.05]

        unique = []
        for n in N:
            for m in M:
                for t in T:
                    unique.append((n,m*n,t))

        bounded = []
        for w in W:
            for (n,m,t) in unique:
                bounded.append((n,m,w,t))

        for (n,m,t) in unique:
            #if party == 1:
            #    time.sleep(5)
            run_unique(n,m,t,party,address)

        for (n,m,w,t) in bounded:
            #if party == 1:
            #    time.sleep(5)
            run_bounded(n,m,w,t,party,address)
    elif sys.argv[4] == 'tsp':
        for s in ['berlin52', 'brg180', 'gr666', 'nrw1379', 'pcb1173']:
            run_tsp(s,party,address)
    else:
        n = int(sys.argv[4])
        m = int(sys.argv[5])
        t = int(sys.argv[7])
        if sys.argv[6] == 'unique':
            run_unique(n,m,t,party,address)
        else:
            run_bounded(n,m,float(sys.argv[6]),t,party,address)
elif op == 'mt':
    if len(sys.argv) < 5:
        print('Usage: python3 run_msf.py [Party] [Address] mt [Count: any integer, or "all" in order to repeat this for a predefined list of numbers]')
        sys.exit(0)
    if sys.argv[4] == 'all':
        N = []
        for i in [10,100,1000,10000,100000,1000000,10000000,100000000,1000000000]:
            for m in [1,2,5]:
                if i*m > 4000000000:
                    continue
                N.append(i*m)
        N.append(4000000000)
        for n in N:
            #if party == 1:
            #    time.sleep(1)
            run_genmts(n, party, address)
    else:
        run_genmts(int(sys.argv[4]), party, address)
elif op == 'connectivity':
    if len(sys.argv) < 5:
        print('Usage:       python3 run_msf.py [Party] [Address] connectivity N M')
        print('                  This will run the Connectivity sub-protocol with input size N, on M instances at the same time.')
        print('Alternative: python3 run_msf.py [Party] [Address] connectivity all')
        print("                  This will run tests for a variety of N's and M's")
        sys.exit(0)
    if sys.argv[4] == 'all':
        N = list(range(1,151))
        for count in [1,10,100,1000]:
            for n in N:
                run_connectivity(n, count, party, address)
    else:
        n = int(sys.argv[4])
        count = int(sys.argv[5])
        run_connectivity(n, count, party, address)
elif op == 'subgraph':
    if len(sys.argv) < 5:
        print('Usage:       python3 run_msf.py [Party] [Address] subgraph N M')
        print('                  This will run the IsolatedMSF sub-protocol with input size N, on M instances at the same time.')
        print('Alternative: python3 run_msf.py [Party] [Address] subgraph all')
        print("                  This will run tests for a variety of N's and M's")
        sys.exit(0)
    if sys.argv[4] == 'all':
        for count in [1,10,100,1000]:
            if count <= 10:
                maxN = 60
            elif count == 10:
                maxN = 48
            else:
                maxN = 22
            for n in range(1,maxN+1):
                run_subgraph(n, count, party, address)
    else:
        n = int(sys.argv[4])
        count = int(sys.argv[5])
        run_subgraph(n, count, party, address)
else:
    print('Unknown operation.')
