import sys, os

import pylink
from pylink import JLink
import time
import re
from dataclasses import dataclass


@dataclass
class Command:
    command: str
    executed: bool = False
    callback: callable = None

def test_runner(jlink: JLink, response) -> None:
    has_error = False
    text = remove_ansi_colors(bytes(response).decode("utf-8", errors="ignore"))
    # Find commands ending with '_test'
    test_commands = [line.strip() for line in text.splitlines() if line.strip().endswith('_test')]
    if not test_commands:
        print("::warning::No tests found in this build.")
        return False
    for test_cmd in test_commands:
        # Send test command
        try:
            jlink.rtt_write(0, test_cmd.encode("utf-8"))
            print(f"Sent test command: {test_cmd}")
            time.sleep(5.0)  # Wait for response
            response = jlink.rtt_read(0, 1024)
            if response:
                resp_text = remove_ansi_colors(bytes(response).decode("utf-8", errors="ignore"))
                # Extract report info
                match = re.search(
                    r'REPORT\s*\|\s*File:\s*(.*?)\s*\|\s*Test case:\s*(.*?)\s*\|\s*Passes:\s*(\d+)\s*\|\s*Failures:\s*(\d+)',
                    resp_text
                )
                if match:
                    file_path = match.group(1)
                    test_case = match.group(2)
                    passed = match.group(3)
                    failed = match.group(4)
                    print(f"Test result for {test_cmd}: {passed} passed, {failed} failed (File: {file_path}, Test case: {test_case})")
                    if failed != '0':
                        has_error = True
                else:
                    print(f"::error::No test report found for {test_cmd}. Output:\n{resp_text}")
                    has_error = True
        except Exception as e:
            print(f"Error sending test command {test_cmd}: {e}")

    return has_error


command_map = {
    3.0: Command("?", callback=test_runner),
}


def remove_ansi_colors(text: str) -> str:
    """Remove ANSI color codes from text."""
    return re.sub(r"\x1b\[[0-9;]*m", "", text)


def run_tests_by_rtt(jlink: JLink, command_map: dict, duration: float = 0.0) -> None:
    has_error = False
    try:
        jlink.rtt_start()
        time.sleep(11.0) # I don't know why, but it's necessary for rtt to start in BMPLC_XL.

        start_time = time.time()
        while True:
            elapsed = time.time() - start_time
            if duration > 0.0 and elapsed >= duration:
                break

            for cmd_time, cmd_data in command_map.items():
                if not cmd_data.executed and elapsed >= cmd_time:
                    try:
                        w_b = jlink.rtt_write(0, cmd_data.command.encode("utf-8"))
                        print(f"Sent at {elapsed:.2f}s: {cmd_data.command} wrote {w_b} bytes")
                        time.sleep(0.2)  # Wait for response
                        data = jlink.rtt_read(0, 1024)
                        if not data:
                            print("::error::No response on command")
                            return True

                        if cmd_data.callback:
                            cmd_data.callback(jlink, data)
                    except Exception as e:
                        print(f"Failed to send command: {cmd_data.command} ({e})")
                    finally:
                        cmd_data.executed = True

    except Exception as e:
        print(f"Error RTT: {e}")
    finally:
        jlink.rtt_stop()
    return has_error

def rtt_device_by_usb(jlink_serial: int, mcu: str) -> None:
    jlink = pylink.JLink()
    jlink.open(serial_no=jlink_serial)

    if jlink.opened():
        if not jlink.set_tif(pylink.enums.JLinkInterfaces.SWD):
            print("Not updated")
        jlink.connect(mcu)
        if not jlink.connected():
            print("Not connected")

        # jlink.reset(halt=False)
        has_error = run_tests_by_rtt(jlink, command_map, 30.0)

    jlink.close()

    return has_error

def get_args():
    if len(sys.argv) < 3 or not sys.argv[1].strip() or not sys.argv[2].strip():
        raise ValueError("Usage: python units.py <jlink_serial> <mcu>")
    jlink_serial = int(sys.argv[1].strip())
    mcu = sys.argv[2].strip()
    return jlink_serial, mcu

def main():
    try:
        jlink_serial, mcu = get_args()
        if rtt_device_by_usb(jlink_serial, mcu):
            sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
