# pyTools
Useful functions for Python written in either C, Cython, and Python.


## TIFF Stacks
Functions used for reading and writting tiff stacks. Stacks are always saved as single precission arrays. To compile the cython and c code, run the following in the command line

>> CC=gcc python setup.py build_ext --inplace

To use the code in Python import the module by running 

>> import tiff_stacks_python
