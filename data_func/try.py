import numpy as np
import pylab
import data_module
import time
import ctypes

t1=time.time()
bb=ctypes.c_int(0)
x = 110
y = 110
z = 110
label_file = "/home/papa/sung/label_data/label_num"
#python2
c=data_module.data_func("/home/papa/sung/object/102/102.obj",ctypes.POINTER(ctypes.c_int)(bb),x,y,z,label_file)
#python3
#temp=data_module.data_func(bytes(file_name, encoding="utf-8"),ctypes.POINTER(ctypes.c_int)(bb),x,y,z,bytes(label_file,encoding="utf-8"))
a=np.ctypeslib.as_array(
	(ctypes.c_double*7*bb.value).from_address(c)
)

print a.shape
print a
print time.time()-t1
