"""
Created on Tues Apr 10 2018
Used for reading and writing tiff stacks in python through c and cython.
@author: CSmith

Please run the following line to build extension
python setup_all.py build_ext --inplace
"""


from distutils.core import setup
from distutils.extension import Extension
from Cython.Distutils import build_ext
import numpy
import Cython.Compiler.Options
#from Cython.Build import cythonize

Cython.Compiler.Options.annotate = True

ext_modules = [Extension("tiff_stacks_python",sources=["tiff_stacks.pyx", "tiff_io.c"], 
                         extra_link_args = ['-ltiff', '-lm'],
                         include_dirs=[numpy.get_include()],
                         extra_compile_args=['-std=c99'])]

setup(
    name = 'tiff stacks module',
    cmdclass = {'build_ext': build_ext},
    ext_modules = ext_modules
)

#setup(
#  name = 'trying_tiff',
#  ext_modules = cythonize(["tiff_stacks.pyx"], annotate=True),
#  include_dirs=[numpy.get_include()],
#  extra_link_args = ["-ltiff"],
#  extra_compile_args=['-std=c99']
#)



