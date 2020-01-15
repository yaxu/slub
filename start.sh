#!/bin/bash

killall esd artsd spread tm.pl datadirt
/etc/init.d/spread restart &
sleep 2;
/yaxu/lc/tm/tm.pl &
sudo /yaxu/datadirt/datadirt

