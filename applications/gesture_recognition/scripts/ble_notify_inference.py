import asyncio
import sys
from bleak import BleakClient

# Replace with your target BLE device MAC address
DEVICE_ADDRESS = "CB:68:53:B5:B0:A3"
# Replace with the specific characteristic UUID you want to monitor
CHARACTERISTIC_UUID = "516a51c4-b1e1-47fa-8327-8acaeb3399eb" # NEUTON_OUT_CHARACTERISTIC
# Output file path for data redirection
OUTPUT_FILE = "ble_inference.log"

def notification_handler(sender: int, data: bytearray):
    """Handles incoming BLE data and writes it to a file."""
    output_line = data.decode().rstrip() + "\n"

    # Print to console (stdout)
    sys.stdout.write(output_line)
    sys.stdout.flush()

    # Append to external log file
    with open(OUTPUT_FILE, "a") as f:
        f.write(output_line)

async def main():
    print(f"Connecting to {DEVICE_ADDRESS}...")
    async with BleakClient(DEVICE_ADDRESS) as client:
        if client.is_connected:
            print("Connected! Starting notifications...")

            with open(OUTPUT_FILE, "w") as f:
                pass
            # Subscribe to the characteristic
            await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

            # Keep the script running to receive data
            while True:
                await asyncio.sleep(1)
        else:
            print("Failed to connect.")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nStopping notification listener.")
