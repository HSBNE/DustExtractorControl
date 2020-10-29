# Arduino Make file. Refer to https://github.com/sudar/Arduino-Makefile

ARDUINO_DIR   = /Applications/Arduino.app/Contents/Java
ARDMK_DIR     = /usr/local/opt/arduino-mk/
AVR_TOOLS_DIR = /usr/local
MONITOR_PORT  = /dev/ttyACM0
BOARD_TAG     = mega
BOARD_SUB     = atmega2560
ARDUINO_LIBS  = ezButton JLed

OBJDIR        = build/$(BOARD_TAG)-$(BOARD_SUB)

$(info $$OBJDIR is [${OBJDIR}])

include $(ARDMK_DIR)/Arduino.mk

