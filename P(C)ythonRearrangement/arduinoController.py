# -*- coding: utf-8 -*-
"""
Created on Thu Aug 03 23:32:23 2017

@author: Cody
"""

import serial

class Arduino(object):
    """
    Acts as a python prototype of an Arduino.
    All communication between Arduino and Python is handled through as serial connection.
    Python can not have access to all arduino constants because several are board specific. 
    The most common are implemented here as alternatives to memorizing integers for convenience.
    """

    def __init__(self, port, baudrate=115200,timeout=1,dtr=False,rts=True,openc=True):
        """
        Constructs an Arduino object which allows for access to arduino functions from python.

        Parameters:
        port -- The Computer port to which the Arduino is physically connected. Examples include "COM5" and "/dev/ttyUSB1"
        baudrate -- Rate at which information is communicated in the serial connection. 
        Must match the baudrate selected by the Arduino software. (Default = 9600)
        timeout -- Time in seconds that getData() is willing to wait before timing out. (Default = 1)
        dtr -- dtr and rts control whether or not the Arduino resets upon python opening the serial connection.
        The default values ensure no reset on the Arduino DUE (Default = False)
        Resetting the Arduino program can cause unforseen communications issues. 
        rts -- see dtr (Default = True)
        open -- If open is set to true, the constructor opens the serial connection. (Default = True)
        """
        self.ser = serial.Serial()
        self.ser.port = port
        self.ser.baudrate = baudrate
        self.ser.timeout = timeout
        self.ser.setDTR(dtr)
        self.ser.setRTS(rts)
        if (openc):
            self.ser.open()

    
    def sendString(self,serial_string):
        """
        Waits for a signal from the Arduino to need a command, then sends it out.

        Parameters:
        serial_string -- The string to send.
        """
        #tmp = self.getData()
        #while tmp != "r":
        #   tmp = self.getData()
        self.ser.write(serial_string)
        
    def getData(self):
        """
        Reads input from Arduino until a newline character is encountered.

        Returns: Input from Arduino converted to utf-8 with leading and trailing whitespace removed.
        """
        input_string = self.ser.readline()
        input_string = input_string.decode("utf-8",errors="ignore")
        return input_string.strip()

    def closeConnection(self):
        """Closes the serial connection."""
        self.ser.close()


    def openConnection(self, reset = False ):
        """Opens the serial connection

        Parameters:
        reset -- Set true to reset Arduino on opening connection. (Default = False) 
        Known to work for DUE. May require modification for other boards.
        Setting True can cause unforseen communications issues.
        """
        self.ser.setDTR(reset)
        self.ser.open()



        
        
        