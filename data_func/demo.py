import numpy as np
import data_module
import time
import ctypes

t1=time.time()

#bb is set to convey the number of points that returned from the data_func
bb=ctypes.c_int(0)

import argparse
cmd_parser = argparse.ArgumentParser(description="config")
cmd_parser.add_argument('-d', '--dim', metavar='dim', type=int, default=100, help='dim')
cmd_parser.add_argument('-o', '--obj', metavar='obj', type=int, default=694, help='obj')

args = cmd_parser.parse_args()

#x,y,z are the divides of the corresponding dimension
x = args.dim
y = args.dim
z = args.dim

#please use absolute path to locate the label file
label_file = "/home/papa/scn2pointcloud_tool/data_func/label_num"
#please use absolute path to locate the object file
obj_file = "/home/papa/scn2pointcloud_tool/"+str(args.obj)+"/"+str(args.obj)+".obj"

#for python2:
temp=data_module.data_func(obj_file,ctypes.POINTER(ctypes.c_int)(bb),x,y,z,label_file)
#for python3:
#temp=data_module.data_func(bytes(obj_file, encoding="utf-8"),ctypes.POINTER(ctypes.c_int)(bb),x,y,z,bytes(label_file,encoding="utf-8"))

#tansform the data from ctypes format into numpy format
a=np.ctypeslib.as_array(
	(ctypes.c_double*7*bb.value).from_address(temp)
)
#every line of the ouput indicates a point, which consists of the format that goes [x,y,z,r.float,g.float,b.float,label number]

print(a.shape)
f = open("{}_{}.txt".format(args.obj,args.dim), "w+")
for i in range(a.shape[0]):
	f.write("{} {} {} {} {} {}\n".format(int(a[i][0]),int(a[i][1]),int(a[i][2]),a[i][3],a[i][4],a[i][5]))
f.close()
print('Time:'+str(time.time()-t1)+'s')


'''
NOTE:
If you want to run the data convert function for every batch when 
you are training, instead of dealing with the data before training, 
you should run the function as child processes. Because there are 
some memory leaks in the original toolbox, and it is hard to say if 
the memory leaks are from SUNCGtoolbox or from the earlier version--GAPS. 
If you run the function directly when you are training, memory will 
be filled fully as time goes, and finally the program will be forced 
to close. The package named multiprocessing can be used to manage 
child processes.

You can use the following two functions to implement data conversion
as child processes. The function named data_worker is for child processes,
and the variable pool in the function named get_data is the manager of
child processes.
'''

'''
import multiprocess as mp
def data_worker(x,y,z,file_name):
	obj_name = "/home/papa/sung/object/"+file_name+"/"+file_name+".obj"
	label_file = "/home/papa/scn2pointcloud_tool/data_func/label_num"
	bb=ctypes.c_int(0)
	temp=data_module.data_func(obj_file,ctypes.POINTER(ctypes.c_int)(bb),x,y,z,label_file)
	a=np.ctypeslib.as_array(
		(ctypes.c_double*7*bb.value).from_address(temp)
	)
	return a

def get_data(batch_size):
	file_batch = random.sample(files_list,batch_size)
	x = 110
	y = 110
	z = 110
	pool = mp.Pool(processes=batch_size)
	for i in range(batch_size):	
		t1=time.time()
		res = pool.apply_async(data_worker,(x,y,z,file_batch[i]))
		tmp = np.ctypeslib.as_array(res.get())
		print(tmp.shape)
		print(tmp)
		print('Time:'+str(time.time()-t1)+'s')
	pool.close()
	pool.join()

'''









