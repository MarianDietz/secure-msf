import os
from random import *

def write_inputs(name, s1, s2):
    if not os.path.exists('inputs/' + name):
        os.mkdir('inputs/' + name)
    f1 = open('inputs/' + name + '/p0.txt', 'w')
    f1.write(s1)
    f1.close()
    f2 = open('inputs/' + name + '/p1.txt', 'w')
    f2.write(s2)
    f2.close()

def generate_unique(n,m,t):
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

def generate_bounded(n,m,w,t):
    name = 'bounded-' + str(n) + '-' + str(m) + '-' + str(w) + '-' + str(t)
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

if not os.path.exists('inputs'):
    os.mkdir('inputs')

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
    generate_unique(n,m,t)

for (n,m,w,t) in bounded:
    #if party == 1:
    #    time.sleep(5)
    generate_bounded(n,m,w,t)
