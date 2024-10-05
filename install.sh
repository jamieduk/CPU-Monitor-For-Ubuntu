#!/bin/bash
# (c) J~Net 2024
#
# ./install.sh
#
function x(){
sudo chmod +x $1
}

echo "Installing..."

sudo cp ./cpu-monitor /usr/local/bin/
x /usr/local/bin/cpu-monitor
