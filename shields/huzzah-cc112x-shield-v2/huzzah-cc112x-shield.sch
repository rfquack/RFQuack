EESchema Schematic File Version 4
LIBS:huzzah-cc112x-shield-cache
EELAYER 29 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title "ESP8266 (Huzzah) - CC1120 (V-Chip) Shield"
Date "2019-05-13"
Rev "1"
Comp "Trend Micro Inc."
Comment1 "Forward-looking threat research team"
Comment2 "Designed by Philippe Lin"
Comment3 "License: GPLv2"
Comment4 ""
$EndDescr
Text Label 4250 2700 0    50   ~ 0
3V3
Wire Wire Line
	4250 2700 4650 2700
Text Label 4250 2000 0    50   ~ 0
ISR
Wire Wire Line
	4250 2000 4650 2000
Text Label 4250 1800 0    50   ~ 0
RST
Wire Wire Line
	4250 1800 4650 1800
Text Label 4250 2400 0    50   ~ 0
ISR_PKT
Wire Wire Line
	4650 2400 4250 2400
$Comp
L VChip:VT-CC112x-433M U1
U 1 1 5CD9ABDA
P 5550 1800
F 0 "U1" V 5400 1000 50  0000 L CNN
F 1 "VT-CC112x-433M" V 7050 1000 50  0000 L CNN
F 2 "VChip:VT-CC112x-433M" V 5300 1350 50  0001 C CNN
F 3 "" H 5550 1800 50  0001 C CNN
	1    5550 1800
	0    1    1    0   
$EndComp
$Comp
L Device:Antenna AE1
U 1 1 5CD84312
P 4100 1250
F 0 "AE1" H 4180 1239 50  0000 L CNN
F 1 "Antenna" H 4180 1148 50  0000 L CNN
F 2 "Connector_Coaxial:SMA_Amphenol_132289_EdgeMount" H 4100 1250 50  0001 C CNN
F 3 "~" H 4100 1250 50  0001 C CNN
	1    4100 1250
	1    0    0    -1  
$EndComp
Wire Wire Line
	4100 1450 4100 3000
Wire Wire Line
	4100 3000 4650 3000
NoConn ~ 2400 1600
NoConn ~ 2500 1600
Text Label 2000 3750 1    50   ~ 0
3V3
Wire Wire Line
	2000 3750 2000 3300
$Comp
L VChip:FEATHERWING MS1
U 1 1 5CDCA380
P 1700 3100
F 0 "MS1" H 1700 3100 50  0001 C CNN
F 1 "FEATHERWING" H 1700 3100 50  0001 C CNN
F 2 "VChip:FEATHERWING_DIM" H 1700 3100 50  0001 C CNN
F 3 "" H 1700 3100 50  0001 C CNN
	1    1700 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	3300 1150 3300 1600
Text Label 3100 1150 1    50   ~ 0
RST
Wire Wire Line
	3100 1150 3100 1600
Text Label 2900 1150 1    50   ~ 0
ISR_PKT
Text Label 4250 2500 0    50   ~ 0
CS
Wire Wire Line
	4250 2500 4650 2500
Text Label 2200 3750 1    50   ~ 0
GND
Wire Wire Line
	2200 3750 2200 3300
NoConn ~ 3400 1600
NoConn ~ 2800 1600
NoConn ~ 3400 3300
NoConn ~ 3300 3300
NoConn ~ 3200 3300
NoConn ~ 2800 3300
NoConn ~ 2700 3300
NoConn ~ 2600 3300
NoConn ~ 2500 3300
NoConn ~ 2400 3300
NoConn ~ 2300 3300
NoConn ~ 2100 3300
NoConn ~ 1900 3300
Text Label 2300 1150 1    50   ~ 0
VBAT
Wire Wire Line
	2300 1150 2300 1600
$Comp
L power:PWR_FLAG #FLG0101
U 1 1 5CDE4098
P 700 800
F 0 "#FLG0101" H 700 875 50  0001 C CNN
F 1 "PWR_FLAG" H 700 973 50  0000 C CNN
F 2 "" H 700 800 50  0001 C CNN
F 3 "~" H 700 800 50  0001 C CNN
	1    700  800 
	1    0    0    -1  
$EndComp
$Comp
L power:PWR_FLAG #FLG0102
U 1 1 5CDE42D6
P 1100 800
F 0 "#FLG0102" H 1100 875 50  0001 C CNN
F 1 "PWR_FLAG" H 1100 973 50  0000 C CNN
F 2 "" H 1100 800 50  0001 C CNN
F 3 "~" H 1100 800 50  0001 C CNN
	1    1100 800 
	1    0    0    -1  
$EndComp
$Comp
L power:PWR_FLAG #FLG0103
U 1 1 5CDE46D8
P 1500 800
F 0 "#FLG0103" H 1500 875 50  0001 C CNN
F 1 "PWR_FLAG" H 1500 973 50  0000 C CNN
F 2 "" H 1500 800 50  0001 C CNN
F 3 "~" H 1500 800 50  0001 C CNN
	1    1500 800 
	1    0    0    -1  
$EndComp
Text Label 700  1100 1    50   ~ 0
VBAT
Wire Wire Line
	700  1100 700  800 
Text Label 1100 1100 1    50   ~ 0
3V3
Wire Wire Line
	1100 1100 1100 800 
Text Label 1500 1100 1    50   ~ 0
GND
Wire Wire Line
	1500 1100 1500 800 
Text Label 5550 3350 0    50   ~ 0
GND
Wire Wire Line
	5550 3350 5550 3150
Connection ~ 5550 1950
Wire Wire Line
	5550 1950 5550 1800
Connection ~ 5550 2100
Wire Wire Line
	5550 2100 5550 1950
Connection ~ 5550 2850
Wire Wire Line
	5550 2850 5550 2100
Connection ~ 5550 3000
Wire Wire Line
	5550 3000 5550 2850
Connection ~ 5550 3150
Wire Wire Line
	5550 3150 5550 3000
Text Label 4250 2850 0    50   ~ 0
GND
Wire Wire Line
	4250 2850 4650 2850
Text Label 4250 3150 0    50   ~ 0
GND
Wire Wire Line
	4250 3150 4650 3150
Text Label 4250 2600 0    50   ~ 0
GND
Wire Wire Line
	4650 2600 4250 2600
Text Label 4250 2100 0    50   ~ 0
SI
Wire Wire Line
	4250 2100 4650 2100
Text Label 4250 2200 0    50   ~ 0
SCLK
Wire Wire Line
	4250 2200 4650 2200
Text Label 4250 2300 0    50   ~ 0
SO
Wire Wire Line
	4250 2300 4650 2300
Text Label 2900 3750 1    50   ~ 0
SCLK
Wire Wire Line
	2900 3750 2900 3300
Text Label 3000 3750 1    50   ~ 0
SI
Wire Wire Line
	3000 3750 3000 3300
Text Label 3100 3750 1    50   ~ 0
SO
Wire Wire Line
	3100 3750 3100 3300
Text Label 4250 1900 0    50   ~ 0
GPIO3
Wire Wire Line
	4250 1900 4650 1900
Text Label 2700 1150 1    50   ~ 0
GPIO3
NoConn ~ 2600 1600
Wire Wire Line
	3200 1600 3200 1150
Wire Wire Line
	2700 1150 2700 1600
Text Label 3200 1150 1    50   ~ 0
CS
Text Label 3300 1150 1    50   ~ 0
ISR
Wire Wire Line
	2900 1600 2900 1150
NoConn ~ 3000 1600
$EndSCHEMATC
