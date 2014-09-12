import ntpath
import os;

class Utils(object):
    """description of class"""

    def __init__(self):
     self.dibbler_name = "\\dibbler-requestor.exe"
     self.requestorPath = "..\\Port-win32\\Debug32\\bin"

   

    def chceckIfFileExist(self,path):
        if os.path.isfile(path) and os.access(path, os.R_OK):
            print "File "+ntpath.basename(path)+" exists and is readable"
            return True
        else:
            print "File is missing or is not readable"
            return False


