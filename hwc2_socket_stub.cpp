#include "hwc2_compat_layer.h"
#include "socket_msg.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <chrono>

static int g_socket_fd = -1;

static void connect_socket() {
    if (g_socket_fd >= 0) return;
    g_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (g_socket_fd < 0) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    const char* socket_path = "/tmp/display_daemon.sock";
    
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(g_socket_fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) < 0) {
        close(g_socket_fd);
        g_socket_fd = -1;
    }
}

static int send_fd_and_msg(int sockfd, const DisplayMsg* msg, int fd_to_send) {
    struct iovec iov[1];
    struct msghdr msgh;
    union {
        struct cmsghdr cmh;
        char control[CMSG_SPACE(sizeof(int))];
    } control_un;

    iov[0].iov_base = (void*)msg;
    iov[0].iov_len = sizeof(DisplayMsg);

    msgh.msg_name = NULL;
    msgh.msg_namelen = 0;
    msgh.msg_iov = iov;
    msgh.msg_iovlen = 1;

    if (fd_to_send >= 0) {
        msgh.msg_control = control_un.control;
        msgh.msg_controllen = sizeof(control_un.control);

        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msgh);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int));
        memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));
        msgh.msg_controllen = cmsg->cmsg_len;
    } else {
        msgh.msg_control = NULL;
        msgh.msg_controllen = 0;
    }

    return sendmsg(sockfd, &msgh, 0);
}

extern "C" {

struct hwc2_compat_device {};
struct hwc2_compat_display {
    int64_t id;
    HWC2DisplayConfig config;
};
struct hwc2_compat_layer {};

static hwc2_compat_device g_dev;
static hwc2_compat_display g_disp;
static hwc2_compat_layer g_layer;
static HWC2EventListener* g_event_listener = nullptr;
static int32_t g_sequence_id = 0;

hwc2_compat_device_t* hwc2_compat_device_new(bool) {
    connect_socket();
    g_disp.config.width = 1920;
    g_disp.config.height = 1080;
    g_disp.config.vsyncPeriod = 16666666;
    return &g_dev;
}

void hwc2_compat_device_register_callback(hwc2_compat_device_t*, HWC2EventListener* listener, int32_t sequenceId) {
    g_event_listener = listener;
    g_sequence_id = sequenceId;
    // Fire a synthetic hotplug event after a short delay to trigger display connection
    std::thread([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (g_event_listener && g_event_listener->onHotplugReceived) {
            fprintf(stderr, "[hwc2_stub] Firing synthetic hotplug for display 0 (connected)\n");
            g_event_listener->onHotplugReceived(g_event_listener, g_sequence_id, 0, true, true);
        }
    }).detach();
}
void hwc2_compat_device_on_hotplug(hwc2_compat_device_t*, hwc2_display_t, bool) {}
hwc2_compat_display_t* hwc2_compat_device_get_display_by_id(hwc2_compat_device_t*, hwc2_display_t display) {
    g_disp.id = (int64_t)display;
    return &g_disp;
}

hwc2_error_t hwc2_compat_display_set_client_target(hwc2_compat_display_t* display, uint32_t, RemoteWindowBuffer* buffer, int32_t, int32_t) {
    if (!buffer) return HWC2_ERROR_NONE;
    if (g_socket_fd < 0) connect_socket();
    if (g_socket_fd >= 0 && buffer->handle && buffer->handle->numFds > 0) {
        DisplayMsg msg;
        msg.cmd = CMD_SET_BUFFER;
        msg.displayId = display->id;
        msg.width = buffer->w;
        msg.height = buffer->h;
        msg.stride = buffer->stride;
        msg.format = buffer->format;
        
        int fd = buffer->handle->data[0];
        if (send_fd_and_msg(g_socket_fd, &msg, fd) < 0) {
            close(g_socket_fd);
            g_socket_fd = -1;
        }
    }
    return HWC2_ERROR_NONE;
}

hwc2_error_t hwc2_compat_display_validate(hwc2_compat_display_t*, uint32_t* numTypes, uint32_t* numRequests) {
    if (numTypes) *numTypes = 0;
    if (numRequests) *numRequests = 0;
    return HWC2_ERROR_NONE;
}

hwc2_error_t hwc2_compat_display_accept_changes(hwc2_compat_display_t*) { return HWC2_ERROR_NONE; }
hwc2_error_t hwc2_compat_display_present(hwc2_compat_display_t*, int32_t* outRetireFence) {
    if (outRetireFence) *outRetireFence = -1;
    return HWC2_ERROR_NONE;
}

void hwc2_compat_display_destroy_layer(hwc2_compat_display_t*, hwc2_compat_layer_t*) {}
hwc2_compat_layer_t* hwc2_compat_display_create_layer(hwc2_compat_display_t*) { return &g_layer; }
void hwc2_compat_display_set_power_mode(hwc2_compat_display_t*, int32_t) {}
void hwc2_compat_display_set_vsync_enabled(hwc2_compat_display_t*, int32_t) {}

HWC2DisplayConfig* hwc2_compat_display_get_active_config(hwc2_compat_display_t* display) {
    return &display->config;
}

void hwc2_compat_layer_set_blend_mode(hwc2_compat_layer_t*, int32_t) {}
void hwc2_compat_layer_set_composition_type(hwc2_compat_layer_t*, int32_t) {}
void hwc2_compat_layer_set_source_crop(hwc2_compat_layer_t*, float, float, float, float) {}
void hwc2_compat_layer_set_display_frame(hwc2_compat_layer_t*, int32_t, int32_t, int32_t, int32_t) {}
void hwc2_compat_layer_set_visible_region(hwc2_compat_layer_t*, int32_t, int32_t, int32_t, int32_t) {}

} // extern "C"

extern "C" {
int hybris_gralloc_allocate(int width, int /*height*/, int /*format*/, int /*usage*/, buffer_handle_t* handle, uint32_t* stride) {
    // Provide a minimal dummy native_handle so that (r == 0 && handle) passes
    static native_handle_t dummy_handle_storage;
    memset(&dummy_handle_storage, 0, sizeof(dummy_handle_storage));
    dummy_handle_storage.version = sizeof(native_handle_t);
    if (handle) *handle = &dummy_handle_storage;
    if (stride) *stride = (uint32_t)width;
    return 0;
}
int hybris_gralloc_release(buffer_handle_t, int) { return 0; }
int hybris_gralloc_lock(buffer_handle_t, int, int, int, int, int, void**) { return -1; }
int hybris_gralloc_unlock(buffer_handle_t) { return 0; }
}
