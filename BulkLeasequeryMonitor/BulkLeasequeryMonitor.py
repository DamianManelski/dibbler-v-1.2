# -*- coding: utf-8 -*-
import os
import subprocess
import datetime
import codecs
import getopt
import BulkTest
#import exeptions
from BulkTest import BulkTest
import sys
#reload(sys)
#sys.setdefaultencoding('utf-8')

def appPrint():
    print('\n-----------------------------------------------')
    print('--------Bulk Leasequery Management Tool--------')
    print('-----------------------------------------------')
    print('--------@author: Damian Manelski---------------')
    print('-----------------------------------------------')
    print('\n')
    print('--- 1:query by address')
    print('--- 2:query by link-address')
    print('--- 3: query by client-duid')
    print('--- 4:query by relay-id')
    print('--- 5:query by remote-id')
    print('\n')

def main():
    appPrint()

    queryType = input('Choose a number:')
    requestorPath = input('\nType absolute path to dibbler-requestor.exe path:')
    if (requestorPath == "" ):
        requestorPath = 'C:\\Users\\bulk\\Desktop\\dibbler-windows\\dibbler-v-1.2\\Port-win32\\Debug32\\bin'
    arguments=""
    test1 = BulkTest('Just a name',queryType,requestorPath,arguments)
    test1.displayTestParameters()
    test1.runTest()
    

if __name__ == "__main__":
    main()








    





