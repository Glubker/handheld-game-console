import gpiod
import time

DEBOUNCE_MS = 20

BUTTONS = [
    {"chip": "gpiochip1", "line": 8,  "name": "JOY_LEFT_UP",     "pin": 18},
    {"chip": "gpiochip1", "line": 5,  "name": "JOY_LEFT_DOWN",   "pin": 16},
    {"chip": "gpiochip4", "line": 2,  "name": "JOY_LEFT_LEFT",   "pin": 36},
    # {"chip": "gpiochipX", "line": X, "name": "JOY_LEFT_RIGHT",  "pin": 37}, # Not available
    {"chip": "gpiochip4", "line": 10, "name": "JOY_LEFT_BUTTON", "pin": 13},
    {"chip": "gpiochip4", "line": 1,  "name": "JOY_RIGHT_UP",    "pin": 12},
    {"chip": "gpiochip4", "line": 11, "name": "JOY_RIGHT_DOWN",  "pin": 11},
    {"chip": "gpiochip4", "line": 0,  "name": "JOY_RIGHT_LEFT",  "pin": 35},
    {"chip": "gpiochip4", "line": 5,  "name": "JOY_RIGHT_RIGHT", "pin": 38},
    {"chip": "gpiochip4", "line": 9,  "name": "JOY_RIGHT_BUTTON","pin": 40},
]

def request_lines(buttons):
    requested = []
    chips = {}
    for btn in buttons:
        chip_name = btn["chip"]
        if chip_name not in chips:
            chips[chip_name] = gpiod.Chip(chip_name)
        chip = chips[chip_name]
        line = chip.get_line(btn["line"])
        line.request(consumer="rock5c-joystick", type=gpiod.LINE_REQ_DIR_IN, flags=gpiod.LINE_REQ_FLAG_BIAS_PULL_UP)
        requested.append(line)
    return requested

def main():
    requested_lines = request_lines(BUTTONS)
    last_state = [line.get_value() for line in requested_lines]
    last_change_time = [time.time() for _ in requested_lines]

    print("Monitoring joystick GPIOs (active LOW, internal pull-up)...")
    try:
        while True:
            for i, line in enumerate(requested_lines):
                state = line.get_value()
                now = time.time()
                if state not in (0, 1): continue
                if state != last_state[i]:
                    last_change_time[i] = now
                if state != last_state[i] and (now - last_change_time[i]) * 1000 > DEBOUNCE_MS:
                    last_state[i] = state
                    btn = BUTTONS[i]
                    if state == 0:
                        print(f"{btn['name']} (Pin {btn['pin']}) PRESSED (LOW)")
                    else:
                        print(f"{btn['name']} (Pin {btn['pin']}) RELEASED (HIGH)")
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("Exiting.")

if __name__ == "__main__":
    main()