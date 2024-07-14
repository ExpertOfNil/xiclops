#include <memory.h>
#include <pthread.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <time.h>
#include <xiApi.h>

typedef struct {
    int x;
    int y;
} Vec2i;

static const Color BACKGROUND_COLOR = {18, 18, 18, 255};
static const int WIN_W = 3840;
static const int WIN_H = 2160;
static float ZOOM = 1.0;
static int FONT_SIZE = 20;

enum LEVEL {
    ERROR,
    WARN,
    INFO,
    DEBUG,
    TRACE,
};

static enum LEVEL VERBOSITY = INFO;

const char* LevelStr(enum LEVEL lvl) {
    switch (lvl) {
        case ERROR: {
            return "ERROR";
        }
        case WARN: {
            return "WARN";
        }
        case INFO: {
            return "INFO";
        }
        case DEBUG: {
            return "DEBUG";
        }
        case TRACE: {
            return "TRACE";
        }
    }
}

void Log(enum LEVEL v, char* msg) {
    if (v <= VERBOSITY) {
        printf("[XICLOPS %s] %s", LevelStr(v), msg);
    }
}

void help() {
    printf("xiclops [options]\n");
    printf("  options:\n");
    printf("    -h      \tShow this message\n");
    printf("    -c int  \tCamera ID (default = 0)\n");
    printf("    -v int  \tVerbosity level (default = 2 a.k.a INFO)\n");
    printf("    -z float\tZoom level (new/original) (default = 1.0)\n");
}

typedef struct {
    PHANDLE handle;
    XI_IMG* image;
    pthread_mutex_t* mutex;
    pthread_barrier_t* barrier;
} ThreadArgs;

const struct timespec tp = {0, 100};
void* get_xi_img(void* targs) {
    char* log_msg;
    while (1) {
        ThreadArgs args = *(ThreadArgs*)targs;
        pthread_mutex_lock(args.mutex);
        int status = xiGetImage(*(args.handle), 10000, args.image);
        pthread_mutex_unlock(args.mutex);
        if (status != XI_OK) {
            printf("Failed to get image on camera 0\n");
        }
        // pthread_barrier_wait(args->barrier);
        nanosleep(&tp, NULL);
    }
    free(log_msg);
}

typedef struct {
    int width;
    int height;
} ImageSize;

int setupCamera(PHANDLE handle, ImageSize* img_sz) {
    XI_RETURN status = XI_OK;
    char* log_msg;
    status += xiSetParamInt(*handle, XI_PRM_EXPOSURE, 20000);
    status += xiSetParamInt(*handle, XI_PRM_IMAGE_DATA_FORMAT, XI_RGB32);
    status += xiSetParamInt(*handle, XI_PRM_OUTPUT_DATA_BIT_DEPTH, XI_BPP_8);
    // Set width
    int w_inc;
    status += xiGetParamInt(*handle, XI_PRM_WIDTH XI_PRM_INFO_INCREMENT, &w_inc);
    asprintf(&log_msg, "Width inc: %d\n", w_inc);
    Log(DEBUG, log_msg);
    int width = (WIN_W / w_inc) * w_inc;
    status += xiSetParamInt(*handle, XI_PRM_WIDTH, width);
    if (status != XI_OK) {
        printf("Failed to set width: %d\n", width);
        return 1;
    }
    // Set height
    int h_inc;
    status +=
        xiGetParamInt(*handle, XI_PRM_HEIGHT XI_PRM_INFO_INCREMENT, &h_inc);
    asprintf(&log_msg, "Height inc: %d\n", h_inc);
    Log(DEBUG, log_msg);
    int height = (WIN_H / h_inc) * h_inc;
    status += xiSetParamInt(*handle, XI_PRM_HEIGHT, height);
    if (status != XI_OK) {
        printf("Failed to set height: %d\n", height);
        return 1;
    }
    asprintf(&log_msg, "Image size: [%d, %d]\n", width, height);
    Log(DEBUG, log_msg);
    // Set x-offset
    int x_offset_inc;
    status += xiGetParamInt(
        *handle, XI_PRM_OFFSET_X XI_PRM_INFO_INCREMENT, &x_offset_inc);
    int x_offset = ((WIN_W - width) / 2 / x_offset_inc) * x_offset_inc;
    status += xiSetParamInt(*handle, XI_PRM_OFFSET_X, x_offset);
    if (status != XI_OK) {
        printf("Failed to set x-offset\n");
        return 1;
    }
    // Set y-offset
    int y_offset_inc;
    status += xiGetParamInt(
        *handle, XI_PRM_OFFSET_Y XI_PRM_INFO_INCREMENT, &y_offset_inc);
    int y_offset = ((WIN_H - height) / 2 / y_offset_inc) * y_offset_inc;
    status += xiSetParamInt(*handle, XI_PRM_OFFSET_Y, y_offset);
    if (status != XI_OK) {
        printf("Failed to set y-offset\n");
        return 1;
    }

    // Set bandwidth limit to max
    status += xiSetParamInt(*handle, XI_PRM_LIMIT_BANDWIDTH_MODE, XI_ON);
    int bw_max;
    status += xiGetParamInt(*handle, XI_PRM_HEIGHT XI_PRM_INFO_MAX, &bw_max);
    status += xiSetParamInt(*handle, XI_PRM_LIMIT_BANDWIDTH, 2664);

    // Set white balance
    status += xiSetParamFloat(*handle, XI_PRM_WB_KR, 1.29);
    status += xiSetParamFloat(*handle, XI_PRM_WB_KG, 1.0);
    status += xiSetParamFloat(*handle, XI_PRM_WB_KB, 3.04);

    // Set alpha default
    status += xiSetParamInt(*handle, XI_PRM_IMAGE_DATA_FORMAT_RGB32_ALPHA, 255);

    // Start acquisition
    status += xiStartAcquisition(*handle);
    if (status != XI_OK) {
        printf("Failed to setup camera 0\n");
        return 1;
    }
    img_sz->width = width;
    img_sz->height = height;
    free(log_msg);
    return status;
}

xiProcessingHandle_t proc;
int main(int argc, char** argv) {
    int cam_id = 0;
    char* log_msg;
    for (size_t i = 1; i < argc; ++i) {
        asprintf(&log_msg, "CL Argument: %s\n", argv[i]);
        Log(TRACE, log_msg);
        if (strcmp(argv[i], "-z") == 0) {
            if (i + 1 >= argc) {
                asprintf(
                    &log_msg, "No valid value given for option -z (zoom)\n");
                Log(WARN, log_msg);
                help();
                break;
            }
            asprintf(&log_msg, "CL Argument Value: %s\n", argv[i + 1]);
            Log(TRACE, log_msg);
            ZOOM = atof(argv[i + 1]);
            asprintf(&log_msg, "ZOOM updated to %f\n", ZOOM);
            Log(DEBUG, log_msg);
            i += 1;
        } else if (strcmp(argv[i], "-v") == 0) {
            if (i + 1 >= argc) {
                asprintf(
                    &log_msg,
                    "No valid value given for option -v (verbosity)\n");
                Log(WARN, log_msg);
                help();
                break;
            }
            asprintf(&log_msg, "CL Argument Value: %s\n", argv[i + 1]);
            Log(TRACE, log_msg);
            VERBOSITY = atoi(argv[i + 1]);
            asprintf(&log_msg, "VERBOSITY updated to %d\n", VERBOSITY);
            Log(DEBUG, log_msg);
            i += 1;
        } else if (strcmp(argv[i], "-c") == 0) {
            if (i + 1 >= argc) {
                asprintf(
                    &log_msg,
                    "No valid value given for option -c (camera ID)\n");
                Log(WARN, log_msg);
                help();
                break;
            }
            asprintf(&log_msg, "CL Argument Value: %s\n", argv[i + 1]);
            Log(TRACE, log_msg);
            cam_id = atoi(argv[i + 1]);
            asprintf(&log_msg, "cam_id updated to %d\n", cam_id);
            Log(DEBUG, log_msg);
            i += 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            help();
            return 0;
        } else {
            asprintf(&log_msg, "Unknown option: %s\n", argv[i]);
            Log(WARN, log_msg);
        }
    }

    XI_RETURN status = XI_OK;
    unsigned int ndevices = 0;
    status = xiGetNumberDevices(&ndevices);

    // HANDLE handle = NULL;
    HANDLE* handles = calloc(ndevices, sizeof(HANDLE));
    ImageSize img_sz;
    for (size_t i = 0; i < ndevices; ++i) {
        asprintf(&log_msg, "Opening Camera %zu\n", i);
        Log(INFO, log_msg);
        status = xiOpenDevice(i, &(handles[i]));
        if (status != XI_OK) {
            printf("Failed to open camera %zu\n", i);
            return 1;
        }
        setupCamera(&(handles[i]), &img_sz);
    }
    // TODO: Remove this stopgap
    int width = img_sz.width;
    int height = img_sz.height;

    int img_size_bytes = width * height * 4;
    XI_IMG image;
    memset(&image, 0, sizeof(image));
    image.size = sizeof(XI_IMG);
    image.bp = (unsigned char*)malloc(img_size_bytes);
    image.bp_size = img_size_bytes;
    asprintf(&log_msg, "Payload size: %d\n", img_size_bytes);
    Log(DEBUG, log_msg);

    XI_IMG* images = calloc(ndevices, sizeof(XI_IMG));
    ThreadArgs* targs = calloc(ndevices, sizeof(ThreadArgs));
    pthread_t* threads = calloc(ndevices, sizeof(pthread_t));
    pthread_mutex_t* mutexes = calloc(ndevices, sizeof(pthread_mutex_t));
    pthread_barrier_t barrier;
    for (size_t i = 0; i < ndevices; ++i) {
        images[i].size = sizeof(XI_IMG);
        images[i].bp = (unsigned char*)malloc(img_size_bytes);
        images[i].bp_size = img_size_bytes;

        targs[i].handle = &(handles[i]);
        targs[i].image = &images[i];
        targs[i].mutex = &mutexes[i];
        targs[i].barrier = &barrier;

        pthread_create(&threads[i], NULL, &get_xi_img, &targs[i]);
    }

    Camera2D camera = {
        .zoom = ZOOM,
        .offset = {.x = 0.0f, .y = 0.0f},
    };

    Image rl_img = {
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };

    Texture2D texture = {
        .width = width,
        .height = height,
        .mipmaps = 4,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };

    Texture2D* textures = calloc(ndevices, sizeof(Texture2D));

    Vec2i positions[4] = {
        {.x = 0, .y = 0},
        {.x = texture.width, .y = 0},
        {.x = 0, .y = texture.height},
        {.x = texture.width, .y = texture.height},
    };

    float w = WIN_W * ZOOM * 2;
    float h = WIN_H * ZOOM * 2;

    bool got_first = false;

    asprintf(&log_msg, "Initializing window...\n");
    Log(INFO, log_msg);
    InitWindow(w, h, "Xiclops");
    SetWindowPosition(100 * cam_id + 100, 200 * cam_id + 100);
    SetTargetFPS(60);
    printf("Starting render loop\n");
    while (!WindowShouldClose()) {
        double rl_all_start = GetTime();

        w = GetScreenWidth();
        asprintf(&log_msg, "Screen width: %f\n", w);
        Log(TRACE, log_msg);
        h = GetScreenHeight();
        asprintf(&log_msg, "Screen height: %f\n", h);
        Log(TRACE, log_msg);

        // Image acquisition
        asprintf(&log_msg, "Acquiring image...\n");
        Log(TRACE, log_msg);

        // double rl_acq_start = GetTime();
        //// status = xiGetImage(handle, 10000, &image);
        //// if (status != XI_OK) {
        ////     printf("Failed to get image on camera 0\n");
        ////     return 1;
        //// }
        // double rl_acq_end = GetTime();

        // asprintf(
        //     &log_msg,
        //     "Image Acquisition: %lf ms\n",
        //     (rl_acq_end - rl_acq_start) * 1000);
        // Log(TRACE, log_msg);

        // asprintf(&log_msg, "Image acquired!\n");
        // Log(TRACE, log_msg);
        // unsigned char* pixels = (unsigned char*)image.bp;
        // rl_img.data = pixels;

        // Load image to texture
        if (got_first) {
            // asprintf(&log_msg, "Updating texture...\n");
            // Log(TRACE, log_msg);
            // UpdateTexture(texture, pixels);

            double rl_start = GetTime();
            for (size_t i = 0; i < ndevices; ++i) {
                pthread_mutex_lock(&mutexes[i]);
                unsigned char* pixels = (unsigned char*)(images[i].bp);
                pthread_mutex_unlock(&mutexes[i]);
                UpdateTexture(textures[i], pixels);
            }
            double rl_end = GetTime();

            asprintf(
                &log_msg,
                "UpdateTexture: %lf ms\n",
                (rl_end - rl_start) * 1000);
            Log(TRACE, log_msg);

            asprintf(&log_msg, "Texture updated\n");
            Log(TRACE, log_msg);
        } else {
            asprintf(&log_msg, "Loading texture...\n");
            Log(TRACE, log_msg);
            texture = LoadTextureFromImage(rl_img);
            // Experimental
            for (size_t i = 0; i < ndevices; ++i) {
                textures[i] = LoadTextureFromImage(rl_img);
            }
            // END Experimental
            got_first = true;
            asprintf(&log_msg, "Texture loaded\n");
            Log(TRACE, log_msg);
        }

        asprintf(&log_msg, "Starting drawing...\n");
        Log(TRACE, log_msg);
        BeginDrawing();
        BeginMode2D(camera);
        {
            ClearBackground(BACKGROUND_COLOR);
            for (size_t i = 0; i < ndevices; ++i) {
                Vec2i* pos = &positions[i];
                DrawTexture(textures[i], pos->x, pos->y, WHITE);
            }
            char* fps_msg;
            int fps = GetFPS();
            asprintf(&fps_msg, "FPS: %d", fps);
            int adj_font_size = FONT_SIZE / ZOOM;
            DrawText("Graphics: Raylib", 20, 20, adj_font_size, LIGHTGRAY);
            DrawText(fps_msg, 20, 20 + adj_font_size, adj_font_size, LIGHTGRAY);
        }
        EndMode2D();
        EndDrawing();
        double rl_all_end = GetTime();

        asprintf(
            &log_msg,
            "Frame Time: %lf ms\n",
            (rl_all_end - rl_all_start) * 1000);
        Log(TRACE, log_msg);
        Log(TRACE,
            "=================================================================="
            "==============\n");
    }
    for (size_t i = 0; i < ndevices; ++i) {
        xiStopAcquisition(handles[i]);
        xiCloseDevice(handles[i]);
    }
    free(textures);
    free(log_msg);
    return 0;
}
