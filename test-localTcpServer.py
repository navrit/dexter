#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Tue Jun 19 19:04:54 2018

@author: n
"""

import sys
import traceback
import socket
import time
import numpy as np
import matplotlib.pyplot as plt

address = "127.0.0.1"
port = 6000
rcv_buffer_size = 4096
sleep_time = 0.005

sock = socket.socket()
try:
    sock.connect((address, port))
except ConnectionRefusedError:
    print("\nCONNECTION REFUSED: THE SOFTWARE IS NOT RUNNING OR THE ADDRESS OR\
          PORT ARE WRONG/BLOCKED\n")
except BrokenPipeError:
    print("\nBroken Pipe error")
global done
done = 0
global skipped
skipped = 0


# Define test functions
def hello_test():
    sock.send(b"Hello;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Hello From Dexter Server" in str(msg)
    global done
    done += 1


def bye_test():
    sock.send(b"Bye;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Lorem Ipsum" in str(msg)
    # There is more in this string but Kia will probably change it
    global done
    done += 1


def open_test():
    global done
    global skipped
    print(str(skipped) + " TEST INCOMPLETE \t\t" + sys._getframe().f_code.co_name)
    skipped += 1
    '''sock.send(b"Open;")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Lorem Ipsum" in str(msg)
    done += 1'''


def close_test():
    global skipped
    print(str(skipped) + " TEST INCOMPLETE \t\t" + sys._getframe().f_code.co_name)
    skipped += 1
    '''sock.send(b"\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "" in str(msg)
    global done
    done += 1'''


def start_test():
    global skipped
    print(str(skipped) + " TEST INCOMPLETE \t\t" + sys._getframe().f_code.co_name)
    skipped += 1
    '''sock.send(b"\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "" in str(msg)
    global done
    done += 1'''


def readout_mode_test():
    global done

    sock.send(b"SetReadoutMode;cont\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Readout mode is set to continuous" in str(msg)
    done += 1

    sock.send(b"GetReadoutMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "continuous" in str(msg)
    done += 1

    sock.send(b"SetReadoutMode;seq\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Readout mode is set to sequential" in str(msg)
    done += 1

    sock.send(b"GetReadoutMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "sequential" in str(msg)
    done += 1


def counter_depth_test():
    global done

    sock.send(b"SetCounterDepth;1\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Counter depth is set to 1 bit" in str(msg)
    done += 1

    sock.send(b"GetCounterDepth;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "1" in str(msg)
    done += 1

    sock.send(b"SetCounterDepth;6\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Counter depth is set to 6 bit" in str(msg)
    done += 1

    sock.send(b"GetCounterDepth;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "6" in str(msg)
    done += 1

    sock.send(b"SetCounterDepth;12\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Counter depth is set to 12 bit" in str(msg)
    done += 1

    sock.send(b"GetCounterDepth;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "12" in str(msg)
    done += 1

    sock.send(b"SetCounterDepth;24\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Counter depth is set to 24 bit" in str(msg)
    done += 1

    sock.send(b"GetCounterDepth;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "24" in str(msg)
    done += 1


def operation_mode_test():
    global done
    global skipped

    sock.send(b"SetOperationalMode;csm\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Operational mode is set to csm" in str(msg)
    done += 1

    sock.send(b"GetOperationalMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    print(str(skipped) + " INCOMPLETE SERVER CODE\t" +
          sys._getframe().f_code.co_name + "\t" + str(msg))
    assert "Getting the operational mode" in str(msg)
    skipped += 1

    sock.send(b"SetOperationalMode;spm\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Operational mode is set to spm" in str(msg)
    done += 1

    sock.send(b"GetOperationalMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    print(str(skipped) + " INCOMPLETE SERVER CODE\t" +
          sys._getframe().f_code.co_name + "\t" + str(msg))
    assert "Getting the operational mode" in str(msg)
    skipped += 1


def recvall(sock):
    rcv_buff_size = 8191
    data = b''
    while True:
        part = sock.recv(rcv_buff_size)
        data += part
        if len(part) < rcv_buff_size:
            # either 0 or end of data
            break
    return data


def get_image_test():
    global done
    global skipped

#    print(str(skipped) + " TEST INCOMPLETE \t\t" + sys._getframe().f_code.co_name)

    sock.send(b"GetImage;\n")
    # 1. Header data
    msg, ancdata, flags, addr = sock.recvmsg(46)  # This could be bigger
    msg = str(msg).replace("b'", "").replace("\\n'", "").split(";")[:-1]

    n_x = abs(int(msg[0]))
    n_y = abs(int(msg[1]))
    n_chips = int(msg[2])
    n_layers = int(msg[3])

    # Valid for a quad only
    n_chipLayout = (int(msg[4]), int(msg[5])), (int(msg[6]), int(msg[7])), (int(msg[8]), int(msg[9])), (int(msg[10]), int(msg[11]))

    # Based on the following ENUM
    '''
    Enumerations to define the coordinate system of the chips.
    (L)eft, (R)ight, (t)o, (T)op, and (B)ottom.

    enum globals {
        orientationLtRTtB = 0,
        orientationRtLTtB = 1,
        orientationLtRBtT = 2,
        orientationRtLBtT = 3,
        orientationTtBLtR = 4,
        orientationTtBRtL = 5,
        orientationBtTLtR = 6,
        orientationBtTRtL = 7
    };
    '''
    n_chipOrientation = (int(msg[12]), int(msg[13]), int(msg[14]), int(msg[15]))
    # This is only valid for colour mode...
    n_keys = [int(msg[16]), int(msg[17]), int(msg[18]), int(msg[19])]

#    print(n_x, n_y, n_chips, n_layers, n_chipLayout, n_chipOrientation, n_keys)
#    print("\n")

    ''' Total image size of n_x * n_y * n_chips * n_layers '''

    assert (n_x * n_y * n_chips) % 2 == 0
    assert n_x == 128 or n_x == 256
    assert n_y == 128 or n_y == 256
    assert n_chips >= 0 and n_chips <= 4
    assert n_keys == [0, 2, 4, 6]

    # These next 2 are VERY unlikely to be changed on the server
    assert n_chipLayout == ((1, 1), (1, 0), (0, 0), (0, 1))
    assert n_chipOrientation == (6, 6, 5, 5)

    # This does not handle fragmented packets, hence all the error crap below
    msg = recvall(sock)  # total size = n_x * n_y * n_chips * n_layers
#    msg, ancdata, flags, addr = sock.recvmsg(n_x * n_y * n_chips * n_layers)

    data = np.asarray(memoryview(bytearray(msg)).cast('i'))

    size = 128
    data = data[:(size*size)]

    assert data[0] == 32  # Test data in the most basic way
    assert data[-1] == 99
    assert data[1] == 35   # Orientation isn't screwed up

    if (data.shape[0] % (size*size)):
        print("\n\tNETWORK FRAGMENTATION?")
        print(data.shape)
    print("\n[INFO]\tCHIP 0 - test data shown below")
    plt.imshow(data.reshape(size, size), vmin=0, vmax=100)

    done += 1


def snap_test():
    global done
    global skipped

    print(str(skipped) + " TEST INCOMPLETE \t\t" + sys._getframe().f_code.co_name)
#    sock.send(b"Snap;\n")
#    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
#    print(str(msg))
    # done += 1
    skipped += 1


def autosave_test():
    global done
    global skipped

    sock.send(b"SetAutoSave;enable\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
#    print(str(msg))
    assert "enabled" in str(msg)
    done += 1

    print(str(skipped) + " INCOMPLETE SERVER CODE\t" +
          sys._getframe().f_code.co_name + "\t")
    '''sock.send(b"GetAutoSave;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    print(str(msg))
    assert "" in str(msg)'''
    skipped += 1


def record_path_test():
    global done
    global skipped

    sock.send(b"SetRecordPath;/\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Path is set" in str(msg)
    done += 1

    print(str(skipped) + " INCOMPLETE SERVER CODE\t" +
          sys._getframe().f_code.co_name + "\t")
    '''sock.send(b"GetRecordPath;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    print(str(msg))
    assert "" in str(msg)'''
    skipped += 1


# Get Record Format does not exist on the server yet
def record_format_test():
    global done
    global skipped

    sock.send(b"SetRecordFormat;txt\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Format is set to txt" in str(msg)
    done += 1

    print(str(skipped) + " INCOMPLETE SERVER CODE\t" +
          sys._getframe().f_code.co_name + "\t")
    '''sock.send(b"GetRecordFormat;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    print(str(msg))
    #assert "" in str(msg)'''
    skipped += 1

    sock.send(b"SetRecordFormat;tiff\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Format is set to tiff" in str(msg)
    done += 1

    print(str(skipped) + " INCOMPLETE SERVER CODE\t" +
          sys._getframe().f_code.co_name + "\t")
    '''sock.send(b"GetRecordFormat;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    print(str(msg))
    #assert "" in str(msg)'''
    skipped += 1

    sock.send(b"SetRecordFormat;rawtiff\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Format is set to rawtiff" in str(msg)
    done += 1

    print(str(skipped) + " INCOMPLETE SERVER CODE\t" +
          sys._getframe().f_code.co_name + "\t")
    '''sock.send(b"GetRecordFormat;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    print(str(msg))
    #assert "" in str(msg)'''
    skipped += 1


def gain_mode_test():
    global done

    sock.send(b"SetGainMode;shigh\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Gain mode is set to super high" in str(msg)
    done += 1

    time.sleep(sleep_time)

    sock.send(b"GetGainMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "SuperHigh" in str(msg)
    done += 1

    time.sleep(sleep_time)

    sock.send(b"SetGainMode;high\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Gain mode is set to high" in str(msg)
    done += 1

    time.sleep(sleep_time)

    sock.send(b"GetGainMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "High" in str(msg)
    done += 1

    time.sleep(sleep_time)

    sock.send(b"SetGainMode;low\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Gain mode is set to low" in str(msg)
    done += 1

    time.sleep(sleep_time)

    sock.send(b"GetGainMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Low" in str(msg)
    done += 1

    time.sleep(sleep_time)

    sock.send(b"SetGainMode;slow\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Gain mode is set to super low" in str(msg)
    done += 1

    time.sleep(sleep_time)

    sock.send(b"GetGainMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "SuperLow" in str(msg)
    done += 1


def polarity_test():
    global done

    sock.send(b"SetPolarity;neg\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Polarity is set to negative" in str(msg)
    done += 1

    sock.send(b"GetPolarity;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Negative" in str(msg)
    done += 1

    sock.send(b"SetPolarity;pos\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Polarity is set to positive" in str(msg)
    done += 1

    sock.send(b"GetPolarity;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Positive" in str(msg)
    done += 1


def continuous_RW_frequency_test():
    global done

    sock.send(b"SetCounterSelectFrequency;80085\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "CRW frequency is set to 80085" in str(msg)
    done += 1

    sock.send(b"GetCounterSelectFrequency;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "80085" in str(msg)
    done += 1


def shutter_length_test():
    global done

    sock.send(b"SetShutterLength;open\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Too many or too few arguments" in str(msg)
    done += 1

    sock.send(b"GetShutterLength;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Invalid argument" in str(msg)
    done += 1

    sock.send(b"SetShutterLength;open;80085\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Shutter open length is set to 80085" in str(msg)
    done += 1

    sock.send(b"GetShutterLength;open\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "80085" in str(msg)
    done += 1

    sock.send(b"SetShutterLength;down;5318008\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Shutter down length is set to 5318008" in str(msg)
    done += 1

    sock.send(b"GetShutterLength;down\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "5318008" in str(msg)
    done += 1


def both_counter_test():
    global done

    sock.send(b"SetBothCounters;enable\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Both counters is enabled" in str(msg)
    done += 1

    sock.send(b"GetBothCounters;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "1" in str(msg)
    done += 1

    sock.send(b"SetBothCounters;disable\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Both counters is disabled" in str(msg)
    done += 1

    sock.send(b"GetBothCounters;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "0" in str(msg)
    done += 1


def colour_mode_test():
    global done

    sock.send(b"SetColourMode;enable\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Colour mode is enabled" in str(msg)
    done += 1

    sock.send(b"GetColourMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "1" in str(msg)
    done += 1

    sock.send(b"SetColourMode;disable\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "Colour mode is disabled" in str(msg)
    done += 1

    sock.send(b"GetColourMode;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "0" in str(msg)
    done += 1


def lut_table_test():
    global done

    sock.send(b"SetLutTable;disable\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "LUT table is disabled" in str(msg)
    done += 1

    sock.send(b"GetLutTable;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "0" in str(msg)
    done += 1

    sock.send(b"SetLutTable;enable\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "LUT table is enabled" in str(msg)
    done += 1

    sock.send(b"GetLutTable;\n")
    msg, ancdata, flags, addr = sock.recvmsg(rcv_buffer_size)
    assert "1" in str(msg)
    done += 1


# Run tests
try:
    print("\nI need an isConnected call because some of these depend on\
 knowing that\n")
    hello_test()
    bye_test()
    open_test()   # Incomplete
    close_test()  # Incomplete
    start_test()  # Incomplete
    readout_mode_test()
    counter_depth_test()
    operation_mode_test()  # Incomplete server code x2
    snap_test()            # Incomplete
    autosave_test()        # Incomplete server code
    record_path_test()     # Incomplete server code
    record_format_test()   # Incomplete server code x3
#    ''' I had to add 1ms sleeps here otherwise it hasn't updated it time
#     it reports the previous value '''
    gain_mode_test()
    get_image_test()       # Incomplete
    polarity_test()
    continuous_RW_frequency_test()
    shutter_length_test()
    both_counter_test()
    colour_mode_test()
    lut_table_test()

except BrokenPipeError:
    print("\nBroken Pipe error")
except (AssertionError, ValueError, TypeError):
    _, _, tb = sys.exc_info()
    traceback.print_tb(tb)  # Fixed format
    tb_info = traceback.extract_tb(tb)
    filename, line, func, text = tb_info[-1]

    print('An error occurred on line {} in statement {}\n'.format(line, text))

print("\nPASSED {} TESTS".format(done))
print("SKIPPED {} TESTS".format(skipped))
# Cleanup
sock.close()
