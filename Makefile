SN = 683339521
RUNNER = jlink

.PHONY: build flash_sn flash monitor clean

build: nrf52840

nrf52840:
	west build -b nrf52840dk/nrf52840

flash:
	west -v flash --runner=$(RUNNER)

# run on the raspberrypi
# 	> openocd -f board/nordic_nrf52_dk.cfg -c 'bindto 0.0.0.0'
remote_flash: build
	gdb-multiarch -q -batch \
  		-ex "target extended-remote 192.168.10.154:3333" \
  		-ex "monitor reset halt" \
  		-ex "load" \
  		-ex "monitor reset run" \
  		-ex "quit" \
  		build/zephyr/ble-linky-tic.elf

debug:
	west debugserver

menuconfig:
	west build -t menuconfig

flash_sn:
	west -v flash -r nrfjprog --snr $(SN) --runner=$(RUNNER)

monitor:
	picocom -b 115200 /dev/ttyACM0
# 	python3 -m serial.tools.miniterm --eol LF --raw /dev/ttyACM0 115200

rust:
	cargo build --release

format:
	find src -iname *.c -o -iname *.h | xargs clang-format -i
	find include -iname *.h | xargs clang-format -i

clean:
	rm -rf build