#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct native_handle {
    int version;
    int numFds;
    int numInts;
    int data[0];
} native_handle_t;

typedef const native_handle_t* buffer_handle_t;

struct RemoteWindowBuffer {
    int w, h, format, usage;
    uint32_t stride;
    buffer_handle_t handle;
    RemoteWindowBuffer(int w, int h, uint32_t stride, int format, int usage, buffer_handle_t handle)
        : w(w), h(h), stride(stride), format(format), usage(usage), handle(handle) {}
};

typedef struct hwc2_compat_device hwc2_compat_device_t;
typedef struct hwc2_compat_display hwc2_compat_display_t;
typedef struct hwc2_compat_layer hwc2_compat_layer_t;

typedef void* hwc2_display_t;
typedef int32_t hwc2_error_t;

#define HWC2_ERROR_NONE 0
#define HWC2_ERROR_HAS_CHANGES 1
#define HWC2_POWER_MODE_OFF 0
#define HWC2_POWER_MODE_ON 2
#define HWC2_VSYNC_ENABLE 1
#define HWC2_BLEND_MODE_NONE 1
#define HWC2_COMPOSITION_CLIENT 1

struct HWC2DisplayConfig {
    uint32_t width;
    uint32_t height;
    int32_t vsyncPeriod;
    int32_t dpiX;
    int32_t dpiY;
};

struct HWC2EventListener {
    void (*onVsyncReceived)(HWC2EventListener* listener, int32_t sequenceId, hwc2_display_t display, int64_t timestamp);
    void (*onHotplugReceived)(HWC2EventListener* listener, int32_t sequenceId, hwc2_display_t display, bool connected, bool primaryDisplay);
    void (*onRefreshReceived)(HWC2EventListener* listener, int32_t sequenceId, hwc2_display_t display);
    void (*onDpmsReceived)(HWC2EventListener* listener, int32_t sequenceId, hwc2_display_t display, int32_t powerMode);
};

#ifdef __cplusplus
extern "C" {
#endif

hwc2_compat_device_t* hwc2_compat_device_new(bool use_real_hwc);
void hwc2_compat_device_register_callback(hwc2_compat_device_t* device, HWC2EventListener* listener, int32_t sequenceId);
void hwc2_compat_device_on_hotplug(hwc2_compat_device_t* device, hwc2_display_t display, bool connected);
hwc2_compat_display_t* hwc2_compat_device_get_display_by_id(hwc2_compat_device_t* device, hwc2_display_t display);

hwc2_error_t hwc2_compat_display_set_client_target(hwc2_compat_display_t* display, uint32_t slot, RemoteWindowBuffer* buffer, int32_t acquireFence, int32_t dataspace);
hwc2_error_t hwc2_compat_display_validate(hwc2_compat_display_t* display, uint32_t* outNumTypes, uint32_t* outNumRequests);
hwc2_error_t hwc2_compat_display_accept_changes(hwc2_compat_display_t* display);
hwc2_error_t hwc2_compat_display_present(hwc2_compat_display_t* display, int32_t* outRetireFence);
void hwc2_compat_display_destroy_layer(hwc2_compat_display_t* display, hwc2_compat_layer_t* layer);
hwc2_compat_layer_t* hwc2_compat_display_create_layer(hwc2_compat_display_t* display);
void hwc2_compat_display_set_power_mode(hwc2_compat_display_t* display, int32_t mode);
void hwc2_compat_display_set_vsync_enabled(hwc2_compat_display_t* display, int32_t enabled);
HWC2DisplayConfig* hwc2_compat_display_get_active_config(hwc2_compat_display_t* display);

void hwc2_compat_layer_set_blend_mode(hwc2_compat_layer_t* layer, int32_t blendMode);
void hwc2_compat_layer_set_composition_type(hwc2_compat_layer_t* layer, int32_t type);
void hwc2_compat_layer_set_source_crop(hwc2_compat_layer_t* layer, float left, float top, float right, float bottom);
void hwc2_compat_layer_set_display_frame(hwc2_compat_layer_t* layer, int32_t left, int32_t top, int32_t right, int32_t bottom);
void hwc2_compat_layer_set_visible_region(hwc2_compat_layer_t* layer, int32_t left, int32_t top, int32_t right, int32_t bottom);

#ifdef __cplusplus
}
#endif

#define GRALLOC_USAGE_HW_TEXTURE 0x00000100U
#define GRALLOC_USAGE_HW_RENDER 0x00000200U
#define GRALLOC_USAGE_HW_COMPOSER 0x00000800U
#define GRALLOC_USAGE_SW_READ_OFTEN 0x00000003U
#define GRALLOC_USAGE_SW_WRITE_OFTEN 0x00000030U

#define HAL_PIXEL_FORMAT_RGBA_8888 1
#define HAL_PIXEL_FORMAT_RGBX_8888 2
#define HAL_DATASPACE_UNKNOWN 0

#ifdef __cplusplus
extern "C" {
#endif

int hybris_gralloc_allocate(int width, int height, int format, int usage, buffer_handle_t* handle, uint32_t* stride);
int hybris_gralloc_release(buffer_handle_t handle, int closeFds);
int hybris_gralloc_lock(buffer_handle_t handle, int usage, int l, int t, int w, int h, void** vaddr);
int hybris_gralloc_unlock(buffer_handle_t handle);

#ifdef __cplusplus
}
#endif
