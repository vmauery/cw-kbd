/*
 * ex: set syntax=c tabstop=8 noexpandtab shiftwidth=8:
 *
 * cw-kbd is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as  published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * cw-kbd is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with cw-kbd.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright © 2009-2010, Vernon Mauery (N7OH)
*/

#include "descriptors.h"

/* This struct gets passed wholesale to the host, which uses it to determine
 * what kind of device it is dealing with.  The HID specs can tell you what all
 * the hard-coded values mean.
 */
USB_Descriptor_HIDReport_Datatype_t PROGMEM kbd_report[] = {
	0x05, 0x01,  /* Usage Page (Generic Desktop) */
	0x09, 0x06,  /* Usage (Keyboard) */
	0xa1, 0x01,  /* Collection (Application) */
	0x75, 0x01,  /*   Report Size (1) */
	0x95, 0x08,  /*   Report Count (8) */
	0x05, 0x07,  /*   Usage Page (Key Codes) */
	0x19, 0xe0,  /*   Usage Minimum (Keyboard LeftControl) */
	0x29, 0xe7,  /*   Usage Maximum (Keyboard Right GUI) */
	0x15, 0x00,  /*   Logical Minimum (0) */
	0x25, 0x01,  /*   Logical Maximum (1) */
	0x81, 0x02,  /*   Input (Data, Variable, Absolute) */
	0x95, 0x01,  /*   Report Count (1) */
	0x75, 0x08,  /*   Report Size (8) */
	0x81, 0x03,  /*   Input (Const, Variable, Absolute) */
	0x95, 0x05,  /*   Report Count (5) */
	0x75, 0x01,  /*   Report Size (1) */
	0x05, 0x08,  /*   Usage Page (LEDs) */
	0x19, 0x01,  /*   Usage Minimum (Num Lock) */
	0x29, 0x05,  /*   Usage Maximum (Kana) */
	0x91, 0x02,  /*   Output (Data, Variable, Absolute) */
	0x95, 0x01,  /*   Report Count (1) */
	0x75, 0x03,  /*   Report Size (3) */
	0x91, 0x03,  /*   Output (Const, Variable, Absolute) */
	0x95, 0x06,  /*   Report Count (6) */
	0x75, 0x08,  /*   Report Size (8) */
	0x15, 0x00,  /*   Logical Minimum (0) */
	0x25, 0x65,  /*   Logical Maximum (101) */
	0x05, 0x07,  /*   Usage Page (Keyboard) */
	0x19, 0x00,  /*   Usage Minimum (Reserved (no event indicated)) */
	0x29, 0x65,  /*   Usage Maximum (Keyboard Application) */
	0x81, 0x00,  /*   Input (Data, Array, Absolute) */
	0xc0         /* End Collection */
};

/* Device descriptor tells the high level view of the device.  This is the
 * first thing the host asks for when enumerating the device.  Then it goes on
 * to the configuration descriptor.
 */
USB_Descriptor_Device_t PROGMEM device_descriptor = {
	.Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},
	.USBSpecification = VERSION_BCD(01.10),
	.Class = 0xEF,
	.SubClass = 0x02,
	.Protocol = 0x01,
	.Endpoint0Size = FIXED_CONTROL_ENDPOINT_SIZE,
	.VendorID = 0xF055,
	.ProductID = 0x1337,
	.ReleaseNumber = 0x0000,
	.ManufacturerStrIndex = 0x01,
	.ProductStrIndex = 0x02,
	.SerialNumStrIndex = NO_DESCRIPTOR,
	.NumberOfConfigurations = FIXED_NUM_CONFIGURATIONS
};

/* The configuration descriptor is the one the describes the devices in all
 * their glory.  Ours is special because we are a compound device when compiled
 * in DEBUG mode and a single device otherwise.
 */
struct USB_descriptor_configuration PROGMEM configuration_descriptor = {
	.config = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Configuration_Header_t),
			.Type = DTYPE_Configuration
		},
		.TotalConfigurationSize = sizeof(struct USB_descriptor_configuration),
		.TotalInterfaces = 3,
		.ConfigurationNumber = 1,
		.ConfigurationStrIndex = NO_DESCRIPTOR,
		.ConfigAttributes = (USB_CONFIG_ATTR_BUSPOWERED | USB_CONFIG_ATTR_SELFPOWERED),
		.MaxPowerConsumption = USB_CONFIG_POWER_MA(100)
	},

	.HID_interface = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Interface_t),
			.Type = DTYPE_Interface
		},
		.InterfaceNumber = 0x00,
		.AlternateSetting = 0x00,
		.TotalEndpoints = 1,
		.Class = 0x03,
		.SubClass = 0x01,
		.Protocol = HID_BOOT_KEYBOARD_PROTOCOL,
		.InterfaceStrIndex = NO_DESCRIPTOR
	},

	.HID_keyboard_HID = {
		.Header = {
			.Size = sizeof(USB_HID_Descriptor_t),
			.Type = DTYPE_HID
		},
		.HIDSpec = VERSION_BCD(01.11),
		.CountryCode = 0x00,
		.TotalReportDescriptors = 1,
		.HIDReportType = DTYPE_Report,
		.HIDReportLength = sizeof(kbd_report)
	},

	.HID_keyboard_endpoint = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Endpoint_t),
			.Type = DTYPE_Endpoint
		},
		.EndpointAddress = (ENDPOINT_DESCRIPTOR_DIR_IN | KEYBOARD_EPNUM),
		.Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize = KEYBOARD_EPSIZE,
		.PollingIntervalMS = 0x0A
	},
#ifdef DEBUG
	.CDC_IAD = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Interface_Association_t),
			.Type = DTYPE_InterfaceAssociation
		},
		.FirstInterfaceIndex = 1,
		.TotalInterfaces = 2,
		.Class = 0x02,
		.SubClass = 0x02,
		.Protocol = 0x01,
		.IADStrIndex = NO_DESCRIPTOR
	},
	.CDC_CCI_interface = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Interface_t),
			.Type = DTYPE_Interface
		},
		.InterfaceNumber = 0x01,
		.AlternateSetting = 0,
		.TotalEndpoints = 1,
		.Class = 0x02,
		.SubClass = 0x02,
		.Protocol = 0x01,
		.InterfaceStrIndex = NO_DESCRIPTOR
	},

	.CDC_functional_int_header = {
		.Header = {
			.Size = sizeof(CDC_FUNCTIONAL_DESCRIPTOR(2)),
			.Type = 0x24
		},
		.SubType = 0x00,
		.Data = {0x01, 0x10}
	},

	.CDC_functional_ACM = {
		.Header = {
			.Size = sizeof(CDC_FUNCTIONAL_DESCRIPTOR(1)),
			.Type = 0x24
		},
		.SubType = 0x02,
		.Data = {0x06}
	},

	.CDC_functional_union = {
		.Header = {
			.Size = sizeof(CDC_FUNCTIONAL_DESCRIPTOR(2)),
			.Type = 0x24
		},
		.SubType = 0x06,
		.Data = {0x01, 0x02}
	},

	.CDC_notification_ep = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Endpoint_t),
			.Type = DTYPE_Endpoint
		},
		.EndpointAddress = (ENDPOINT_DESCRIPTOR_DIR_IN | CDC_NOTIFICATION_EPNUM),
		.Attributes = (EP_TYPE_INTERRUPT | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize = CDC_NOTIFICATION_EPSIZE,
		.PollingIntervalMS = 0xFF
	},

	.CDC_DCI_interface = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Interface_t),
			.Type = DTYPE_Interface
		},
		.InterfaceNumber = 0x02,
		.AlternateSetting = 0,
		.TotalEndpoints = 2,
		.Class = 0x0A,
		.SubClass = 0x00,
		.Protocol = 0x00,
		.InterfaceStrIndex = NO_DESCRIPTOR
	},

	.CDC_data_out_ep = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Endpoint_t),
			.Type = DTYPE_Endpoint
		},
		.EndpointAddress = (ENDPOINT_DESCRIPTOR_DIR_OUT | CDC_RX_EPNUM),
		.Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize = CDC_TXRX_EPSIZE,
		.PollingIntervalMS = 0x00
	},

	.CDC_data_in_ep = {
		.Header = {
			.Size = sizeof(USB_Descriptor_Endpoint_t),
			.Type = DTYPE_Endpoint
		},
		.EndpointAddress = (ENDPOINT_DESCRIPTOR_DIR_IN | CDC_TX_EPNUM),
		.Attributes = (EP_TYPE_BULK | ENDPOINT_ATTR_NO_SYNC | ENDPOINT_USAGE_DATA),
		.EndpointSize = CDC_TXRX_EPSIZE,
		.PollingIntervalMS = 0x00
	},
#endif /* DEBUG */
};

USB_Descriptor_String_t PROGMEM lang_str = {
	.Header = {
		.Size = USB_STRING_LEN(1),
		.Type = DTYPE_String
	},
	.UnicodeString = {LANGUAGE_ID_ENG}
};

USB_Descriptor_String_t PROGMEM mfg_str = {
	.Header = {
		.Size = USB_STRING_LEN(14),
		.Type = DTYPE_String
	},
	.UnicodeString = L"V Mauery, N7OH"
};

USB_Descriptor_String_t PROGMEM product_str = {
	.Header = {
		.Size = USB_STRING_LEN(15),
		.Type = DTYPE_String
	},
	.UnicodeString = L"CW USB Keyboard"
};

/* This is the function that interfaces with the library and where we pass back
 * the right values for what it asks for.  All those structs we just defined
 * are not registered anywhere, so they must be passed up here.
 */
uint16_t CALLBACK_USB_GetDescriptor(
	const uint16_t value,
	const uint8_t index,
	void** const address)
{
	uint8_t desc_type = (value >> 8);
	uint8_t desc_num = (value & 0xFF);
	uint16_t size;

	switch (desc_type) {
	case DTYPE_Device:
		*address = (void*)&device_descriptor;
		size = sizeof(USB_Descriptor_Device_t);
		break;

	case DTYPE_Configuration:
		*address = (void*)&configuration_descriptor;
		size = sizeof(struct USB_descriptor_configuration);
		break;

	case DTYPE_String:
		switch (desc_num) {
		case 0x00:
			*address = (void*)&lang_str;
			size = pgm_read_byte(&lang_str.Header.Size);
			break;

		case 0x01:
			*address = (void*)&mfg_str;
			size = pgm_read_byte(&mfg_str.Header.Size);
			break;

		case 0x02:
			*address = (void*)&product_str;
			size = pgm_read_byte(&product_str.Header.Size);
			break;
		default:
			size = 0;
			break;
		}
		break;

	case DTYPE_HID:
		*address = (void*)&configuration_descriptor.HID_keyboard_HID;
		size = sizeof(USB_HID_Descriptor_t);
		break;

	case DTYPE_Report:
		*address = (void*)&kbd_report;
		size = sizeof(kbd_report);
		break;
	default:
		*address = NULL;
		size = NO_DESCRIPTOR;
		break;
	}

	return size;
}
