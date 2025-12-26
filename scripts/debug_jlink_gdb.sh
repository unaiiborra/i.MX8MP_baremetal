#!/bin/bash

DEVICE="MIMX8MN6_A53_0"
SPEED=4000
IFACE=JTAG
ELF="/home/unab/files/master/tfm/imx8mp/project/bin/kernel.elf"
GDB="gdb"

_start=0x40000000 

sleep 2

JLinkGDBServer -if $IFACE -device $DEVICE -speed $SPEED &

if [[ -e /dev/ttyACM0 ]]; then
    kitty --hold --title "ACM0" screen /dev/ttyACM0 115200 &
#   kitty --hold --title "ACM0" screen /dev/ttyACM1 115200 &
fi

if [[ -e /dev/ttyCH343USB0 ]]; then
    kitty --hold --title "CH343USB0" screen /dev/ttyCH343USB0 115200 &
#   kitty --hold --title "CH343USB0" screen /dev/ttyCH343USB1 115200 &
fi

kitty --title "GDB A53" \
    $GDB $ELF \
        --init-command=/home/unab/.gdbinit \
        -ex "set architecture aarch64" \
        -ex "target remote localhost:2331" \
        -ex "monitor halt" \
        -ex "set \$pc = $_start" \
        -ex "load" \
        -ex "layout split"

pkill screen

exit