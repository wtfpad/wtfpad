'''
 This code extracts the features for kNN attack and it ignores the features related to the packet size.

 It has been modified with respect to the original code developed by Wang et
 al. to fit our purposes.
'''

import sys
import os
import argparse


#-----------Aurguments Parser----------------------
parser = argparse.ArgumentParser()
parser.add_argument("-c","--classes", type=int, help="number of classes", default=10 )
parser.add_argument("-i","--inst" ,type=int, help="total number of instances for each class", default=10)
parser.add_argument("-p","--path" ,type=str, help="path to where instances are", required=True)
parser.add_argument("-o","--output" ,type=str, help="path to where features are output", required=True)

args = parser.parse_args()
PATH_TO_READ_DATA = args.path # the location where we need to read the csv files
PATH_TO_SAVE_FEATURES = args.output # where we need to save the features
NO_CLASSES = args.classes # number of classes
NO_INST = args.inst # number of instances per class
tmp = '' # when it sets to 'b', it reads the M-Nb files


def extract(times, sizes, features):

    #Transmission size features
    features.append(len(times))

    count = 0
    for x in sizes:
        if x > 0:
            count += 1
    features.append(count)
    features.append(len(times)-count)

    features.append(times[-1] - times[0])

    # commented out the packet length based features
    # because they are useless for Tor data and speed up
    # the feature extraction stage:
    '''
    #Unique packet lengths
    #for i in range(-4, 4):
    for i in range(-2000, 2001):
        if i in sizes:
            features.append(1)
        else:
            features.append(0)
    '''

    #Transpositions (similar to good distance scheme)
    count = 0
    for i in range(0, len(sizes)):
        if sizes[i] > 0:
            count += 1
            features.append(i)
        if count == 300:
            break
    for i in range(count, 300):
        features.append("X")
        
    count = 0
    prevloc = 0
    for i in range(0, len(sizes)):
        if sizes[i] > 0:
            count += 1
            features.append(i - prevloc)
            prevloc = i
        if count == 300:
            break
    for i in range(count, 300):
        features.append("X")


    #Packet distributions (where are the outgoing packets concentrated)
    count = 0
    for i in range(0, min(len(sizes), 3000)):
        if i % 30 != 29:
            if sizes[i] > 0:
                count += 1
        else:
            features.append(count)
            count = 0
    for i in range(len(sizes)//30, 100):
        features.append(0)

    #Bursts
    bursts = []
    curburst = 0
    stopped = 0
    for x in sizes:
        if x < 0:
            stopped = 0
            curburst -= x
        if x > 0 and stopped == 0:
            stopped = 1
        if x > 0 and stopped == 1:
            stopped = 0
            bursts.append(curburst)
    features.append(max(bursts))
    features.append(sum(bursts)/len(bursts))
    features.append(len(bursts))
    counts = [0, 0, 0]
    for x in bursts:
        if x > 5:
            counts[0] += 1
        if x > 10:
            counts[1] += 1
        if x > 15:
            counts[2] += 1
    features.append(counts[0])
    features.append(counts[1])
    features.append(counts[2])
    for i in range(0, 5):
        try:
            features.append(bursts[i])
        except:
            features.append("X")

    for i in range(0, 20):
        try:
            features.append(sizes[i] + 2000)
        except:
            features.append("X")


#this takes quite a while
for site in range(0, NO_CLASSES):
    #print site
    for instance in range(0, NO_INST):
        fname = str(site) + "-" + str(instance) + tmp
        #Set up times, sizes
        f = open(os.path.join(PATH_TO_READ_DATA,fname), "r")
        times = []
        sizes = []
        for x in f:
            x = x.split("\t")
            times.append(float(x[0]))
            sizes.append(int(x[1]))
        f.close()

        #Extract features. All features are non-negative numbers or X. 
        features = []
        extract(times, sizes, features)
        fout = open(os.path.join(PATH_TO_SAVE_FEATURES,fname + "f"), "w")
        for x in features:
            fout.write(repr(x) + " ")
        fout.close()
