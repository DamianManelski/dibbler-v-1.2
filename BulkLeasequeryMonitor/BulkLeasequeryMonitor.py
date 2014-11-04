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
#from  SubNetCalc import *
from Utils import Utils
import ntpath
import string
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
    #init application data
    appPrint()
    utils = Utils()
    isBulkFind = False
    #interactiveMode = raw_input('         Interactive mode [Y/N]:')
    
    queryType = input('Choose query type [number]:')
    arguments = ""
    if utils.chceckIfFileExist(utils.requestorPath+utils.dibbler_name):
        isBulkFind = True
        test1 = BulkTest('Just a name',queryType,utils.requestorPath,arguments)
        test1.displayTestParameters()
        test1.runTest()

if __name__ == "__main__":
    main()








    





