#!/bin/sh
cd /sys/kernel/config/usb_gadget/
mkdir g1
cd g1
echo "0x0200" > bcdUSB
echo "0x02" > bDeviceClass
echo "0x00" > bDeviceSubClass
echo "0x3066" > bcdDevice
echo "0x0b1f" > idVendor
echo "0x04ee" > idProduct
echo "1" > os_desc/use
echo "0xcd" > os_desc/b_vendor_code
echo "MSFT100" > os_desc/qw_sign

mkdir strings/0x409
echo "Insyde" > strings/0x409/manufacturer
echo "Virtual Lan" > strings/0x409/product
mkdir configs/c.1
mkdir configs/c.1/strings/0x409
echo "RNDIS" > configs/c.1/strings/0x409/configuration
mkdir functions/rndis.usb0
echo "RNDIS" > functions/rndis.usb0/os_desc/interface.rndis/compatible_id
echo "5162001" > functions/rndis.usb0/os_desc/interface.rndis/sub_compatible_id
echo "a0:42:3f:3b:66:8a" > functions/rndis.usb0/host_addr
echo "a0:42:3f:3b:66:89" > functions/rndis.usb0/dev_addr
ln -s functions/rndis.usb0 configs/c.1
ln -s configs/c.1 os_desc
echo 1e6a0000.usb-vhub:p5 > UDC