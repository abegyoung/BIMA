import telnetlib
import time
import sys

# config
HOST = "192.168.1.204"
USERNAME = "root"
PASSWORD = "baytech"

# prompt strings
PROMPT_USER = b"DS62 login:"
PROMPT_PASS = b"Password:"

# commands
if (sys.argv[1] == "on"):
    CMD = b"on\r"
elif (sys.argv[1] == "off"):
    CMD = b"off\r"

def wait_for(tn, expected, timeout=10):
    """Wait for substring in the incoming stream."""
    output = tn.read_until(expected, timeout=timeout)
    if expected not in output:
        print("Warning: Did not see expected prompt:", expected)
    return output

def main():
    print("Connecting...")
    tn = telnetlib.Telnet(HOST, timeout=10)
    tn.set_debuglevel(0)

    # --- Username ---
    wait_for(tn, PROMPT_USER)
    tn.write(USERNAME.encode() + b"\r")

    # --- Password ---
    wait_for(tn, PROMPT_PASS)
    tn.write(PASSWORD.encode() + b"\r")

    # --- Select DS-RPC from menu #5
    wait_for(tn, b"Enter Request :")
    tn.write(b"5\r")

    # --- Second password ---
    wait_for(tn, b"Enter Password: ")
    tn.write(PASSWORD.encode() + b"\r")

    # --- Send ON command
    wait_for(tn, b"DS-RPC>")
    tn.write(CMD)

    # --- Wait for Y/N prompt ---
    wait_for(tn, b"Y/N")
    tn.write(b"y\r")

    # --- Send ';;;;;' ---
    wait_for(tn, b"DS-RPC>")
    tn.write(b";;;;;\r")

    # --- Logout via 't' ---
    wait_for(tn, b"Enter Request :")
    tn.write(b"t\r")

    # Give server a moment before closing
    time.sleep(0.5)
    tn.close()
    print("Connection closed.")

if __name__ == "__main__":
    main()

