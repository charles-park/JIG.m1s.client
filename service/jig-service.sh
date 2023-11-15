#!/bin/bash

# device stable delay
sleep 10 && sync

#--------------------------
# ODROID-M1S Client enable
#--------------------------
/root/JIG.m1s.client/JIG.m1s.client > /dev/null 2>&1
