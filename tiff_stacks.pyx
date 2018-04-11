
# ========================== Imported Modules ========================== ====#
cimport cython
import numpy as np
cimport numpy as np
# xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx #
import ctypes

cdef extern from "tiff_io.h":
    int PythonReadTiffStacks(float* array, int* shape, const char* filename);
    
    int WriteTiffStacks(float* img_array, int* shape, const char* filename);

    int TiffInfo(const char* filename, int* shape);
    
def read_stack(filename):
    ''' this is a ...
    ''' 
    filename1 = str.encode(filename)
    cdef:
        const char* c_filename = filename1
        np.ndarray[int, ndim=1, mode="c"] sshape
        np.ndarray[float, ndim=3, mode="c"] img_stack
    Shape = np.zeros([3], dtype=np.float32)
    sshape = np.ascontiguousarray(Shape, dtype=ctypes.c_int)	
    TiffInfo(c_filename, &sshape[0])
    img_stack = np.zeros(sshape, dtype = np.float32)
    PythonReadTiffStacks(&img_stack[0,0,0], &sshape[0], c_filename)

    return np.squeeze(np.swapaxes(img_stack,0,2))
    
def write_stack_function(np.ndarray[float, ndim=3, mode="c"] img_stack, filename):
    ''' This is a ...
    '''
    filename1 = str.encode(filename)
    cdef:
        const char* c_filename = filename1 
        np.ndarray[int, ndim=1, mode='c'] sshape
        np.ndarray[float, ndim=1, mode="c"] Shape = np.empty([3], dtype=np.float32) 

    img_stack = np.ascontiguousarray(np.swapaxes(img_stack, 0, 2), 
                                         dtype=np.float32)
    Shape[0] = int(img_stack.shape[0])
    Shape[1] = int(img_stack.shape[1])
    Shape[2] = int(img_stack.shape[2])       
    sshape = np.ascontiguousarray(Shape, dtype=ctypes.c_int)
    WriteTiffStacks(&img_stack[0,0,0], &sshape[0], c_filename)

def write_stacks(img_stack, filename):
    
    if img_stack.ndim == 2:
        new_img = np.ascontiguousarray(img_stack[:,:,np.newaxis])
    
    filename = str(filename)
    
    write_stack_function(new_img, filename)










