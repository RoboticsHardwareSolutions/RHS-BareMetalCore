# usb_bridge

One USB bridge for every device of the board: the target says what USB is bridged to, the mode says who is on the
other side (the device, the application or both).

- CMake option: `RHS_SERVICE_USB_BRIDGE` (also enables `rhs_hal_can`, `rhs_hal_usb`, tinyusb). Replaces
  `RHS_SERVICE_USB_SERIAL_BRIDGE`, `RHS_SERVICE_USB_CAN_BRIDGE` and `RHS_SERVICE_USB_ETH_BRIDGE` (still accepted
  with a deprecation warning, ON when any of them is ON)
- Start-up hook: `usb_bridge_start_up` (registers the CLI command, no own task)
- CLI: `usb` (over RTT)
- One bridge at a time

## Targets

| Target | USB side | Device | Boards |
|---|---|---|---|
| `UsbBridgeTargetCan` | CDC, SLCAN (python-can `interface="slcan"`) | `rhs_hal_can` | XL (bxCAN1 PD0/PD1, strap PG10 high), G0B1 (FDCAN1 PB8/PB9); M only app mode: CAN and USB share SRAM on STM32F103 |
| `UsbBridgeTargetRs232` / `Rs485` | CDC, raw bytes; host line coding sets the UART baud | `rhs_hal_serial` | XL, M; App mode on every board |
| `UsbBridgeTargetEth` | NCM network adapter, L2 frames | Ethernet MAC (mongoose STM32F driver), promiscuous | XL (`RHS_HAL_ETH`) |

On BMPLC_XL with strap PG10 low PA11/PA12 belong to CAN1 and USB is off: `usb_bridge_start()` returns NULL.

## Modes

| Mode | From the host | From the device | `usb_bridge_*_send()` |
|---|---|---|---|
| `UsbBridgeModeBridge` | to the device | to the host | not used |
| `UsbBridgeModeApp` | to the app callback | device not initialized | to the host |
| `UsbBridgeModeBridgeApp` | to the device and the app callback | to the host and the app callback | to the host and/or the device |

In App mode the application is the other end of the host's adapter: a CAN node, a serial peer or an Ethernet host.

## API (`usb_bridge.h`)

| Function | Description |
|---|---|
| `usb_bridge_check(cfg)` | NULL when `cfg` can start now, otherwise the reason (USB off, another bridge running, target / mode not supported on this board) |
| `usb_bridge_start(cfg)` | Start, returns a handle or NULL when `usb_bridge_check()` fails (the reason is logged) |
| `usb_bridge_stop(bridge)` | Stop, release USB and the device, free the handle |
| `usb_bridge_get_config()` / `usb_bridge_get_state()` | Config; `UsbBridgeState`: open, baudrate, usb/device/app counters (frames for CAN/Eth, bytes for serial) |
| `usb_bridge_set_can_callback()` / `usb_bridge_can_send()` | App side of the CAN target, `RHSHalCANFrameType` frames |
| `usb_bridge_set_data_callback()` / `usb_bridge_data_send()` | App side of RS232/RS485 (bytes) and Eth (one L2 frame per call) |

Callbacks run in the bridge thread. `*_send()` from a callback sends right away, from another thread the data is
queued (false when the queue is full). Sending to `UsbBridgePortDevice` in App mode fails.

```c
static void on_frame(UsbBridgePort from, const RHSHalCANFrameType* frame, void* context)
{
    RHSHalCANFrameType reply = *frame;
    reply.id++;
    usb_bridge_can_send(context, UsbBridgePortUsb, &reply);
}

UsbBridgeConfig cfg = {.target = UsbBridgeTargetCan, .mode = UsbBridgeModeApp, .baudrate = 500000};
UsbBridge*      bridge = usb_bridge_start(&cfg);
usb_bridge_set_can_callback(bridge, on_frame, bridge);
```

## Structure

`usb_bridge.c` is the core: one-bridge lock, bridge thread, USB CDC transport (`rhs_hal_cdc_*`), application queue.
Each target is a `UsbBridgeBackend` (`usb_bridge_i.h`): `usb_bridge_can.c` (SLCAN, `slcan.c`), `usb_bridge_serial.c`,
`usb_bridge_eth.c` (switches USB to NCM and runs TinyUSB in the bridge thread). `usb_bridge_cli.c` is the CLI command `usb`.

## CAN: SLCAN commands

| Command | Answer |
|---|---|
| `Sn` (0..8: 10k, 20k, 50k, 100k, 125k, 250k, 500k, 750k, 1M -- the standard Lawicel/python-can table) | `\r`, `\a` when the channel is open |
| `O` | `\r`, starts the physical CAN (not in App mode); `\a` when already open |
| `C` | `\r`, also when already closed |
| `tiiildd..` / `Tiiiiiiiildd..` / `riiil` / `Riiiiiiiil` | `z\r` / `Z\r`, `\a` when closed or the CAN has no free mailbox |
| `F` | `Fxx\r`: bit 0 RX queue full, bit 3 data overrun, bit 7 bus error; clears the flags |
| `V` / `v` / `N` | `V1010\r` / `v1010\r` / `Nxxxx\r` (last UID bytes) |
| `Z0` / `Mxxxxxxxx` / `mxxxxxxxx` | `\r`, accepted and ignored |
| `L`, `Z1`, anything else | `\a`: no listen-only mode, no timestamps |

Frames go to the host only while the channel is open. The answer to a command comes before frames the application
sends from its callback.

### Bitrate limitations

`O` opens cleanly at any of the 9 standard rates (`rhs_hal_can_init()`/`rcan_start()` reports a clean error
instead of crashing when no exact bit-timing exists for a rate on this peripheral's clock -- see
`rhs_hal_can.c`). Two rates are known to not interoperate with every adapter even though both sides open
without error:

- **10000 bps**: no exact integer (prescaler, BS1, BS2) solution exists for this rate on BMPLC_XL's bxCAN
  peripheral clock (`rcan_timing.c`'s search space, verified against the real PCLK1).
- **750000 bps**: BMPLC_XL computes an exact solution, but some third-party adapters' own bit-timing tables are
  themselves approximate at this rate (e.g. CANable 2.0's slcan firmware hard-codes a timing explicitly marked
  `// THIS IS VERY INACCURATE!!! FIXME` in its own source for S7) -- the two sides' actual bit rates differ by
  enough that they never ACK each other (`CAN0 Acknowledgment error`, `tec` climbing to 128).

Both show up as the channel opening fine but no frame ever crossing the bus. Confirmed on real hardware with a
CANable 2.0 adapter; test coverage (`test_usb_bridge_can_bridge_bitrates.py`) excludes these two rates for that
reason -- see its module docstring.

## CLI

```
usb can|rs232|rs485|eth [bridge|app|both] [baud]   start, default bridge, 500000 for CAN, 9600 for serial
usb stop
usb state
```

A target / mode the board does not support is reported, nothing starts:

```
> usb ethernet            (G0B1)
error: eth bridge: not supported on this board: no Ethernet (RHS_HAL_ETH)
> usb can bridge          (BMPLC_M)
error: can bridge: not supported on this board: CAN and USB share SRAM on STM32F103 and cannot work together, only app mode
```

## Tests

`user_tests/usb_bridge_test`: `slcan_test` (on-target unit tests of `slcan.c`, `RHS_TEST_USB_BRIDGE`) and the App
mode echo application `usb_bridge_echo` (`RHS_TEST_USB_BRIDGE_APP`) with host tests in `testbench/`:

- `test_usb_bridge_can_app.py` / `test_usb_bridge_serial_app.py` -- App mode, both boards
- `test_usb_bridge_cli.py` -- CLI/unsupported-target reporting, both boards
- `test_usb_bridge_can_bridge.py` / `test_usb_bridge_can_both.py` -- CAN Bridge / Both (BridgeApp) end-to-end
  against the bench's `[iface.canbus]` adapter, BMPLC_XL only
- `test_usb_bridge_can_bridge_frames.py` -- exhaustive CAN frame-shape matrix (std/ext ID, data/RTR, every DLC),
  BMPLC_XL only
- `test_usb_bridge_can_bridge_bitrates.py` -- sweep of the standard SLCAN bitrates this bench pairing
  interoperates with (see "Bitrate limitations" above), BMPLC_XL only
- `test_usb_bridge_can_bridge_stress.py` -- empirical throughput/burst/latency characterization, BMPLC_XL only

See `testbench/usb_bridge_hw.py`'s module docstring for the full fixture/env-var reference.

```
# board on the desk (USB_BRIDGE_PORT is the PLC's own CDC port, an explicit /dev/cu.* path -- no auto-detection)
USB_BRIDGE_PORT=/dev/cu.usbmodemXXXX JLINK_SERIAL=772440188 MCU=STM32G0B1KC uv run pytest user_tests/usb_bridge_test/testbench/
# HIL stand: power and USB of the PLC through io_hub, firmware flashed by the fixture, port resolved by usb_location
PLC_TYPE=BMPLC_XL PLC_BIN=bmplc.bin uv run pytest user_tests/usb_bridge_test/testbench/
```
