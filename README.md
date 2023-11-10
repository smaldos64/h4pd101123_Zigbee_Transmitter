# h4pd101123_Zigbee_Transmitter

Dette Projekt er et projekt, der skal vise brugen af en ZigBee Transmitter (ZigBee Coordinator). Når der modtages karakterern 1 på UART2, så aflæses værdien på en AD-Konverter og denne værdi
sendes til en ZigBee Receiver. ZigBee Receiveren kan enten være koblet direkte til en PC ved brug af "det røde print" og brug af XCTU SW'eren. Eller også kan ZigBee Receiveren være kørende på 
f.eks. et andet AtMega2560 print eller en Raspberry Pi.
