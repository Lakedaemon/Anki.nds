#!/usr/bin/python
#-*- coding: utf-8 -*-
# ---------------------------------------------------------------------------
# This is a plugin for Anki: http://ichi2.net/anki/
#
# To install, place this file in your ~/.anki/plugins/ directory.
# To remove, delete this file from your ~/.anki/plugins/ directory.
# Restart Anki after installation or removal for the changes to 
# take effect.
# ---------------------------------------------------------------------------
# File:        NoWifi-SyncAnki.py
# Description: used to sync Anki with Anki.nds without wifi
# Author:      Olivier Binda (I rewrote most of the original code) & Jack Probst
# License:     GNU GPL
# ---------------------------------------------------------------------------


from sys import argv
import socket
import struct
from os.path import join
from os  import unlink
from ankiqt import mw
from PyQt4.QtCore import *
from PyQt4.QtGui import *

Directory = join(mw.config.configPath, "plugins")


def ShowStatus(Message):
        mw.help.showText(Message)
        
class NoWifiSync(QThread):
        
    def __init__(self,parent = None):
        QThread.__init__(self,parent)
        self.connect(self, SIGNAL("UpdateSyncStatus"), ShowStatus)
        
    def run(self):
        Message =""
        sockfd = socket.socket()

        sockfd.connect(("localhost",24550))  
        l = struct.unpack("I", sockfd.recv(4))[0]

        name = sockfd.recv(l)

        srs = join(Directory,name + ".srs")
        rep = join(Directory,name + ".rep")
    
    
        try:
                frep = file(rep, "r")
                rdat = frep.read()
        except:
                rdat = ""
                
        if len(rdat) == 0:
                sl = struct.pack("i",   0)
                sockfd.send(sl)
                Message += "No reviews...<br>"
        else:
                Message += "Updating deck<br>"
                sl = struct.pack("i", len(rdat))
                sockfd.send(sl)
                sockfd.sendall(rdat)
        
                frep.close()
                unlink(rep)
    
        fsrs = file(srs, "w")
    
        l = struct.unpack("i", sockfd.recv(4))[0]
        Message += "Exporting srs file (%d bytes)<br>" % l
        dsrs = sockfd.recv(l)
        fsrs.write(dsrs)
        fsrs.close()
        Message += "Sync Done"
        self.emit(SIGNAL("UpdateSyncStatus"), Message )
        




NoWIfySyncThread = NoWifiSync(mw) 
def init_NoWifiSync():
        """Initialises the Anki GUI to present an option to invoke the plugin."""
        from PyQt4 import QtGui, QtCore
	

	# creates menu entry
	mw.mainWin.actionNDSync = QtGui.QAction('NDSync', mw)
	mw.mainWin.actionNDSync.setStatusTip('Sync with Anki.nds without wifi')
	mw.mainWin.actionNDSync.setEnabled(not not mw.deck)
	mw.connect(mw.mainWin.actionNDSync, QtCore.SIGNAL('triggered()'), NoWIfySyncThread.start)

	# adds the plugin icons in the Anki Toolbar
	
	mw.mainWin.toolBar.addAction(mw.mainWin.actionNDSync)
	
	# to enable or disable Jstats whenever a deck is opened/closed
	mw.deckRelatedMenuItems = mw.deckRelatedMenuItems + ("NDSync",)	

if __name__ == "__main__":
    print "Don't run me : I'm an Anki plugin."
else:
        # adds NoWifiSync to the list of plugin to process in Anki 

        mw.addHook('init', init_NoWifiSync)
        mw.registerPlugin("Button to Sync with Anki.nds without wifi", 668)
    
