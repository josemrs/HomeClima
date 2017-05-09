#!/bin/bash

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
#                                                         #
# rpi_monitor.sh                                          #
# This collectd Exec-Script retrieves the status of your  #
# Raspberry Pi.                                           #
# As it's not possible to gain the values as user this    #
# script will execute the vcgencmd-binary with sudo.      #
# Make sure your Exec-user in collect d is allowed to     #
# execute the bin without being asked for a password.     #
#                                                         #
# Author: Rico Ullmann                                    #
# web: https://erinnerungsfragmente.de                    #
# github: https://github.com/rullmann                     #
#                                                         #
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # 

HOSTNAME="${COLLECTD_HOSTNAME:-RPi}"
INTERVAL="${COLLECTD_INTERVAL:-10}"

vcgencmd="sudo /opt/vc/bin/vcgencmd"

while sleep "$INTERVAL"; do
    # retrieve new values
    # define the commands to get the value from the raw data
    temp=$($vcgencmd measure_temp | tr -d "temp=" | tr -d "'C")
    temp_limit=$($vcgencmd get_config int | grep "temp_limit" | tr -d "temp_limit=")
    arm=$($vcgencmd measure_clock arm | tr -d "arm: frequency(45)=")
    core=$($vcgencmd measure_clock core | tr -d "core:  frequency(1)=")
    h264=$($vcgencmd measure_clock h264 | tr -d "h264:  frequency(28)=")
    isp=$($vcgencmd measure_clock isp | tr -d "isp: frequency(42)=")
    v3d=$($vcgencmd measure_clock v3d | tr -d "v3d: frequency(43)=")
    uart=$($vcgencmd measure_clock uart | tr -d "uart:  frequency(22)=")
    pwm=$($vcgencmd measure_clock pwm | tr -d "pwm: frequency(25)=")
    emmc=$($vcgencmd measure_clock emmc | tr -d "emmc:  frequency(47)=")
    pixel=$($vcgencmd measure_clock pixel | tr -d "pixel:   frequency(29)=")
    vec=$($vcgencmd measure_clock vec | tr -d "vec: frequency(10)=")
    hdmi=$($vcgencmd measure_clock hdmi | tr -d "hdmi:  frequency(9)=")
    dpi=$($vcgencmd measure_clock dpi | tr -d "dpi: frequency(4)=")
    vcore=$($vcgencmd measure_volts core | tr -d "core: volt=" | tr -d "V")
    vsdram_c=$($vcgencmd measure_volts sdram_c | tr -d "sdram_c:    volt=" | tr -d "V")
    vsdram_i=$($vcgencmd measure_volts sdram_i | tr -d "sdram_i:    volt=" | tr -d "V")
    vsdram_p=$($vcgencmd measure_volts sdram_p | tr -d "sdram_p:    volt=" | tr -d "V")

    # collectd would love to get the values:
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-temperature\" interval=$INTERVAL N:$temp"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-templimit\" interval=$INTERVAL N:$temp_limit"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-arm\" interval=$INTERVAL N:$arm"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-core\" interval=$INTERVAL N:$core"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-h264\" interval=$INTERVAL N:$h264"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-isp\" interval=$INTERVAL N:$isp"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-v3d\" interval=$INTERVAL N:$v3d"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-uart\" interval=$INTERVAL N:$uart"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-pwm\" interval=$INTERVAL N:$pwm"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-emmc\" interval=$INTERVAL N:$emmc"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-pixel\" interval=$INTERVAL N:$pixel"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-vec\" interval=$INTERVAL N:$vec"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-hdmi\" interval=$INTERVAL N:$hdmi"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-freq-dpi\" interval=$INTERVAL N:$dpi"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-volt-core\" interval=$INTERVAL N:$vcore"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-volt-sdram_c\" interval=$INTERVAL N:$vsdram_c"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-volt-sdram_i\" interval=$INTERVAL N:$vsdram_i"
    echo "PUTVAL \"$HOSTNAME/exec-rpimonitor/gauge-volt-sdram_p\" interval=$INTERVAL N:$vsdram_p"
done
