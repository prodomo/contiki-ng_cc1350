#! /usr/bin/env python

import sys
from socket import *
from socket import error
import struct
from collections import OrderedDict
import MySQLdb
from datetime import datetime, timedelta
import thread
import time

PORT = 5678
BUFSIZE = 1024

dataList=[]
key_map = OrderedDict({'SEQNO':0, 'BEST_NEIGHBOR':1, 'BEST_NEIGHBOR_ETX':2,
 'RTMETRIC':3, 'NUM_NEIGHBORS':4, 'RSSI':5 ,'TEMP':6, 'EXT_T':7, 'INT_T':8 ,'BATTERY':9})

class SingleDataInfo(object):
  """docstring for SingleSensorInFo"""
  def __init__(self, addr, data, dTime):
    super(SingleDataInfo, self).__init__()
    self.addr = addr
    self.data = data
    self.dTime = dTime

#------------------------------------------------------------#
# Start a client or server application for testing
#------------------------------------------------------------#
def main():
    if len(sys.argv) < 2:
        usage()
    if sys.argv[1] == '-s':
        server()
    elif sys.argv[1] == '-c':
        client()
    else:
        usage()
  
#------------------------------------------------------------#
# Prints the instructions
#------------------------------------------------------------#
def usage():
    sys.stdout = sys.stderr
    print 'Usage: udpecho -s [port] (server)'
    print 'or: udpecho -c host [port] <file (client)'
    sys.exit(2)
    
#------------------------------------------------------------#
# Creates a server, echoes the message back to the client
#------------------------------------------------------------#
def server():
    if len(sys.argv) > 2:
        port = eval(sys.argv[2])
    else:
        port = PORT
        
    try:
        s = socket(AF_INET6, SOCK_DGRAM)
        s.bind(('fd00::1', port))
    except Exception, e:
        print 'ERROR: Server Port Binding Failed: ' + str(e)
        return
    
    print 'udp echo server ready: %s' % port
    while 1:
        data, addr = s.recvfrom(BUFSIZE)
        print 'server received', `data`, 'from', `addr`
        newdata = SingleDataInfo(addr ,data, datetime.now())
        dataList.append(newdata)
        print "newdata: {0}\n".format(newdata)

        # mac=addr[0].split(":")[5]
        # print mac
        # print len(data)
        # datalist = struct.unpack("10H", data)
        # print datalist

        # upload_to_DB(mac, datalist, datetime.now())


        #s.sendto(data, addr)

def updateThread(id):
  print "in thread ", id
  global dataList
  while  True:
    if len(dataList) != 0: 
      firstData = dataList.pop(0)
      try:
        mac = firstData.addr[0].split(":")[5]
        # print "addr=:{0}".format(firstData.addr)
        # print "mac={0}".format(mac)
        # print len(firstData.data)
        templist=[]
        templist=struct.unpack("10H", firstData.data)
        print mac, templist
        upload_to_DB(mac, templist , firstData.dTime)
      except:
        pass
    else:
      time.sleep(1)        
#------------------------------------------------------------#
# Creates a client that sends an UDP message to a server
#------------------------------------------------------------#
def client():
    if len(sys.argv) < 3:
        usage()
    host = sys.argv[2]
        
    if len(sys.argv) > 3:
        port = eval(sys.argv[3])
    else:
        port = PORT
        addr = host, port
        s = socket(AF_INET6, SOCK_DGRAM)
        s.bind(('', 0))
        print 'udp echo client ready, reading stdin'
    
    try:
        s.sendto("hello", addr)
    except error as msg:
        print msg
    
    data, fromaddr = s.recvfrom(BUFSIZE)
    print 'client received', `data`, 'from', `fromaddr`


#------------------------------------------------------------#
# write data into DB
#------------------------------------------------------------#
def upload_to_DB(mac, data, dTime):
  try:
    db = MySQLdb.connect("127.0.0.1","root","sakimaru","ITRI_OpenWSN" )
    # db = MySQLdb.connect("http://carbooky.dsmynas:59506","root","sakimaru","ITRI_OpenWSN" )
    cursor = db.cursor()
    # sql= "CREATE TABLE IF NOT EXISTS itri_MOEA_current_sensor ( `sn` INT(11), `position` VARCHAR(80), `mac_addr` VARCHAR(45), `led_status` VARCHAR(45), `pyranometer` INT(11), `int_temperature` FLOAT, `ext_temperature` FLOAT, `battery_volt` FLOAT, datetime DATETIME, `ID` INT(11), PRIMARY KEY(`mac_addr`))"

    # try:
    #     cursor.execute(sql)
    #     db.commit()
    # except:
    #     #db.rollback()
    #     print '1.connect SQL failed !!'

    
    # dTime = datetime.now()

      #mySql cmd
    ext_value = 0.0
    ext_value += float(data[key_map['EXT_T']]) /100
    int_value = 0.0
    int_value += float(data[key_map['INT_T']]) /1000

    # print "ext_value: {0}, {1}".format(int(data[key_map['EXT_T']]), ext_value)
    # print "ext_value: {0}, {1}".format(int(data[key_map['INT_T']]), int_value)



      #add by Nancy
    etx = 0.0
    etx += int(data[key_map['BEST_NEIGHBOR_ETX']])
    # print etx
    pdr = 0.0
    pdr += 1.0/((etx/1.0)/128)
    # print pdr
    rssi = 0.0
    rssi += int(data[key_map['RSSI']])-65535
    # print rssi

    insert_sql = "INSERT INTO itri_MOEA_sensor(sn, mac_addr, ext_temperature, pyranometer, datetime, int_temperature, battery_volt) \
    VALUES({0}, '{1}', {2}, {3}, '{4}', {5}, {6})".format(int(data[key_map['SEQNO']]), mac,\
     ext_value, data[key_map['INT_T']], dTime, int_value, data[key_map['BATTERY']])

    # print insert_sql
    
    rps_sql = "REPLACE INTO itri_MOEA_current_sensor(sn, mac_addr, ext_temperature, pyranometer, datetime, int_temperature, battery_volt) \
    VALUES({0}, '{1}', {2}, {3}, '{4}', {5}, {6})".format(int(data[key_map['SEQNO']]), mac,\
     ext_value, data[key_map['INT_T']], datetime.now(), int_value, data[key_map['BATTERY']])
    
    # insertR_sql = "INSERT INTO rowdata(mac, hops, seqno, parentMac, ETX, rank , neighborNum, rssi, dataTime)VALUES('{0}', {1}, {2}, '{3}', {4}, {5}, {6}, {7}, '{8}')".format(data[key_map['NODE_ID']], int(data[key_map['HOPS']]),int(data[key_map['SEQNO']]), data[key_map['BEST_NEIGHBOR']], pdr, int(data[key_map['RTMETRIC']]), int(data[key_map['NUM_NEIGHBORS']]), rssi, dTime)

    # print rps_sql
    # print insertR_sql
    try:
        # Execute the SQL command
      # print (insert_sql)
      cursor.execute(insert_sql)
      db.commit()
      # cursor.execute(insertR_sql)
      # db.commit()
      cursor.execute(rps_sql)
        # Commit your changes in the database
      db.commit()
      print 'insert sensor success'
    except:
          # Rollback in case there is any error
          #db.rollback()
      print 'insert DB failed, do not rollback'
      

      ## topology part

    temp = str(hex(data[key_map['BEST_NEIGHBOR']]))
    parentmac = temp.split("x",1)[1]
    print parentmac


    sql = "INSERT INTO itri_topology_neighbors(mode, neighborNum, devAddr,PDR, parentAddr, datetime, SN, rank, n1, rssi1)VALUES('{0}', {1}, '{2}', {3}, '{4}', '{5}', {6}, {7}, '{8}', {9})"\
    .format('111', 1, mac, pdr, parentmac, datetime.now(), int(data[key_map['SEQNO']]), int(data[key_map['RTMETRIC']]), parentmac, rssi)
    
    nbr_sql = "REPLACE INTO itri_topology_current_neighbors(mode, neighborNum, devAddr,PDR, parentAddr, datetime, SN, rank, n1, rssi1)VALUES('{0}', {1}, '{2}', {3}, '{4}', '{5}', {6}, {7}, '{8}', {9})"\
    .format('111', 1, mac, pdr, parentmac, datetime.now(), int(data[key_map['SEQNO']]), int(data[key_map['RTMETRIC']]), parentmac, rssi)
   
    try:
          # Execute the SQL command
      # print sql
      cursor.execute(sql)
      db.commit()
      cursor.execute(nbr_sql)
          # Commit your changes in the database
      db.commit()
      print 'insert neighbor success'
    except:
          # Rollback in case there is any error
          # db.rollback()
      print 'insert DB failed, do not rollback'


    db.close()
  except:
    print '3.connect to SQL failed !'

        
#------------------------------------------------------------#
# MAIN APP
#------------------------------------------------------------#
thread.start_new_thread(updateThread, (1, ))
main()

