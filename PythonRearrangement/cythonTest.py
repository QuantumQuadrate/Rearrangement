from Rearranger import pyRearranger
oracle = pyRearranger()
import numpy as np
from time import clock
patternList = [2,2,2,2,2,2,2, 2,0,0,0,0,0,2, 2,0,1,1,1,0,2, 2,0,1,1,1,0,2, 2,0,1,1,1,0,2, 2,0,0,0,0,0,2,2,2,2,2,2,2,2]
pattern = np.array(patternList)
top=0
left=0
width=7
height=7
camlength=49
camList = [0,0,1,0,0,0,0,  1,1,1,0,0,1,0,  1,0,0,0,1,0,1,  0,0,1,0,0,0,1, 1,1,0,1,0,0,0, 1,1,0,1,1,1,1, 0,0,1,0,0,1,0 ]
threshList = np.ones(49)
mask = np.zeros(2401)
for index in range(2401):
	if(index%50==0):
		mask[index]=2
	else:
		mask[index]=0
camList = np.array(camList)
threshList = np.array(threshList)
pattern = pattern.astype(np.int32)
camList = camList.astype(np.uint16)
threshList = threshList.astype(np.float32)
mask = mask.astype(np.float32)
oracle.setPattern(pattern,top,left,threshList,mask,width,height,camlength)
repetitions = 100
tic = clock()
for i in range(repetitions):
    oracle.instructions(camList)
    oracle.resetAssignmentData()
toc = clock()
print (toc-tic)/float(repetitions)
print oracle.instructions(camList)
oracle.resetAssignmentData()
print oracle.instructions(camList)
oracle.resetAssignmentData()
pattern2 = np.array([2,1,2,1]).astype(np.int32)
width2 = 2
height2 = 2
camList2 = np.array([0,0,1,1]).astype(np.uint16)
thresh2 = np.array([1,1,1,1]).astype(np.float32)
mask2 = np.zeros(16)
for index in range(16):
	if(index%5==0):
		mask2[index]=2
	else:
		mask2[index]=0
mask2 = mask2.astype(np.float32)
camlength2 = 4
oracle.setPattern(pattern2,top,left,thresh2,mask2,width2,height2,camlength2)
print oracle.instructions(camList2)
oracle.resetAssignmentData()