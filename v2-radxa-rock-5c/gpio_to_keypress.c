// MARK: OLD CODE
// ======================

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <linux/uinput.h>
// #include <pthread.h>
// #include <signal.h>
// #include <sys/types.h>
// #include <errno.h>
// #include <time.h>
// #include <mraa/gpio.h>

// // GPIO definitions -- use BOARD numbering (physical 40-pin header)
// /*#define JOY_LEFT_UP      18
// #define JOY_LEFT_DOWN    16
// #define JOY_LEFT_LEFT    36
// // #define JOY_LEFT_RIGHT   37
// #define JOY_LEFT_BUTTON  13

// #define JOY_RIGHT_UP     12
// // #define JOY_RIGHT_DOWN   11
// #define JOY_RIGHT_LEFT   35
// // #define JOY_RIGHT_RIGHT  38
// #define JOY_RIGHT_BUTTON 40*/

// #define JOY_LEFT_UP      11   // MRAA index 11
// #define JOY_LEFT_DOWN    13   // MRAA index 13
// #define JOY_LEFT_RIGHT   18   // MRAA index 18
// #define JOY_RIGHT_UP     24   // MRAA index 24 (your original confirmed)
// #define JOY_RIGHT_BUTTON 40   // MRAA index 40

// typedef struct {
//     int pin_number;
//     mraa_gpio_context gpio;
//     int keycode;
//     int current_state;
//     int last_stable_state;
//     unsigned long last_change_time;
//     int key_pressed;
//     pthread_mutex_t mutex;
// } button_state_t;

// int uinput_fd = -1;
// int debounce_time = 20;
// int repeat_delay = 500;
// int repeat_rate = 100;

// // Update button array to use pin numbers
// /*button_state_t buttons[] = {
//     {JOY_LEFT_UP,     NULL, KEY_W},
//     {JOY_LEFT_DOWN,   NULL, KEY_S},
//     {JOY_LEFT_LEFT,   NULL, KEY_A},
//     // {JOY_LEFT_RIGHT,  NULL, KEY_D},
//     {JOY_LEFT_BUTTON, NULL, KEY_ENTER},
//     {JOY_RIGHT_UP,     NULL, KEY_UP},
//     // {JOY_RIGHT_DOWN,   NULL, KEY_DOWN},
//     {JOY_RIGHT_LEFT,   NULL, KEY_LEFT},
//     // {JOY_RIGHT_RIGHT,  NULL, KEY_RIGHT},
//     {JOY_RIGHT_BUTTON, NULL, KEY_ESC}
// };*/

// button_state_t buttons[] = {
//     {JOY_LEFT_UP,     NULL, KEY_W},
//     //{JOY_LEFT_DOWN,   NULL, KEY_S},
//     //{JOY_LEFT_RIGHT,  NULL, KEY_D},
//     //{JOY_RIGHT_UP,    NULL, KEY_UP},
//     {JOY_RIGHT_BUTTON,NULL, KEY_ESC}
// };

// int num_buttons = sizeof(buttons) / sizeof(button_state_t);

// FILE *log_file = NULL;

// #define LOG(fmt, ...) do { \
//     if (log_file) { \
//         fprintf(log_file, "[%ld] " fmt "\n", time(NULL), ##__VA_ARGS__); \
//         fflush(log_file); \
//     } \
// } while(0)

// unsigned long get_time_ms() {
//     struct timespec ts;
//     clock_gettime(CLOCK_MONOTONIC, &ts);
//     return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
// }

// int setup_uinput_device() {
//     struct uinput_user_dev uidev;
//     int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
//     if (fd < 0) {
//         LOG("ERROR: open /dev/uinput failed: %s", strerror(errno));
//         perror("open /dev/uinput");
//         return -1;
//     }
//     LOG("Opened /dev/uinput fd=%d", fd);

//     if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0)
//         LOG("ERROR: UI_SET_EVBIT failed: %s", strerror(errno));

//     int keys[] = {KEY_W, KEY_A, KEY_S, KEY_D, KEY_ENTER,
//                   KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ESC};

//     for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
//         if (ioctl(fd, UI_SET_KEYBIT, keys[i]) < 0)
//             LOG("ERROR: UI_SET_KEYBIT key=%d failed: %s", keys[i], strerror(errno));
//     }

//     memset(&uidev, 0, sizeof(uidev));
//     snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "dual-joystick-controller");
//     uidev.id.bustype = BUS_USB;
//     uidev.id.vendor = 0x1234;
//     uidev.id.product = 0x5678;
//     uidev.id.version = 1;

//     if (write(fd, &uidev, sizeof(uidev)) != sizeof(uidev))
//         LOG("ERROR: write uidev failed: %s", strerror(errno));
//     if (ioctl(fd, UI_DEV_CREATE) < 0)
//         LOG("ERROR: UI_DEV_CREATE failed: %s", strerror(errno));
//     LOG("uinput device created");
//     return fd;
// }

// void send_key_event(int keycode, int value, int pin_number) {
//     struct input_event ev = {0};

//     ev.type = EV_KEY;
//     ev.code = keycode;
//     ev.value = value;
//     if (write(uinput_fd, &ev, sizeof(ev)) != sizeof(ev))
//         LOG("ERROR: write key event failed: %s", strerror(errno));

//     ev.type = EV_SYN;
//     ev.code = SYN_REPORT;
//     ev.value = 0;
//     if (write(uinput_fd, &ev, sizeof(ev)) != sizeof(ev))
//         LOG("ERROR: write syn event failed: %s", strerror(errno));

//     LOG("Pin %d %s (keycode: %d)", pin_number, value ? "pressed" : "released", keycode);
// }

// void* button_thread(void* arg) {
//     button_state_t* btn = (button_state_t*)arg;
//     unsigned long last_repeat_time = 0;
//     unsigned long press_start_time = 0;
//     int repeat_started = 0;

//     LOG("Thread started for Pin %d", btn->pin_number);

//     while (1) {
//         pthread_mutex_lock(&btn->mutex);

//         int current_reading = mraa_gpio_read(btn->gpio);
//         unsigned long now = get_time_ms();

//         if (current_reading != 0 && current_reading != 1) {
//             LOG("WARNING: Pin %d read invalid value %d", btn->pin_number, current_reading);
//         }

//         if (current_reading != btn->current_state) {
//             btn->last_change_time = now;
//             btn->current_state = current_reading;
//             LOG("Pin %d state changed to %d", btn->pin_number, current_reading);
//         }

//         if ((now - btn->last_change_time) > debounce_time) {
//             if (btn->current_state != btn->last_stable_state) {
//                 LOG("Pin %d stable state change: %d -> %d", btn->pin_number, btn->last_stable_state, btn->current_state);
//                 if (btn->current_state == 0 && btn->last_stable_state == 1) {
//                     if (!btn->key_pressed) {
//                         send_key_event(btn->keycode, 1, btn->pin_number);
//                         btn->key_pressed = 1;
//                         press_start_time = now;
//                         last_repeat_time = now;
//                         repeat_started = 0;
//                         LOG("Pin %d key press event triggered", btn->pin_number);
//                     }
//                 } else if (btn->current_state == 1 && btn->last_stable_state == 0) {
//                     if (btn->key_pressed) {
//                         send_key_event(btn->keycode, 0, btn->pin_number);
//                         btn->key_pressed = 0;
//                         repeat_started = 0;
//                         LOG("Pin %d key release event triggered", btn->pin_number);
//                     }
//                 }
//                 btn->last_stable_state = btn->current_state;
//             }
//         }

//         if (btn->key_pressed && btn->keycode != KEY_ESC) {
//             if (!repeat_started && (now - press_start_time) > repeat_delay) {
//                 repeat_started = 1;
//                 last_repeat_time = now;
//                 LOG("Pin %d repeat started", btn->pin_number);
//             }

//             if (repeat_started && (now - last_repeat_time) > repeat_rate) {
//                 LOG("Pin %d repeat event", btn->pin_number);
//                 send_key_event(btn->keycode, 0, btn->pin_number);
//                 usleep(1000);
//                 send_key_event(btn->keycode, 1, btn->pin_number);
//                 last_repeat_time = now;
//             }
//         }

//         pthread_mutex_unlock(&btn->mutex);
//         usleep(1000);
//     }

//     return NULL;
// }

// void setup_gpio(button_state_t* btn) {
//     btn->gpio = mraa_gpio_init(btn->pin_number); // Use BOARD numbering (pin number)
//     if (!btn->gpio) {
//         LOG("ERROR: Failed to initialize Pin %d", btn->pin_number);
//         fprintf(stderr, "Failed to initialize Pin %d\n", btn->pin_number);
//         exit(1);
//     }

//     mraa_gpio_dir(btn->gpio, MRAA_GPIO_IN);
//     mraa_gpio_mode(btn->gpio, MRAA_GPIO_PULLUP);

//     int level = mraa_gpio_read(btn->gpio);
//     if (level != 0 && level != 1)
//         LOG("WARNING: Pin %d initial read value is %d", btn->pin_number, level);

//     btn->current_state = level;
//     btn->last_stable_state = level;
//     btn->last_change_time = get_time_ms();
//     btn->key_pressed = 0;
//     pthread_mutex_init(&btn->mutex, NULL);

//     LOG("Pin %d initialized, state = %d", btn->pin_number, level);

//     pthread_t thread_id;
//     pthread_create(&thread_id, NULL, button_thread, btn);
// }

// void cleanup(int signum) {
//     LOG("Signal %d received, cleaning up", signum);
//     for (int i = 0; i < num_buttons; i++) {
//         pthread_mutex_lock(&buttons[i].mutex);
//         if (buttons[i].key_pressed)
//             send_key_event(buttons[i].keycode, 0, buttons[i].pin_number);
//         pthread_mutex_unlock(&buttons[i].mutex);

//         if (buttons[i].gpio)
//             mraa_gpio_close(buttons[i].gpio);
//     }

//     if (uinput_fd != -1) {
//         ioctl(uinput_fd, UI_DEV_DESTROY);
//         close(uinput_fd);
//         LOG("uinput device destroyed");
//     }

//     mraa_deinit();
//     LOG("MRAA deinitialized, exiting");
//     if (log_file) fclose(log_file);
//     exit(signum);
// }

// int main() {
//     log_file = fopen("./joystick.log", "a");
//     if (!log_file) {
//         fprintf(stderr, "ERROR: Could not open log file\n");
//         exit(1);
//     }

//     LOG("Starting dual joystick controller (MRAA-only)...");

//     if (mraa_init() != MRAA_SUCCESS) {
//         LOG("ERROR: Failed to initialize MRAA");
//         fprintf(stderr, "Failed to initialize MRAA\n");
//         fclose(log_file);
//         return -1;
//     }

//     uinput_fd = setup_uinput_device();
//     if (uinput_fd < 0) {
//         LOG("ERROR: Could not set up uinput device");
//         fprintf(stderr, "Could not set up uinput device\n");
//         fclose(log_file);
//         return -1;
//     }

//     for (int i = 0; i < num_buttons; i++)
//         setup_gpio(&buttons[i]);

//     signal(SIGINT, cleanup);
//     signal(SIGTERM, cleanup);

//     while (1)
//         sleep(1);

//     return 0;
// }




// MARK: NEW CODE
// ======================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <gpiod.h>

#define DEBOUNCE_MS 20
#define REPEAT_DELAY_MS 500
#define REPEAT_RATE_MS 100

// Mapping: pin name, physical pin, logical name, key code
struct button_config {
    const char *pin_name;   // e.g. "PIN_18"
    int physical_pin;       // e.g. 18
    const char *logical_name; // e.g. "JOY_LEFT_UP"
    int keycode;            // Linux keycode
};

#include <linux/input-event-codes.h>
struct button_config configs[] = {
    {"PIN_18", 18, "JOY_LEFT_UP", KEY_W},
    {"PIN_16", 16, "JOY_LEFT_DOWN", KEY_S},
    {"PIN_36", 36, "JOY_LEFT_LEFT", KEY_A},
    {"PIN_33", 33, "JOY_LEFT_RIGHT", KEY_D}, // Analog-only, skip –– Replace with 33 instead?
    {"PIN_13", 13, "JOY_LEFT_BUTTON", KEY_ENTER},
    {"PIN_12", 12, "JOY_RIGHT_UP", KEY_UP},
    {"PIN_11", 11, "JOY_RIGHT_DOWN", KEY_DOWN},
    {"PIN_35", 35, "JOY_RIGHT_LEFT", KEY_LEFT},
    {"PIN_38", 38, "JOY_RIGHT_RIGHT", KEY_RIGHT},
    {"PIN_40", 40, "JOY_RIGHT_BUTTON", KEY_ESC},
};
int num_buttons = sizeof(configs)/sizeof(configs[0]);

struct button_state {
    struct button_config *cfg;
    struct gpiod_line *line;
    int current_state;
    int last_stable_state;
    unsigned long last_change_time;
    int key_pressed;
    pthread_mutex_t mutex;
};

struct button_state *buttons = NULL;
int uinput_fd = -1;

// Console logging version
#define LOG(fmt, ...) do { \
    fprintf(stdout, "[%ld] " fmt "\n", time(NULL), ##__VA_ARGS__); \
    fflush(stdout); \
} while(0)

unsigned long get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Use gpiofind + gpiod to get a line by pin name
struct gpiod_line *find_line_by_pin(const char *pin_name) {
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "gpiofind %s", pin_name);
    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;
    char line[128];
    if (!fgets(line, sizeof(line), fp)) {
        pclose(fp);
        return NULL;
    }
    pclose(fp);
    char chip_name[64];
    int offset;
    if (sscanf(line, "%s %d", chip_name, &offset) != 2) return NULL;
    struct gpiod_chip *chip = gpiod_chip_open_by_name(chip_name);
    if (!chip) return NULL;
    struct gpiod_line *gline = gpiod_chip_get_line(chip, offset);
    // chip will be closed when line is released
    return gline;
}

int setup_uinput_device() {
    struct uinput_user_dev uidev;
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        LOG("ERROR: open /dev/uinput failed: %s", strerror(errno));
        perror("open /dev/uinput");
        return -1;
    }
    LOG("Opened /dev/uinput fd=%d", fd);

    ioctl(fd, UI_SET_EVBIT, EV_KEY);

    int keys[] = {KEY_W, KEY_A, KEY_S, KEY_D, KEY_ENTER,
                  KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ESC};
    for (int i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        ioctl(fd, UI_SET_KEYBIT, keys[i]);

    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "dual-joystick-controller");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x1234;
    uidev.id.product = 0x5678;
    uidev.id.version = 1;

    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);

    LOG("uinput device created");
    return fd;
}

void send_key_event(int keycode, int value, const char *logical_name, int pin_number) {
    struct input_event ev = {0};

    ev.type = EV_KEY;
    ev.code = keycode;
    ev.value = value;
    write(uinput_fd, &ev, sizeof(ev));

    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(uinput_fd, &ev, sizeof(ev));

    LOG("Pin %d [%s] %s (keycode: %d)", pin_number, logical_name, value ? "pressed" : "released", keycode);
}

void* button_thread(void* arg) {
    struct button_state* btn = (struct button_state*)arg;
    unsigned long last_repeat_time = 0;
    unsigned long press_start_time = 0;
    int repeat_started = 0;

    LOG("Thread started for Pin %d [%s]", btn->cfg->physical_pin, btn->cfg->logical_name);

    while (1) {
        pthread_mutex_lock(&btn->mutex);

        int current_reading = gpiod_line_get_value(btn->line);
        unsigned long now = get_time_ms();

        if (current_reading != 0 && current_reading != 1) {
            LOG("WARNING: Pin %d [%s] read invalid value %d", btn->cfg->physical_pin, btn->cfg->logical_name, current_reading);
        }

        if (current_reading != btn->current_state) {
            btn->last_change_time = now;
            btn->current_state = current_reading;
            LOG("Pin %d [%s] state changed to %d", btn->cfg->physical_pin, btn->cfg->logical_name, current_reading);
        }

        if ((now - btn->last_change_time) > DEBOUNCE_MS) {
            if (btn->current_state != btn->last_stable_state) {
                LOG("Pin %d [%s] stable state change: %d -> %d", btn->cfg->physical_pin, btn->cfg->logical_name, btn->last_stable_state, btn->current_state);
                // Active LOW: pressed = 0, released = 1
                if (btn->current_state == 0 && btn->last_stable_state == 1) {
                    if (!btn->key_pressed) {
                        send_key_event(btn->cfg->keycode, 1, btn->cfg->logical_name, btn->cfg->physical_pin);
                        btn->key_pressed = 1;
                        press_start_time = now;
                        last_repeat_time = now;
                        repeat_started = 0;
                        LOG("Pin %d [%s] key press event triggered", btn->cfg->physical_pin, btn->cfg->logical_name);
                    }
                } else if (btn->current_state == 1 && btn->last_stable_state == 0) {
                    if (btn->key_pressed) {
                        send_key_event(btn->cfg->keycode, 0, btn->cfg->logical_name, btn->cfg->physical_pin);
                        btn->key_pressed = 0;
                        repeat_started = 0;
                        LOG("Pin %d [%s] key release event triggered", btn->cfg->physical_pin, btn->cfg->logical_name);
                    }
                }
                btn->last_stable_state = btn->current_state;
            }
        }

        if (btn->key_pressed && btn->cfg->keycode != KEY_ESC) {
            if (!repeat_started && (now - press_start_time) > REPEAT_DELAY_MS) {
                repeat_started = 1;
                last_repeat_time = now;
                LOG("Pin %d [%s] repeat started", btn->cfg->physical_pin, btn->cfg->logical_name);
            }

            if (repeat_started && (now - last_repeat_time) > REPEAT_RATE_MS) {
                LOG("Pin %d [%s] repeat event", btn->cfg->physical_pin, btn->cfg->logical_name);
                send_key_event(btn->cfg->keycode, 0, btn->cfg->logical_name, btn->cfg->physical_pin);
                usleep(1000);
                send_key_event(btn->cfg->keycode, 1, btn->cfg->logical_name, btn->cfg->physical_pin);
                last_repeat_time = now;
            }
        }

        pthread_mutex_unlock(&btn->mutex);
        usleep(1000);
    }

    return NULL;
}

void setup_gpio(struct button_state* btn) {
    btn->line = find_line_by_pin(btn->cfg->pin_name);
    if (!btn->line) {
        LOG("ERROR: Failed to get line for Pin %d [%s] (%s)", btn->cfg->physical_pin, btn->cfg->logical_name, btn->cfg->pin_name);
        fprintf(stderr, "Failed to get line for Pin %d [%s] (%s)\n", btn->cfg->physical_pin, btn->cfg->logical_name, btn->cfg->pin_name);
        exit(1);
    }

    if (gpiod_line_request_input_flags(btn->line, "joystick", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP) < 0) {
        LOG("ERROR: Failed to request input for Pin %d [%s] (%s)", btn->cfg->physical_pin, btn->cfg->logical_name, btn->cfg->pin_name);
        fprintf(stderr, "Failed to request input for Pin %d [%s] (%s)\n", btn->cfg->physical_pin, btn->cfg->logical_name, btn->cfg->pin_name);
        exit(1);
    }

    int level = gpiod_line_get_value(btn->line);
    if (level != 0 && level != 1)
        LOG("WARNING: Pin %d [%s] initial read value is %d", btn->cfg->physical_pin, btn->cfg->logical_name, level);

    btn->current_state = level;
    btn->last_stable_state = level;
    btn->last_change_time = get_time_ms();
    btn->key_pressed = 0;
    pthread_mutex_init(&btn->mutex, NULL);

    LOG("Pin %d [%s] initialized, state = %d", btn->cfg->physical_pin, btn->cfg->logical_name, level);

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, button_thread, btn);
}

void cleanup(int signum) {
    LOG("Signal %d received, cleaning up", signum);
    for (int i = 0; i < num_buttons; i++) {
        pthread_mutex_lock(&buttons[i].mutex);
        if (buttons[i].key_pressed)
            send_key_event(buttons[i].cfg->keycode, 0, buttons[i].cfg->logical_name, buttons[i].cfg->physical_pin);
        pthread_mutex_unlock(&buttons[i].mutex);

        if (buttons[i].line)
            gpiod_line_release(buttons[i].line);
    }

    if (uinput_fd != -1) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        LOG("uinput device destroyed");
    }

    LOG("Exiting");
    exit(signum);
}

int main() {
    LOG("Starting GPIO handler (libgpiod)...");

    uinput_fd = setup_uinput_device();
    if (uinput_fd < 0) {
        LOG("ERROR: Could not set up uinput device");
        fprintf(stderr, "Could not set up uinput device\n");
        return -1;
    }

    buttons = calloc(num_buttons, sizeof(struct button_state));
    if (!buttons) {
        LOG("ERROR: Could not allocate buttons");
        return -1;
    }

    for (int i = 0; i < num_buttons; i++) {
        buttons[i].cfg = &configs[i];
        setup_gpio(&buttons[i]);
    }

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);

    while (1)
        sleep(1);

    return 0;
}