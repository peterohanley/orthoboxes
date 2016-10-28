#!/opt/local/bin/python

his = [730, 570, 670, 830, 700, 710]
los = [250, 90, 50, 540, 40, 100]
mids = [0,0,0,0,0,0]
for i in range(0,6):
    mids[i] = (his[i]+4*los[i]) / 5

def swizzle(x):
    return x[2:4] + " " + x[0:2]

print(" ".join([swizzle("%04x" % x) for x in mids]))
