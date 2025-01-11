import csv
import time
import serial.tools.list_ports
import keyboard
import pandas as pd
import os
from matplotlib import pyplot as plt

boolean_value = True    #Wurde in der CSV-Datei bereits die Überschrift geschrieben?

#Signale, die an den Arduino gesendet werden
string1 = "ON"
string2 = "OFF"

ports = serial.tools.list_ports.comports()  #Finde die verschiedenen Ports des Gerätes
serialInst = serial.Serial()    #Definiere eine Instanz für den Arduino
portsList = []  #Liste mit allen verfügbaren  Ports

for one in ports:   #Gebe alle verfügbaren Ports aus
    portsList.append(str(one))
    print(str(one))

com = input("Select Arduino com port: ")    #Eingabe für die Portauswahl

for i in range(0, len(portsList)):  #Ist die Eingabe valide, dann akzeptiere sie
    if portsList[i].startswith("COM" + str(com)):
        portVar = "COM" + str(com)
        print(portVar)

serialInst.baudrate = 9600  #Baudrate des Arduino festlegen
serialInst.port = portVar   #Den Port festlegen
serialInst.open()   #Öffne die Kommunikation

while True:

    if serialInst.in_waiting:   #Wenn im Eingang ein Signal ist, dann
        x = serialInst.readline()   #Lies das Signal
        packet1 = x.decode('utf')   #Entschlüssel das Signal
        data = packet1.split(',')   #Teile den Sting an den Kommas auf
        with open('data.csv', 'a', newline='') as csvfile:  #Öffne eine CSV-Datei
            fieldnames = ['xCoord', 'yCoord', 'NDMI']   #Definiere die Überschriften
            writer = csv.DictWriter(csvfile, fieldnames=fieldnames) #Definiere eine Schreiber-Instanz
            if boolean_value == True:   #Ist noch keine Überschrift geschrieben, dann schreibe eine
                writer.writeheader()
                boolean_value = False
            writer.writerow({"xCoord": data[0],"yCoord": data[1],"NDMI": data[2]})  #Schreibe den Inhalt des Signals in die Datei
        print(data) #Gib die Daten aus

    if keyboard.is_pressed('m'):    #Wenn die Taste "m" gedrückt wird, dann...
        SCdata = pd.read_csv('data.csv')    #Lies die CSV-Datei
        xCoord = SCdata['xCoord']
        yCoord = SCdata['yCoord']
        NDMI = SCdata['NDMI']

        plt.scatter(xCoord, yCoord, c=NDMI, cmap='Blues', alpha=0.75, s=100)    #Erstelle eine Streudiagramm
        cbar = plt.colorbar()
        cbar.set_label('NDMI')

        plt.title('Feuchtigkeitskarte')
        plt.xlabel('x-Achse')
        plt.ylabel('y-Achse')

        plt.tight_layout()
        plt.show()  #Zeige das Streudiagramm

    if keyboard.is_pressed('1'):    #Wird die Taste "1" gedrückt, dann...
        serialInst.write(string1.encode())  #Sende "ON" an den Arduino (startet die Messung)
        time.sleep(0.2)
    if keyboard.is_pressed('0'):    #Wird die Taste "0" gedrückt, dann...
        os.remove('data.csv')   #Beende das Programm und lösche die CSV-Datei
        exit()
    if keyboard.is_pressed(('2')):  #Wird die Taste "2" gedrückt, dann...
        serialInst.write(string2.encode())  #Sende "OFF" an den Arduino (stoppt die Messung)
        time.sleep(0.2)
