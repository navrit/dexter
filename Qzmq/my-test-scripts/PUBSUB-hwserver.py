import time
import zmq
import random

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind('tcp://127.0.0.1:50000')

client = context.socket(zmq.SUB)
client.bind('tcp://127.0.0.1:50001')
client.setsockopt(zmq.SUBSCRIBE, b"")

time.sleep(1)

count = 0
sendJSON = True
cmd = 'set number of frames'
arg1 = 2
arg2 = ''
lastUUID = ''

print("\nSTART NOW\n", flush=True)

UUID = random.randint(1, 123456789123456789123456789)
reply = ''
rep = ''
rep2 = ''
reply_type = ''
rep = {'component': 'medipix', 'comp_phys': 'medipix', 'command': cmd,
                       'arg1': str(arg1), 'arg2': str(arg2), 'reply': reply, 'reply type': reply_type,
                       'comp_type': 'other', 'tick count': count, 'UUID': UUID}
socket.send_json(rep, flags=0)
print("SENT JSON : ", rep["UUID"])
rep2 = client.recv_json()
print('REPLY:\tUUID : {0} \t reply type : {1}'.format(rep2["UUID"], rep2["reply type"]), flush=True)

try:
    while True:
        # time.sleep(0.1)
        # print("...\n")
        try:
            reply = ''
            rep = ''
            rep2 = ''
            reply_type = ''
            # arg1 = ''
            if sendJSON:
                UUID = random.randint(1, 123456789123456789123456789)
                time.sleep(1)
                if cmd == 'save image':
                    print("---------------------------------")
                if count % 5 == 0:
                    arg1 = 'off'  # random.randint(1,100)
                    arg2 = ''
                    cmd = 'set integration'
                elif count % 5 == 1:
                    arg1 = '/tmp/all'  # random.randint(1,100)
                    arg2 = ''
                    cmd = 'take and save image sequence'
                elif count % 5 == 2:
                    arg1 = 'on'  # random.randint(1,100)
                    arg2 = ''
                    cmd = 'set integration'
                elif count % 5 == 3:
                    arg1 = ''  # random.randint(100,1000)
                    arg2 = ''
                    cmd = 'take image'
                else:
                    arg1 = '/tmp/summed'  # random.randint(1,100)
                    arg2 = ''
                    cmd = 'save image'

                count += 1
                # cmd = 'set integration'
                print(cmd, arg1, arg2)
                rep = {'component': 'medipix', 'comp_phys': 'medipix', 'command': cmd,
                       'arg1': str(arg1), 'arg2': str(arg2), 'reply': reply, 'reply type': reply_type,
                       'comp_type': 'other', 'tick count': count, 'UUID': UUID}
                socket.send_json(rep, flags=0)
                print("SENT JSON : ", rep["UUID"])
                lastUUID = UUID

            rep2 = client.recv_json(flags=zmq.NOBLOCK)
            print('REPLY:\tUUID : {0} \t reply type : {1}'.format(rep2["UUID"], rep2["reply type"]), flush=True)

            if rep2["reply type"] == "RCV" or rep2["reply type"] == "FDB":
                sendJSON = False
                # time.sleep(1)

            if rep2["reply type"] == "ACK" and rep2["UUID"] == lastUUID:
                # print('Reply UUID: ', rep2["UUID"], '\tlast UUID: ', lastUUID)
                time.sleep(1)
                print('\n\tSEND ANOTHER JSON')
                sendJSON = True

            if rep2["reply type"] == "ERR":
                print(rep2)
                time.sleep(3)
                sendJSON = True

            # print('\nEND COMM', flush=True)
            # time.sleep(0.1)
        except zmq.Again as e:
            pass

except KeyboardInterrupt:
    client.close()
    socket.close()
    context.term()
