/*
 * Copyright (C) 2013 Fingerprint Cards AB
 */


#include "fpc102xdriver.h"

const char* Fpc1020Driver::kDevFile = "/dev/fpc1020";

int Fpc1020Driver::open()
{
    if (file_descriptor_)
        return 0;

    if (!sysfs_path_)
        sysfs_path_ = FpcLinuxUtil::getSysfsSpiDevicePath("fpc1020");

    if (!sysfs_path_)
        return -errno;

    file_descriptor_ = ::open(kDevFile,  O_RDONLY);

    if(file_descriptor_ < 0) {
        file_descriptor_ = 0;
        return -errno;
    }

    return 0;
}

void Fpc1020Driver::close()
{
    if (file_descriptor_) {
        ::close(file_descriptor_);
        file_descriptor_ = 0;
    }
}

int Fpc1020Driver::poll(unsigned int timeout)
{
    struct pollfd pfd;
    pfd.fd = file_descriptor_;
    pfd.events = POLLIN | POLLHUP;

    int status = ::poll(&pfd, 1, timeout);

    if (status < 0)
        return -errno;

    return pfd.revents;
}

int Fpc1020Driver::read(uint8_t* buffer, unsigned int size)
{
    int status = ::read(file_descriptor_, buffer, size);
    if (status < 0)
        return -errno;

     return status;
}

int Fpc1020Driver::writeAttribute_u8(const char* name, uint8_t value_u8) {
    return writeAttribute_u16(name, value_u8);
}

int Fpc1020Driver::writeAttribute_u16(const char* name, uint16_t value_u16) {
    char file_name[PATH_MAX];

    int status = snprintf(file_name, PATH_MAX, "%s/%s", sysfs_path_, name);

    if (status < 0)
        return status;
    else if (status >= PATH_MAX)
        return -EOVERFLOW;

    FILE* file = fopen(file_name, "w");
    if (!file)
        return -errno;

    status = fprintf(file, "%u", value_u16);

    if (fflush(file) == EOF)
        status = -errno;

    fclose(file);

    return status < 0 ? status : 0;
}

int Fpc1020Driver::readAttribute_u16(const char* name, uint16_t* value_u16)
{
    char file_name[PATH_MAX];

    int status = snprintf(file_name, PATH_MAX, "%s/%s", sysfs_path_, name);

    if (status < 0)
        return status;
    else if (status >= PATH_MAX)
        return -EOVERFLOW;

    FILE* file = fopen(file_name, "r");
    if (!file)
        return -errno;

    unsigned int value;

    if (fscanf(file, "%i", &value) == 1) {
        *value_u16 = value;
        status = 0;
    } else {
        status = -errno;
    }

    fclose(file);

    return status;
}

int Fpc1020Driver::setCaptureMode(fpc1020_capture_mode_t mode)
{
    int status;
    switch (mode) {
        case FPC1020_MODE_WAIT_AND_CAPTURE:
        case FPC1020_MODE_WAIT_FINGER_DOWN:
        case FPC1020_MODE_WAIT_FINGER_UP:
            status = setNonBlocking(true);
            break;
        default:
            status = setNonBlocking(false);
            break;
    }
    return (status < 0)? status : writeAttribute_u8("setup/capture_mode", mode);
}

int Fpc1020Driver::setNonBlocking(bool mode)
{
    int flags = fcntl(file_descriptor_, F_GETFL, 0);

    if (flags < 0)
        return flags;

    if (mode)
        flags |= O_NONBLOCK;
    else
        flags &= ~O_NONBLOCK;

    return fcntl(file_descriptor_, F_SETFL, flags);
}

