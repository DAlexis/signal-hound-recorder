#!/bin/bash

set -e

tar -xvzf bb60_linux_x64_api_3016.tar.gz
cp linux_bb60_sdk/include/* /usr/include
cp linux_bb60_sdk/lib/* /usr/lib
cp linux_bb60_sdk/bb60.rules /etc/udev/rules.d/
ln -s /usr/lib/libbb_api.so.3.0.16 /usr/lib/libbb_api.so.3
ln -s /usr/lib/libbb_api.so.3 /usr/lib/libbb_api.so
