#include <memory.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <xiApi.h>

#include "core.h"

#define TEXT_SZ 256

static const Color BACKGROUND_COLOR = {18, 18, 18, 255};
static const int WIN_W = 1280;
static const int WIN_H = 720;
static float ZOOM = 1.0;
static int FONT_SIZE = 20;

void help() {
    printf("xiclops [options]\n");
    printf("  options:\n");
    printf("    -h      \tShow this message\n");
    printf("    -c int  \tCamera ID (default = 0)\n");
    printf("    -v int  \tVerbosity level (default = 2 a.k.a INFO)\n");
    printf("    -z float\tZoom level (new/original) (default = 1.0)\n");
}

int main(int argc, char** argv) {
    int cam_id = 0;
    String log_msg = {0};
    for (size_t i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-z") == 0) {
            if (i + 1 >= argc) {
                String_printf(&log_msg,
                              "No valid value given for option -z (zoom)\n");
                Log(WARN, &log_msg);
                help();
                break;
            }
            ZOOM = atof(argv[i + 1]);
            String_printf(&log_msg, "ZOOM updated to %f\n", ZOOM);
            Log(DEBUG, &log_msg);
            i += 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            if (i + 1 >= argc) {
                String_printf(
                    &log_msg,
                    "No valid value given for option -v (verbosity)\n");
                Log(WARN, &log_msg);
                help();
                break;
            }
            VERBOSITY = atoi(argv[i + 1]);
            String_printf(&log_msg, "VERBOSITY updated to %d\n", VERBOSITY);
            Log(DEBUG, &log_msg);
            i += 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 >= argc) {
                String_printf(
                    &log_msg,
                    "No valid value given for option -c (camera ID)\n");
                Log(WARN, &log_msg);
                help();
                break;
            }
            cam_id = atoi(argv[i + 1]);
            String_printf(&log_msg, "cam_id updated to %d\n", cam_id);
            Log(DEBUG, &log_msg);
            i += 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            help();
            return 0;
        } else {
            String_printf(&log_msg, "Unknown option: %s\n", argv[i]);
            Log(WARN, &log_msg);
        }
    }

    String_printf(&log_msg, "Opening Camera %d\n", cam_id);
    Log(INFO, &log_msg);
    return 0;

    HANDLE handle = NULL;
    XI_RETURN status = XI_OK;
    status = xiOpenDevice(cam_id, &handle);
    if (status != XI_OK) {
        printf("Failed to open camera %d\n", cam_id);
        return 1;
    }
    printf("Camera %d open\n", cam_id);

    status += xiSetParamInt(handle, XI_PRM_EXPOSURE, 10000);
    int image_data_format;
    status +=
        xiGetParamInt(handle, XI_PRM_IMAGE_DATA_FORMAT, &image_data_format);
    // Only change color format if this is a grayscale camera
    if(image_data_format != XI_MONO8 && image_data_format != XI_MONO16) {
        status += xiSetParamInt(handle, XI_PRM_IMAGE_DATA_FORMAT, XI_RGB32);
    }
    status += xiSetParamInt(handle, XI_PRM_OUTPUT_DATA_BIT_DEPTH, XI_BPP_8);

    // Set width
    int w_inc;
    status += xiGetParamInt(handle, XI_PRM_WIDTH XI_PRM_INFO_INCREMENT, &w_inc);
    String_printf(&log_msg, "Width inc: %d\n", w_inc);
    Log(DEBUG, &log_msg);
    int width = (WIN_W / w_inc) * w_inc;
    status += xiSetParamInt(handle, XI_PRM_WIDTH, width);
    if (status != XI_OK) {
        printf("Failed to set width: %d\n", width);
        return 1;
    }
    // Set height
    int h_inc;
    status +=
        xiGetParamInt(handle, XI_PRM_HEIGHT XI_PRM_INFO_INCREMENT, &h_inc);
    String_printf(&log_msg, "Height inc: %d\n", h_inc);
    Log(DEBUG, &log_msg);
    int height = (WIN_H / h_inc) * h_inc;
    status += xiSetParamInt(handle, XI_PRM_HEIGHT, height);
    if (status != XI_OK) {
        printf("Failed to set height: %d\n", height);
        return 1;
    }
    String_printf(&log_msg, "Image size: [%d, %d]\n", width, height);
    Log(DEBUG, &log_msg);
    // Set x-offset
    int x_offset_inc;
    status += xiGetParamInt(handle, XI_PRM_OFFSET_X XI_PRM_INFO_INCREMENT,
                            &x_offset_inc);
    int x_offset = ((WIN_W - width) / 2 / x_offset_inc) * x_offset_inc;
    status += xiSetParamInt(handle, XI_PRM_OFFSET_X, x_offset);
    if (status != XI_OK) {
        printf("Failed to set x-offset\n");
        return 1;
    }
    // Set y-offset
    int y_offset_inc;
    status += xiGetParamInt(handle, XI_PRM_OFFSET_Y XI_PRM_INFO_INCREMENT,
                            &y_offset_inc);
    int y_offset = ((WIN_H - height) / 2 / y_offset_inc) * y_offset_inc;
    status += xiSetParamInt(handle, XI_PRM_OFFSET_Y, y_offset);
    if (status != XI_OK) {
        printf("Failed to set y-offset\n");
        return 1;
    }

    // Set bandwidth limit to max
    // status += xiSetParamInt(handle, XI_PRM_LIMIT_BANDWIDTH_MODE, XI_ON);
    int bw_max;
    status += xiGetParamInt(handle, XI_PRM_HEIGHT XI_PRM_INFO_MAX, &bw_max);
    String_printf(&log_msg, "Max bandwidth: %d\n", bw_max);
    Log(DEBUG, &log_msg);
    // status += xiSetParamInt(handle, XI_PRM_LIMIT_BANDWIDTH, bw_max);

    // Set white balance
    status += xiSetParamFloat(handle, XI_PRM_WB_KR, 1.29);
    status += xiSetParamFloat(handle, XI_PRM_WB_KG, 1.0);
    status += xiSetParamFloat(handle, XI_PRM_WB_KB, 3.04);

    if(image_data_format == XI_RGB32) {
        // Set alpha default
        status += xiSetParamInt(handle, XI_PRM_IMAGE_DATA_FORMAT_RGB32_ALPHA, 255);
    }

    int img_size_bytes = width * height;
    if(image_data_format == XI_RGB32) img_size_bytes *= 4;
    status += xiGetParamInt(handle, XI_PRM_IMAGE_PAYLOAD_SIZE, &img_size_bytes);

    // Start acquisition
    status += xiStartAcquisition(handle);
    if (status != XI_OK) {
        printf("Failed to setup camera %d\n", cam_id);
        return 1;
    }

    XI_IMG image = {0};
    memset(&image, 0, sizeof(image));
    image.size = sizeof(XI_IMG);
    image.bp = (unsigned char*)malloc(img_size_bytes);
    image.bp_size = img_size_bytes;
    String_printf(&log_msg, "Payload size: %d\n", img_size_bytes);
    Log(DEBUG, &log_msg);

    Camera2D camera = {
        .zoom = ZOOM,
        .offset = {.x = 0.0f, .y = 0.0f},
    };

    // Set the output image format
    int rl_format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    if (image_data_format == XI_RGB32) {
        rl_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    }

    Image rl_img = {
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = rl_format,
    };

    Texture2D texture = {
        .width = (int)width * ZOOM,
        .height = (int)height * ZOOM,
        .mipmaps = 1,
        .format = rl_format,
    };

    float w = WIN_W * ZOOM;
    float h = WIN_H * ZOOM;

    bool got_first = false;

    String_printf(&log_msg, "Initializing window...\n");
    Log(INFO, &log_msg);
    InitWindow(w, h, "Xiclops");
    SetWindowPosition(100 * cam_id + 100, 200 * cam_id + 100);
    SetTargetFPS(60);
    printf("Starting render loop\n");
    while (!WindowShouldClose()) {
        w = GetScreenWidth();
        String_printf(&log_msg, "Screen width: %f\n", w);
        Log(TRACE, &log_msg);
        h = GetScreenHeight();
        String_printf(&log_msg, "Screen height: %f\n", h);
        Log(TRACE, &log_msg);
        // camera.offset.x = -w / 2.0f;
        // camera.offset.y = -h / 2.0f;

        String_printf(&log_msg, "Acquiring image...\n");
        Log(TRACE, &log_msg);
        status = xiGetImage(handle, 100000, &image);
        if (status != XI_OK) {
            printf("Failed to get image on camera 0\n");
            return 1;
        }
        String_printf(&log_msg, "Image acquired!\n");
        Log(TRACE, &log_msg);
        unsigned char* pixels = (unsigned char*)image.bp;
        rl_img.data = pixels;
        if (got_first) {
            String_printf(&log_msg, "Updating texture...\n");
            Log(TRACE, &log_msg);
            UpdateTexture(texture, pixels);
            String_printf(&log_msg, "Texture updated\n");
            Log(TRACE, &log_msg);
        } else {
            String_printf(&log_msg, "Loading texture...\n");
            Log(TRACE, &log_msg);
            texture = LoadTextureFromImage(rl_img);
            got_first = true;
            String_printf(&log_msg, "Texture loaded\n");
            Log(TRACE, &log_msg);
        }

        String_printf(&log_msg, "Starting drawing...\n");
        Log(TRACE, &log_msg);
        BeginDrawing();
        BeginMode2D(camera);
        {
            ClearBackground(BACKGROUND_COLOR);
            DrawTexture(texture, 0, 0, WHITE);
            int fps = GetFPS();
            int adj_font_size = FONT_SIZE / ZOOM;
            DrawText("Graphics: Raylib", 20, 20, adj_font_size, LIGHTGRAY);
            DrawText(TextFormat("FPS: %d", fps), 20, 20 + adj_font_size,
                     adj_font_size, LIGHTGRAY);
        }
        EndMode2D();
        EndDrawing();
    }
    xiStopAcquisition(handle);
    xiCloseDevice(handle);
    return 0;
}
