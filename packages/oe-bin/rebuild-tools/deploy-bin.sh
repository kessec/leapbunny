#!/bin/sh

cd image
tar -cf $1 *
cp -f *.tar ..
cp -f *.tar ~/workspace/LinuxDist/packages/oe-bin
