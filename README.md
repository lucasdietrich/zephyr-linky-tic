# Documentation

- Enedis Linky specifications: <https://www.enedis.fr/media/2035/download>

## TIC payload structure - `Mode HISTORIQUE`

```
[15:44:32.338,592] <inf> main: TIC ADCO:999999999999 [0]
[15:44:33.005,035] <inf> main: TIC OPTARIF:BASE [0]
[15:44:33.005,096] <inf> main: TIC ISOUSC:30 [0]
[15:44:33.005,157] <inf> main: TIC BASE:013444363 [0]
[15:44:33.005,218] <inf> main: TIC PTEC:TH.. [0]
[15:44:33.687,500] <inf> main: TIC IINST:024 [0]
[15:44:33.687,561] <inf> main: TIC IMAX:090 [0]
[15:44:33.687,622] <inf> main: TIC PAPP:05400 [0]
[15:44:33.687,652] <inf> main: TIC HHPHC:A [0]
[15:44:33.687,713] <inf> main: TIC MOTDETAT:000000 [0]
```

Data representation:

- `ADCO`: 12 digits, unique meter identifier -> 12B (constant)
- `OPTARIF`: always `BASE` in `Mode HISTORIQUE` -> 0B (constant)
- `ISOUSC`: 2 digits, subscribed current in amperes -> U16=2B (constant)
- `BASE`: 9 digits, cumulative energy consumption in Wh -> U32=4B (variable)
- `PTEC`: `THxx` in `Mode HISTORIQUE` -> 0B (variable but not used)
- `IINST`: 3 digits, instantaneous current in amperes -> U8=2B (variable)
- `IMAX`: 3 digits, maximum current in amperes -> U8=2B (constant)
- `PAPP`: 5 digits, apparent power in VA -> U16=2B (variable)
- `HHPHC`: always `A` in `Mode HISTORIQUE` -> 0B (constant)

Total data to store:
- Variable: MODE (1B) + BASE (4B) + IINST (2B) + PAPP (2B) = 9B
- Constant: ADCO (12B) + SOUSC (2B) + IMAX (2B) = 16B

## Expected output:

```
*** Booting Zephyr OS build v4.2.0-1-g9a844090fd2a ***
[00:00:00.383,056] <inf> main: Linky TIC reader started
[00:00:00.389,831] <inf> bt_hci_core: HW Platform: Nordic Semiconductor (0x0002)
[00:00:00.389,862] <inf> bt_hci_core: HW Variant: nRF52x (0x0002)
[00:00:00.389,892] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00) Version 4.2 Build 0
[00:00:00.390,960] <inf> bt_hci_core: HCI transport: Controller
[00:00:00.391,082] <inf> bt_hci_core: Identity: D3:5F:A1:2E:37:92 (random)
[00:00:00.391,113] <inf> bt_hci_core: HCI: version 5.4 (0x0d) revision 0x0000, manufacturer 0x05f1
[00:00:00.391,143] <inf> bt_hci_core: LMP: version 5.4 (0x0d) subver 0xffff
[00:00:00.392,364] <inf> main: Enabling RX
[00:00:00.470,489] <wrn> main: RX stopped, reason 4
[00:00:00.478,698] <wrn> main: RX stopped, reason 4
[00:00:00.487,976] <wrn> main: RX stopped, reason 4
[00:00:00.519,775] <wrn> main: RX disabled
[00:00:00.519,805] <inf> main: Enabling RX
[00:00:00.528,808] <wrn> main: RX stopped, reason 4
[00:00:00.537,963] <wrn> main: RX stopped, reason 4
[00:00:00.556,304] <wrn> main: RX stopped, reason 4
[00:00:00.564,544] <wrn> main: RX stopped, reason 4
[00:00:00.573,791] <wrn> main: RX stopped, reason 4
[00:00:00.577,972] <wrn> main: RX disabled
[00:00:00.578,033] <inf> main: Enabling RX
[00:00:03.259,765] <inf> main: TIC PAPP: 1090 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501801 Wh - ADCO 999999999999 (rx B: 265 N dt: 10 C err: 0)
[00:00:04.592,315] <inf> main: TIC PAPP: 1090 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501801 Wh - ADCO 999999999999 (rx B: 393 N dt: 20 C err: 0)
[00:00:06.604,980] <inf> main: TIC PAPP: 1090 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501802 Wh - ADCO 999999999999 (rx B: 585 N dt: 30 C err: 0)
[00:00:07.937,072] <inf> main: TIC PAPP: 1080 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501802 Wh - ADCO 999999999999 (rx B: 713 N dt: 40 C err: 0)
[00:00:09.288,757] <inf> main: TIC PAPP: 1080 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501803 Wh - ADCO 999999999999 (rx B: 841 N dt: 50 C err: 0)
[00:00:11.280,609] <inf> main: TIC PAPP: 1070 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501803 Wh - ADCO 999999999999 (rx B: 1033 N dt: 60 C err: 0)
[00:00:12.629,821] <inf> main: TIC PAPP: 1070 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501803 Wh - ADCO 999999999999 (rx B: 1161 N dt: 70 C err: 0)
[00:00:13.980,682] <inf> main: TIC PAPP: 1080 W IINST: 4 A (IMAX 90 A ISOUSC 30) BASE 43501804 Wh - ADCO 999999999999 (rx B: 1289 N dt: 80 C err: 0)
```