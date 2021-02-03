#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

extern const u8_ptr USB_STRING_DESC[];

extern const u8 sc_FullSpeedDeviceDescriptor[];
extern const u8 sc_FullSpeedUSBConfigData[];
extern const u8 sc_FullSpeedDeviceQualifierDescriptor[];
extern const u8 sc_FullSpeedOtherSpeedConfigData[];

extern const u8 sc_HighSpeedDeviceDescriptor[];
extern const u8 sc_HighSpeedUSBConfigData[];
extern const u8 sc_HighSpeedDeviceQualifierDescriptor[];
extern const u8 sc_HighSpeedOtherSpeedConfigData[];

extern const unsigned char device_info[];
extern const int device_info_size;
#endif	/* USB_DESCRIPTORS_H_ */
