#ifndef FINGERPRINT_SENSOR_H
#define FINGERPRINT_SENSOR_H

#include <hardware/hardware.h>
#include "fpc_image.h"

#define FPC_FINGERPRINT_MODULE_API_VERSION_0 HARDWARE_MAKE_API_VERSION(0, 0)
#define FPC_FINGERPRINT_DEVICE_API_VERSION_0 HARDWARE_DEVICE_API_VERSION(0, 0)

#define FPC_FINGERPRINT_HARDWARE_MODULE_ID  "fingerprint_module"
#define FPC_FINGERPRINT_SENSOR_DEVICE       "fingerprint_sensor"
#define FPC_FINGERPRINT_SYSTEM_DEVICE       "fingerprint_system"

typedef struct fpc_fingerprint_moudule_t {
    struct hw_module_t common; //inheritance

} fpc_fingerprint_moudule_t;

typedef enum {
    FPC_SENSOR_EVENT_WAITING_FINGER     = 1,
    FPC_SENSOR_EVENT_FINGER_DOWN        = 2,
    FPC_SENSOR_EVENT_FINGER_UP          = 3
} fpc_sensor_event_t;

typedef void (fpc_sensor_event_callback_t) (fpc_sensor_event_t event, void* user);
typedef bool (fpc_exit_condition_t) (void* user);

typedef struct fpc_image_format_t {
    fpc_frame_format_t frame_format;
    uint32_t max_frames;
} fpc_image_format_t;

typedef struct fpc_fingerprint_device_t {
    struct hw_device_t common; //inheritance

    int32_t (*open)(fpc_fingerprint_device_t* device);
	int32_t (*close)(fpc_fingerprint_device_t* device);

    /**
     * captureImage must call shouldExit() when waiting for IO and return
     * -EINTR if the exit condition was true.
     */
    void (*setCallbacks)(fpc_fingerprint_device_t* device,
                         fpc_sensor_event_callback_t* callback,
                         fpc_exit_condition_t* shouldExit, void* user);
    /**
     * captureImage shall block untill the capture is completed or the abort_condition
     * is set. All callbacks must be executed on the calling thread.
     */
    int32_t (*captureImage)(fpc_fingerprint_device_t* device, fpc_image_t* image);
	int32_t (*captureSingleFrame)(fpc_fingerprint_device_t* device, fpc_image_t* image);

    int32_t (*setProperty)(fpc_fingerprint_device_t* device, const char* property, uint8_t value);
    /**
     * must return fpc_image_t with all fields of
     */
    fpc_image_format_t (*getImageFormat)(fpc_fingerprint_device_t* device);

} fpc_fingerprint_device_t;

/**
  * below types for system implementations not final
  **/
typedef struct fpc_client_id_t {
    char* id;
    uint32_t length;
} fpc_client_id_t;

typedef struct fpc_db_entry_t {
    uint32_t userId;
    uint32_t fingerId;
} fpc_db_entry_t;

typedef struct fpc_db_table_t {
    fpc_db_entry_t* indices;
    uint32_t length;
} fpc_db_table_t;

typedef struct fpc_fingerprint_system_device_t {
    struct hw_device_t common; //inheritance

    int32_t (*open)(fpc_fingerprint_system_device_t* device);
    int32_t (*close)(fpc_fingerprint_system_device_t* device);

    int32_t (*enrol)(fpc_fingerprint_system_device_t* device, fpc_client_id_t client_id, fpc_db_entry_t entry);
    int32_t (*identify)(fpc_fingerprint_system_device_t* device, fpc_client_id_t client_id);
    int32_t (*verify)(fpc_fingerprint_system_device_t* device, fpc_client_id_t client_id, fpc_db_entry_t candidate);

    int32_t (*getDataBaseTable)(fpc_fingerprint_system_device_t* devcie, fpc_client_id_t client_id, fpc_db_table_t** db_index);
    void (*deleteDataBaseTable)(fpc_fingerprint_system_device_t* device, fpc_db_table_t* db_table);
    int32_t (*removeDataBaseEntry)(fpc_fingerprint_system_device_t* device, fpc_client_id_t client_id, fpc_db_entry_t entry);

    int32_t (*abort)(fpc_fingerprint_system_device_t* device);

} fpc_fingerprint_system_device_t;

#endif // FINGERPRINT_SENSOR_H
