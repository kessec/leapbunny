/* versions.h  -- emerald-boot versions
 *
 * Copyright 2010-2011 LeapFrog Enterprises Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef VERSIONS_H
#define VERSIONS_H

// NOTE: Whenever there is a new release or a version changes, review the
//       #defines in this file and revise them as necessary

 
#define EMERALD_BOOT_VERSION_STRING "EB2.2.2\n"
        // This is the string that is sent to the serial port during boot,
        // whether or not the boot module was built with DEBUG defined.
        // The numeric fields are the same as those in the firmware release
        // version.

#define INQ_PRODUCT_REVISION_LEVEL 	'2', '.', '2', '2'
        // This is the product revision level that's in bytes 32-35 of
        // the response to a scsi INQUIRY command.
        // The first character represents the first field of the firmware
        // release version.
        // The third character represents the second field of the firmware
        // release version.
        // The fourth character represents the third field of the firmware
        // release version.
        // If the firmware version field has the value 'x', the corresponding
        // character in INQ_PRODUCT_REVISION_LEVEL is the x-th character in 
        // the sequence 0,1,2,...,9,A,B,C,...,Z,a,b,c,...,z.
        // Obviously this assumes each field of a firmware version is in the
        // interval [0,61].

#endif  /*VERSIONS_H*/

