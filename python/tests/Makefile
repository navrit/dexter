all:test08_th_scan_noise.so

%.so:%.o
	gcc -shared $< -o $@

%.o:%.c
	gcc -c -fPIC -I/usr/include/python2.7/ $< -o $@

%.c:%.py
	cython $<
