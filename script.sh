#!/bin/bash

ESP32_IP="192.168.1.5"
PORT="80"

get_system_stats() {
    CPU_USAGE=$(top -bn1 | grep "Cpu(s)" | sed "s/.*, *\([0-9]*\.[0-9]*\)%* id.*/\1/" | awk '{print 100 - $1}')
    RAM_USAGE=$(free | grep Mem | awk '{print ($3/$2 * 100.0)}')
    TEMP=$(sensors | grep -m 1 'CPU' | awk '{print $3}' | tr -d '+°C')
    DISK_CACHE=$(df -h | grep 'cache' | awk '{print $5}' | tr -d '%')
    DISK_01=$(df -h | grep '/mnt/disk1' | awk '{print $5}' | tr -d '%')
    DISK_02=$(df -h | grep '/mnt/disk2' | awk '{print $5}' | tr -d '%')

    UPTIME=$(uptime -p | sed 's/;/,/g')

    echo "CPU=$CPU_USAGE;RAM=$RAM_USAGE;TEMP=$TEMP;CACHE=$DISK_CACHE;DISK01=$DISK_01;DISK02=$DISK_02;UPTIME=$UPTIME"
}

while true; do
    STATS=$(get_system_stats)
    curl --silent --data "plain=$STATS" "http://$ESP32_IP:$PORT/stats"
    
    # Update every hour
    sleep 3600
done
