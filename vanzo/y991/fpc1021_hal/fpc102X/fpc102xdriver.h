/*
 * Copyright (C) 2013 Fingerprint Cards AB
 */


#ifndef FPC1020DRIVER_H
#define FPC1020DRIVER_H

#include <sys/ioctl.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>

#include "fpc_linux_helper.h"

// todo: import from kernel header
typedef enum {
    FPC1020_MODE_IDLE			= 0,
    FPC1020_MODE_WAIT_AND_CAPTURE		= 1,
    FPC1020_MODE_SINGLE_CAPTURE		= 2,
    FPC1020_MODE_CHECKERBOARD_TEST_NORM	= 3,
    FPC1020_MODE_CHECKERBOARD_TEST_INV	= 4,
    FPC1020_MODE_BOARD_TEST_ONE		= 5,
    FPC1020_MODE_BOARD_TEST_ZERO		= 6,
    FPC1020_MODE_WAIT_FINGER_DOWN		= 7,
    FPC1020_MODE_WAIT_FINGER_UP		= 8,
    FPC1020_MODE_SINGLE_CAPTURE_CAL		= 9,
    FPC1020_MODE_CAPTURE_AND_WAIT_FINGER_UP	= 10,
} fpc1020_capture_mode_t;


#define FPC1020_BUFFER_MAX_IMAGES 3


class Fpc1020Driver {
public:

    Fpc1020Driver() : file_descriptor_(0), sysfs_path_(NULL) {}
    ~Fpc1020Driver() { this->close(); }

    int open();
    void close();
    int poll(unsigned int timeout);
    int read(uint8_t* buffer, unsigned int size);

    // Settings multiplexer
    int setSettingsMux(uint8_t val)    {return writeAttribute_u8("setup/capture_settings_mux", val);}

    // Sensor settings, multiplexed by "capture_settings_mux"
    int setAdcGain(uint8_t gain)       {return writeAttribute_u8 ("setup/adc_gain", gain);}
    int setAdcShift(uint8_t offset)    {return writeAttribute_u8 ("setup/adc_shift", offset);}
    int setPxlCtrl(uint16_t val)       {return writeAttribute_u16("setup/pxl_ctrl", val);}

    // Capture settings
    int setCaptureCount(uint8_t count) {return writeAttribute_u8("setup/capture_count", count);}
    int setCaptureMode(fpc1020_capture_mode_t mode);

    int setImageCaptureRowStart(uint8_t val)  {return writeAttribute_u8("setup/capture_row_start", val);}
    int setImageCaptureRowCount(uint8_t val)  {return writeAttribute_u8("setup/capture_row_count", val);}
    int setImageCaptureColStart(uint8_t val)  {return writeAttribute_u8("setup/capture_col_start", val);}
    int setImageCaptureColGroups(uint8_t val) {return writeAttribute_u8("setup/capture_col_groups", val);}
    int selfTest(u_int16_t* val) {return readAttribute_u16("diag/selftest", val);}
    int readFingerPresentStatus(uint16_t* val)  {return readAttribute_u16("diag/finger_present_status", val);}

    static const uint8_t  kFramePixelsPerAdcGroup = 8;
    static const char* kDevFile;

    static const uint8_t kCaptureCount = FPC1020_BUFFER_MAX_IMAGES;

private:
    int writeAttribute_u8 (const char* name, uint8_t value_u8);
    int writeAttribute_u16(const char* name, uint16_t value_u16);
    int readAttribute_u16(const char* name, uint16_t* value_u16);
    int setNonBlocking(bool mode);

    int file_descriptor_;
    const char* sysfs_path_;

};

#endif // FPC1080DRIVER_H
