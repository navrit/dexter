import zmq
import time
import random

remote_ip_addr = r'127.0.0.1' # 127.0.0.1 is localhost - this computer
print('Going to connect to {}'.format(remote_ip_addr))
'''If this is running on a remote machine, this should match that computer's IP address
 and this computer must have a network route to that IP address.
 
 The remote computer should really be connected directly to this computer, working through
 a router is possible but requires more configuration and could be less secure. 
 
 A basic, incomplete route test would be to ping the remote computer from this one.
'''

context = zmq.Context()
socket = context.socket(zmq.SUB)

# We must declare the socket as of type SUBSCRIBER, and pass a prefix filter.
# Here, the filter is the empty string, wich means we receive all messages.
# We may subscribe to several filters, thus receiving from all.
socket.setsockopt_string(zmq.SUBSCRIBE, "")

print('Connecting to a socket')
# We can connect to several endpoints if we desire, and receive from all.
''' SUBSCRIBE, OUTBOUND from FleX-ray/Acquila's point of view, 192.168.178.9'''
socket.connect('tcp://{}:50000'.format(remote_ip_addr))

print('About to publish on a socket')
''' PUBLISH, INBOUND from FleX-ray/Acquila's point of view, 192.168.178.9'''
pub = context.socket(zmq.PUB)
pub.connect('tcp://{}:50001'.format(remote_ip_addr))

time.sleep(1)

#tmp = {'component':'medipix','comp_phys':'medipix','command':'set exposure',
#'arg1':'1000','arg2':'...','reply':'Yeeee boiiii','reply type':'RCV','comp_type':'other','tick count':0,'UUID': random.randint(1,101)}
#pub.send_json(tmp)

print('Starting loop')

while True:
    #print("Waiting for JSON")
    msg = socket.recv_json()
    if msg["component"] != "medipix":
        continue
    print(msg)
    if msg:
        print(msg["reply"])
        print(msg["reply type"])
        print(msg["tick count"])
    msg["reply type"] = "RCV"
    pub.send_json(msg,flags=0)
    print("Sent RCV JSON: ",  msg)
    #time.sleep(0.1)

    #msg["reply type"] = "FDB"
    #msg["reply"] = "Yeeee boiii"
    #pub.send_json(msg,flags=0)
    #print("Sent FDB JSON: ",  msg)
    #time.sleep(0.1)

    msg["reply"] = "Cheeese"

    msg["reply type"] = "ACK"
    pub.send_json(msg,flags=0)
    print("Sent ACK JSON: ",  msg)
    time.sleep(0.1)

    break

#message = socket.recv_json()
#print(message)
