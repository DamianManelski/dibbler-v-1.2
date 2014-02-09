import exceptions
import os
import subprocess

print('--------Bulk Leasequery Management Tool--------')
print('-----------------------------------------------')
print('--------@author: Damian Manelski---------------')
print('-----------------------------------------------')


#Constants definition:
DIR = 'C:\Users\bulk\Desktop\NajnowszaWersja\dibbler-v-1.2\Port-win32\Debug32\bin'
APP_NAME='\dibbler-requestor'
IFACE_NAME = 'Po³¹czenie lokalne 2'
DST_ADDR='2001:4070:11:300:233::1'
LINK_ADDR = 'fe80:1459:2ed8:f5eb:fed5'
DUID = 'fe80:1459:2ed8:f5eb:fed5'
QUERY_TYPE = 'RELAY_ID'
#example
#-i "Po³¹czenie lokalne 2" -dstaddr 2001:4070:11:300:233::1  -bulk -m RELAY_ID fe80:1459:2ed8:f5eb:fed3 -linkAddr fe80:1459:2ed8:f5eb:fed5


tmpApp = DIR+APP_NAME


result = subprocess.check_output([tmpApp, '-i', IFACE_NAME,'-dstaddr', DST_ADDR, 'bulk', '-m',QUERY_TYPE, DUID,'-linkAddr', LINK_ADDR ])

#subprocess.call(['C:\\Temp\\a b c\\Notepad.exe', 'C:\\test.txt'])
