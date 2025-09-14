import os
import time
import serial
import glob
import sys
import subprocess

# -------------------- CONFIG --------------------
hex_file = ".pio/build/teensy41/firmware.hex"
teensy_cli = os.path.expanduser("~/.platformio/packages/tool-teensy/teensy_loader_cli")
n
# ------------------ CHECK HEX ------------------
if not os.path.exists(hex_file):
    print(f"[ERROR] Firmware not found at {hex_file}")
    print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
    sys.exit(1)

# ------------------ CHECK CLI ------------------
if not os.path.exists(teensy_cli):
    print(f"[ERROR] teensy_loader_cli not found at {teensy_cli}")
    print("[ERROR] Make sure Teensy platform is installed in PlatformIO")
    print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
    sys.exit(1)

# ------------------ TEENSY IDENTIFICATION ------------------
def identify_teensy_on_port(port, attempt=1, debug=False):
    """Try to identify a Teensy on a specific port with retry logic"""
    try:
        if debug:
            print(f"\n[DEBUG] Opening {port} at 115200 baud...")
        
        with serial.Serial(port, 115200, timeout=2) as ser:
            if debug:
                print(f"[DEBUG] Connected. Waiting for stabilization...")
            
            # Allow connection to stabilize
            time.sleep(0.8)  # Slightly longer stabilization
            
            # Clear any pending data
            ser.reset_input_buffer()
            
            if debug:
                print(f"[DEBUG] Sending IDENTIFY command...")
            
            # Send identification request
            ser.write(b"IDENTIFY\n")
            ser.flush()
            
            # Wait for response
            response = ""
            all_responses = []
            start_time = time.time()
            while time.time() - start_time < 3.0:  # Even longer timeout
                if ser.in_waiting > 0:
                    try:
                        line = ser.readline().decode('utf-8', errors='ignore').strip()
                        if line:  # Any non-empty response
                            all_responses.append(line)
                            if debug:
                                print(f"[DEBUG] Received: '{line}'")
                            
                            if "[IDENT]" in line:
                                response = line.split("[IDENT] ")[1]
                                break
                    except Exception as e:
                        if debug:
                            print(f"[DEBUG] Error reading line: {e}")
                        pass
                time.sleep(0.05)
            
            if debug and not response:
                print(f"[DEBUG] No IDENT response. All received: {all_responses}")
            
            return response
            
    except Exception as e:
        if debug:
            print(f"[DEBUG] Exception: {e}")
        return None

def identify_teensys():
    """Scan all ports and identify connected Teensys with retry logic"""
    # Find potential Teensy ports
    ports = glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/ttyACM*") + glob.glob("COM*")
    teensys = []
    
    if not ports:
        print("[SCAN] No potential Teensy ports found!")
        return teensys
    
    print("[SCAN] Scanning for connected Teensys !! PLEASE CLOSE SERIAL MONITOR !!")
    
    for port in ports:
        print(f"[SCAN] Checking {port}...", end=" ")
        
        # Try up to 3 times per port (first attempt often fails)
        response = None
        for attempt in range(3):
            if attempt > 0:
                print(f"retry {attempt+1}...", end=" ")
                time.sleep(0.5)  # Brief delay between attempts
            
            # Enable debug on the last attempt to see what's happening
            debug_mode = (attempt == 2)
            response = identify_teensy_on_port(port, attempt + 1, debug=debug_mode)
            if response:
                break
        
        if response:
            teensys.append((port, response))
            print(f"✓ {response}")
        else:
            print("✗ No response after 3 attempts")
    
    return teensys

def select_teensy(teensys):
    """Let user select which Teensy to upload to"""
    if len(teensys) == 0:
        print("\n[ERROR] No Teensys found!")
        print("[ERROR] Make sure your Teensy is connected and running compatible firmware")
        print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
        return None
    elif len(teensys) == 1:
        port, name = teensys[0]
        print(f"\n[FOUND] One Teensy found: {port} - {name}")
        # Always require explicit confirmation even if only one device
        while True:
            try:
                ans = input("Proceed to upload to this device? (y/N): ").strip().lower()
                if ans in ("y", "yes"):
                    print(f"[SELECT] Confirmed: {port} - {name}")
                    return port
                elif ans in ("n", "no", ""):
                    print("[CANCEL] Upload cancelled by user")
                    return None
            except KeyboardInterrupt:
                print("\n[CANCEL] Upload cancelled by user")
                return None
    else:
        print(f"\n[SELECT] Found {len(teensys)} Teensys:")
        for i, (port, name) in enumerate(teensys):
            print(f"  {i+1}. {port} - {name}")
        
        while True:
            try:
                choice = input(f"\nSelect Teensy to upload to (1-{len(teensys)}): ").strip()
                choice_num = int(choice)
                if 1 <= choice_num <= len(teensys):
                    selected_port = teensys[choice_num - 1][0]
                    selected_name = teensys[choice_num - 1][1]
                    print(f"[SELECT] Selected: {selected_port} - {selected_name}")
                    return selected_port
                else:
                    print(f"[ERROR] Please enter a number between 1 and {len(teensys)}")
            except ValueError:
                print("[ERROR] Please enter a valid number")
            except KeyboardInterrupt:
                print("\n[CANCEL] Upload cancelled by user")
                print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
                sys.exit(0)

def send_reboot_command(port, device_name):
    """Send reboot command to the selected Teensy"""
    try:
        print(f"[REBOOT] Connecting to {device_name} on {port}...")
        with serial.Serial(port=port, baudrate=115200, timeout=2) as ser:
            # Allow connection to stabilize
            time.sleep(0.5)
            
            # Clear any pending data
            ser.reset_input_buffer()
            
            print(f"[REBOOT] Sending REBOOT_BOOTLOADER command to {device_name}...")
            ser.write(b"REBOOT_BOOTLOADER\n")
            ser.flush()
            
            # Wait for response from Teensy
            response_received = False
            start_time = time.time()
            
            while time.time() - start_time < 3:
                if ser.in_waiting > 0:
                    try:
                        response = ser.readline().decode('utf-8', errors='ignore').strip()
                        if response:
                            print(f"[REBOOT] {device_name}: {response}")
                            if "Entering bootloader" in response or "REBOOT" in response:
                                response_received = True
                                break
                    except Exception:
                        pass
                time.sleep(0.1)
            
            if response_received:
                print(f"[REBOOT] ✓ {device_name} reboot command confirmed")
                return True
            else:
                print(f"[REBOOT] ⚠ No response from {device_name} (proceeding anyway)")
                return False
                
    except Exception as e:
        print(f"[REBOOT] Failed to send reboot command to {device_name}: {e}")
        print("[REBOOT] ⚠ Proceeding with upload anyway...")
        print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
        return False

# ------------------ MAIN EXECUTION ------------------
print("=== Teensy Multi-Device Upload Tool ===")

# Step 1: Identify all connected Teensys
teensys = identify_teensys()

# Step 2: Let user select which Teensy to upload to
selected_port = select_teensy(teensys)
if not selected_port:
    sys.exit(1)

# Find the device name for the selected port
selected_name = next((name for port, name in teensys if port == selected_port), "Unknown Device")

# Step 3: Send reboot command to the selected Teensy
send_reboot_command(selected_port, selected_name)

# Step 4: Wait for bootloader to be ready
print(f"[WAIT] Waiting for {selected_name} bootloader to initialize", end="", flush=True)
for i in range(20):  # 2 seconds worth of dots
    time.sleep(0.1)
    print(".", end="", flush=True)

print(" ✓")
print(f"[WAIT] {selected_name} bootloader should now be ready for programming...")

# Step 5: Upload hex file
print(f"[UPLOAD] Uploading {hex_file} to {selected_name}...")

for attempt in range(2):  # Try up to 2 times
    try:
        # Use subprocess for better error handling  
        cmd = [teensy_cli, "-mmcu=TEENSY41", "-w", hex_file]
        print(f"[UPLOAD] Attempt {attempt + 1}: {' '.join(cmd)}")
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
        
        if result.returncode == 0:
            print(f"[UPLOAD] ✓ Upload to {selected_name} successful!")
            if result.stdout.strip():
                print(f"[UPLOAD] Output: {result.stdout.strip()}")
            break  # Success - exit retry loop
        else:
            print(f"[UPLOAD] ⚠ Attempt {attempt + 1} failed")
            if attempt == 0:  # First attempt failed
                print("[UPLOAD] This is normal - trying again in 2 seconds...")
                time.sleep(2)  # Wait before retry
            else:  # Second attempt failed
                print("[UPLOAD] ✗ Both attempts failed!")
                if result.stderr.strip():
                    print(f"[UPLOAD] Error: {result.stderr.strip()}")
                if result.stdout.strip():
                    print(f"[UPLOAD] Output: {result.stdout.strip()}")
                print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
                sys.exit(1)
        
    except subprocess.TimeoutExpired:
        print(f"[UPLOAD] ✗ Attempt {attempt + 1} timed out")
        if attempt == 1:  # Last attempt
            print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
            sys.exit(1)
    except Exception as e:
        print(f"[UPLOAD] ✗ Attempt {attempt + 1} failed with exception: {e}")
        if attempt == 1:  # Last attempt
            print("[REMINDER] If upload fails, make sure to close any open serial monitors first")
            sys.exit(1)

print(f"[SUCCESS] Firmware uploaded successfully to {selected_name}!")
print("=== Upload Complete ===")
