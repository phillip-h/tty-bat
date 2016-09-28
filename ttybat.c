#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h> 

#include <ncurses.h>

const int EXIT_OK = 0;
const int EXIT_DONE = 1;
const int EXIT_BAD_ARGS = -1;
const int EXIT_BAD_FILE = -2;
const int EXIT_BAD_ALLOC = -3;

const char *vn = "0.1";
const char *bat_file = "/sys/class/power_supply/BAT0/capacity";

void help(void);
void version(void);
int parse_args(int argc, char **argv);
const char* check_settings(void);

void start_nc(void);
void end_nc(void);

void set_colors(int power);
int read_bat(char *buf);
void draw_bat_edge(void);
void draw_bat_middle(int power);
void draw_stat(const char *buf);

void update_pads(const char *buf);
void pad_x(size_t amount);
void pad_y(size_t amount);

bool glyphs[] = 
{
true,  true,  true,  false, true,  false, true, true, true,
true,  true,  true,  true,  false, true, true, true, true,
true,  true,  true,  true,  true,  true, true, true, true,
true,  true,  true,  true,  false, false,
true,  false, true,  false, true,  false, false, false, true,
false, false, true,  true,  false, true, true, false, false,
true,  false, false, false, false, true,  true, false, true,
true,  false, true,  false, false, true,
true,  false, true,  false, true,  false, true, true, true,
true,  true,  true,  true,  true,  true, true, true, true,
true,  true,  true,  false, false, true, true, true, true,
true,  true,  true,  false, true,  false,
true,  false, true,  false, true,  false, true, false, false,
false, false, true,  false, false, true, false, false, true,
true,  false, true,  false, false, true, true, false, true,
false, false, true,  true,  false, false,
true,  true,  true,  false, true,  false, true, true, true,
true,  true,  true,  false, false, true, true, true, true,
true,  true,  true,  false, false, true, true, true, true,
false, false, true,  false, false, true
};

int tick = 1000;
char *block = "  ";

int bat_height = 2;
int bat_width = 10;

bool draw_percent = true;
int blink = -1;

int color = -1;
int color_low = -1;
int b_color = -1;
int b_color_low = -1;
bool bold = false;

int low = 10;

int bat_pad = 0;
int stat_pad = 0;
int y_pad = 0;

int main(int argc, char **argv) {
    int arg_rc = parse_args(argc, argv);
    if (arg_rc != EXIT_OK) return arg_rc;


    char *buf = malloc(4 * sizeof(char));
    if (!buf) {
        printf("Failed to allocate buffer!\n");
        printf("Abort!\n");
        return EXIT_BAD_ALLOC;
    }

    start_nc();
    while (true) {
        int rc = read_bat(buf);
        if (rc != EXIT_OK) {
            printw("Error reading battery level (%d)\n", rc);
            printw("Press any key to exit.\n");
            timeout(-1);
            getch();
            break;
        }

        const char *ok = check_settings();
        if (ok) {
            printw("Error--%s\n", ok);
            printw("Press any key to exit.\n");
            timeout(-1);
            getch();
            break;
        }

        update_pads(buf);
        int power = strtol(buf, NULL, 10);

        if (blink != -1 && power <= low && tick >= 500) {
            blink = (blink + 1) % 2;
        }

        set_colors(power);
        pad_y(y_pad);

        if (power > low || blink != 0) {
            draw_bat_edge();
            draw_bat_middle(power);
            draw_bat_edge();
        } else {
            pad_y(bat_height + 2);
        }

        if (draw_percent) draw_stat(buf);

        refresh();
        if (getch() == 'q') break;

        erase();
    }

    free(buf);
    end_nc();

    return EXIT_OK;
}

const char* check_settings(void) {
    if (bat_width <= 0 || bat_height <= 0)
        return "Invalid battery dimensions.";

    if (bat_width + 2 > COLS ||
        bat_height + 2 + 6 > LINES) {
        return "Terminal too small for current settings.";
    }

    if (tick <= 0) {
        return "Tick rate must be positive.";
    }

    return NULL;
}

void draw_glyph(size_t glyph, size_t row) {
    for (size_t j = 0; j < 3; ++j) {
        if (glyphs[33 * row + 3 * glyph + j]) 
            attron(A_REVERSE);
        else
            attroff(A_REVERSE);
        printw("%s", block);
    }
    attroff(A_REVERSE);
    printw(" ");
}

void draw_stat(const char *buf) {
    printw("\n", buf);

    size_t len = strlen(buf) + 1; 
    for (size_t i = 0; i < 5; ++i) {
        pad_x(stat_pad);

        for (size_t k = 0; k < len - 1; ++k)
            draw_glyph(buf[k] - 0x30, i);

        draw_glyph(10, i);
        printw("\n");
    }
}

void draw_bat_middle(int power) {
    int off = power * bat_width / 100;

    for (int y = 0; y < bat_height; ++y) {
        pad_x(bat_pad);

        attron(A_REVERSE);
        printw("%s", block);
        for (int x = 0; x < bat_width; ++x) {
            if (x >= off) attroff(A_REVERSE);
            printw("%s", block);
        }

        attron(A_REVERSE);
        printw("%s\n", block);
        attroff(A_REVERSE);
    }
}

void draw_bat_edge(void) {
    pad_x(bat_pad);

    attron(A_REVERSE);
    for (int i = 0; i < bat_width + 1; ++i)
        printw("%s", block);
    printw("\n");
    attroff(A_REVERSE);
}

void update_pads(const char *buf) {
    bat_pad = (COLS - (strlen(block) * (bat_width + 2))) / 2;
    stat_pad = (COLS - ((strlen(buf) + 1) * 4 * strlen(block))) / 2;
    y_pad = (LINES - bat_height - 2) / 2;
    if (draw_percent) y_pad -= 3;
}

void set_colors(int power) {
    if (has_colors()) {
        if (bold)
            attron(A_BOLD);

        if (power <= low) {
            attron(COLOR_PAIR(2));
        } else {
            attron(COLOR_PAIR(1));
        }
    }
}

void pad_x(size_t amount) {
    printw("%*s", amount, "");
}

void pad_y(size_t amount) {
    for (size_t i = 0; i < amount; ++i)
        printw("\n");
}

int read_bat(char *buf) {
    FILE *bat = fopen(bat_file, "r");
    if (!bat) {
        fclose(bat);
        return EXIT_BAD_FILE;
    }

    size_t pos = 0;
    do {
        fread(buf + pos, 1, 1, bat);
        if (buf[pos] == '\n')
            break;
    }
    while (++pos < 4);

    buf[pos] = '\0';

    fclose(bat);
    return EXIT_OK;
}

void start_nc(void) {
    initscr();
    raw();
    use_default_colors();
    if (has_colors()) {
        start_color();
        init_pair(1, color, b_color);
        init_pair(2, color_low, b_color_low);
    }
    curs_set(0);
    noecho();
    timeout(tick);
}

void end_nc(void) {
    endwin();
    curs_set(1);
}

int parse_args(int argc, char **argv) {
    int c = '\0';
    while ((c = getopt(argc, argv, "hvNt:x:y:c:l:L:bB")) != -1) {
        switch (c) {
            case 'h':
                help();
                return EXIT_DONE;
            case 'v':
                version();
                return EXIT_DONE;
            case 'N':
                draw_percent = false;
                break;
            case 't':
                tick = strtol(optarg, NULL, 10);
                break;
            case 'x':
               bat_width = strtol(optarg, NULL, 10);
               break;
            case 'y':
               bat_height = strtol(optarg, NULL, 10);
               break;
            case 'c':
               color = strtol(optarg, NULL, 10);
               break;
            case 'l':
               color_low = strtol(optarg, NULL, 10);
               break;
            case 'L':
               low = strtol(optarg, NULL, 10);
               break;
            case 'b':
               blink = 0;
               break;
            case 'B':
               bold = true;
               break;
            case '?':
            case ':':
               return EXIT_BAD_ARGS;
            default:
                printf("unknown argument '%c'\n", c);
                return EXIT_BAD_ARGS;
        }
    }

    return EXIT_OK;
}

void help(void) {
    printf("usage: tty-bat [args]\n");
    printf("\n");
    printf("args:\n");
    printf("-b       \tblink the battery when battery level is below 10%%\n");
    printf("-B       \tuse bold colors\n");
    printf("-c       \tset the color\n");
    printf("-h       \tshow program usage\n");
    printf("-l       \tset the color used when battery level is below 10%%\n");
    printf("-L num   \tset the battery level considered 'low' (default 10)\n");
    printf("-N       \tdon't draw battery percentage\n");
    printf("-t RATE  \tset the update rate to RATE milliseconds\n");
    printf("-x width \tset the battery width to X lines\n");
    printf("-y height\tset the battery height to Y lines\n");
    printf("-v       \tshow program version information\n");
    printf("\n");
    printf("notes:\n");
    printf(" * -b will only blink if the tick rate is greater than\n");
    printf("      or equal to '500'.\n");
    printf("\n");
    printf(" * -c will also set the low battery color, if a custom one has\n");
    printf("      not been set with -l.\n");
}

void version(void) {
    printf("tty-bat v%s\n", vn);
    printf("(c) Phillip Heikoop 2016\n");
}
