import time
import zmq
import random

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind('tcp://127.0.0.1:50000')

client = context.socket(zmq.SUB)
client.bind('tcp://127.0.0.1:50001')
client.setsockopt(zmq.SUBSCRIBE,b"")


time.sleep(2)

count = 0

print("\nSTART NOW\n", flush=True)

try:
    while True:
        #time.sleep(0.1)
        #print("...\n")
        try:
            reply = ''
            rep = ''
            rep2 = ''
            reply_type = ''
            count += 1
            if ( count % 1000 == 0):
                rep = {'component':'medipix','comp_phys':'medipix','command':'take image',
'arg1':'1000','arg2':'...','reply':reply,'reply type':reply_type,'comp_type':'other','tick count':count,'UUID': random.randint(1,123456789123456789123456789)}
                socket.send_json(rep, flags=0)
                print("SENT JSON : ", rep["UUID"])
                time.sleep(1)

            rep2 = client.recv_json(flags=zmq.NOBLOCK)
            print('REPLY:\tUUID : {0} \t reply type : {1}'.format(rep2["UUID"], rep2["reply type"]),flush=True)

            #if rep2["reply"] == "got it":
            #    print("WOW we got it finally\n", flush=True)

            #print('\nEND COMM', flush=True)
            time.sleep(0.1)
        except zmq.Again as e:
            pass
except KeyboardInterrupt:
    client.close()
    socket.close()
    context.term()

