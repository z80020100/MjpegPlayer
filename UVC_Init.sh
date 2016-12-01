#!/bin/sh
echo -e "UVC Inin\n"
chmod 777 /dev/video0
chmod 777 /dev/video1
echo 1 > /sys/bus/platform/drivers/usb20_otg/dwc_otg_conn_en
echo -e "Network adb Inin!\n"
setprop service.adb.tcp.port 5555
stop adbd
start adbd
exit 0