import matplotlib.pyplot as plt
import sys
from mpl_toolkits.mplot3d import Axes3D, art3d
from matplotlib.patches import Polygon
import mpl_toolkits.mplot3d.art3d as art3d
import numpy as np
from ROOT import TFile, TTree, gDirectory


class Ctel_drawer:
	def __init__(self):
		save_name = "ChipData.root"
		print "Using file:", save_name
		self.save_name = save_name
		self.show_plot = "true"
		self.fig = plt.figure(figsize = [15,8])
		self.ax = self.fig.gca(projection='3d')
		plt.subplots_adjust(left=0.03, right=0.97, top=0.95, bottom=0.05)
		self.z_lower = 0.0
		self.z_upper = 0.0
		self.f = TFile(save_name, "update")
		# self.ops = gDirectory.Get("DQM_options")
		# self.t_lower = self.ops.GetBinContent(60)
		# self.t_upper = self.ops.GetBinContent(61)

		# print self.t_lower, self.t_upper

		self.corners = []





#______________________________________________________________________________

	def draw_tel(self):
		self.draw_chips() #sets zlower and zupper.
		# self.add_clusters()
		# self.add_tracks()
		# self.add_track_volume()
		
		plt.savefig('test2.png', bbox_inches='tight')
		plt.tick_params(labelsize=16)
		self.ax.set_xlabel('Z (mm)', fontsize=16)
		self.ax.set_ylabel('Y (mm)', fontsize=16)
		self.ax.set_zlabel('X (mm)', fontsize=16)
		if self.show_plot == "true": plt.show()





#______________________________________________________________________________

	def draw_chips(self):
		my_chips = gDirectory.Get("all_chips")
		n = my_chips.GetEntries()
		for i in range(n):
			my_chips.GetEntry(i)
			print my_chips.gz
			self.corners = np.array([[my_chips.gz, my_chips.gy, my_chips.gx],
								[my_chips.corner1gz, my_chips.corner1gy, my_chips.corner1gx],
								[my_chips.corner2gz, my_chips.corner2gy, my_chips.corner2gx],
								[my_chips.corner3gz, my_chips.corner3gy, my_chips.corner3gx]])
				

			#Draw.
			if i==0: self.z_lower = my_chips.gz
			if i==n-1:self.z_upper = my_chips.gz

			plane = art3d.Poly3DCollection([self.corners], alpha = 0.14)
			plane.set_color('b')
			self.ax.add_collection3d(plane)





#______________________________________________________________________________

	def add_clusters(self):
		my_clusters = gDirectory.Get("all_clusters")
		for i in range(my_clusters.GetEntries()):
			my_clusters.GetEntry(i)
			if self.t_lower < my_clusters.gt < self.t_upper: self.ax.scatter(my_clusters.gz, my_clusters.gy, my_clusters.gx, s=1, c='k')





#______________________________________________________________________________

	def add_track_volume(self):
		track_r = self.ops.GetBinContent(51)
		track_vol_theta = self.ops.GetBinContent(52)
		# x0 = 0.5*(self.corners[0][2] + self.corners[2][2])
		# y0 = 0.5*(self.corners[0][1] + self.corners[2][1])

		# for theta in np.linspace(0, 6.281, 150):
		# 	x = self.x1 + track_r*np.sin(theta)
		# 	y = self.y1 + track_r*np.cos(theta)
		# 	plt.plot([self.z_lower, self.z_upper], [y, y], [x, x], alpha = 0.17, c='r', lw = 1.5)

		print "Dan!", track_vol_theta

		for theta in np.linspace(0, 6.281, 150):
			xmid = self.x1 + track_r*np.sin(theta)
			ymid = self.y1 + track_r*np.cos(theta)

			delz = 0.5*(self.z_upper - self.z_lower)
			x = xmid + track_vol_theta*delz*np.sin(theta)
			y = ymid + track_vol_theta*delz*np.cos(theta)


			plt.plot([self.z_lower, 0.5*(self.z_lower+self.z_upper), self.z_upper], [y, ymid, y], [x, xmid, x], alpha = 0.17, c='r', lw = 1.5)





#______________________________________________________________________________

	def add_tracks(self):
		t = gDirectory.Get("all_tracks")
		for i in range(t.GetEntries()):
			t.GetEntry(i)
			if self.t_lower < t.gTOA < self.t_upper:
				x1 = t.mx*self.z_lower + t.cx
				x2 = t.mx*self.z_upper + t.cx

				self.x1 = t.cx
				# x2 = t.cx

				y1 = t.my*self.z_lower + t.cy
				y2 = t.my*self.z_upper + t.cy

				self.y1 = t.cy
				# y2 = t.cy

				plt.plot([self.z_lower, self.z_upper], [y1, y2], [x1, x2])

			if t.gTOA > self.t_upper: break 





#______________________________________________________________________________


my_drawer = Ctel_drawer()
my_drawer.draw_tel()
