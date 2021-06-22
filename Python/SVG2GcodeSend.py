'''
Created on Jan 25, 2021

@author: mk
'''
#!/usr/bin/python

import os
import argparse
import sys
import shutil
import winsound
import serial
import string
from svg_to_gcode.svg_parser import parse_file
from svg_to_gcode.compiler import Compiler, interfaces
#GCODE_name = "circle_003_laser_0002.gcode"
GO_HOME = "G1 F1000 X0 Y0"
#---------------------------------------   
class com_port:
#    baudrate_list = [300,600,1200,2400,4800,9600,19200,38400,57600,115200,230400,460800,921600]
#    parity_list = ['N','O','E','S','M']
#    port_s = 'COM6'

    def __init__(self, comport, baudrate, to_read):
        '''
        Constructor
        '''
        self.port_s = comport
        self.baudrate = baudrate
        self.timeout = to_read
    
    def OpenPort(self):  
        try:
            self.serobj = serial.Serial(self.port_s, self.baudrate, timeout = self.timeout)
        except IOError:
            raise IOError
        except serial.SerialException:
            raise IOError
    
        if self.serobj.isOpen()== True:
            print(' is Opened')
            print('----------------------------')
        else:
            self.serobj.open()
    
        self.serobj.flushInput()
    
    def ReadLine(self):
        line = self.serobj.read_until(b'\r\n')
        return(line)
    
    def WriteLine(self, lineout):
        self.serobj.write(lineout.encode('utf-8'))
        
    def PortClose(self):
        self.serobj.close()

#---------------------------------------   
def CreateGcode(parsed):
    # Instantiate a compiler, specifying the interface type and the speed at which the tool should move. pass_depth controls
    # how far down the tool moves after every pass. Set it to 0 if your machine does not support Z axis movement.
    gcode_compiler = Compiler(interfaces.Gcode, movement_speed=1000, cutting_speed=300, pass_depth=0)
    
    if os.path.isfile(parsed.svgfile) == False:
        print('File :' + parsed.svgfile + ' NOT exists')
        return 1
    
    curves = parse_file(parsed.svgfile) # Parse an svg file into geometric curves
    
    gcode_compiler.append_curves(curves) 
    gcode_compiler.compile_to_file(parsed.gfile, passes=1)
    return 0
#---------------------------------------   
def GcodeSend(param):
    gcodelist = []
    if Read_Script(gcodelist, param.gfile) > 0:
        print("Error in read :" + param.gfile)
        sys.exit(2)

    comport = com_port(param.comport, 115200, 1) # timeout vo float sekundach
    comport.OpenPort()
    line_in = comport.ReadLine()
    print(line_in.decode('UTF-8'), end ='')
    line_in = comport.ReadLine()
    if b'Grbl 1.1h' in line_in:
        print(line_in.decode('UTF-8'), end ='')
    
    gcodelist.append(GO_HOME  + '\n')
    for gline in gcodelist:
        line = gline.split('\n')
        comport.WriteLine(line[0] + '\r')
        print(line[0])
        while True:
            line_in = comport.ReadLine()
            if line_in == b'':
                comport.WriteLine('?')
                print('?', end ='')
                line_in = comport.ReadLine()
                str1 = line_in.decode('UTF-8').split('\r')
                print(str1[0]) #, end ='')
            else:
                break
        print(line_in.decode('UTF-8'), end ='')
        
    comport.PortClose()
    print('COM closed.')
    print('End')

#---------------------------------------
def Read_Script(gcodelist, g_name):
    g_file = open(g_name, 'rt')
    try:
        for instr in g_file.readlines():
            if 'S255' in instr:
                instr = instr.replace('S255', 'S1000')
            gcodelist.append(instr)
        g_file.close()
        return 0
    except IOError:
        return 1

#---------------------------------------

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-p', '--gfile', help='Path to Gcode file')
    parser.add_argument('-c', '--comport', help='COMx port')
    parser.add_argument('-s', '--svgfile', help='Path to SVG file')
    
    parsed = parser.parse_args()
    
    parsed.gfile = parsed.gfile.replace('/', '\\')
    parsed.svgfile = parsed.svgfile.replace('/', '\\')
    
#    parsed.svgfile = "H:\Work\Workspace\Python\SVG2GcodeSend\surface_final.svg"
#    parsed.svgfile = "surface_final.svg"
    print('SVG : ' + parsed.svgfile)
    print('Gcode: ' + parsed.gfile)
    print('Port : ' + parsed.comport)
    
    if CreateGcode(parsed) > 0:
        sys.exit(1)
        
    print('Prenos GCode to HW.')
    GcodeSend(parsed)
    sys.exit(0)
#---------------------------------------
    
              