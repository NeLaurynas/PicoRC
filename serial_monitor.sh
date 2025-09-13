#!/bin/bash
while true; do
		# use CTRL + A -> CTRL + X to quit
		picocom -b 115200 /dev/tty.usbmodem101
		echo "Device disconnected. Trying to reconnect"
		sleep 1
done
