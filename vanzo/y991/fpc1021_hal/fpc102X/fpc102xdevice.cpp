#include "fpc102xdevice.h"
#include "conf_parser.h"
#include <limits.h>
#include <unistd.h>
#include <sys/time.h>

#ifdef FPC_DEBUG_FUNCTIONS
// Debug functions included
#include "hardware/fingerprint_sensor.h"
#define DEBUG_HANDLE_CAPTURED_IMAGE(m,r,i,s) debug_handle_captured_image(m,r,i,s)
void debug_handle_captured_image(
  int mode,
  fpc_image_t* raw,
  fpc_image_t* image,
  const char *print_str);

#else
// Debug functions not included
#define DEBUG_HANDLE_CAPTURED_IMAGE(m,r,i,s) (void)0
#endif

#define FPC1011_CONFIGURATION_FILE "/etc/fpc1011.conf"
#define FPC1020_CONFIGURATION_FILE "/etc/fpc1020.conf"
#define FPC1021_CONFIGURATION_FILE "/etc/fpc1021.conf"
#define FPC1080_CONFIGURATION_FILE "/etc/fpc1080.conf"
#define FPC1150_CONFIGURATION_FILE "/etc/fpc1150.conf"
#define FPC1140_CONFIGURATION_FILE "/etc/fpc1140.conf"


#define DEFAULT_WET_ADC_GAIN    2
#define DEFAULT_WET_ADC_SHIFT   10
#define DEFAULT_WET_ADC_PXL     0x1a

#define MAX_FINGER_STABLE_TIME_US 600000

#define DEFAULT_IQ_ADC_GAIN     2
#define DEFAULT_IQ_ADC_SHIFT    10
#define DEFAULT_IQ_ADC_PXL      0x0e

void fpc102x_device_init(fpc102x_device_t *dev, unsigned int sensor_width,
                  unsigned int sensor_height, fpc102x_sensor_type_t sensor_type)
{
    dev->cfg.capture_count = Fpc1020Driver::kCaptureCount;
    dev->sensor_dimen = fpc::Rect(sensor_width, sensor_height);
    dev->cfg.dimen = dev->sensor_dimen;
    dev->frame_format.fmt.bits_per_pixels = 8;
    dev->frame_format.fmt.channels = 1;
    dev->frame_format.fmt.greyscale_polarity = 0;
    dev->frame_format.fmt.height = dev->sensor_dimen.height();
    dev->frame_format.fmt.width = dev->sensor_dimen.width();
    dev->frame_format.fmt.ppi = 508;
    dev->frame_format.offset_x = 0;
    dev->frame_format.offset_y = 0;
    dev->sensor_type = sensor_type;

    dev->pp_DATA_config[0] = '\0';
    dev->raw_capture_enable = 0;
    dev->cfg.have_capture_settings = false;
    dev->cfg.kpi_enabled = false;
    dev->cfg.debug_enabled = 0;

    dev->cfg.cac_fallback_settings.fallback_enabled = true;
    dev->cfg.cac_fallback_settings.thresholdIntensity = 13;
    dev->cfg.cac_fallback_settings.thresholdPermille = 500;

    switch (sensor_type)
    {
    case FPC102X_SENSOR_TYPE_FPC1011:
    case FPC102X_SENSOR_TYPE_FPC1021:
    case FPC102X_SENSOR_TYPE_FPC1080:
    case FPC102X_SENSOR_TYPE_FPC1150:
    case FPC102X_SENSOR_TYPE_FPC1020:
    case FPC102X_SENSOR_TYPE_FPC1155:
    case FPC102X_SENSOR_TYPE_FPC1025:
    case FPC102X_SENSOR_TYPE_FPC1140:
    case FPC102X_SENSOR_TYPE_FPC1145:
    default:
        dev->cfg.capture_settings[0].AdcGain = DEFAULT_WET_ADC_GAIN;
        dev->cfg.capture_settings[0].AdcShift = DEFAULT_WET_ADC_SHIFT;
        dev->cfg.capture_settings[0].PxlCtrl = DEFAULT_WET_ADC_PXL;
        break;
    }
}

void fpc102x_load_configuration(fpc102x_device_t* dev)
{
    const char* conf_file;
    switch (dev->sensor_type)
    {
    case FPC102X_SENSOR_TYPE_FPC1011:
        conf_file = FPC1011_CONFIGURATION_FILE;
        break;
    case FPC102X_SENSOR_TYPE_FPC1021:
    case FPC102X_SENSOR_TYPE_FPC1025:
        conf_file = FPC1021_CONFIGURATION_FILE;
        break;
    case FPC102X_SENSOR_TYPE_FPC1080:
        conf_file = FPC1080_CONFIGURATION_FILE;
        break;
    case FPC102X_SENSOR_TYPE_FPC1150:
    case FPC102X_SENSOR_TYPE_FPC1155:
        conf_file = FPC1150_CONFIGURATION_FILE;
        break;

    case FPC102X_SENSOR_TYPE_FPC1140:
    case FPC102X_SENSOR_TYPE_FPC1145:
        conf_file = FPC1140_CONFIGURATION_FILE;
        break;

    case FPC102X_SENSOR_TYPE_FPC1020:
    default:
        conf_file = FPC1020_CONFIGURATION_FILE;
        break;

    }

    FILE* file = fopen(conf_file, "r");
    if (!file) {
        LOGE("failed to load configuration file %s %i\n", conf_file, -errno);
    } else {
        LOGD("loaded configuration file %s\n", conf_file);

        const char* sensor = "1020"; // rely on configuration to only include one version of "1020"
        int function_enabled;
        int cac_fallback_enabled;
        fpc::Rect dimension;
        bool complete_adc_ok = false;
        fpc102x_capture_settings_t capture_settings[Fpc1020Driver::kCaptureCount];
        int numAdc;
        if (!getIntValue(file,sensor, "numAdcSetup", &numAdc)) {
            if (numAdc > Fpc1020Driver::kCaptureCount && numAdc < 1) {
                LOGE("invalid numAdc\n");
            } else {
                for (int i = 0; i < numAdc; ++i) {
                    int value;
                    bool has_gain = getIntValue(file, sensor, create_key("Adc","gain", i), &value) == 0;
                    capture_settings[i].AdcGain = value;

                    bool has_shift = getIntValue(file, sensor, create_key("Adc","shift", i), &value) == 0;
                    capture_settings[i].AdcShift = value;

                    bool has_pxl = getIntValue(file, sensor, create_key("Adc","pxl", i), &value) == 0;
                    capture_settings[i].PxlCtrl = value;

                    complete_adc_ok = (has_gain && has_pxl && has_shift);

                    if (!complete_adc_ok)
                        break;
                }
            }
        }
        if (complete_adc_ok) {
            LOGD("loaded adc configuration");
            memcpy(dev->cfg.capture_settings, capture_settings, sizeof(capture_settings));
            dev->cfg.capture_count = numAdc;
            dev->cfg.have_capture_settings = true;
            for (int i = 0; i < dev->cfg.capture_count; ++i) {
                LOGD("Adc%i gain %i shift %i pxl %i",i,
                     dev->cfg.capture_settings[i].AdcGain,
                     dev->cfg.capture_settings[i].AdcShift,
                     dev->cfg.capture_settings[i].PxlCtrl);
            }
        }
        bool complete_area = (getIntValue(file, sensor, "crop_area.left", &dimension.left) == 0 &&
            getIntValue(file, sensor, "crop_area.top", &dimension.top) == 0 &&
            getIntValue(file, sensor, "crop_area.right", &dimension.right) == 0 &&
            getIntValue(file, sensor, "crop_area.bottom", &dimension.bottom) == 0);


        if (complete_area) {
            if (dev->sensor_dimen.canContain(dimension) &&
                    dimension.left % Fpc1020Driver::kFramePixelsPerAdcGroup == 0 &&
                    dimension.width() % Fpc1020Driver::kFramePixelsPerAdcGroup == 0) {
                dev->cfg.dimen = dimension;
                dev->frame_format.offset_x = dimension.left;
                dev->frame_format.offset_y = dimension.top;
                LOGD("loaded crop_area: %i,%i,%i,%i", dimension.left,
                     dimension.top, dimension.right, dimension.bottom);
            } else {
                LOGD("invalid area configuration");
            }
        }

        if (getStringValue(file, sensor, "pp.DATA", dev->pp_DATA_config, sizeof(dev->pp_DATA_config)))
            LOGD("pp.DATA not set");

        if (getIntValue(file,sensor, "kpi", &function_enabled) == 0 && function_enabled > 0) {
            LOGD("KPI measurements enabled");
            dev->cfg.kpi_enabled = true;
        }

        if (getIntValue(file, "debug", "mode", &function_enabled) == 0 && function_enabled > 0) {
            LOGD("debug functions enabled");
            dev->cfg.debug_enabled = function_enabled;
        }

        if (getIntValue(file,sensor, "cac.enable_fallback_image", &cac_fallback_enabled) == 0) {
            if (cac_fallback_enabled == 0) {
                dev->cfg.cac_fallback_settings.fallback_enabled = false;
            } else {
                int thresholdIntensity;
                int thresholdPermille;

                dev->cfg.cac_fallback_settings.fallback_enabled = true;

                if (getIntValue(file,sensor, "cac.threshold_intensity", &thresholdIntensity) == 0) {
                    LOGD("CAC fallback intensity threshold: %d", thresholdIntensity);
                    dev->cfg.cac_fallback_settings.thresholdIntensity = (int16_t) thresholdIntensity;
                }

                if (getIntValue(file,sensor, "cac.threshold_permille", &thresholdPermille) == 0) {
                    LOGD("CAC fallback threshold permille: %d", thresholdPermille);
                    dev->cfg.cac_fallback_settings.thresholdPermille = (int32_t) thresholdPermille;
                }
            }
        }

        if (dev->cfg.cac_fallback_settings.fallback_enabled)
            LOGD("CAC fallback image enabled");
        else
            LOGD("CAC fallback image disabled");

        LOGD("conf parse completed\n");
    }

    dev->frame_format.fmt.width = dev->cfg.dimen.width();
    dev->frame_format.fmt.height = dev->cfg.dimen.height();

    if(file)
        fclose(file);
}

int32_t fpc102x_open(fpc_fingerprint_device_t* device)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    int status =  fpc1020device->driver->open();
    if (status)
        return status;

    if (fpc1020device->cac_calibration == NULL)
        status = fpc102x_calibrate(device);

    return status;
}

int32_t fpc102x_close(fpc_fingerprint_device_t* device)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    fpc1020device->driver->close();
    return 0;
}

int32_t fpc102x_writeSensorSetup(fpc_fingerprint_device_t* device)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*)device;

    int32_t error = 0;

    error = fpc1020device->driver->setCaptureCount(1);

    if (!error)
        error = fpc1020device->driver->setImageCaptureRowStart(fpc1020device->cfg.dimen.top);

    if (!error)
        error = fpc1020device->driver->setImageCaptureRowCount(fpc1020device->cfg.dimen.height());

    int colStart = fpc1020device->cfg.dimen.left / Fpc1020Driver::kFramePixelsPerAdcGroup;
    if (!error)
        error = fpc1020device->driver->setImageCaptureColStart(colStart);

    int colGroups = fpc1020device->cfg.dimen.width() / Fpc1020Driver::kFramePixelsPerAdcGroup;

    if (!error)
        error = fpc1020device->driver->setImageCaptureColGroups(colGroups);

    return error;
}

int32_t fpc102x_waitForFinger(fpc102x_device_t* device, bool down)
{
    int status =
            device->driver->setCaptureMode(down ?
                  FPC1020_MODE_WAIT_FINGER_DOWN : FPC1020_MODE_WAIT_FINGER_UP);

    if (status)
        return status;

    const unsigned int poll_timeout_ms = 50;
    uint8_t garbage;
    uint32_t bytes_read = 0;
    while (true) {
        if (device->shouldExit) {
            if (device->shouldExit(device->callback_data)) {
                status = -EINTR;
                return status;
            }
        }


        status = device->driver->read(&garbage, 1);

        if (status == -EWOULDBLOCK) {
            status = device->driver->poll(poll_timeout_ms);

            if (status < 0) {
                return status;
            }
            status = 0;
        } else if (status < 0) {
            return status;
        } else {
            bytes_read += status;
        }
        if (bytes_read == 4)
            return 0;
    }
}

int32_t fpc102x_setAdc(fpc_fingerprint_device_t* device,
                       uint8_t gain,
                       uint8_t shift,
                       uint16_t pxl_ctrl)
{
    fpc102x_device_t* dev = (fpc102x_device_t*) device;

    int32_t status = dev->driver->setCaptureCount(1);
    if (status)
        goto out;

    status = dev->driver->setSettingsMux(0);
    if (status)
        goto out;

    status = dev->driver->setAdcGain(gain);
    if (status)
        goto out;

    status = dev->driver->setAdcShift(shift);
    if (status)
        goto out;

    status = dev->driver->setPxlCtrl(pxl_ctrl);

out:
    return status;
}

int32_t fpc102x_setCrop(fpc_fingerprint_device_t* device,
                        uint32_t row_start,
                        uint32_t row_count,
                        uint32_t col_start,
                        uint32_t col_groups)
{
    fpc102x_device_t* dev = (fpc102x_device_t*) device;
    LOGD("setCrop row_start %u row_count %u, col_start %u col_groups %u\n",
         row_start, row_count, col_start, col_groups);

    int32_t status = dev->driver->setImageCaptureRowStart(row_start);
    if (status)
        goto out;

    status = dev->driver->setImageCaptureRowCount(row_count);
    if (status)
        goto out;

    status = dev->driver->setImageCaptureColStart(col_start);
    if (status)
        goto out;

    status = dev->driver->setImageCaptureColGroups(col_groups);

out:
    return status;
}

int32_t fpc102x_capture(fpc_fingerprint_device_t* device,
                                fpc_image_t* image,
                                fpc1020_capture_mode_t mode,
                                uint32_t row_start,
                                uint32_t row_count,
                                uint32_t col_start,
                                uint32_t col_groups,
                                uint8_t gain,
                                uint8_t shift,
                                uint16_t pxl_ctrl)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    int status = fpc1020device->driver->setCaptureMode(mode);
    if (status)
        return status;

    status = fpc102x_setAdc(device, gain, shift, pxl_ctrl);
    if (status)
        return status;

    status = fpc102x_setCrop(device, row_start, row_count,
                                    col_start, col_groups);

    if (status)
        return status;

    int frame_size = row_count *
                    col_groups * Fpc1020Driver::kFramePixelsPerAdcGroup;

    int bytes_read = 0;

    while (true) {
        int size = image->capacity - bytes_read;
        if (size < 0) {
            status =  -ENOBUFS;
            break;
        }
        status = fpc1020device->driver->read(image->buffer + bytes_read, size);

        if (mode == FPC1020_MODE_WAIT_AND_CAPTURE && status == -EWOULDBLOCK) {
            if (fpc1020device->shouldExit) {
                if (fpc1020device->shouldExit(fpc1020device->callback_data)) {
                    status = -EINTR;
                    return status;
                }
            }

            status = fpc1020device->driver->poll(50);

            if (status < 0) {
                return status;
            }
            status = 0 ;
        } else if (status < 0) {
            break;
        }

        bytes_read += status;

        if (bytes_read == frame_size) {
            status = 0;
            break;
        }
    }

    if (status)
        return status;

    image->format = fpc1020device->frame_format.fmt;
    image->format.height = row_count;
    image->format.width = col_groups * Fpc1020Driver::kFramePixelsPerAdcGroup;
    image->frame_count = 1;

    return status;
}

int32_t fpc102x_calibrate(fpc_fingerprint_device_t* device)
{
    image_requirements_t req;
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    calibration_session_t* session = calibrationInit();
    int32_t status = 0;
    fpc_image_t image;
    image.buffer = NULL;
    fpc1020_capture_mode_t mode = FPC1020_MODE_BOARD_TEST_ONE;
    adc_setup_t adc_setup;

    uint32_t row_count;
    uint32_t col_groups;
    uint32_t row_start ;
    uint32_t col_start;

    if (!session) {
        status = -ENOMEM;
        goto out;
    }

    req.full_image_height = fpc1020device->sensor_dimen.height();
    req.full_image_width = fpc1020device->sensor_dimen.width();

    status = calibrationGetReq(session, &req);
    if (status)
        goto out;

    switch(req.sensor_mode) {
    case FPC_SENSOR_MODE_REGULAR:
        mode = FPC1020_MODE_SINGLE_CAPTURE;
        break;
    case FPC_SENSOR_MODE_TEST_BLACK:
        mode = FPC1020_MODE_BOARD_TEST_ONE;
        break;
    }

    row_count = req.image_height;
    col_groups =
            req.image_width / Fpc1020Driver::kFramePixelsPerAdcGroup;

    row_start = req.image_row_start;

    col_start = req.image_col_start / Fpc1020Driver::kFramePixelsPerAdcGroup;

    image.capacity= row_count * col_groups *
            Fpc1020Driver::kFramePixelsPerAdcGroup;

    image.buffer = (u_int8_t*) malloc(image.capacity);

    if (!image.buffer) {
        status = -ENOMEM;
        goto out;
    }

    while (true) {
        calibrationGetAdc(session, &adc_setup);

        status = fpc102x_capture(device, &image, mode, row_start, row_count,
                        col_start, col_groups,
                        adc_setup.gain,
                        adc_setup.shift,
                        0x0f00 | adc_setup.pxl);

        if (status)
            goto free_buffer;

        status = calibrationProcessImage(session, &image);
        if (status < 0)
            goto free_buffer;
        else if (status > 0)
            break;
    }

    status = calibrationFinish(session, &fpc1020device->cac_calibration);

free_buffer:
    free(image.buffer);

out:
    return status;
}

static void fpc102x_setCACSensorType(fpc102x_device_t* fpc1020device, image_requirements_t* req)
{
    switch (fpc1020device->sensor_type)
    {
    case FPC102X_SENSOR_TYPE_FPC1020:
        req->sensor_type = FPC_SENSOR_TYPE_1020;
        break;
    case FPC102X_SENSOR_TYPE_FPC1021:
        req->sensor_type = FPC_SENSOR_TYPE_1021;
        break;
    case FPC102X_SENSOR_TYPE_FPC1150:
        req->sensor_type = FPC_SENSOR_TYPE_1150;
        break;
    case FPC102X_SENSOR_TYPE_FPC1025:
        req->sensor_type = FPC_SENSOR_TYPE_1025;
        break;
    case FPC102X_SENSOR_TYPE_FPC1155:
    default:
        req->sensor_type = FPC_SENSOR_TYPE_1155;
        break;
    }
}

static int32_t fpc102x_checkFingerPresentStatus(fpc102x_device_t* fpc1020device, uint16_t* finger_present_status)
{
    int status = fpc1020device->driver->readFingerPresentStatus(finger_present_status);
    if (status == 0) {
        LOGD("readFingerPresentStatus: 0x%04x\n", *finger_present_status);

        if (*finger_present_status == 0)
            status = -EAGAIN;
    }
    return status;

}

static int32_t fpc102x_captureFallbackImage(fpc_fingerprint_device_t* device)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    int status = 0;
    fpc1020_capture_mode_t mode = FPC1020_MODE_SINGLE_CAPTURE;
    adc_setup_t adc_setup;
    uint32_t row_count;
    uint32_t col_groups;
    uint32_t row_start ;
    uint32_t col_start;

    row_start = fpc1020device->cfg.dimen.top;
    row_count = fpc1020device->cfg.dimen.height();
    col_start = fpc1020device->cfg.dimen.left /
            Fpc1020Driver::kFramePixelsPerAdcGroup;
    col_groups = fpc1020device->cfg.dimen.width() /
            Fpc1020Driver::kFramePixelsPerAdcGroup;

    adc_setup.gain = fpc1020device->cfg.capture_settings[0].AdcGain;
    adc_setup.shift = fpc1020device->cfg.capture_settings[0].AdcShift;
    adc_setup.pxl = fpc1020device->cfg.capture_settings[0].PxlCtrl;
    LOGD("fallback setting gain %u shift %u pxl %x\n", adc_setup.gain, adc_setup.shift,
             adc_setup.pxl);
    status = fpc102x_capture(device, &fpc1020device->raw_image_fallback,
                             mode,
                             row_start, row_count, col_start, col_groups,
                             adc_setup.gain,
                             adc_setup.shift,
                             0x0f00 | adc_setup.pxl);

    return status;
}

static int32_t fpc102x_fingerprintSensorDebounce(fpc_fingerprint_device_t* device, cac_session_t* session, uint16_t* finger_present_status)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    int status = 0;
    fpc_image_t sub_image;
    image_requirements_t req;
    adc_setup_t adc_setup;
    uint32_t row_count;
    uint32_t col_groups;
    uint32_t row_start ;
    uint32_t col_start;
    int fsd_complete = 0;

    struct timeval ts_subt1, ts_subt2, ts_start, ts_delta, ts_delta_total;
    int fs_total = 0;

    status = fpc102x_checkFingerPresentStatus(fpc1020device, finger_present_status);
    if (status != 0)
        goto out;

    fsdGetReq(session, &req);
    fpc102x_setCACSensorType(fpc1020device, &req);
    fsdGetAdc(session, &adc_setup, req.sensor_type);

    req.full_image_height = fpc1020device->sensor_dimen.height();
    req.full_image_width = fpc1020device->sensor_dimen.width();

    status = getDynamicAreaSelection(&req, *finger_present_status);
    if (status) {
        status = -EAGAIN;
        goto out;
    }

    row_count = req.image_height;
    col_groups =
            req.image_width / Fpc1020Driver::kFramePixelsPerAdcGroup;

    row_start = req.image_row_start;

    col_start = req.image_col_start / Fpc1020Driver::kFramePixelsPerAdcGroup;

    sub_image.capacity= row_count * col_groups *
            Fpc1020Driver::kFramePixelsPerAdcGroup;

    sub_image.buffer = (u_int8_t*) malloc(sub_image.capacity);

    if (!sub_image.buffer) {
        status = -ENOMEM;
        goto out;
    }

    gettimeofday(&ts_start, NULL);

    while ((fs_total < MAX_FINGER_STABLE_TIME_US)) {
        int sub_capture = 0;

        if (fpc1020device->cfg.kpi_enabled)
            gettimeofday(&ts_subt1, NULL);

        LOGD("DEBOUNCE: cac gain %u shift %u pxl %x\n", adc_setup.gain, adc_setup.shift,
                adc_setup.pxl);

        status = fpc102x_capture(device, &sub_image, FPC1020_MODE_SINGLE_CAPTURE_CAL, row_start, row_count,
                        col_start, col_groups,
                        adc_setup.gain,
                        adc_setup.shift,
                        0x0f00 | adc_setup.pxl);

        if (status)
            goto free_buffer;

        status = fpc102x_checkFingerPresentStatus(fpc1020device, finger_present_status);
        if (status != 0)
            goto free_buffer;

        fsdGetAdc(session, &adc_setup, req.sensor_type);
        status = fsdProcessImage(session, &sub_image, *finger_present_status);

        gettimeofday(&ts_subt2, NULL);
        timersub(&ts_subt2, &ts_start, &ts_delta_total);
        fs_total = ts_delta_total.tv_sec * 1000 + ts_delta_total.tv_usec;

        if (fpc1020device->cfg.kpi_enabled) {
            timersub(&ts_subt2, &ts_subt1, &ts_delta);
            sub_capture = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
            LOGD("KPI - CAC - Fingerprint sensor debounce sub-capture: %d ms total: %d",
                    sub_capture / 1000, fs_total / 1000);
        }

        if (status == 1) {
            fsd_complete = 1;
            status = 0;
            break;
        }
    }
    /* To not loose finger up sync, we need to dummpy capture an image with finger up */
    if (fsd_complete == 0)
    {
        /* Since it's only a dummy read, let's use the previous parameters for simplicity */
        fpc102x_capture(device, &sub_image, FPC1020_MODE_CAPTURE_AND_WAIT_FINGER_UP, row_start, row_count,
                        col_start, col_groups,
                        adc_setup.gain,
                        adc_setup.shift,
                        0x0f00 | adc_setup.pxl);
        status = -EAGAIN;
    }

free_buffer:
    free(sub_image.buffer);

out:
    return status;
}

static int32_t fpc102x_runCAC(fpc_fingerprint_device_t* device, cac_session_t* session, uint16_t finger_present_status, adc_setup_t* adc_setup)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    int status = 0;
    fpc_image_t sub_image;
    sub_image.buffer = NULL;
    fpc1020_capture_mode_t mode = FPC1020_MODE_SINGLE_CAPTURE;
    image_requirements_t req;
    uint32_t row_count;
    uint32_t col_groups;
    uint32_t row_start ;
    uint32_t col_start;

    struct timeval ts_subt1, ts_subt2, ts_delta;

    req.full_image_height = fpc1020device->sensor_dimen.height();
    req.full_image_width = fpc1020device->sensor_dimen.width();

    status = cacGetReq(session, &req);
    if (status)
        goto out;

    fpc102x_setCACSensorType(fpc1020device, &req);

    status = getDynamicAreaSelection(&req, finger_present_status);
    if (status != 0) {
        status = -EAGAIN;
        goto out;
    }

    switch(req.sensor_mode) {
    case FPC_SENSOR_MODE_REGULAR:
        mode = FPC1020_MODE_SINGLE_CAPTURE_CAL;
        break;
    case FPC_SENSOR_MODE_TEST_BLACK:
        mode = FPC1020_MODE_BOARD_TEST_ONE;
        break;
    }

    row_count = req.image_height;
    col_groups =
            req.image_width / Fpc1020Driver::kFramePixelsPerAdcGroup;

    row_start = req.image_row_start;

    col_start = req.image_col_start / Fpc1020Driver::kFramePixelsPerAdcGroup;

    sub_image.capacity= row_count * col_groups *
            Fpc1020Driver::kFramePixelsPerAdcGroup;

    sub_image.buffer = (u_int8_t*) malloc(sub_image.capacity);

    if (!sub_image.buffer) {
        status = -ENOMEM;
        goto out;
    }

    while (true) {
        int sub_capture = 0;

        if (fpc1020device->cfg.kpi_enabled)
            gettimeofday(&ts_subt1, NULL);

        cacGetAdc(session, adc_setup);
        LOGD("cac gain %u shift %u pxl %x\n", adc_setup->gain, adc_setup->shift,
             adc_setup->pxl);
        status = fpc102x_capture(device, &sub_image, mode, row_start, row_count,
                        col_start, col_groups,
                        adc_setup->gain,
                        adc_setup->shift,
                        0x0f00 | adc_setup->pxl);

        if (status)
            goto free_buffer;

        status = cacProcessImage(session, &sub_image);

        if (fpc1020device->cfg.kpi_enabled) {
            gettimeofday(&ts_subt2, NULL);
            timersub(&ts_subt2, &ts_subt1, &ts_delta);
            sub_capture = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;

            LOGD("KPI - CAC - Sub-capture: %d ms", sub_capture / 1000);
        }

        if (status < 0) {
            goto free_buffer;
        } else if (status > 0) {
            status = 0;
            break;
        }
    }

    cacGetAdc(session, adc_setup);
    LOGD("cac completed: gain %u shift %u pxl %x\n", adc_setup->gain,
         adc_setup->shift, adc_setup->pxl);

free_buffer:
    free(sub_image.buffer);

out:
    return status;
}

int32_t fpc102x_captureImage(fpc_fingerprint_device_t* device, fpc_image_t* image)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    int status = 0;
    cac_session_t* session = NULL;
    uint16_t finger_present_status;
    uint16_t finger_present_status_capture;
    adc_setup_t adc_setup;
    uint32_t row_count;
    uint32_t col_groups;
    uint32_t row_start ;
    uint32_t col_start;
    verify_image_parameters_t verifyParameters;

    struct timeval ts_cac_end, ts_capture_end, ts_end_pp, ts_start, ts_fb_end, ts_fsd_end, ts_delta;
    int fb_us = 0;
    int fsd_us = 0;
    int pp_us = 0;
    int cac_us = 0;
    int capture_us = 0;
    int total_us = 0;

    if (fpc1020device->callback) {
        fpc1020device->callback(FPC_SENSOR_EVENT_WAITING_FINGER,
                                fpc1020device->callback_data);
    }

    status = fpc102x_waitForFinger(fpc1020device, true);
    if (status)
        goto out;

    if (fpc1020device->callback) {
        fpc1020device->callback(FPC_SENSOR_EVENT_FINGER_DOWN,
                                fpc1020device->callback_data);
    }

    /* Capture full image with wet settings as fallback solution */
    if (fpc1020device->cfg.cac_fallback_settings.fallback_enabled) {
        if (fpc1020device->cfg.kpi_enabled)
            gettimeofday(&ts_start, NULL);

        status = fpc102x_captureFallbackImage(device);
        if (status != 0)
            goto out;

        if (fpc1020device->cfg.kpi_enabled) {
            gettimeofday(&ts_fb_end, NULL);
            timersub(&ts_fb_end, &ts_start, &ts_delta);
            fb_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
        }
    }
    else {
        if (fpc1020device->cfg.kpi_enabled)
            gettimeofday(&ts_start, NULL);
            gettimeofday(&ts_fb_end, NULL);
    }

    session = cacInit(fpc1020device->cac_calibration);
    if (!session) {
        status = -ENOMEM;
        goto out;
    }

    /* Fingerprint sensor debounce implementation */
    status = fpc102x_fingerprintSensorDebounce(device, session, &finger_present_status);
    if (status != 0)
        goto free_session;

    if (fpc1020device->cfg.kpi_enabled) {
        gettimeofday(&ts_fsd_end, NULL);
        timersub(&ts_fsd_end, &ts_fb_end, &ts_delta);
        fsd_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
    }

    /* Run CAC */
    status = fpc102x_runCAC(device, session, finger_present_status, &adc_setup);
    if (status != 0)
        goto free_session;

    if (fpc1020device->cfg.kpi_enabled) {
        gettimeofday(&ts_cac_end, NULL);
        timersub(&ts_cac_end, &ts_fsd_end, &ts_delta);
        cac_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
    }

    /* Capture full image */
    row_start = fpc1020device->cfg.dimen.top;
    row_count = fpc1020device->cfg.dimen.height();
    col_start = fpc1020device->cfg.dimen.left /
            Fpc1020Driver::kFramePixelsPerAdcGroup;
    col_groups = fpc1020device->cfg.dimen.width() /
            Fpc1020Driver::kFramePixelsPerAdcGroup;

    status = fpc102x_capture(device, &fpc1020device->raw_image,
                             FPC1020_MODE_CAPTURE_AND_WAIT_FINGER_UP,
                             row_start, row_count, col_start, col_groups,
                             adc_setup.gain,
                             adc_setup.shift,
                             0x0f00 | adc_setup.pxl);

    /* Delay 10ms for making sure finger is stable on the sensor */
    usleep(10 * 1000);

    /* Check finger present status */
    status = fpc102x_checkFingerPresentStatus(fpc1020device, &finger_present_status_capture);
    if (status != 0)
        goto free_session;

    /* Check that we have the same (or more) active zones as after FSD */
    if ((finger_present_status & finger_present_status_capture) != finger_present_status) {
        status = -EAGAIN;
        goto free_session;
    }

    /* Check for saturation in image */
    if (fpc1020device->cfg.cac_fallback_settings.fallback_enabled) {
        verifyParameters.thresholdIntensity = fpc1020device->cfg.cac_fallback_settings.thresholdIntensity;
        verifyParameters.thresholdPermille = fpc1020device->cfg.cac_fallback_settings.thresholdPermille;
        status = cacVerifyImage(&fpc1020device->raw_image, &verifyParameters);
        if (status != 0) {
            status = -ENOMEM;
            goto free_session;
        }

        if (verifyParameters.discardCacImage) {
            LOGD("Discard CAC image, score: %d", verifyParameters.verifyScore);
            memcpy(fpc1020device->raw_image.buffer,
                fpc1020device->raw_image_fallback.buffer,
                (fpc1020device->raw_image_fallback.frame_count *
                fpc1020device->raw_image_fallback.format.width *
                fpc1020device->raw_image_fallback.format.height));
        }
    }

    if (fpc1020device->cfg.kpi_enabled) {
        gettimeofday(&ts_capture_end, NULL);
        timersub(&ts_capture_end, &ts_cac_end, &ts_delta);
        capture_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
    }

    status = preprocessor(&fpc1020device->raw_image, image);

    if (status)
        goto free_session;

    if (fpc1020device->cfg.kpi_enabled) {
        gettimeofday(&ts_end_pp, NULL);
        timersub(&ts_end_pp, &ts_capture_end, &ts_delta);
        pp_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
        timersub(&ts_end_pp, &ts_start, &ts_delta);
        total_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;

        LOGD("KPI - FB: %d ms FSD: %d ms CAC: %d ms Sensor capture: %d ms Preprocessor: %d ms Total: %d ms",
            fb_us / 1000, fsd_us / 1000, cac_us / 1000, capture_us / 1000, pp_us / 1000, total_us / 1000);
    }

    if (fpc1020device->cfg.debug_enabled) {
      DEBUG_HANDLE_CAPTURED_IMAGE(fpc1020device->cfg.debug_enabled,
                       &fpc1020device->raw_image, image, "captureImage");
    }

    if (fpc1020device->raw_capture_enable) {
        unsigned int total_frames = fpc1020device->raw_image.frame_count +
                    image->frame_count;

        unsigned int frame_size = fpc1020device->raw_image.format.width *
                fpc1020device->raw_image.format.height;

        size_t total_size = total_frames * frame_size;
        if (image->capacity < total_size) {
            status = -ENOBUFS;
            goto free_session;
        }

        memcpy(image->buffer + image->frame_count * frame_size,
               fpc1020device->raw_image.buffer, frame_size *
               fpc1020device->raw_image.frame_count);

        image->frame_count += fpc1020device->raw_image.frame_count;
    }

free_session:
    cacFinish(session);

out:
    if (fpc1020device->callback) {
        fpc1020device->callback(FPC_SENSOR_EVENT_FINGER_UP,
                                fpc1020device->callback_data);
    }

    if (status)
        LOGE("fpc102x_captureImage failed with error %i", status);

    return status;
}

int32_t fpc102x_captureSingleFrame(fpc_fingerprint_device_t* device, fpc_image_t* image)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    int status = 0;
    cac_session_t* session = NULL;
    fpc1020_capture_mode_t mode = FPC1020_MODE_SINGLE_CAPTURE;
    adc_setup_t adc_setup;
    uint32_t row_count;
    uint32_t col_groups;
    uint32_t row_start;
    uint32_t col_start;
    uint16_t finger_present_status = 0xFFF;
    int run_preprocessor = 1;
    int run_cac = 1;

    struct timeval ts_cac_end, ts_capture_end, ts_end_pp, ts_start, ts_delta;
    int pp_us = 0;
    int cac_us = 0;
    int capture_us = 0;
    int total_us = 0;

    if (fpc1020device->test_capture_mode >= FPC1020_MODE_CHECKERBOARD_TEST_NORM &&
            fpc1020device->test_capture_mode <= FPC1020_MODE_BOARD_TEST_ZERO) {
        run_preprocessor = 0;
        run_cac = 0;
        mode = fpc1020device->test_capture_mode;

        /* ADC parameters are set by the driver for CB but we need to define them here */
        adc_setup.gain = 0;
        adc_setup.shift = 0;
        adc_setup.pxl = 0;

        /* Reset test flag after first capture */
        fpc1020device->test_capture_mode = FPC1020_MODE_IDLE;
    } else if (fpc1020device->image_quality_capture_enable) {
        run_preprocessor = 0;
        run_cac = 0;
        mode = FPC1020_MODE_SINGLE_CAPTURE;

        /* Set correct ADC parameters for image quality capture */
        adc_setup.gain = fpc1020device->cfg.capture_settings[1].AdcGain;
        adc_setup.shift = fpc1020device->cfg.capture_settings[1].AdcShift;
        adc_setup.pxl = fpc1020device->cfg.capture_settings[1].PxlCtrl;

        /* Reset flag after first capture */
        fpc1020device->image_quality_capture_enable = false;
    }

    if (fpc1020device->cfg.kpi_enabled)
        gettimeofday(&ts_start, NULL);

    /* Run CAC */
    if (run_cac) {
        session = cacInit(fpc1020device->cac_calibration);
        if (!session) {
            status = -ENOMEM;
            goto out;
        }
        status = fpc102x_runCAC(device, session, finger_present_status, &adc_setup);
        if (status != 0)
            goto free_session;
    }

    if (fpc1020device->cfg.kpi_enabled) {
        gettimeofday(&ts_cac_end, NULL);
        timersub(&ts_cac_end, &ts_start, &ts_delta);
        cac_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
    }

    row_start = fpc1020device->cfg.dimen.top;
    row_count = fpc1020device->cfg.dimen.height();
    col_start = fpc1020device->cfg.dimen.left /
            Fpc1020Driver::kFramePixelsPerAdcGroup;
    col_groups = fpc1020device->cfg.dimen.width() /
            Fpc1020Driver::kFramePixelsPerAdcGroup;

    status = fpc102x_capture(device, &fpc1020device->raw_image,
                             mode, row_start, row_count, col_start, col_groups,
                             adc_setup.gain,
                             adc_setup.shift,
                             0x0f00 | adc_setup.pxl);

    if (status)
        goto free_session;

    if (fpc1020device->cfg.kpi_enabled) {
        gettimeofday(&ts_capture_end, NULL);
        timersub(&ts_capture_end, &ts_cac_end, &ts_delta);
        capture_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
    }

    if (run_preprocessor) {
        status = preprocessor(&fpc1020device->raw_image, image);
    } else {
        memcpy(image->buffer,
                fpc1020device->raw_image.buffer,
                fpc1020device->raw_image.format.width *
                fpc1020device->raw_image.format.height);

        image->format = fpc1020device->raw_image.format;
        image->frame_count = 1;
    }

    if (fpc1020device->cfg.kpi_enabled) {
        gettimeofday(&ts_end_pp, NULL);
        timersub(&ts_end_pp, &ts_capture_end, &ts_delta);
        pp_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;
        timersub(&ts_end_pp, &ts_start, &ts_delta);
        total_us = ts_delta.tv_sec * 1000 + ts_delta.tv_usec;

        LOGD("KPI - CAC: %d ms Sensor capture: %d ms Preprocessor: %d ms Total: %d ms",
            cac_us / 1000, capture_us / 1000, pp_us / 1000, total_us / 1000);
    }

    if (fpc1020device->cfg.debug_enabled) {
      DEBUG_HANDLE_CAPTURED_IMAGE(fpc1020device->cfg.debug_enabled,
                       &fpc1020device->raw_image, image, "singleFrame");
    }

    if (fpc1020device->raw_capture_enable) {
        unsigned int total_frames = fpc1020device->raw_image.frame_count +
                    image->frame_count;

        unsigned int frame_size = fpc1020device->raw_image.format.width *
                fpc1020device->raw_image.format.height;

        size_t total_size = total_frames * frame_size;
        if (image->capacity < total_size) {
            status = -ENOBUFS;
            goto free_session;
        }

        memcpy(image->buffer + image->frame_count * frame_size,
               fpc1020device->raw_image.buffer, frame_size *
               fpc1020device->raw_image.frame_count);

        image->frame_count += fpc1020device->raw_image.frame_count;
    }

free_session:
    if (run_cac)
        cacFinish(session);

out:
    if (status)
        LOGE("fpc102x_captureImage failed with error %i", status);

    return status;
}

fpc_image_format_t fpc102x_getImageFormat(fpc_fingerprint_device_t* device)
{
    fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
    fpc_image_format_t image_format;
    image_format.frame_format = fpc1020device->frame_format.fmt;
    image_format.max_frames = 1;
    if (fpc1020device->raw_capture_enable) {
        image_format.max_frames += Fpc1020Driver::kCaptureCount;
    }
    return image_format;
}

void fpc102x_setCallbacks(fpc_fingerprint_device_t* device,
                                 fpc_sensor_event_callback_t* callback,
                                 fpc_exit_condition_t* shouldExit, void* user)
{
   fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
   fpc1020device->callback = callback;
   fpc1020device->shouldExit = shouldExit;
   fpc1020device->callback_data = user;
}

int32_t fpc102x_setProperty(fpc_fingerprint_device_t* device, const char* property, uint8_t value)
{
    if (strcmp("raw_capture_enable", property) == 0) {
        fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
        fpc1020device->raw_capture_enable = value;
        return 0;
    } else if (strcmp("selftest", property) == 0) {
        fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
        u_int16_t test_result;
        int status = fpc1020device->driver->selfTest(&test_result);
        if (status)
            return status;

        return test_result == 1 ? 0 : -EIO;
    } else if (strcmp("test_image_format", property) == 0) {
        fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;

        /* Validate input */
        if (value < FPC1020_MODE_CHECKERBOARD_TEST_NORM ||
            value > FPC1020_MODE_BOARD_TEST_ZERO)
            return -EINVAL;

        fpc1020device->test_capture_mode = (fpc1020_capture_mode_t) value;
        return 0;
    } else if (strcmp("image_quality_capture", property) == 0) {
        fpc102x_device_t* fpc1020device = (fpc102x_device_t*) device;
        fpc1020device->image_quality_capture_enable = (value != 0);
        return 0;
    }
    return -ENOTTY;
}
