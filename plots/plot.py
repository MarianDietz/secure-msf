import matplotlib.pyplot as plt
import os

party = 0
N = [50,100,200,500,1000,2000,5000,10000,20000,50000,100000,200000]

laud_3 = { # time in seconds for laud with |E|=3|V|
    50: 3,
    100: 6,
    200: 12,
    500: 30,
    1000: 60,
    2000: 120,
    5000: 300,
    10000: 600,
    20000: 1300,
    50000: 3400,
    100000: 8000,
    200000: 20000
}

laud_6 = {
    50: 4,
    100: 9,
    200: 18,
    500: 45,
    1000: 95,
    2000: 200,
    5000: 500,
    10000: 1000,
    20000: 2200,
    50000: 7000,
    100000: 15000,
    200000: 32000
}

laud_tsp = {
    'berlin52': 10,
    'brg180': 100,
    'gr666': 2000,
    'pcb1173': 7000,
    'nrw1379': 9000,
}

time_per_mt = {
    'lan': 0.0000000531899,
    'wan': 0.000000321851
}

def sum_lists(a, b):
    while len(a) < len(b):
        a.append(0)
    while len(a) > len(b):
        b.append(0)
    return list([x+y for (x,y) in zip(a, b)])

def math_label(xx):
    s = xx
    e = 0
    while xx % 10 == 0:
        e += 1
        xx //= 10
    if e <= 1:
        return s
    if xx == 1:
        return '$10^{' + str(e) + '}$'
    return '$' + str(xx) + ' \\cdot 10^{' + str(e) + '}$'

def convert_subgraphs(l):
    m = {}
    largest = 0
    for s in l:
        size = int(s.split(':')[0])
        count = int(s.split(':')[1])
        m[size] = count*(size-1)
        largest = max(largest, size)
    res = [0 for _ in range(largest-1)]
    for (x,y) in m.items():
        res[x-2] = y
    return res

def read_stats(network, name, t):
    with open('stats_' + network + '/' + name + '-' + str(t) + '-p' + str(party) + '.txt') as f:
        stats = f.read()
    stats = stats.splitlines()
    #print(stats)
    stats_iterations = int(stats[0].split()[1])
    stats_phase1 = [x/1000 for x in list(map(float, stats[1].split()[1:]))]
    comm_phase1 = int(stats[2].split()[1]) + int(stats[2].split()[2])
    stats_phase2 = [x/1000 for x in list(map(float, stats[3].split()[1:]))]
    comm_phase2 = int(stats[4].split()[1]) + int(stats[4].split()[2])
    stats_sum = sum_lists(stats_phase1, stats_phase2)
    stats_mults = int(stats[5].split()[1]) + int(stats[5].split()[2]) + int(stats[5].split()[3])
    stats_subgraphs = convert_subgraphs(stats[6].split()[1:])
    return {
        'iterations': stats_iterations,
        'timing_phase1': stats_phase1,
        'timing_phase2': stats_phase2,
        'timing_sum': stats_sum,
        'mults': stats_mults,
        'subgraphs': stats_subgraphs,
        'comm': comm_phase1 + comm_phase2
    }

def read_stats_average(network, name):
    res = {
        'iterations': 0,
        'timing_phase1': [0,0,0,0],
        'timing_phase2': [0,0,0,0],
        'timing_sum': [0,0,0,0],
        'mults': 0,
        'subgraphs': [],
        'comm': 0
    }
    for t in [1,2,3]:
        for (x,y) in read_stats(network, name, t).items():
            if isinstance(y, list):
                res[x] = sum_lists(res[x], y)
            else:
                res[x] += y
    for x in res:
        if isinstance(res[x], list):
            res[x] = [ a/3 for a in res[x] ]
        else:
            res[x] = res[x]/3
    return res

def get_stats_unique(network, n, m):
    return read_stats_average(network, 'unique-' + str(n) + '-' + str(m))

def get_stats_bounded(network, n, m, w):
    if w == 'unique':
        return get_stats_unique(network, n, m)
    return read_stats_average(network, 'bounded-' + str(n) + '-' + str(m) + '-' + str(w))

def get_stats_ot(network, n):
    with open('stats_' + network + '/genmt-' + str(n) + '-p' + str(party) + '.txt') as f:
        stats = f.read()
    stats = stats.splitlines()
    return {
        'time': float(stats[0].split()[1])/1000
    }

def get_stats_connectivity(network, size, count):
    with open('stats_' + network + '/connctivity-' + str(size) + '-' + str(count) + '-p' + str(party) + '.txt') as f:
        stats = f.read()
    stats = stats.splitlines()
    return {
        'time': float(stats[0].split()[1])/1000,
        'mults': float(stats[1].split()[1])
    }

def get_stats_subgraph(network, size, count):
    with open('stats_' + network + '/subgraph-' + str(size) + '-' + str(count) + '-p' + str(party) + '.txt') as f:
        stats = f.read()
    stats = stats.splitlines()
    return {
        'time': float(stats[0].split()[1])/1000,
        'mults': float(stats[1].split()[1])
    }

def plot_time(network, w, mfactor):
    x = N
    y1 = []
    y2 = []
    for n in x:
        y1.append(get_stats_bounded(network, n, mfactor*n, w)['timing_phase1'][0])
        y2.append(get_stats_bounded(network, n, mfactor*n, w)['timing_phase2'][0])

    fig,ax = plt.subplots()
    ax.plot(x, y1)
    ax.plot(x, y2)
    ax.grid()

    ax.set_ylabel('Time (s)')
    ax.set_xlabel('|V|')
    plt.xscale('log')
    plt.yscale('log')
    plt.xticks(x, x, rotation=90)

    plt.savefig('results/time-' + str(w) + '-' + str(mfactor) + '-' + network + '.png')
    plt.show()

def plot_time_multi(network, W, mfactor):
    x = N
    y = []
    for w in W:
        ycur = []
        for n in x:
            ycur.append(get_stats_bounded(network, n, mfactor*n, w)['timing_sum'][0])
        y.append(ycur)

    fig,ax = plt.subplots()
    for (ycur,w) in zip(y,W):
        if w == 'unique':
            label = 'unique weights'
        else:
            label = 'w=' + str(w)
        ax.plot(x, ycur, label=label)
    ax.grid()

    ax.set_ylabel('Time (s)')
    ax.set_xlabel('|V|')
    plt.xscale('log')
    plt.yscale('log')
    plt.xticks(x, x, rotation=90)
    ax.tick_params(axis='x', which='minor', bottom=False)

    #plt.tight_layout()
    fig.set_size_inches(6.4, 6)
    fig.subplots_adjust(bottom=0.25, top=0.97, left=0.15, right=0.98)
    plt.savefig('results/time-multi-' + str(mfactor) + '-' + network + '.png')
    #plt.show()

    ax.legend(loc='best')
    label_params = ax.get_legend_handles_labels() 
    figl, axl = plt.subplots()
    axl.axis(False)
    figl.set_size_inches(11,0.5)
    axl.legend(*label_params, ncol=len(W), loc="center")
    figl.savefig('results/legend.png')

def plot_time_withlaud(network, w, mfactor):
    x = N
    yonline = []
    ycombined = []
    for n in x:
        stats = get_stats_bounded(network, n, mfactor*n, w)
        online = stats['timing_sum'][0]
        yonline.append(online)
        ycombined.append(online + time_per_mt[network]*stats['mults'])

    ylaud = []
    for n in x:
        ylaud.append((laud_3 if mfactor == 3 else laud_6)[n])

    fig,ax = plt.subplots()
    ax.plot(x, ycombined, label='Ours (combined)')
    ax.plot(x, yonline, label='Ours (online)')
    ax.plot(x, ylaud, label='Laud', color='C7')
    ax.grid()

    ax.set_ylabel('Time (s)')
    ax.set_xlabel('|V|')
    plt.xscale('log')
    plt.yscale('log')
    plt.xticks(x, x, rotation=90)
    ax.tick_params(axis='x', which='minor', bottom=False)

    #plt.tight_layout()
    fig.set_size_inches(4.4, 3.8)
    fig.subplots_adjust(bottom=0.35, top=0.97, left=0.2, right=0.98)
    plt.savefig('results/time-withlaud' + str(mfactor) + '-' + network + '.png')
    #plt.show()

    ax.legend(loc='best')
    label_params = ax.get_legend_handles_labels() 
    figl, axl = plt.subplots()
    axl.axis(False)
    figl.set_size_inches(7,0.5)
    axl.legend(*label_params, ncol=3, loc="center")
    figl.savefig('results/legend-withlaud.png')

def plot_time_tsp(names, network='lan'):
    yonline = []
    ycombined = []
    for name in names:
        stats = read_stats(network, name, 1)
        online = stats['timing_sum'][0]
        yonline.append(online)
        ycombined.append(online + time_per_mt[network]*stats['mults'])
    
    x = range(len(names))
    width = 0.25

    fig,ax = plt.subplots()
    ax.bar([i-width/2 for i in x], ycombined, width, label='Ours (combined)')
    ax.bar([i-width/2 for i in x], yonline, width, label='Ours (online)')
    ax.bar([i+width/2 for i in x], [laud_tsp[name] for name in names], width, label='Laud', bottom=0, color='C7')
    ax.legend(loc='upper right', bbox_to_anchor=(1.51,1))

    ax.set_ylabel('Time (s)')
    plt.yscale('log')
    plt.yticks([1,10,100,1000,10000])
    plt.xticks(x, names, rotation=90)
    ax.minorticks_on()
    ax.tick_params(axis='x', which='minor', bottom=False)

    fig.set_size_inches(9, 4)
    fig.subplots_adjust(bottom=0.3, top=0.97, left=0.1, right=0.7)
    plt.savefig('results/time-tsp.png')

def plot_time_split(network, W, n, mfactor):
    y1 = []
    y2 = []
    for w in W:
        stats = get_stats_bounded(network, n, mfactor*n, w)
        y1.append(stats['timing_phase1'][0])
        y2.append(stats['timing_phase2'][0])
    
    x = range(len(W))
    width = 0.25

    fig,ax = plt.subplots()
    ax.bar([i-width/2 for i in x], y1, width, label='Phase 1')
    ax.bar([i+width/2 for i in x], y2, width, label='Phase 2')
    #ax.bar([i+width/2 for i in x], [laud_tsp[name] for name in names], width, label='Laud', bottom=0, color='C7')
    ax.legend(loc='upper left')

    ax.set_xlabel('w')
    ax.set_ylabel('Time (s)')
    #plt.yscale('log')
    plt.xticks(x, W) #, rotation=90)
    #plt.minorticks_off()

    fig.set_size_inches(6.4, 4)
    fig.subplots_adjust(bottom=0.15, top=0.97, left=0.15, right=0.98)
    plt.savefig('results/time-split-' + network + '.png')

def plot_subgraph_sizes(network, W, n, mfactor):
    y = []
    for w in W:
        y.append(get_stats_bounded(network, n, mfactor*n, w)['subgraphs'])

    fig,ax = plt.subplots()
    for (ycur,w) in zip(y,W):
        if w == 'unique':
            label = None
        else:
            label = 'w=' + str(w)
        ax.plot(range(3,len(ycur)+2), ycur[1:], label=label)
    ax.grid()

    ax.set_ylabel('number of edges')
    ax.set_xlabel('size of isolatable subgraph')
    #plt.xticks(x, x, rotation=90)
    plt.minorticks_off()

    ax.legend(loc='upper right')

    #plt.tight_layout()
    fig.set_size_inches(6.4, 4)
    fig.subplots_adjust(bottom=0.15, top=0.97, left=0.18, right=0.98)
    plt.savefig('results/time-subgraphs-' + network + '.png')
    #plt.show()

def plot_mults(network, w, mfactor):
    x = N
    y = []
    for n in x:
        y.append(get_stats_bounded(network, n, mfactor*n, w)['mults'])

    fig,ax = plt.subplots()
    ax.plot(x, y)
    ax.grid()

    ax.set_ylabel('Multiplications')
    ax.set_xlabel('|V|')
    plt.xscale('log')
    plt.yscale('log')
    plt.xticks(x, x, rotation=90)

    plt.savefig('results/mults-' + str(w) + '-' + str(mfactor) + '-' + network + '.png')
    #plt.show()

def plot_mults_multi(network, W, mfactor):
    x = N
    y = []
    for w in W:
        ycur = []
        for n in x:
            ycur.append(get_stats_bounded(network, n, mfactor*n, w)['mults'])
        y.append(ycur)

    fig,ax = plt.subplots()
    for ycur in y:
        ax.plot(x, ycur)
    ax.grid()

    ax.set_ylabel('Multiplications')
    ax.set_xlabel('|V|')
    plt.xscale('log')
    plt.yscale('log')
    plt.xticks(x, x, rotation=90)
    ax.tick_params(axis='x', which='minor', bottom=False)

    fig.set_size_inches(6.4, 6)
    fig.subplots_adjust(bottom=0.25, top=0.97, left=0.15, right=0.98)
    plt.savefig('results/mults-multi-' + str(mfactor) + '-' + network + '.png')
    #plt.show()

def plot_comm_multi(W, mfactor):
    network = 'lan'
    x = N
    y = []
    for w in W:
        ycur = []
        for n in x:
            ycur.append(get_stats_bounded(network, n, mfactor*n, w)['comm'])
        y.append(ycur)

    fig,ax = plt.subplots()
    for ycur in y:
        ax.plot(x, ycur)
    ax.grid()

    ax.set_ylabel('Communicated bytes')
    ax.set_xlabel('|V|')
    plt.xscale('log')
    plt.yscale('log')
    plt.xticks(x, x, rotation=90)
    ax.tick_params(axis='x', which='minor', bottom=False)

    fig.set_size_inches(6.4, 6)
    fig.subplots_adjust(bottom=0.25, top=0.97, left=0.15, right=0.98)
    plt.savefig('results/comm-multi-' + str(mfactor) + '-' + network + '.png')
    #plt.show()

def plot_ot_time(network):
    x = []
    for i in [100,1000,10000,100000,1000000,10000000,100000000,1000000000]:
        for m in [1,2,5]:
            if i*m > 4000000000:
                continue
            x.append(i*m)
    #x.append(4000000000)

    y = []
    for n in x:
        y.append(get_stats_ot(network, n)['time'])

    fig,ax = plt.subplots()
    ax.plot(x, y)
    ax.grid()

    ax.set_ylabel('Time (s)')
    ax.set_xlabel('# MT\'s')
    plt.xscale('log')
    plt.yscale('log')

    ticks = [100,1000,10000,100000,1000000,10000000,100000000,1000000000]
    plt.xticks(ticks, [math_label(xx) for xx in ticks], rotation=90)

    fig.set_size_inches(4.4, 3.2)
    fig.subplots_adjust(bottom=0.25, top=0.97, left=0.21, right=0.96)
    plt.savefig('results/time-ot-' + network + '.png')
    #plt.show()

def plot_connectivity_time(network):
    x = []
    for i in range(5,151,5):
        x.append(i)

    y1 = []
    y10 = []
    y100 = []
    y1000 = []
    for n in x:
        y1.append(get_stats_connectivity(network, n, 1)['time'])
        y10.append(get_stats_connectivity(network, n, 10)['time']/1)
        y100.append(get_stats_connectivity(network, n, 100)['time']/1)
        y1000.append(get_stats_connectivity(network, n, 1000)['time']/1)

    fig,ax = plt.subplots()
    #plt.yscale('log')
    ax.plot(x, y1, label='k=1')
    #ax.plot(x, y10)
    ax.plot(x, y100, label='k=100')
    ax.plot(x, y1000, label='k=1000')
    ax.grid()

    ax.set_ylabel('Time (s)')
    ax.set_xlabel('|S| = number of vertices')
    ax.set_xticks([0,50,100,150])

    fig.set_size_inches(4.4, 3.2)
    fig.subplots_adjust(bottom=0.18, top=0.97, left=0.16, right=0.96)
    plt.savefig('results/time-connectivity-' + network + '.png')

    ax.legend(loc='best')
    label_params = ax.get_legend_handles_labels() 
    figl, axl = plt.subplots()
    axl.axis(False)
    figl.set_size_inches(7,0.5)
    axl.legend(*label_params, ncol=3, loc="center")
    figl.savefig('results/legend-subprotocol.png')

def plot_connectivity_mults(network='lan'):
    x = []
    for i in range(5,151,5):
        x.append(i)

    y1 = []
    for n in x:
        y1.append(get_stats_connectivity(network, n, 1)['mults'])

    fig,ax = plt.subplots()
    #plt.yscale('log')
    ax.plot(x, y1)
    ax.grid()

    ax.set_ylabel('Multiplications')
    ax.set_xlabel('|S| = number of vertices')
    ax.set_xticks([0,50,100,150])

    fig.set_size_inches(4.4, 3.2)
    fig.subplots_adjust(bottom=0.18, top=0.93, left=0.16, right=0.96)
    plt.savefig('results/mults-connectivity.png')

def plot_subgraph_time(network):
    fig,ax = plt.subplots()
    #plt.yscale('log')
    #plt.xscale('log')

    for simd in [1,100,1000]:
        x = []
        for i in range(5,61,1):
            if os.path.isfile('stats_' + network + '/subgraph-' + str(i) + '-' + str(simd) + '-p' + str(party) + '.txt'):
                x.append(i)
        y = []
        for n in x:
            y.append(get_stats_subgraph(network, n, simd)['time'])
        ax.plot(x, y)

    ax.grid()

    ax.set_ylabel('Time (s)')
    ax.set_xlabel('|V| = number of vertices')

    fig.set_size_inches(4.4, 3.2)
    fig.subplots_adjust(bottom=0.18, top=0.97, left=0.19, right=0.96)
    plt.savefig('results/time-subgraph-' + network + '.png')

def plot_subgraph_mults(network = 'lan'):
    fig,ax = plt.subplots()
    #plt.yscale('log')
    #plt.xscale('log')

    for simd in [1]:
        x = []
        for i in range(5,61,1):
            if os.path.isfile('stats_' + network + '/subgraph-' + str(i) + '-' + str(simd) + '-p' + str(party) + '.txt'):
                x.append(i)
        y = []
        for n in x:
            y.append(get_stats_subgraph(network, n, simd)['mults'])
        ax.plot(x, y)

    ax.grid()

    ax.set_ylabel('Multiplications')
    ax.set_xlabel('|V| = number of vertices')

    fig.set_size_inches(4.4, 3.2)
    fig.subplots_adjust(bottom=0.18, top=0.93, left=0.19, right=0.96)
    plt.savefig('results/mults-subgraph.png')

plt.rc('font', size=16)

if not os.path.exists('results'):
    os.mkdir('results')

try:
    plot_ot_time('lan')
except FileNotFoundError as e:
    print('Could not plot LAN time required for OT, because the file ' + e.filename + ' is missing.')
try:
    plot_ot_time('wan')
except FileNotFoundError as e:
    print('Could not plot WAN time required for OT, because the file ' + e.filename + ' is missing.')

try:
    plot_connectivity_time('lan')
except FileNotFoundError as e:
    print('Could not plot LAN time required for Connectivity, because the file ' + e.filename + ' is missing.')
try:
    plot_connectivity_time('wan')
except FileNotFoundError as e:
    print('Could not plot WAN time required for Connectivity, because the file ' + e.filename + ' is missing.')
try:
    plot_connectivity_mults()
except FileNotFoundError as e:
    print('Could not plot mt\'s required for Connectivity, because the file ' + e.filename + ' is missing.')

try:
    plot_subgraph_time('lan')
except FileNotFoundError as e:
    print('Could not plot LAN time required for IsolatedMSF, because the file ' + e.filename + ' is missing.')
try:
    plot_subgraph_time('wan')
except FileNotFoundError as e:
    print('Could not plot WAN time required for IsolatedMSF, because the file ' + e.filename + ' is missing.')
try:
    plot_subgraph_mults()
except FileNotFoundError as e:
    print('Could not plot mt\'s required for IsolatedMSF, because the file ' + e.filename + ' is missing.')

try:
    plot_time_multi('lan', ['unique',0.5,0.2,0.1,0.05], 3)
except FileNotFoundError as e:
    print('Could not plot LAN time, because the file ' + e.filename + ' is missing.')
try:
    plot_time_multi('wan', ['unique',0.5,0.2,0.1,0.05], 3)
except FileNotFoundError as e:
    print('Could not plot WAN time, because the file ' + e.filename + ' is missing.')
try:
    plot_mults_multi('lan', ['unique',0.5,0.2,0.1,0.05], 3)
except FileNotFoundError as e:
    print('Could not plot LAN number of multiplications, because the file ' + e.filename + ' is missing.')
try:
    plot_mults_multi('wan', ['unique',0.5,0.2,0.1,0.05], 3)
except FileNotFoundError as e:
    print('Could not plot WAN number of multiplications, because the file ' + e.filename + ' is missing.')
try:
    plot_comm_multi(['unique',0.5,0.2,0.1,0.05], 3)
except FileNotFoundError as e:
    print('Could not plot communication, because the file ' + e.filename + ' is missing.')

try:
    plot_time_withlaud('lan', 0.05, 3)
except FileNotFoundError as e:
    print('Could not plot LAN time (with Laud comparison) for M=3N, because the file ' + e.filename + ' is missing.')
try:
    plot_time_withlaud('lan', 0.05, 6)
except FileNotFoundError as e:
    print('Could not plot LAN time (with Laud comparison) for M=6N, because the file ' + e.filename + ' is missing.')

try:
    plot_time_split('lan', ['unique',0.5,0.2,0.1,0.05], 200000, 3)
except FileNotFoundError as e:
    print('Could not plot LAN time split across phases, because the file ' + e.filename + ' is missing.')
try:
    plot_time_split('wan', ['unique',0.5,0.2,0.1,0.05], 200000, 3)
except FileNotFoundError as e:
    print('Could not plot WAN time split across phases, because the file ' + e.filename + ' is missing.')

try:
    plot_subgraph_sizes('lan', ['unique',0.5,0.2,0.1,0.05], 200000, 3)
except FileNotFoundError as e:
    print('Could not plot subgraph size distribution, because the file ' + e.filename + ' is missing.')

try:
    plot_time_tsp(['berlin52', 'brg180', 'gr666', 'pcb1173', 'nrw1379'])
except FileNotFoundError as e:
    print('Could not plot time on TSP graphs, because the file ' + e.filename + ' is missing.')
