#ifdef __FPC_SW_TEE
#include <tee_internal_api.h>
#else
#include <stdint.h>
#endif

#include <fpc_image.h>

#define FPC_INVALID_PARAMETER   (-3)
#define FPC_MEM_ALLOCATION_FAILURE (-12)
#define FPC_OK                      0 
typedef struct fpc1020_frame_format_t {
    fpc_frame_format_t fmt;

    uint32_t offset_x;
    uint32_t offset_y;
} fpc1020_frame_format_t;

#ifdef __cplusplus
extern "C" {
#endif

int preprocessor_init(const char* config_fname, const fpc1020_frame_format_t *format);

void preprocessor_cleanup();

int preprocessor(fpc_image_t* src, fpc_image_t* result);

#ifdef __cplusplus
}
#endif
