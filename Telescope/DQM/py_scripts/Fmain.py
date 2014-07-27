from Ctel_drawer import *

def tel_drawer(save_file_name):
	print "Drawing the telescope from file: ", save_file_name
	my_drawer = Ctel_drawer(save_file_name)
	my_drawer.draw_tel()