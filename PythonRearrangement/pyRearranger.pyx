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
        void setPattern(int[], int, int, int, int)
        string generateInstructionsCorner(unsigned char[])


cdef class pyRearranger:
    cdef Rearranger oracle

    def __cinit__(self):
        self.oracle = Rearranger()

    def resetAssignmentData(self):
        self.oracle.resetAssignmentData()

    @cython.boundscheck(False)
    @cython.wraparound(False)
    def setPattern(self, np.ndarray[int, ndim=1, mode="c"] pattern not None, int top, int left, int width, int height):
        self.oracle.setPattern(&pattern[0], top, left, width, height)

    @cython.boundscheck(False)
    @cython.wraparound(False)
    def instructions(self, np.ndarray[unsigned char, ndim=1, mode="c"] occupationVector not None):
        return self.oracle.generateInstructionsCorner(&occupationVector[0])
