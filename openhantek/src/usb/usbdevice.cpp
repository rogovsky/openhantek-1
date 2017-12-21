// SPDX-License-Identifier: GPL-2.0+

#include <QCoreApplication>
#include <QList>
#include <iostream>

#include "usbdevice.h"

#include "controlgetspeed.h"
#include "controlcode.h"
#include "models.h"
#include "utils/printutils.h"


USBDevice::USBDevice(DSOModel *model, libusb_device *device) : model(model), device(device) {
    libusb_ref_device(device);
    libusb_get_device_descriptor(device, &descriptor);
}

bool USBDevice::connectDevice(QString &errorMessage) {
    if (needsFirmware()) return false;
    if (isConnected()) return true;

    // Open device
    int errorCode = libusb_open(device, &(handle));
    if (errorCode != LIBUSB_SUCCESS) {
        handle = nullptr;
        errorMessage = QCoreApplication::translate("", "Couldn't open device: %1").arg(libUsbErrorString(errorCode));
        return false;
    }

    // Find and claim interface
    errorCode = LIBUSB_ERROR_NOT_FOUND;
    libusb_config_descriptor *configDescriptor;
    libusb_get_config_descriptor(device, 0, &configDescriptor);
    for (int interfaceIndex = 0; interfaceIndex < (int)configDescriptor->bNumInterfaces; ++interfaceIndex) {
        const libusb_interface *interface = &configDescriptor->interface[interfaceIndex];
        if (interface->num_altsetting < 1) continue;

        const libusb_interface_descriptor *interfaceDescriptor = &interface->altsetting[0];
        if (interfaceDescriptor->bInterfaceClass == LIBUSB_CLASS_VENDOR_SPEC &&
            interfaceDescriptor->bInterfaceSubClass == 0 && interfaceDescriptor->bInterfaceProtocol == 0 &&
            interfaceDescriptor->bNumEndpoints == 2) {
            errorCode = claimInterface(interfaceDescriptor, HANTEK_EP_OUT, HANTEK_EP_IN);
            break;
        }
    }

    libusb_free_config_descriptor(configDescriptor);

    if (errorCode != LIBUSB_SUCCESS) {
        errorMessage = QString("%1 (%2:%3)")
                           .arg(libUsbErrorString(errorCode))
                           .arg(libusb_get_bus_number(device), 3, 10, QLatin1Char('0'))
                           .arg(libusb_get_device_address(device), 3, 10, QLatin1Char('0'));
        return false;
    }

    return true;
}

USBDevice::~USBDevice() { connectionLost(); }

int USBDevice::claimInterface(const libusb_interface_descriptor *interfaceDescriptor, int endpointOut, int endPointIn) {
    int errorCode = libusb_claim_interface(this->handle, interfaceDescriptor->bInterfaceNumber);
    if (errorCode < 0) { return errorCode; }

    interface = interfaceDescriptor->bInterfaceNumber;

    // Check the maximum endpoint packet size
    const libusb_endpoint_descriptor *endpointDescriptor;
    this->outPacketLength = 0;
    this->inPacketLength = 0;
    for (int endpoint = 0; endpoint < interfaceDescriptor->bNumEndpoints; ++endpoint) {
        endpointDescriptor = &(interfaceDescriptor->endpoint[endpoint]);
        if (endpointDescriptor->bEndpointAddress == endpointOut) {
            this->outPacketLength = endpointDescriptor->wMaxPacketSize;
        } else if (endpointDescriptor->bEndpointAddress == endPointIn) {
            this->inPacketLength = endpointDescriptor->wMaxPacketSize;
        }
    }
    return LIBUSB_SUCCESS;
}

void USBDevice::connectionLost() {
    libusb_unref_device(device);

    if (!this->handle) return;

    // Release claimed interface
    libusb_release_interface(this->handle, this->interface);
    this->interface = -1;

    // Close device handle
    libusb_close(this->handle);
    this->handle = 0;

    emit deviceDisconnected();
}

bool USBDevice::isConnected() { return this->handle != 0; }

bool USBDevice::needsFirmware() {
    return this->descriptor.idProduct != model->productID || this->descriptor.idVendor != model->vendorID;
}

int USBDevice::bulkTransfer(unsigned char endpoint, unsigned char *data, unsigned int length, int attempts,
                            unsigned int timeout) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = LIBUSB_ERROR_TIMEOUT;
    int transferred;
    for (int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
        errorCode = libusb_bulk_transfer(this->handle, endpoint, data, length, &transferred, timeout);

    if (errorCode == LIBUSB_ERROR_NO_DEVICE) connectionLost();
    if (errorCode < 0)
        return errorCode;
    else
        return transferred;
}

/// \brief Bulk write to the oscilloscope.
/// \param data Buffer for the sent/recieved data.
/// \param length The length of the packet.
/// \param attempts The number of attempts, that are done on timeouts.
/// \return Number of sent bytes on success, libusb error code on error.
int USBDevice::bulkWrite(unsigned char *data, unsigned int length, int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = this->getConnectionSpeed();
    if (errorCode < 0) return errorCode;

    return this->bulkTransfer(HANTEK_EP_OUT, data, length, attempts);
}

/// \brief Bulk read from the oscilloscope.
/// \param data Buffer for the sent/recieved data.
/// \param length The length of the packet.
/// \param attempts The number of attempts, that are done on timeouts.
/// \return Number of received bytes on success, libusb error code on error.
int USBDevice::bulkRead(unsigned char *data, unsigned int length, int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = this->getConnectionSpeed();
    if (errorCode < 0) return errorCode;

    return this->bulkTransfer(HANTEK_EP_IN, data, length, attempts);
}

/// \brief Send a bulk command to the oscilloscope.
/// \param command The command, that should be sent.
/// \param attempts The number of attempts, that are done on timeouts.
/// \return Number of sent bytes on success, libusb error code on error.
int USBDevice::bulkCommand(DataArray<unsigned char> *command, int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    if (!allowBulkTransfer) return LIBUSB_SUCCESS;

    // Send BeginCommand control command
    int errorCode = this->controlWrite(CONTROL_BEGINCOMMAND, beginCommandControl.data(),
                                       beginCommandControl.getSize());
    if (errorCode < 0) return errorCode;

    // Send bulk command
    return this->bulkWrite(command->data(), command->getSize(), attempts);
}

/// \brief Multi packet bulk read from the oscilloscope.
/// \param data Buffer for the sent/recieved data.
/// \param length The length of data contained in the packets.
/// \param attempts The number of attempts, that are done on timeouts.
/// \return Number of received bytes on success, libusb error code on error.
int USBDevice::bulkReadMulti(unsigned char *data, unsigned length, int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = 0;

    errorCode = this->getConnectionSpeed();
    if (errorCode < 0) return errorCode;

    errorCode = this->inPacketLength;
    unsigned int packet, received = 0;
    for (packet = 0; received < length && errorCode == this->inPacketLength; ++packet) {
        errorCode = this->bulkTransfer(HANTEK_EP_IN, data + packet * this->inPacketLength,
                                       qMin(length - received, (unsigned int)this->inPacketLength), attempts,
                                       HANTEK_TIMEOUT_MULTI);
        if (errorCode > 0) received += errorCode;
    }

    if (received > 0)
        return received;
    else
        return errorCode;
}

/// \brief Control transfer to the oscilloscope.
/// \param type The request type, also sets the direction of the transfer.
/// \param request The request field of the packet.
/// \param data Buffer for the sent/recieved data.
/// \param length The length field of the packet.
/// \param value The value field of the packet.
/// \param index The index field of the packet.
/// \param attempts The number of attempts, that are done on timeouts.
/// \return Number of transferred bytes on success, libusb error code on error.
int USBDevice::controlTransfer(unsigned char type, unsigned char request, unsigned char *data, unsigned int length,
                               int value, int index, int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    int errorCode = LIBUSB_ERROR_TIMEOUT;
    for (int attempt = 0; (attempt < attempts || attempts == -1) && errorCode == LIBUSB_ERROR_TIMEOUT; ++attempt)
        errorCode = libusb_control_transfer(this->handle, type, request, value, index, data, length, HANTEK_TIMEOUT);

    if (errorCode == LIBUSB_ERROR_NO_DEVICE) connectionLost();
    return errorCode;
}

/// \brief Control write to the oscilloscope.
/// \param request The request field of the packet.
/// \param data Buffer for the sent/recieved data.
/// \param length The length field of the packet.
/// \param value The value field of the packet.
/// \param index The index field of the packet.
/// \param attempts The number of attempts, that are done on timeouts.
/// \return Number of sent bytes on success, libusb error code on error.
int USBDevice::controlWrite(unsigned char request, unsigned char *data, unsigned int length, int value, int index,
                            int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    return this->controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT, request, data, length, value, index,
                                 attempts);
}

/// \brief Control read to the oscilloscope.
/// \param request The request field of the packet.
/// \param data Buffer for the sent/recieved data.
/// \param length The length field of the packet.
/// \param value The value field of the packet.
/// \param index The index field of the packet.
/// \param attempts The number of attempts, that are done on timeouts.
/// \return Number of received bytes on success, libusb error code on error.
int USBDevice::controlRead(unsigned char request, unsigned char *data, unsigned int length, int value, int index,
                           int attempts) {
    if (!this->handle) return LIBUSB_ERROR_NO_DEVICE;

    return this->controlTransfer(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN, request, data, length, value, index,
                                 attempts);
}

/// \brief Gets the speed of the connection.
/// \return The ::ConnectionSpeed of the USB connection.
int USBDevice::getConnectionSpeed() {
    int errorCode;
    ControlGetSpeed response;

    errorCode = this->controlRead(CONTROL_GETSPEED, response.data(), response.getSize());
    if (errorCode < 0) return errorCode;

    return response.getSpeed();
}

/// \brief Gets the maximum size of one packet transmitted via bulk transfer.
/// \return The maximum packet size in bytes, -1 on error.
int USBDevice::getPacketSize() {
    const int s = this->getConnectionSpeed();
    if (s == CONNECTION_FULLSPEED)
        return 64;
    else if (s == CONNECTION_HIGHSPEED)
        return 512;
    else if (s > CONNECTION_HIGHSPEED) {
        std::cerr << "Unknown USB speed. Please correct source code in USBDevice::getPacketSize()" << std::endl;
        throw new std::runtime_error("Unknown USB speed");
    }
    return -1;
}

libusb_device *USBDevice::getRawDevice() const { return device; }

const DSOModel* USBDevice::getModel() const { return model; }

void USBDevice::setEnableBulkTransfer(bool enable) { allowBulkTransfer = enable; }

void USBDevice::overwriteInPacketLength(int len) { inPacketLength = len; }
