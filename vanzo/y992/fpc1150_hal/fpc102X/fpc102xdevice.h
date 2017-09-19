#ifndef FPC1020DEVICE_H
#define FPC1020DEVICE_H

#include <poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#include <hardware/fingerprint_sensor.h>

#include "linux/errno.h"
#include "fpc102xdriver.h"
#include "fpc_log.h"
#include <fpc_image.h>
#include "fpc_rect.h"
#include "fpc_hal_preprocessor.h"
#include "cac.h"

typedef enum
{
    FPC102X_SENSOR_TYPE_FPC1011,
    FPC102X_SENSOR_TYPE_FPC1020,
    FPC102X_SENSOR_TYPE_FPC1021,
    FPC102X_SENSOR_TYPE_FPC1080,
    FPC102X_SENSOR_TYPE_FPC1150,
    FPC102X_SENSOR_TYPE_FPC1025,
    FPC102X_SENSOR_TYPE_FPC1155,
    FPC102X_SENSOR_TYPE_FPC1140,
    FPC102X_SENSOR_TYPE_FPC1145,
} fpc102x_sensor_type_t;

typedef struct
{
    uint8_t AdcGain;
    uint8_t AdcShift;
    uint16_t PxlCtrl;

} fpc102x_capture_settings_t;

typedef struct
{
    bool fallback_enabled;
    int16_t thresholdIntensity;
    int32_t thresholdPermille;
} fpc102x_cac_fallback_settings_t;

typedef struct {
     int capture_count;
     fpc102x_capture_settings_t capture_settings[Fpc1020Driver::kCaptureCount];
     bool have_capture_settings;
     fpc::Rect dimen;
     bool kpi_enabled;
     int debug_enabled;
     fpc102x_cac_fallback_settings_t cac_fallback_settings;
} fpc102x_configuration_t;

typedef struct fpc102x_device_t {
    fpc_fingerprint_device_t device; //inheritance
    fpc::Rect sensor_dimen;

    int32_t aborted;
    Fpc1020Driver* driver;
    fpc_exit_condition_t* shouldExit;
    fpc_sensor_event_callback_t* callback;
    void* callback_data;
    fpc_image_t raw_image;
    fpc_image_t raw_image_fallback;
    fpc102x_configuration_t cfg;
    fpc1020_frame_format_t frame_format;
    fpc102x_sensor_type_t sensor_type;
    char pp_DATA_config[PATH_MAX];
    bool raw_capture_enable;
    cac_calibration_t* cac_calibration;
    fpc1020_capture_mode_t test_capture_mode;
    bool image_quality_capture_enable;
} fpc102x_device_t;

void fpc102x_device_init(fpc102x_device_t* dev, unsigned int sensor_width, unsigned int sensor_height, fpc102x_sensor_type_t sensor_type);
void fpc102x_load_configuration(fpc102x_device_t* dev);
int32_t fpc102x_open(fpc_fingerprint_device_t* device);
int32_t fpc102x_close(fpc_fingerprint_device_t* device);
int32_t fpc102x_writeSensorSetup(fpc_fingerprint_device_t* device);
int32_t fpc102x_captureImage(fpc_fingerprint_device_t* device,
                             fpc_image_t* image);
int32_t fpc102x_captureSingleFrame(fpc_fingerprint_device_t* device,
                                   fpc_image_t* image);
fpc_image_format_t fpc102x_getImageFormat(fpc_fingerprint_device_t* device);

void fpc102x_setCallbacks(fpc_fingerprint_device_t* device,
                          fpc_sensor_event_callback_t* callback,
                          fpc_exit_condition_t* shouldExit, void* user);
int32_t fpc102x_setProperty(fpc_fingerprint_device_t* device,
                            const char* property,
                            uint8_t value);

int32_t fpc102x_calibrate(fpc_fingerprint_device_t* device);
#endif // FPC1080DEVICE_H
