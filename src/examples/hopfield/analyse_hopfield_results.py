#!/usr/bin/python

import pylab as p
import numpy as N
import glob
import os
import sys
import re

def loadResultOfOneRun(basename="test"):
    """ Returns an dictionary of before/after/diff """
    results={}
    results["diff"] = N.loadtxt(basename + 'diff.csv', delimiter=",")
    results["after"] = N.loadtxt(basename + 'after.csv', delimiter=",")
    results["before"] = N.loadtxt(basename + 'before.csv', delimiter=",")
    return results

def loadAllResults(path="./"):
    """Uses naming scheme of test-hopfield.sh script"""
    configs = {}
    result_files = glob.glob(os.path.join(path, "results_n_*_d_*_*.csv"))
    for r in result_files:
	print "Loading " + r
	pattern = r"results_n_(\d+)_d_(\d\.\d+)_(\w+).csv"
	m = re.search(pattern, r)
	g = m.groups()
	configs[(g[0],g[1])] = 0
    configkeys = configs.keys()
    for r in configkeys:
	configs[r] = loadResultOfOneRun(os.path.join(path, "results_n_" + r[0] +
		    "_d_" + r[1] + "_"))
    return configs

def plotAfterAndDiff(results, size, density,path="./"):
    """ Takes structure generated by loadAllResults, and
	plots both diff and after values for run at size
	and density """
    pArray = results[(size,density)]['after'].conj().T
    xticks=N.arange(1,N.size(pArray,axis=0)+1)
    f=p.figure(1)
    f.clear()
    f.text(.5, .94, 'Results for size ' + size + ', link density ' + density, horizontalalignment='center')
    p.subplot(211)
    lines = p.plot(xticks, pArray)
    p.setp(lines, linestyle='-', color='#999999', linewidth=0.5)
    if (pArray.ndim > 1):
	mArray=N.mean(pArray,axis=1)
    else: 
	mArray=pArray
    p.plot(xticks, mArray,'r')
    p.ylabel('Similarity')
    p.title('Similarity between retrieved and imprinted patterns')
    pArray = results[(size,density)]['diff'].conj().T
    sp=p.subplot(212)
    lines = p.plot(xticks, pArray)
    p.setp(lines, linestyle='-', color='#999999', linewidth=0.5)
    if (pArray.ndim > 1):
	mArray=N.mean(pArray,axis=1)
    else: 
	mArray=pArray
    p.plot(xticks, mArray,'r')
    p.xlabel('Cycle')
    p.ylabel('Similarity difference')
    p.title('Difference in cue and retrieved pattern similarities')
    p.savefig(os.path.join(path,'results_n_' + size + '_d_' + density + '.png'))

# Expects pathname for the directory with hopfield network results
def main(argv):
    results = loadAllResults(argv[0])
    for i in results:
	plotAfterAndDiff(results,i[0],i[1],argv[0])

if __name__ == "__main__":
    main(sys.argv[1:])

    

