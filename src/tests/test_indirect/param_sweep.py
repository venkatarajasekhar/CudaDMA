
import sys
import os
import random
import math 

MAX_ELMT_SIZE=8192 # in floats
MAX_NUM_ELMTS=32
MAX_DMA_WARPS=16

def run_experiment(specialized,gather,alignment,offset,elem_size,num_elmts,dma_warps,num_templates,random_seed):
    f = open("params_directed.h",'w')
    if specialized:
        f.write("#define PARAM_SPECIALIZED true\n")
    else:
        f.write("#define PARAM_SPECIALIZED false\n")
    if gather:
        f.write("#define PARAM_GATHER true\n")
    else:
        f.write("#define PARAM_GATHER false\n")
    f.write("#define PARAM_ALIGNMENT "+str(alignment)+"\n")
    f.write("#define PARAM_OFFSET "+str(offset)+"\n")
    f.write("#define PARAM_ELMT_SIZE "+str(elem_size*4)+"\n")
    f.write("#define PARAM_NUM_ELMTS "+str(num_elmts)+"\n")
    f.write("#define PARAM_DMA_THREADS "+str(dma_warps*32)+"\n")
    f.write("#define PARAM_NUM_TEMPLATES "+str(num_templates)+"\n")
    f.write("#define PARAM_RAND_SEED "+str(random_seed)+"\n")
    f.close()
    os.system('make clean; make')
    result = os.system('./test_indirect')
    if not(result):
        assert(False)
    #if result:
    #    print "Experiment: ALIGNMENT-"+str(alignment)+" OFFSET-"+str(offset)+" ELMT_SIZE-"+str(4*elem_size)+" NUM_ELMTS-"+str(num_elmts)+" DMA_WARPS-"+str(dma_warps)+" Result: SUCCESS"
    #else:
    #    print "Experiment: ALIGNMENT-"+str(alignment)+" OFFSET-"+str(offset)+" ELMT_SIZE-"+str(4*elem_size)+" NUM_ELMTS-"+str(num_elmts)+" DMA_WARPS-"+str(dma_warps)+" Result: FAILURE"
    #    assert(False)
 

def run_all_experiments(alignment,offset):
    #for elem_size in range(1,MAX_ELMT_SIZE):
    #    run_experiment(alignment,offset,elem_size)
    for elem_size in range(1,MAX_ELMT_SIZE):
        for num_elems in range(1,MAX_NUM_ELMTS):
    	    for warps in range(1,MAX_DMA_WARPS):
                run_experiment(alignment,offset,elem_size,num_elems,warps)

def run_random_experiments():
    #above = 0
    #below = 0
    #for i in range(10000):
    while True:
        gather = random.randint(0,1)
        alignment = random.sample([4,8,16],1)[0]   
        offset = 0
        if alignment==8:
            offset = random.sample([0,2],1)[0]
        elif alignment==4:
            offset = random.sample([0,1,2,3],1)[0]
        #elem_size = int(math.ceil(random.triangular(1,MAX_ELMT_SIZE,256))) # Probably want to focus on more common element sizes
        elem_size = int(math.ceil(random.gauss(256,256)))
        # Check for alignment issues in the indirect case
        if elem_size%alignment <> 0:
            elem_size = elem_size - (elem_size%alignment)
        assert elem_size%alignment == 0
        if elem_size==0:
            continue
        if elem_size < 0:
            elem_size = -elem_size
        num_elems = random.randint(1,MAX_NUM_ELMTS)
        warps = random.randint(1,MAX_DMA_WARPS)
	num_templates = random.sample([1,2,3,4],1)[0]
        random_seed = random.randint(1,16777216)
        #if elem_size > 256:
        #    above += 1
        #else:
        #    below += 1
        specialized = random.randint(0,1)
        run_experiment(specialized,gather,alignment,offset,elem_size,num_elems,warps,num_templates,random_seed)
    #print "Above "+str(above)+" Below "+str(below)

def main():
    run_random_experiments()
    #run_all_experiments(16,0)
    #run_all_experiments(8,0)
    #run_all_experiments(8,2)
    #run_all_experiments(4,0)
    #run_all_experiments(4,1)
    #run_all_experiments(4,2)
    #run_all_experiments(4,3)

if __name__ == "__main__":
    main()
