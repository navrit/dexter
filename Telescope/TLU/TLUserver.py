# Echo server program
from PyChipsUser import *
import socket

boardIpAddr = "192.168.200.16"  	#This comes from: X"c0a8c8" & dip_switch_i & X"0"; -- 192.168.200.X  => So check your dip switches.
boardPortNum = 50001

addrTable = AddressTable("./tlu_addr_map.txt")

board = ChipsBusUdp(addrTable, boardIpAddr, boardPortNum)

HOST = ''                 # Symbolic name meaning the local host
PORT = 51000              # Arbitrary non-privileged port
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((HOST, PORT))
s.listen(1)
conn, addr = s.accept()

# default Mask
defMaskBits = 0x1

# set T0 width
T0width = board.read("T0syncLength")
T0width = 0x10
board.write("T0syncLength", T0width)
T0width = board.read("T0syncLength") 
print "T0width = ", T0width

# set the last bit of Confbits to 0
ConfBits = board.read("ConfBits")
#ConfBits = ConfBits & 0xfffffff0 
print "ConfBits are: ", hex(ConfBits)


# hexadecimal check
def is_hex(s):
	try:
		int(s, 16)
		return True
	except ValueError:
		return False

print 'Connected by', addr
while 1:
	
	msg = "FAILED"
	cmd = conn.recv(1024)
	if (' ' in cmd):
		cmd, cmdSec = cmd.split()
	else : cmdSec = ''
	
	#print "cmd = ", cmd, " with cmd len = ", len(cmd), " and cmdSec = ", cmdSec , " with cmdSec len = ", len(cmdSec)

	# pulse T0
	if cmd == "pulseT0" :
		# set T0 bit to 1
		PulseBits = board.read("ConfBits")
		print "ConfBits before pulseT0: ", hex(PulseBits)
		PulseBits = PulseBits |  0x2
		board.write("ConfBits", PulseBits)
		print "ConfBits  after pulseT0: ", hex(PulseBits)
		msg = "OK"
		# reset T0 bit 
		PulseBits = board.read("ConfBits")
		PulseBits =  PulseBits & 0xfffffffd
		board.write("ConfBits", PulseBits)
		PulseBits = board.read("ConfBits")
		print "ConfBits after reseting  pulseT0: ", hex(PulseBits)		
	 	msg = "OK"

	# read Veto
	if cmd == "readVeto":	
		VetoBits = board.read("ConfBits")
		print "ConfBits = ", hex(VetoBits)
		msg = "OK"

	# set Veto
	if cmd == "setVeto" :
		VetoBits = board.read("ConfBits")
		print "ConfBits before setVeto: ", hex(VetoBits)
		# set Veto bit to 1
		VetoBits = VetoBits | 0x00000001
		board.write("ConfBits", VetoBits)
		print "ConfBits  after setVeto: ", hex(VetoBits)
		msg = "OK"

	# set Mask: user must provide a hex after the command
	if cmd == "setMask" :
		MaskBits = board.read("BusyMask")
		print "MaskBits before setMask: ", hex(MaskBits) 
		#MaskBits = MaskBits | 0x1
		# play with the 2nd argument
                if len(cmdSec)>0 : 
			if is_hex(cmdSec):
				print "my hex is : ", cmdSec
				board.write("BusyMask", int(cmdSec, 16) )
				msg = "OK"	
			else : print "FAILED - 2nd argument is not a hex.."
		else : 
			msg = "FAILED"
		MaskBits = board.read("BusyMask")
		print "MaskBits after setMask: ", hex(MaskBits) 

	# reset Mask
	if cmd == "resetMask" :
		MaskBits = board.read("BusyMask")
                print "MaskBits before resetMask: ", hex(MaskBits)
		MaskBits = MaskBits & 0x0
                board.write("BusyMask",MaskBits)
                print "MaskBits after resetMask: ", hex(MaskBits)		
		msg = "OK"

	# reset Veto
	if cmd == "resetVeto" :
		VetoBits = board.read("ConfBits")
                print "ConfBits before resetVeto: ", hex(VetoBits)
		# set Veto bit to 0
                VetoBits = VetoBits & 0xFFFFFFF0
		board.write("ConfBits", VetoBits)
                print "ConfBits  after resetVeto: ", hex(VetoBits)
		msg = "OK"

	# check Firmware	
	if cmd == "Firmware" : 
		boardFirmware = board.read("FirmwareId")
		msg = "OK"
		print "Firmware version = " , hex(boardFirmware)
	elif cmd == "DataSync" :
		DataSync = board.read("T0syncDataR")
		Mode="Automatic start"
		if DataSync & 1 == 1: Mode="Manual start"
		print Mode
	elif cmd == "shutter" :
		ShutterDT = board.read("ShutterDeadTimeLengthR")
		print "ShutterDT lenth:", ShutterDT, " clock cycles"

	if cmd == "/q": msg = "/q"  
	if not cmd: break
	print "send to run control: ", msg #, " , length ", len(msg)
	conn.send(msg)
conn.close()

# Echo client program
#import socket

#HOST = 'daring.cwi.nl'    # The remote host
#PORT = 50007              # The same port as used by the server
#s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#s.connect((HOST, PORT))
#s.send('Hello, world')
#cmd = s.recv(1024)
#s.close()
#print 'Received', repr(cmd)
