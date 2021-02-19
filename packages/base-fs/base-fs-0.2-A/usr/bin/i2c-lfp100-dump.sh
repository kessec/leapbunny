#!/bin/sh
# Read LFP100 registers
CHANNEL="/dev/i2c-0"
ADDR="0xCD"

#
# '*PW' in register name listing means this is a password protected register
#
echo -n "CHIPID                      (0x00): "; i2c-ctl $CHANNEL r $ADDR 0x00 2
echo -n "CONTROL                     (0x01): "; i2c-ctl $CHANNEL r $ADDR 0x01 2
echo -n "STATUS 1                    (0x02): "; i2c-ctl $CHANNEL r $ADDR 0x02 2
echo -n "STATUS 2                    (0x03): "; i2c-ctl $CHANNEL r $ADDR 0x03 2
echo -n "INT 1                       (0x04): "; i2c-ctl $CHANNEL r $ADDR 0x04 2
echo -n "INT 2                       (0x05): "; i2c-ctl $CHANNEL r $ADDR 0x05 2
echo -n "INT 3                       (0x06): "; i2c-ctl $CHANNEL r $ADDR 0x06 2
echo -n "MASK 1                      (0x07): "; i2c-ctl $CHANNEL r $ADDR 0x07 2
echo -n "MASK 2                      (0x08): "; i2c-ctl $CHANNEL r $ADDR 0x08 2
echo -n "MASK 3                      (0x09): "; i2c-ctl $CHANNEL r $ADDR 0x09 2
echo -n "WLED                        (0x0A): "; i2c-ctl $CHANNEL r $ADDR 0x0a 2
echo -n "PPATH                       (0x0B): "; i2c-ctl $CHANNEL r $ADDR 0x0b 2
echo -n "IO                          (0x0C): "; i2c-ctl $CHANNEL r $ADDR 0x0c 2
echo -n "PASSWORD                    (0x0D): "; i2c-ctl $CHANNEL r $ADDR 0x0d 2
echo -n "P_ENABLE                    (0x0E): "; i2c-ctl $CHANNEL r $ADDR 0x0e 2
echo -n "DCDC1 *PW                   (0x0F): "; i2c-ctl $CHANNEL r $ADDR 0x0f 2
echo -n "DCDC2 *PW                   (0x10): "; i2c-ctl $CHANNEL r $ADDR 0x10 2
echo -n "SLEW *PW                    (0x11): "; i2c-ctl $CHANNEL r $ADDR 0x11 2
echo -n "LDO1 *PW                    (0x12): "; i2c-ctl $CHANNEL r $ADDR 0x12 2
echo -n "LDO2 *PW                    (0x13): "; i2c-ctl $CHANNEL r $ADDR 0x13 2
echo -n "LDO3 *PW                    (0x14): "; i2c-ctl $CHANNEL r $ADDR 0x14 2
echo -n "PG *PW                      (0x15): "; i2c-ctl $CHANNEL r $ADDR 0x15 2
echo -n "UVLO *PW                    (0x16): "; i2c-ctl $CHANNEL r $ADDR 0x16 2
echo -n "SEQ1 *PW                    (0x17): "; i2c-ctl $CHANNEL r $ADDR 0x17 2
echo -n "SEQ2 *PW                    (0x18): "; i2c-ctl $CHANNEL r $ADDR 0x18 2
echo -n "SEQ3 *PW                    (0x19): "; i2c-ctl $CHANNEL r $ADDR 0x19 2
echo -n "SEQ4 *PW                    (0x1A): "; i2c-ctl $CHANNEL r $ADDR 0x1a 2
echo -n "SEQ5 *PW                    (0x1B): "; i2c-ctl $CHANNEL r $ADDR 0x1b 2
echo -n "FORMAT *PW                  (0x1C): "; i2c-ctl $CHANNEL r $ADDR 0x1c 2
echo -n "FILTER                      (0x1D): "; i2c-ctl $CHANNEL r $ADDR 0x1d 2
echo -n "A_APOP *PW                  (0x1E): "; i2c-ctl $CHANNEL r $ADDR 0x1e 2
echo -n "A_CONTROL                   (0x1F): "; i2c-ctl $CHANNEL r $ADDR 0x1f 2
echo -n "GAINSL                      (0x20): "; i2c-ctl $CHANNEL r $ADDR 0x20 2
echo -n "GAINADJ *PW                 (0x21): "; i2c-ctl $CHANNEL r $ADDR 0x21 2
echo -n "MGAIN                       (0x22): "; i2c-ctl $CHANNEL r $ADDR 0x22 2
echo -n "VOLUME                      (0x23): "; i2c-ctl $CHANNEL r $ADDR 0x23 2
echo -n "VLIMIT *PW                  (0x24): "; i2c-ctl $CHANNEL r $ADDR 0x24 2
echo -n "MICGAIN                     (0x25): "; i2c-ctl $CHANNEL r $ADDR 0x25 2
echo -n "TEST31H                     (0x31): "; i2c-ctl $CHANNEL r $ADDR 0x31 2
echo -n "TEST40H                     (0x40): "; i2c-ctl $CHANNEL r $ADDR 0x40 2
echo -n "TEST41H                     (0x41): "; i2c-ctl $CHANNEL r $ADDR 0x41 2
echo -n "TEST42H                     (0x42): "; i2c-ctl $CHANNEL r $ADDR 0x42 2
echo -n "TEST43H                     (0x43): "; i2c-ctl $CHANNEL r $ADDR 0x43 2
echo -n "TEST4BH                     (0x4B): "; i2c-ctl $CHANNEL r $ADDR 0x4B 2
echo -n "TEST4DH                     (0x4D): "; i2c-ctl $CHANNEL r $ADDR 0x4D 2
