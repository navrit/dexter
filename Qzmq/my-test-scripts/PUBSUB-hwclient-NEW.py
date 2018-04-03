import zmq
import time
import random

context = zmq.Context()
socket = context.socket(zmq.SUB)

# We must declare the socket as of type SUBSCRIBER, and pass a prefix filter.
# Here, the filter is the empty string, wich means we receive all messages.
# We may subscribe to several filters, thus receiving from all.
socket.setsockopt_string(zmq.SUBSCRIBE, "")

# We can connect to several endpoints if we desire, and receive from all.
socket.connect('tcp://192.168.178.9:50000') # SUB - ACQUILA OUTBOUND

pub = context.socket(zmq.PUB)
pub.connect('tcp://192.168.178.9:50001') # PUB - ACQUILA INBOUND

time.sleep(1)

#tmp = {'component':'medipix','comp_phys':'medipix','command':'set exposure',
#'arg1':'1000','arg2':'...','reply':'Yeeee boiiii','reply type':'RCV','comp_type':'other','tick count':0,'UUID': random.randint(1,101)}
#pub.send_json(tmp)

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
