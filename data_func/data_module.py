import numpy as np
import numpy.ctypeslib as npct
from ctypes import c_void_p,c_int
from ctypes import c_char_p

# input type for the cos_doubles function
# must be a double array, with single dimension that is contiguous
array_6d_double = npct.ndpointer(dtype=np.double, ndim=1, flags='CONTIGUOUS')

# load the library, using numpy mechanisms
libcd = npct.load_library("libdata", ".")

# setup the return typs and argument types
libcd.get_data.restype = c_void_p
libcd.get_data.argtypes =  [c_char_p,c_void_p,c_int,c_int,c_int,c_char_p]


def data_func(s, b, x, y, z,label_file):
    return libcd.get_data(s,b,x,y,z,label_file)
