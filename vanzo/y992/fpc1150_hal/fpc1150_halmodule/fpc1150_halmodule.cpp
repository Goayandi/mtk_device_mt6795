#include "fpc102xdevice.h"
#include "cac.h"
#define FPC1150_WIDTH   80
#define FPC1150_HEIGHT  208

int32_t fpc1150_module_close(hw_device_t* device)
{
    if (device) {
        preprocessor_cleanup();
        fpc102x_device_t* fpc1150device = (fpc102x_device_t*) device;
        calibrationDestroy(fpc1150device->cac_calibration);
        delete fpc1150device->raw_image.buffer;
        delete fpc1150device->raw_image_fallback.buffer;
        delete fpc1150device->driver;
        delete fpc1150device;
    }
    return 0;
}

int fpc1150_module_open(const hw_module_t* module, const char* name,
        hw_device_t** device)
{
    *device = NULL;
    int status = 0;
    fpc102x_sensor_type_t this_sensor_type;
    if (strcmp(name, FPC_FINGERPRINT_SENSOR_DEVICE) != 0)
        return -EINVAL;

    Fpc1020Driver* driver = new Fpc1020Driver;
    fpc102x_device_t* dev = new fpc102x_device_t;

    const unsigned int image_size = Fpc1020Driver::kCaptureCount *
            FPC1150_HEIGHT * FPC1150_WIDTH;

    uint8_t* buffer = new uint8_t[image_size];
    uint8_t* fallback_buffer = new uint8_t[image_size];

    if (!dev  || !driver || !buffer)
        goto err_mem;

#ifdef USE_FPC2050
    this_sensor_type = FPC102X_SENSOR_TYPE_FPC1155;
#else
    this_sensor_type = FPC102X_SENSOR_TYPE_FPC1150;
#endif

    fpc102x_device_init(dev, FPC1150_WIDTH, FPC1150_HEIGHT, this_sensor_type);

    dev->raw_image.buffer = buffer;
    dev->raw_image.capacity = image_size;
    dev->raw_image_fallback.buffer = fallback_buffer;
    dev->raw_image_fallback.capacity = image_size;
    dev->device.common.tag = HARDWARE_DEVICE_TAG;
    dev->device.common.version = FPC_FINGERPRINT_DEVICE_API_VERSION_0;
    dev->device.common.module = (struct hw_module_t*) module;
    dev->device.common.close = fpc1150_module_close;
    dev->device.close = fpc102x_close;
    dev->device.open = fpc102x_open;
    dev->device.captureImage = fpc102x_captureImage;
    dev->device.captureSingleFrame = fpc102x_captureSingleFrame;
    dev->device.getImageFormat = fpc102x_getImageFormat;
    dev->device.setCallbacks = fpc102x_setCallbacks;
    dev->device.setProperty = fpc102x_setProperty;
    dev->driver = driver;
    dev->callback = NULL;
    dev->shouldExit = NULL;
    dev->callback_data = NULL;
    dev->cac_calibration = NULL;
    dev->test_capture_mode = FPC1020_MODE_IDLE;

    fpc102x_load_configuration(dev);
    if (strlen(dev->pp_DATA_config))
        status = preprocessor_init(dev->pp_DATA_config, &dev->frame_format);
    else
        status = preprocessor_init(NULL, &dev->frame_format);

    if (status)
        LOGD("pp_init returned %i", status);

    *device = (hw_device_t*) dev;

    return 0;

err_mem:
    delete dev;
    delete driver;
    return -ENOMEM;
}

static struct hw_module_methods_t fpc1150_module_methods = {
    fpc1150_module_open
};

fpc_fingerprint_moudule_t HAL_MODULE_INFO_SYM = {
    {
        HARDWARE_MODULE_TAG,
        FPC_FINGERPRINT_MODULE_API_VERSION_0,
        0,
        FPC_FINGERPRINT_HARDWARE_MODULE_ID,
        "FPC1150 Touch Sensor",
        "Fingerprint Cards AB",
        &fpc1150_module_methods,
    },
};
