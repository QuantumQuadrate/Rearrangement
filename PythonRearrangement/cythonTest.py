from Rearranger import pyRearranger
import numpy as np
from time import clock

oracle = pyRearranger()

pattern = np.array([
    2, 2, 2, 2, 2, 2, 2,
    2, 0, 0, 0, 0, 0, 2,
    2, 0, 1, 1, 1, 0, 2,
    2, 0, 1, 1, 1, 0, 2,
    2, 0, 1, 1, 1, 0, 2,
    2, 0, 0, 0, 0, 0, 2,
    2, 2, 2, 2, 2, 2, 2
], dtype=np.int32)

top = 0
left = 0
width = 7
height = 7

occupation = np.array([
    0, 0, 1, 0, 0, 0, 0,
    1, 1, 1, 0, 0, 1, 0,
    1, 0, 0, 0, 1, 0, 1,
    0, 0, 1, 0, 0, 0, 1,
    1, 1, 0, 1, 0, 0, 0,
    1, 1, 0, 1, 1, 1, 1,
    0, 0, 1, 0, 0, 1, 0
], dtype=np.uint8)

oracle.setPattern(pattern, top, left, width, height)

repetitions = 1
tic = clock()

for _ in range(repetitions):
    oracle.instructions(occupation)
    oracle.resetAssignmentData()

toc = clock()
print (toc - tic) / float(repetitions)
l = oracle.instructions(occupation).split('>')[2:][:-1]
grouped = [tuple(l[n:n+2]) for n in range(0, len(l), 2)]
for m in range(0, len(grouped), 2):
    print('{} -> {}'.format(*grouped[m:m+2]))
