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
