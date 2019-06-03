# -*- coding: utf-8 -*-
"""
Created on Wed Aug 22 13:22:01 2018

@author: Cody
"""

from setuptools import setup
from setuptools import Extension
from Cython.Distutils import build_ext
import numpy as np


setup(
    cmdclass = {'build_ext': build_ext},
    ext_modules = [Extension("Rearranger", sources= ["pyRearranger.pyx","../CPPrearrangement/Rearrangement.cpp"],language='c++',include_dirs=[np.get_include()])])
