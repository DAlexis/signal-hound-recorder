#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu May 18 17:20:00 2017

@author: dalexies
"""

import numpy as np
import matplotlib.pyplot as plt
import scipy
import argparse

parser = argparse.ArgumentParser(description='Plot binary file')
parser.add_argument("--format", "-f", help="Samples format: c64, f32, f64", default="c64")
parser.add_argument("--complex", "-c", help="Target for complex samples: re, im, abs", default="re")
parser.add_argument("--input", "-i", help="Input file name")
parser.add_argument("--offset", "-o", type=int, help="Offset in file in samples (important!)", default=0)
parser.add_argument("--size", "-s", type=int, help="Size to read")
parser.add_argument("--mean", "-m", type=int, help="Samples count to mean", default=1)

args = parser.parse_args()

if args.input is None:
    print("Input file not given!")
    exit(1)

count = -1
if args.size is not None:
    count = args.size

sample_size = 0

with open(args.input) as f:
    if args.format == "c64":
        sample_size = 4+4
        f.seek(sample_size*args.offset)
        raw = np.fromfile(f, dtype=np.complex64, count=count)
        if args.complex == "re":
            data = np.real(raw)
        elif args.complex == "im":
            data = np.imag(raw)
        elif args.complex == "abs":
            data = np.absolute(raw)
            
    elif args.format == "f32":
        sample_size = 4
        f.seek(sample_size*args.offset)
        data = np.fromfile(f, dtype=np.float32, count=count)
    elif args.format == "f64":
        sample_size = 8
        f.seek(sample_size*args.offset)
        data = np.fromfile(f, dtype=np.float64, count=count)
    else:
        print("Invalid format: " + args.format)
        exit(1)

size = data.shape[0]
print("Readed samples: {}".format(size))

if args.mean != 1:
    print("Averaging...")
    step = args.mean
    maximal = []
    minimal = []
    average = []
    for i in range(0, int(size / step)):
        f = i*step
        t = f+step
        sl = data[f:t]
        maximal.append(np.max(sl))
        minimal.append(np.min(sl))
        average.append(np.average(sl))
    plt.plot(average)
    plt.plot(minimal)
    plt.plot(maximal)
else:
    plt.plot(data)
plt.show()
