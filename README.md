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