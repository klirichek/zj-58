"""
Configurator for thermal printers with "GV2" firmware. Sets printer's
flow control to XON/XOFF and baudrate to 19200, for Raspberry Pi CUPS usage.
Settings are stored in printer's nonvolatile memory, so this config should
only be needed once. This is NOT for older 2.X printers!
"""

import serial

PORT = "/dev/ttyS0"  # Could be commandline args, but for now...
BAUD = 19200
ser = serial.Serial(PORT, BAUD, timeout=0.5)

def send(cmd, expect):
    """ Issues command bytes to serial port, compares expected response
        against actual response. Returns True if match, else False. """
    #print(">", cmd)
    ser.write(cmd)
    response = ser.read(len(expect))
    #print("<", response)
    return response == expect

# Raw byte commands and responses for printer. HELLO and GOODBYE commands
# are returned verbatim, while the config commands have distinct responses.
# These sequences were taken directly from the debug log of the Windows
# printer config tool...some of these steps seem odd (HELLO appears to
# change the baud rate to 115200, and the GOODBYEs set it back) and could
# maybe be eliminated...but we have NO documentation for these particular
# commands, so we'll do things exactly by the letter, because the
# alternative could be a bricked printer (ask how I know).

HELLO = b"\x02\x00\x31\x00\x04\x00\x00\x00\x00\x00\x00\x00\x00\x00\x37\x00"
FLOW_CMD1 = (b"\x02\x00\x30\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
             b"\x00\x3A\x00\x01\x02\x04\x08\x10\x20\x40\x80\xFF")
FLOW_RESPONSE1 = (b"\x02\x00\x30\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0A\x00"
                  b"\x38\x00\x41\x32\x00\x00\x00\x00\x00\x00\x00\x01\x72")
FLOW_CMD2 = (b"\x02\x00\x8D\x00\x00\x00\x00\x00\x00"
             b"\x00\x00\x00\x01\x00\x8E\x00\x00\x00")
FLOW_RESPONSE2 = (b"\x02\x00\x8D\x00\x00\x00\x00\x00"
                  b"\x00\x00\x00\x00\x00\x00\x8F\x00")
FLOW_GOODBYE = (b"\x02\x00\x31\x00\x01\x00\x00\x00"
                b"\x00\x00\x00\x00\x00\x00\x32\x00")
BAUD_CMD = (b"\x02\x00\x30\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08"
            b"\x00\x3A\x00\x01\x02\x04\x08\x10\x20\x40\x80\xFF")
BAUD_RESPONSE = (b"\x02\x00\x30\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0A\x00"
                 b"\x38\x00\x41\x32\x00\x00\x00\x00\x00\x00\x00\x01\x72")
BAUD_GOODBYE = (b"\x02\x00\x81\x00\x00\x00\x00\x00\x00"
                b"\x00\x00\x00\x01\x00\x82\x00\x01\x01")

print("Setting flow control...", end="")
STATUS = False
if send(HELLO, HELLO):
    ser.baudrate = 115200
    STATUS = (send(FLOW_CMD1, FLOW_RESPONSE1) and
              send(FLOW_CMD2, FLOW_RESPONSE2) and
              send(FLOW_GOODBYE, FLOW_GOODBYE))
print("OK!" if STATUS else "no.")

print("Setting baudrate...", end="")
ser.baudrate = BAUD
STATUS = False
if send(HELLO, HELLO):
    ser.baudrate = 115200
    STATUS = (send(BAUD_CMD, BAUD_RESPONSE) and
              send(BAUD_GOODBYE, BAUD_GOODBYE))
print("OK!" if STATUS else "no.")

ser.close()
