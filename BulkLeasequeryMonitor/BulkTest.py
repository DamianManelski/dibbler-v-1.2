# -*- coding: cp1250 -*-
import os
import subprocess
import datetime
import sys
#reload(sys)
#sys.setdefaultencoding('cp1250')
import codecs
from time import gmtime, strftime

class BulkTest:
  
   def __init__(self, name,queryType,requestorPath,arguments):
      self.name = name
      self.testFailCount = 0
      self.arguments = arguments
      queryType = int(queryType)
      if (queryType == 1):
          self.querytype='ADDRESS'
      elif (queryType ==2):
          self.querytype ='LINK_ADDRESS'
      elif (queryType ==3):
          self.querytype='CLIENT_ID'
      elif (queryType == 4):
          self.querytype='RELAY_ID'
      elif (queryType == 5):
          self.querytype='REMOTE_ID'


      #Constants definition (default values):
      #default drectory to dibbler-requestor is empty (the same dir as this file)
      
      self.DIR = requestorPath
      if (requestorPath == ""):
          self.DIR = ''

      self.APP_NAME='\dibbler-requestor'
      self.IFACE_NAME = 'Połączenie lokalne 2'
      self.DST_ADDR='2001:4070:11:300:233::1'
      self.LINK_ADDR = 'fe80:1459:2ed8:f5eb:fed5'
      self.DUID = 'fe80:1459:2ed8:f5eb:fed5'
      self.eNUMBER = '343'
       
      self.argList = [self.DIR, self.APP_NAME,self.IFACE_NAME,self.DST_ADDR,self.DUID,self.querytype]


   def displayTestParameters(self):
     #print ("Test Name:" % self.name)
     #print ("Test fail count:" % self.testFailCount)
     print ("----------------------------------")
     
     print ("\nArguments are:")
     print ('\n'.join(self.argList))
     print ('\n')


   def runTest(self):
       
       #example
       #-i "Po??czenie lokalne 2" -dstaddr 2001:4070:11:300:233::1  -bulk -m RELAY_ID fe80:1459:2ed8:f5eb:fed3 -linkAddr fe80:1459:2ed8:f5eb:fed5

       tmpApp = self.DIR+self.APP_NAME
       if(self.querytype == "ADDRESS"):
         result = subprocess.check_output([tmpApp, '-i', self.IFACE_NAME,'-dstaddr',self.DST_ADDR, '-bulk', '-m',self.querytype, self.LINK_ADDR ])
       if(self.querytype == "LINK_ADDRESS"):
         result = subprocess.check_output([tmpApp, '-i', self.IFACE_NAME,'-dstaddr', self.DST_ADDR, '-bulk', '-m',self.querytype, self.LINK_ADDR ])
       if(self.querytype == "CLIENT_ID"):
         result = subprocess.check_output([tmpApp, '-i', self.IFACE_NAME,'-dstaddr', self.DST_ADDR, '-bulk', '-m',self.querytype, self.DUID,'-linkAddr', self.LINK_ADDR ])
       if(self.querytype == "RELAY_ID"):
         result = subprocess.check_output([tmpApp, '-i', self.IFACE_NAME,'-dstaddr', self.DST_ADDR, '-bulk', '-m',self.querytype, self.DUID,'-linkAddr', self.LINK_ADDR ])
       if(self.querytype == "REMOTE_ID"):
         result = subprocess.check_output([tmpApp, '-i', self.IFACE_NAME,'-dstaddr', self.DST_ADDR, '-bulk', '-m',self.querytype, self.DUID,'-enterpriseNumber', self.eNUMBER ])
       self.validateResponse(result)



   def validateResponse(self,str):
        if (self.querytype):
            pass

        print (str)
        var =  strftime("%Y-%m-%d %H:%M:%S", gmtime())
        varTmp = var.replace(':','_')

        f = open(varTmp+'_'+self.querytype,'w')
        if (self.querytype == 'CLIENT_ID' ):
            f.write('Serwer answer:\n'+str.decode('cp1250','ignore'))
        else:
            f.write('Serwer answer:\n'+str.decode('cp1250'))
       
        #if(result.find(pattern1) <0):
        #    print ('No string matched - Bulk application error') 
        #else:
        f.close()


        
   