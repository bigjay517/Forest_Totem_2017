default:
	arduino --upload --board adafruit:avr:protrinket3 ADC_InterruptTest.ino

build b:
	arduino --verify --board adafruit:avr:protrinket3 ADC_InterruptTest.ino
