#include "fpc_linux_helper.h"

char FpcLinuxUtil::device_path_[PATH_MAX];

const char* FpcLinuxUtil::kSysBusSpiDriver = "/sys/bus/spi/drivers/";
const char* FpcLinuxUtil::kSysBusSpiDevices = "/sys/bus/spi/devices/";

const char* FpcLinuxUtil::getSysfsSpiDevicePath(const char* deviceName) {
    const char* result = NULL;
    char driver_path[PATH_MAX];

    int length = snprintf(driver_path, PATH_MAX, "%s%s", kSysBusSpiDriver, deviceName);

    if (length < 0) {
        errno = -length;
        return NULL;
    } else if (length >= PATH_MAX) {
        errno = EOVERFLOW;
        return NULL;
    }

    DIR* driver_dir = opendir(driver_path);
    if (!driver_dir) {
        if (errno == EACCES) {
            LOGE("failed to scan sysfs for driver, permission denied");
        } else if (errno == ENOENT) {
            LOGE("%s no such entry", kSysBusSpiDriver);
        } else {
            LOGE("failed to open %s error %i", kSysBusSpiDriver, errno);
        }
        return NULL;
    }

    while (true) {
        dirent* current_dir = readdir(driver_dir);
        if (!current_dir) {
            LOGE("failed to find any matched spi device in %s", kSysBusSpiDriver);
            errno = -ENOENT;
            goto out;
        }

        if (strncmp("spi", current_dir->d_name, 3) == 0) {
            length = snprintf(device_path_, PATH_MAX,"%s%s",kSysBusSpiDevices, current_dir->d_name);
            if (length < 0) {
                errno = -length;
                return NULL;
            } else if (length >= PATH_MAX) {
                errno = EOVERFLOW;
                return NULL;
            }
            LOGD("getSysfsSpiDevicePath returning %s", device_path_);
            result = device_path_;
            break;
        }
    }

out:
    closedir(driver_dir);
    return result;
}
