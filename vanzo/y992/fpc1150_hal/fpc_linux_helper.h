#ifndef FPC_LINUX_HELPER_H
#define FPC_LINUX_HELPER_H

#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>

#include "fpc_log.h"
#include "errno.h"

class FpcLinuxUtil
{
public:
    static const char* getSysfsSpiDevicePath(const char* deviceName);

    static const char* kSysBusSpiDriver;
    static const char* kSysBusSpiDevices;

private:
    static char device_path_[];
    FpcLinuxUtil();
};

#endif // FPC_LINUX_HELPER_H
