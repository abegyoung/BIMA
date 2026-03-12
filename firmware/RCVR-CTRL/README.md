arduino-cli config add board_manager.additional_urls https://www.pjrc.com/teensy/package_teensy_index.json
arduino-cli core update-index
arduino-cli core install teensy:avr

arduino-cli compile --fqbn teensy:avr:teensy40 --output-dir ./
stty -F /dev/ttyACM0 134
teensy_loader_cli --mcu=TEENSY40 RCVR-CTRL.ino.hex -w -v

