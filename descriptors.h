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

#ifndef _DESCRIPTORS_H_
#define _DESCRIPTORS_H_

#include <avr/pgmspace.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/USB/Class/HID.h>

#ifdef DEBUG

#include <LUFA/Drivers/USB/Class/CDC.h>

#define CDC_NOTIFICATION_EPNUM       2
#define CDC_TX_EPNUM                 3
#define CDC_RX_EPNUM                 4
#define CDC_NOTIFICATION_EPSIZE      8
#define CDC_TXRX_EPSIZE              16

#endif /* DEBUG */

#define KEYBOARD_EPNUM               1
#define KEYBOARD_EPSIZE              8

	
struct USB_descriptor_configuration {
	USB_Descriptor_Configuration_Header_t    config;
	USB_Descriptor_Interface_t               HID_interface;
	USB_HID_Descriptor_t                     HID_keyboard_HID;
	USB_Descriptor_Endpoint_t                HID_keyboard_endpoint;
#ifdef DEBUG
	USB_Descriptor_Interface_Association_t   CDC_IAD;
	USB_Descriptor_Interface_t               CDC_CCI_interface;
	CDC_FUNCTIONAL_DESCRIPTOR(2)             CDC_functional_int_header;
	CDC_FUNCTIONAL_DESCRIPTOR(1)             CDC_functional_ACM;
	CDC_FUNCTIONAL_DESCRIPTOR(2)             CDC_functional_union;
	USB_Descriptor_Endpoint_t                CDC_notification_ep;
	USB_Descriptor_Interface_t               CDC_DCI_interface;
	USB_Descriptor_Endpoint_t                CDC_data_out_ep;
	USB_Descriptor_Endpoint_t                CDC_data_in_ep;
#endif /* DEBUG */
};
				
/* Function Prototypes: */
uint16_t CALLBACK_USB_GetDescriptor(
	const uint16_t wValue,
	const uint8_t wIndex,
	void** const DescriptorAddress
) ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);

#endif
