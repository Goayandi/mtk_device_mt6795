#ifndef FPC_CAC_H
#define FPC_CAC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fpc_image.h>

typedef struct {
    uint8_t gain;
    uint8_t shift;
    uint16_t pxl;
} adc_setup_t;

typedef enum {
    FPC_SENSOR_MODE_REGULAR,
    FPC_SENSOR_MODE_TEST_BLACK
} sensor_mode_t;

typedef enum {
    FPC_SENSOR_TYPE_1020 = 0,
    FPC_SENSOR_TYPE_1021,
    FPC_SENSOR_TYPE_1025,
    FPC_SENSOR_TYPE_1140,
    FPC_SENSOR_TYPE_1145,
    FPC_SENSOR_TYPE_1150,
    FPC_SENSOR_TYPE_1155
} sensor_type_t;

typedef struct {
    sensor_type_t sensor_type;
    sensor_mode_t sensor_mode;
    uint32_t image_col_start;
    uint32_t image_width;
    uint32_t image_row_start;
    uint32_t image_height;
    uint32_t full_image_width;
    uint32_t full_image_height;
} image_requirements_t;

typedef struct {
    int16_t thresholdIntensity;  /* Intensity threshold */
    int32_t thresholdPermille;   /* Fraction of the filtered input image (measured in permille) that at least should be above intensity threshold: Range [0; 1000]*/
    uint8_t discardCacImage;     /* Output from cacVerifyImage will be stored here */
    int32_t verifyScore;         /* Output from cacVerifyImage will be stored here. Range [0; 1000] where "0" is really bad image and "1000" is excellent image */
} verify_image_parameters_t;


typedef struct cac_session_t_ cac_session_t;
typedef struct calibration_session_t_ calibration_session_t;
typedef struct cac_calibration_t_ cac_calibration_t;

/* */
calibration_session_t* calibrationInit();

/* */
int getDynamicAreaSelection(image_requirements_t *req, uint16_t fpStatus);


/* */
int calibrationGetReq(calibration_session_t* session,
                      image_requirements_t* requirements);


int calibrationGetAdc(calibration_session_t* session, adc_setup_t* adc);

int calibrationProcessImage(calibration_session_t* session,
                            const fpc_image_t* image);
/* */
int calibrationFinish(calibration_session_t* session,
                       cac_calibration_t** cac_calibration);

void calibrationDestroy(cac_calibration_t* calibration);

/* */
cac_session_t* cacInit(cac_calibration_t *calibration);
/* */
int cacGetReq(cac_session_t* session, image_requirements_t *requirements);
int cacGetAdc(cac_session_t* session, adc_setup_t* adc);
int cacProcessImage(cac_session_t* session, const fpc_image_t* image);
/* */
int cacVerifyImage(const fpc_image_t* image, verify_image_parameters_t* verifyParameters);
/* */
void cacFinish(cac_session_t* session);

/* ------------------------------------- */
/* Finger down stable functions */
/* Get requirements */
int fsdGetReq(cac_session_t* session, image_requirements_t *requirements);
/* Get ADC values */
int fsdGetAdc(cac_session_t* session, adc_setup_t* adc, sensor_type_t sensorInfo);
/* Process for finger down stable */
int fsdProcessImage(cac_session_t* session, const fpc_image_t* image, uint16_t fpStatus);
/* Free memory */
void fsdFinish(cac_session_t* session);

#ifdef __cplusplus
}
#endif

#endif
