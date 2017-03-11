import numpy as np
import pylab
import data_module
import time
import ctypes

t1=time.time()
b=ctypes.c_int(0)
x = 110
y = 110
z = 110
label_file = "/home/papa/sung/label_data/label_num"
c=data_module.data_func("/home/papa/sung/object/102/102.obj",ctypes.POINTER(ctypes.c_int)(b),x,y,z,label_file)
a=np.ctypeslib.as_array(
	(ctypes.c_double*7*b.value).from_address(c)
)

print a.shape
print a
print time.time()-t1
