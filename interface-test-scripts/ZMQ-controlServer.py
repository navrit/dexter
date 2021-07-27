import time
import random
import zmq

'''
Setup
===========================
OS packages for zmq
---------------------------
This may contain redundant packages

Fedora: python3-zmq czmq-devel cppzmq-devel
Ubuntu: python3-zmq libzmq5 libczmq-dev libzmq3-dev


Networking
---------------------------
If this is running on a remote machine, this should match that computer's IP address
and this computer must have a network route to that IP address.

It is most simple if the remote computer is connected directly to this computer, working through
a router is possible but requires more configuration and could be less secure. 

A basic (but incomplete) route test would be to ping the remote computer from this one.

Debugging: monitor traffic using Wireshark (as root) on the relevant networking interface.


Configuration
-------------------------------
remote_ip_addr: 192.168.178.1 (FleX-ray example)
(replace remote_ip_addr below with IP address above for what you actually put into the software)
    Windows control software configuration:
        PUB: remote_ip_addr:50000
        SUB: remote_ip_addr:50001

dexter_ip_addr: 192.168.178.9 (FleX-ray example)
    Dexter config.json settings:
        PUB: remote_ip_addr:50001
        SUB: remote_ip_addr:50000


Usage
===========================
Start Dexter via the terminal, e.g.
cd ~/Dexter && chmod +x Dexter && ./Dexter

Configure it for the intended use
    Change things like exposure time, equalisation (if not the default loaded) etc.
Once you're happy with the settings, start the server
    Debugging: Communications can be monitored using Wireshark (as root)
You will see messages with 'ZMQ' if it is ok
    Note: You can ignore the messages about 'QObject::disconnect: Unexpected null parameter',
        they happen every time a ZMQ message is received, it isn't actually an error.

Command list
===========================
take image
take and save image sequence
save image
set exposure
set number of frames
set threshold
set gain mode
set csm
load default equalisation
load equalisation from folder
set readout mode
set readout frequency
load configuration
set integration
take and save image
set image save path

Other interface documentation
===================================
The ZMQ interface works by sending JSON encoded commands between system components.

SEND events : broadcast a command to be executed by the responsible component
REPLY events : broadcasted by the component to give feedback on the progress of a command

Example for SEND events:
{
    "component":“name",
    "comp_phys":“physical_name",
    "command":"your_command",
    "arg1":"",
    "arg2":"",
    "reply":"",
    "reply type":"",
    "comp_type":"other",
    "tick count":1380210404,
    "UUID":26481
}

****************************
  All fields are required!
****************************

--> component : medipix (only this in our case)
--> comp_phys : physical name of the addressed component (as described in the Acquila settings file)
--> comp_type : possible values are tube, motor, camera or other (other for us only, it's relatively arbitrary)
--> command : the command issued
--> arg1 : the first argument field (optional, empty if no arguments are needed)
--> arg2 : the second argument field (optional, empty if no arguments are needed)
--> reply : always empty for SEND events (this field is used for the content of replies, see below)
--> reply type :
        Possible values are:
            RCV (confirmation of reception of the command by the code that will execute the command)
            FDB (optional intermediate feedback on the progress of execution)
            ACK (confirmation of finishing execution of the command)
            ERR (notification that an error occurred while executing the command)
            Empty for SEND events
--> tick count : a tick count which holds the time when the command was issued by Acquila
--> UUID : a unique identifier number that is assigned by Acquila for each broadcasted command. When forwarding a command from the client to Acquila, set this value to 0.

When executing a command progress is reported back to the sender through REPLY events.
It is critical that both the tick count and UUID which arrived in the SEND event are copied to any REPLY event that relates to this particular command.

SEND events:
+ "reply type" is always empty

REPLY events:
+ When executing a command progress is reported back to the sender through REPLY events.
+ It is critical that both the tick count and UUID which arrived in the SEND event are copied to any REPLY event that relates to this particular command.
+ REPLY events should be issued as follows : first a RCV event (obligatory!), one or more FDB events (optional), and finally (obligatory!) either an ACK event or ERR in case of an error.

'''

remote_ip_addr = '127.0.0.1'  # 127.0.0.1 is localhost

context = zmq.Context()
socket = context.socket(zmq.PUB)
socket.bind('tcp://{}:50000'.format(remote_ip_addr))  # PUBLISH, INBOUND

client = context.socket(zmq.SUB)
client.bind('tcp://{}:50001'.format(remote_ip_addr))  # SUBSCRIBE, OUTBOUND
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
print('REPLY:\tUUID : {0} \t reply type : {1}'.format(
    rep2["UUID"], rep2["reply type"]), flush=True)

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
            print('REPLY:\tUUID : {0} \t reply type : {1}'.format(
                rep2["UUID"], rep2["reply type"]), flush=True)

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
