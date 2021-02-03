#!/usr/bin/env python

# Script for creating the symlinks to the pngs that serve as the screens in the bootloader
# This is done so that we can preserve the original names the art dept uses in case we need to
# request new art.

import os
import sys

if len(sys.argv) < 2:
	screen_path = "../packages"
else:
	screen_path = sys.argv[1]

symlinks = { 'Lex-ATTENTION_NEEDED.png': screen_path + '/screens/ATTENTION_NEEDED.png',
                     'Lex-DOWNLOAD_IN_PROGRESS.png': screen_path + '/screens/SYS_0005_no_yellow.png',
                     'Lex-LOW_BATTERY.png': screen_path + '/screens/LOW_BATTERY_0009.png',
                     'Lex-VISIT_ES.png': screen_path + '/screens-es/UI_DownloadTimeout.png',
                     'Lex-VISIT_FR.png': screen_path + '/screens-fr/UI_DownloadTimeout.png',
                     'Lex-VISIT.png': screen_path + '/screens/UI_DownloadTimeout.png',
                     'Lpad-ATTENTION_NEEDED.png': screen_path + '/screens/Madrid-Boot-07_SystemScreenAnim01.png',
                     'Lpad-DOWNLOAD_IN_PROGRESS.png': screen_path + '/screens/Madrid-Boot-StaticConnectScreen-01.png',
                     'Lpad-LOW_BATTERY.png': screen_path + '/screens/Madrid-Boot-battery-low.png',
                     'Lpad-VISIT_ES.png': 'Lpad-VISIT.png',
                     'Lpad-VISIT_FR.png': 'Lpad-VISIT.png',
                     'Lpad-VISIT.png': screen_path + '/screens/Madrid-Boot-SystemMessageScreens_02.png',
                  }

for link in symlinks:
	if os.path.lexists(link):
		os.remove(link)
	os.symlink(symlinks[link], link)