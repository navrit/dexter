import time
import zmq
import random

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind('tcp://127.0.0.1:50000')

client = context.socket(zmq.SUB)
client.bind('tcp://127.0.0.1:50001')
client.setsockopt(zmq.SUBSCRIBE,b"")


time.sleep(1)

count = 0
sendJSON = True
cmd = 'set integration'
lastUUID = ''

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
            #arg1 = ''
            if sendJSON:
                UUID = random.randint(1,123456789123456789123456789)
                count += 1
                if (count%2 == 0):
                    arg1 = 'on' #random.randint(1,100)
                    arg2 = count
                    #cmd = 'save image'
                else:
                    arg1 = 'off' #random.randint(100,1000)
                    arg2 = count
                    #cmd = 'take image'
                cmd = 'set integration'
                print(cmd, arg1, arg2)
                rep = {'component':'medipix','comp_phys':'medipix','command':cmd,
'arg1':str(arg1),'arg2':str(arg2),'reply':reply,'reply type':reply_type,'comp_type':'other','tick count':count,'UUID': UUID}
                socket.send_json(rep, flags=0)
                print("SENT JSON : ", rep["UUID"])
                lastUUID = UUID
                time.sleep(1)

            rep2 = client.recv_json(flags=zmq.NOBLOCK)
            print('REPLY:\tUUID : {0} \t reply type : {1}'.format(rep2["UUID"], rep2["reply type"]),flush=True)

            if rep2["reply type"] == "RCV" or rep2["reply type"] == "FDB":
                sendJSON = False
                #time.sleep(1)
            if rep2["reply type"] == "ACK":
                #print('Reply UUID: ', rep2["UUID"], '\tlast UUID: ', lastUUID)
                if (rep2["UUID"] == lastUUID):
                    time.sleep(1)
                    print('\n\tSEND ANOTHER JSON')
                    sendJSON = True
            if rep2["reply type"] == "ERR":
                print(rep2)
                time.sleep(3)
                sendJSON = True

            #print('\nEND COMM', flush=True)
            #time.sleep(0.1)
        except zmq.Again as e:
            pass
except KeyboardInterrupt:
    client.close()
    socket.close()
    context.term()

