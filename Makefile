.PHONY: default build b tags

default:
	arduino --upload --board adafruit:avr:protrinket3 ADC_InterruptTest.ino

build b:
	arduino --verify --board adafruit:avr:protrinket3 ADC_InterruptTest.ino

tags:
	ctags -f tags --langmap=c++:.ino *.ino
