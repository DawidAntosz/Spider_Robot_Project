#!/usr/bin/python
#############################
import smbus
import time
#############################


bus = smbus.SMBus(1)

address = 0x04

def writeNumeber(value):

	bus.write_byte(address, value)

	return -1

def readNumber():

	number = bus.read_byte_data(address, 1)

	return number

while True:

	data = raw_input("Enter the data to be sent :")
	data_list = list(data)
	for i in data_list:
		writeNumber(int(ord(i)))
		time.sleep(.1)
	writeNumber(int(0x0A))
