/* scsi.h  -- SCSI command codes
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

//+=============================================================================+
//|           D - DIRECT ACCESS DEVICE                       Device Column Key  |
//|           .T - SEQUENTIAL ACCESS DEVICE                  M = Mandatory      |
//|           . L - PRINTER DEVICE                           O = Optional       |
//|           .  P - PROCESSOR DEVICE                        V = Vendor Specific|
//|           .  .W - WRITE ONCE READ MULTIPLE DEVICE        R = Reserved       |
//|           .  . R - READ ONLY (CD-ROM) DEVICE                                |
//|           .  .  S - SCANNER DEVICE                                          |
//|           .  .  .O - OPTICAL MEMORY DEVICE                                  |
//|           .  .  . M - MEDIA CHANGER DEVICE                                  |
//|           .  .  .  C - COMMUNICATION DEVICE                                 |
//|           .  .  .  .                                                        |
//|        OP DTLPWRSOMC Description                                            |
//|----------+----------+-------------------------------------------------------|
//|        00 MMMMMMMMMM TEST UNIT GPIFREADY                                        |
#define TEST_UNIT_READY                                        0x00
//|        01  M         REWIND                                                 |
#define REWIND                                                 0x01
//|        01 O V OO OO  REZERO UNIT                                            |
#define REZERO_UNIT                                            0x01
//|        02 VVVVVV  V                                                         |
//|        03 MMMMMMMMMM REQUEST SENSE                                          |
#define REQUEST_SENSE                                          0x03
//|        04   O        FORMAT                                                 |
#define FORMAT                                                 0x04
//|        04 M      O   FORMAT UNIT                                            |
#define FORMAT_UNIT                                            0x04
//|        05 VMVVVV  V  READ BLOCK LIMITS                                      |
#define READ_BLOCK_LIMITS                                      0x05
//|        06 VVVVVV  V                                                         |
//|        07         O  INITIALIZE ELEMENT STATUS                              |
#define INITIALIZE_ELEMENT_STATUS                              0x07
//|        07 OVV O  OV  REASSIGN BLOCKS                                        |
#define REASSIGN_BLOCKS                                        0x07
//|        08          M GET MESSAGE(06)                                        |
#define GET_MESSAGE_06                                        0x08
//|        08 OMV OO OV  READ(06)                                               |
#define READ_06                                               0x08
//|        08    O       RECEIVE                                                |
#define RECEIVE                                                0x08
//|        09 VVVVVV  V                                                         |
//|        0A   M        PRINT                                                  |
#define PRINT                                                  0x0A
//|        0A          M SEND MESSAGE(06)                                       |
#define SEND_MESSAGE_06                                       0x0A
//|        0A    M       SEND(06)                                               |
#define SEND_06                                               0x0A
//|        0A OM  O  OV  WRITE(06)                                              |
#define WRITE_06                                              0x0A
//|        0B O   OO OV  SEEK(06)                                               |
#define SEEK_06                                               0x0B
//|        0B   O        SLEW AND PRINT                                         |
#define SLEW_AND_PRINT                                         0x0B
//|        0C VVVVVV  V                                                         |
//|        0D VVVVVV  V                                                         |
//|        0E VVVVVV  V                                                         |
//|        0F VOVVVV  V  READ REVERSE                                           |
#define READ_REVERSE                                           0x0F
//|        10   O O      SYNCHRONIZE BUFFER                                     |
#define SYNCHRONIZE_BUFFER                                     0x10
//|        10 VM VVV     WRITE FILEMARKS                                        |
#define WRITE_FILEMARKS                                        0x10
//|        11 VMVVVV     SPACE                                                  |
#define SPACE                                                  0x11
//|        12 MMMMMMMMMM INQUIRY                                                |
#define INQUIRY                                                0x12
//|        13 VOVVVV     VERIFY(06)                                             |
#define VERIFY_06                                             0x13
//|        14 VOOVVV     RECOVER BUFFERED DATA                                  |
#define RECOVER_BUFFERED_DATA                                  0x14
//|        15 OMO OOOOOO MODE SELECT(06)                                        |
#define MODE_SELECT_06                                        0x15
//|        16 M   MM MO  RESERVE                                                |
#define RESERVE                                                0x16
//|        16  MM   M    RESERVE UNIT                                           |
#define RESERVE_UNIT                                           0x16
//|        17 M   MM MO  RELEASE                                                |
#define RELEASE                                                0x17
//|        17  MM   M    RELEASE UNIT                                           |
#define RELEASE_UNIT                                           0x17
//|        18 OOOOOOOO   COPY                                                   |
#define COPY                                                   0x18
//|        19 VMVVVV     ERASE                                                  |
#define ERASE                                                  0x19
//|        1A OMO OOOOOO MODE SENSE(06)                                         |
#define MODE_SENSE_06                                         0x1A
//|        1B  O         LOAD UNLOAD                                            |
#define LOAD_UNLOAD                                            0x1B
//|        1B       O    SCAN                                                   |
#define SCAN                                                   0x1B
//|        1B   O        STOP PRINT                                             |
#define STOP_PRINT                                             0x1B
//|        1B O   OO O   STOP START UNIT                                        |
#define STOP_START_UNIT                                        0x1B
//+=============================================================================+
//
//
//Table 365: (continued)
//+=============================================================================+
//|           D - DIRECT ACCESS DEVICE                       Device Column Key  |
//|           .T - SEQUENTIAL ACCESS DEVICE                  M = Mandatory      |
//|           . L - PRINTER DEVICE                           O = Optional       |
//|           .  P - PROCESSOR DEVICE                        V = Vendor Specific|
//|           .  .W - WRITE ONCE READ MULTIPLE DEVICE        R = Reserved       |
//|           .  . R - READ ONLY (CD-ROM) DEVICE                                |
//|           .  .  S - SCANNER DEVICE                                          |
//|           .  .  .O - OPTICAL MEMORY DEVICE                                  |
//|           .  .  . M - MEDIA CHANGER DEVICE                                  |
//|           .  .  .  C - COMMUNICATION DEVICE                                 |
//|           .  .  .  .                                                        |
//|        OP DTLPWRSOMC Description                                            |
//|----------+----------+-------------------------------------------------------|
//|        1C OOOOOOOOOO RECEIVE DIAGNOSTIC RESULTS                             |
#define RECEIVE_DIAGNOSTIC_RESULTS                             0x1C
//|        1D MMMMMMMMMM SEND DIAGNOSTIC                                        |
#define SEND_DIAGNOSTIC                                        0x1D
//|        1E OO  OO OO  PREVENT ALLOW MEDIUM REMOVAL                           |
#define PREVENT_ALLOW_MEDIUM_REMOVAL                           0x1E
//|        1F                                                                   |
//|        20 V   VV V                                                          |
    //|        21 V   VV V                                                          |
//|        22 V   VV V                                                          |
//|        23 V   VV V                                                          |
#define READ_FORMAT_CAPACITIES                                 0x23
//|        24 V   VVM    SET WINDOW                                             |
#define SET_WINDOW                                             0x24
//|        25       O    GET WINDOW                                             |
#define GET_WINDOW                                             0x25
//|        25 M   M  M   READ CAPACITY                                          |
#define READ_CAPACITY                                          0x25
//|        25      M     READ CD-ROM CAPACITY                                   |
#define READ_CD_ROM_CAPACITY                                   0x25
#define SCSI_CAPACITY_SECTOR_COUNT_MSB  2
#define SCSI_CAPACITY_SECTOR_COUNT_LSB  3
#define SCSI_CAPACITY_SECTOR_SIZE_MSB  6
#define SCSI_CAPACITY_SECTOR_SIZE_LSB  7
//|        26 V   VV                                                            |
//|        27 V   VV                                                            |
//|        28          O GET MESSAGE(10)                                        |
#define GET_MESSAGE_10                                        0x28
//|        28 M   MMMM   READ(10)                                               |
#define READ_10                                               0x28
//|        29 V   VV O   READ GENERATION                                        |
#define READ_GENERATION                                        0x29
//|        2A          O SEND MESSAGE(10)                                       |
#define SEND_MESSAGE_10                                       0x2A
//|        2A       O    SEND(10)                                               |
#define SEND_10                                               0x2A
//|        2A M   M  M   WRITE(10)                                              |
#define WRITE_10                                              0x2A
//|        2B  O         LOCATE                                                 |
#define LOCATE                                                 0x2B
//|        2B         O  POSITION TO ELEMENT                                    |
#define POSITION_TO_ELEMENT                                    0x2B
//|        2B O   OO O   SEEK(10)                                               |
#define SEEK_10                                               0x2B
//|        2C V      O   ERASE(10)                                              |
#define ERASE_10                                              0x2C
//|        2D V   O  O   READ UPDATED BLOCK                                     |
#define READ_UPDATED_BLOCK                                     0x2D
//|        2E O   O  O   WRITE AND VERIFY(10)                                   |
#define WRITE_AND_VERIFY_10                                   0x2E
//|        2F O   OO O   VERIFY(10)                                             |
#define VERIFY_10                                             0x2F
//|        30 O   OO O   SEARCH DATA HIGH(10)                                   |
#define SEARCH_DATA_HIGH_10                                   0x30
//|        31       O    OBJECT POSITION                                        |
#define OBJECT_POSITION                                        0x31
//|        31 O   OO O   SEARCH DATA EQUAL(10)                                  |
#define SEARCH_DATA_EQUAL_10                                  0x31
//|        32 O   OO O   SEARCH DATA LOW(10)                                    |
#define SEARCH_DATA_LOW_10                                    0x32
//|        33 O   OO O   SET LIMITS(10)                                         |
#define SET_LIMITS_10                                         0x33
//|        34       O    GET DATA BUFFER STATUS                                 |
#define GET_DATA_BUFFER_STATUS                                 0x34
//|        34 O   OO O   PRE-FETCH                                              |
//#define PRE-FETCH                                              0x34
// preceding line gave warning because of the hyphen
#define PREFETCH                                              0x34
//|        34  O         READ POSITION                                          |
#define READ_POSITION                                          0x34
//|        35 O   OO O   SYNCHRONIZE CACHE                                      |
#define SYNCHRONIZE_CACHE                                      0x35
//|        36 O   OO O   LOCK UNLOCK CACHE                                      |
#define LOCK_UNLOCK_CACHE                                      0x36
//|        37 O      O   READ DEFECT DATA(10)                                   |
#define READ_DEFECT_DATA_10                                   0x37
//|        38     O  O   MEDIUM SCAN                                            |
#define MEDIUM_SCAN                                            0x38
//|        39 OOOOOOOO   COMPARE                                                |
#define COMPARE                                                0x39
//|        3A OOOOOOOO   COPY AND VERIFY                                        |
#define COPY_AND_VERIFY                                        0x3A
//|        3B OOOOOOOOOO WRITE BUFFER                                           |
#define WRITE_BUFFER                                           0x3B
//|        3C OOOOOOOOOO READ BUFFER                                            |
#define READ_BUFFER                                            0x3C
//|        3D     O  O   UPDATE BLOCK                                           |
#define UPDATE_BLOCK                                           0x3D
//|        3E O   OO O   READ LONG                                              |
#define READ_LONG                                              0x3E
//|        3F O   O  O   WRITE LONG                                             |
#define WRITE_LONG                                             0x3F
//+=============================================================================+
//
//
//Table 365: (continued)
//+=============================================================================+
//|           D - DIRECT ACCESS DEVICE                       Device Column Key  |
//|           .T - SEQUENTIAL ACCESS DEVICE                  M = Mandatory      |
//|           . L - PRINTER DEVICE                           O = Optional       |
//|           .  P - PROCESSOR DEVICE                        V = Vendor Specific|
//|           .  .W - WRITE ONCE READ MULTIPLE DEVICE        R = Reserved       |
//|           .  . R - READ ONLY (CD-ROM) DEVICE                                |
//|           .  .  S - SCANNER DEVICE                                          |
//|           .  .  .O - OPTICAL MEMORY DEVICE                                  |
//|           .  .  . M - MEDIA CHANGER DEVICE                                  |
//|           .  .  .  C - COMMUNICATION DEVICE                                 |
//|           .  .  .  .                                                        |
//|        OP DTLPWRSOMC Description                                            |
//|----------+----------+-------------------------------------------------------|
//|        40 OOOOOOOOOO CHANGE DEFINITION                                      |
#define CHANGE_DEFINITION                                      0x40
//|        41 O          WRITE SAME                                             |
#define WRITE_SAME                                             0x41
//|        42      O     READ SUB-CHANNEL                                       |
#define READ_SUB_CHANNEL                                       0x42
//|        43      O     READ TOC                                               |
#define READ_TOC                                               0x43
//|        44      O     READ HEADER                                            |
#define READ_HEADER                                            0x44
//|        45      O     PLAY AUDIO(10)                                         |
#define PLAY_AUDIO_10                                         0x45
//|        46                                                                   |
//|        47      O     PLAY AUDIO MSF                                         |
#define PLAY_AUDIO_MSF                                         0x47
//|        48      O     PLAY AUDIO TRACK INDEX                                 |
#define PLAY_AUDIO_TRACK_INDEX                                 0x48
//|        49      O     PLAY TRACK RELATIVE(10)                                |
#define PLAY_TRACK_RELATIVE_10                                0x49
//|        4A                                                                   |
//|        4B      O     PAUSE RESUME                                           |
#define PAUSE_RESUME                                           0x4B
//|        4C OOOOOOOOOO LOG SELECT                                             |
#define LOG_SELECT                                             0x4C
//|        4D OOOOOOOOOO LOG SENSE                                              |
#define LOG_SENSE                                              0x4D
//|        4E                                                                   |
//|        4F                                                                   |
//|        50                                                                   |
//|        51                                                                   |
//|        52                                                                   |
//|        53                                                                   |
//|        54                                                                   |
//|        55 OOO OOOOOO MODE SELECT(10)                                        |
#define MODE_SELECT_10                                        0x55
//|        56                                                                   |
//|        57                                                                   |
//|        58                                                                   |
//|        59                                                                   |
//|        5A OOO OOOOOO MODE SENSE(10)                                         |
#define MODE_SENSE_10                                         0x5A
//|        5B                                                                   |
//|        5C                                                                   |
//|        5D                                                                   |
//|        5E                                                                   |
//|        5F                                                                   |
//+=============================================================================+
//
//
//Table 365: (concluded)
//+=============================================================================+
//|           D - DIRECT ACCESS DEVICE                       Device Column Key  |
//|           .T - SEQUENTIAL ACCESS DEVICE                  M = Mandatory      |
//|           . L - PRINTER DEVICE                           O = Optional       |
//|           .  P - PROCESSOR DEVICE                        V = Vendor Specific|
//|           .  .W - WRITE ONCE READ MULTIPLE DEVICE        R = Reserved       |
//|           .  . R - READ ONLY (CD-ROM) DEVICE                                |
//|           .  .  S - SCANNER DEVICE                                          |
//|           .  .  .O - OPTICAL MEMORY DEVICE                                  |
//|           .  .  . M - MEDIA CHANGER DEVICE                                  |
//|           .  .  .  C - COMMUNICATION DEVICE                                 |
//|           .  .  .  .                                                        |
//|        OP DTLPWRSOMC Description                                            |
//|----------+----------+-------------------------------------------------------|
//|        A0                                                                   |
//|        A1                                                                   |
//|        A2                                                                   |
//|        A3                                                                   |
//|        A4                                                                   |
//|        A5         M  MOVE MEDIUM                                            |
#define MOVE_MEDIUM                                            0xA5
//|        A5      O     PLAY AUDIO(12)                                         |
#define PLAY_AUDIO_12                                         0xA5
//|        A6         O  EXCHANGE MEDIUM                                        |
#define EXCHANGE_MEDIUM                                        0xA6
//|        A7                                                                   |
//|        A8          O GET MESSAGE(12)                                        |
#define GET_MESSAGE_12                                        0xA8
//|        A8     OO O   READ(12)                                               |
#define READ_12                                               0xA8
//|        A9      O     PLAY TRACK RELATIVE(12)                                |
#define PLAY_TRACK_RELATIVE_12                                0xA9
//|        AA          O SEND MESSAGE(12)                                       |
#define SEND_MESSAGE_12                                       0xAA
//|        AA     O  O   WRITE(12)                                              |
#define WRITE_12                                              0xAA
//|        AB                                                                   |
//|        AC        O   ERASE(12)                                              |
#define ERASE_12                                              0xAC
//|        AD                                                                   |
//|        AE     O  O   WRITE AND VERIFY(12)                                   |
#define WRITE_AND_VERIFY_12                                   0xAE
//|        AF     OO O   VERIFY(12)                                             |
#define VERIFY_12                                             0xAF
//|        B0     OO O   SEARCH DATA HIGH(12)                                   |
#define SEARCH_DATA_HIGH_12                                   0xB0
//|        B1     OO O   SEARCH DATA EQUAL(12)                                  |
#define SEARCH_DATA_EQUAL_12                                  0xB1
//|        B2     OO O   SEARCH DATA LOW(12)                                    |
#define SEARCH_DATA_LOW_12                                    0xB2
//|        B3     OO O   SET LIMITS(12)                                         |
#define SET_LIMITS_12                                         0xB3
//|        B4                                                                   |
//|        B5                                                                   |
//|        B5         O  REQUEST VOLUME ELEMENT ADDRESS                         |
#define REQUEST_VOLUME_ELEMENT_ADDRESS                         0xB5
//|        B6                                                                   |
//|        B6         O  SEND VOLUME TAG                                        |
#define SEND_VOLUME_TAG                                        0xB6
//|        B7        O   READ DEFECT DATA(12)                                   |
#define READ_DEFECT_DATA_12                                   0xB7
//|        B8                                                                   |
//|        B8         O  READ ELEMENT STATUS                                    
#define READ_ELEMENT_STATUS                                   0xB8

#define SENSE_LEN 18

#define senseCRCError               0
#define senseInvalidFieldInCDB      1
#define senseOk                     2
#define senseNoMedia                3
#define senseWriteFault             4
#define senseReadError              5
#define senseAddrNotFound           6
#define senseInvalidOpcode          7
#define senseInvalidLBA             8
#define senseInvalidParameter       9
#define senseCantEject              0xa
#define senseMediaChanged           0xb
#define senseDeviceReset            0xc
#define senseWriteProtected         0xd
#define senseNotReady               0xe

