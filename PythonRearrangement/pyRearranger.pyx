# -*- coding: utf-8 -*-
"""
Created on Wed Aug 22 12:17:34 2018

@author: Cody
"""
#distutils: language = c++
#setuputils: language = c++
# cython: c_string_type=str, c_string_encoding=ascii
import numpy as np
cimport numpy as np
import cython
from libcpp.string cimport string

cdef extern from "../CPPrearrangement/Rearrangement.h":
    cdef cppclass Rearranger:
        Rearranger() except + 
        void resetAssignmentData()
        void setPattern(int[], int, int, float[], float[], int, int, int)
        string generateInstructionsCorner(unsigned short[])
    

cdef class pyRearranger:
    cdef Rearranger oracle
    
    def __cinit__(self):
        self.oracle = Rearranger()
    
    def resetAssignmentData(self):
        self.oracle.resetAssignmentData()
    
    @cython.boundscheck(False)
    @cython.wraparound(False)
    def setPattern(self, np.ndarray[int, ndim=1,mode="c"] pattern not None, int top, int left, np.ndarray[float,ndim=1,mode="c"] thresh not None, np.ndarray[float,ndim=1,mode="c"] mask not None, int width, int height, int camLength ):
        self.oracle.setPattern(&pattern[0],top,left,&thresh[0],&mask[0],width,height,camLength)
    
    @cython.boundscheck(False)
    @cython.wraparound(False)
    def instructions(self, np.ndarray[unsigned short,ndim=1, mode="c"] camData not None):
        return self.oracle.generateInstructionsCorner(&camData[0])

   