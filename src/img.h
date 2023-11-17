/* Example program that shows how to use a Chafa canvas with ncurses.
 *
 * To build:
 *
 * gcc ncurses.c $(pkg-config --libs --cflags chafa) $(ncursesw6-config --libs --cflags) -o ncurses
 */

#include <locale.h>
#include <ncurses.h>
#include <chafa.h>

ChafaCanvasMode detect_canvas_mode (void) {
    /* COLORS is a global variable defined by ncurses. It depends on termcap
     * for the terminal specified in TERM. In order to test the various modes, you
     * could try running this program with either of these:
     *
     * TERM=xterm
     * TERM=xterm-16color
     * TERM=xterm-256color
     * TERM=xterm-direct
     */
    return COLORS >= (1 << 24) ? CHAFA_CANVAS_MODE_TRUECOLOR
        : COLORS >= (1 << 8) ? CHAFA_CANVAS_MODE_INDEXED_240
        : COLORS >= (1 << 4) ? CHAFA_CANVAS_MODE_INDEXED_16
        : COLORS >= (1 << 3) ? CHAFA_CANVAS_MODE_INDEXED_8
        : CHAFA_CANVAS_MODE_FGBG;
}

ChafaCanvas* create_canvas (int screen_height, int screen_width) {
    ChafaSymbolMap *symbol_map;
    ChafaCanvasConfig *config;
    ChafaCanvas *canvas;
    ChafaCanvasMode mode = detect_canvas_mode ();

    /* Specify the symbols we want: Box drawing and block elements are both
     * useful and widely supported. */

    symbol_map = chafa_symbol_map_new ();
    chafa_symbol_map_add_by_tags (symbol_map, CHAFA_SYMBOL_TAG_SPACE);
    chafa_symbol_map_add_by_tags (symbol_map, CHAFA_SYMBOL_TAG_BLOCK);
    chafa_symbol_map_add_by_tags (symbol_map, CHAFA_SYMBOL_TAG_BORDER);

    /* Set up a configuration with the symbols and the canvas size in characters */

    config = chafa_canvas_config_new ();
    chafa_canvas_config_set_canvas_mode (config, mode);
    chafa_canvas_config_set_symbol_map (config, symbol_map);

    /* Reserve one row below canvas for status text */
    chafa_canvas_config_set_geometry (config, screen_width, screen_height);

    /* Apply tweaks for low-color modes */

    if (mode == CHAFA_CANVAS_MODE_INDEXED_240)
    {
        /* We get better color fidelity using DIN99d in 240-color mode.
         * This is not needed in 16-color mode because it uses an extra
         * preprocessing step instead, which usually performs better. */
        chafa_canvas_config_set_color_space (config, CHAFA_COLOR_SPACE_DIN99D);
    }

    if (mode == CHAFA_CANVAS_MODE_FGBG)
    {
        /* Enable dithering in monochromatic mode so gradients become
         * somewhat legible. */
        chafa_canvas_config_set_dither_mode (config, CHAFA_DITHER_MODE_ORDERED);
    }

    /* Create canvas */

    canvas = chafa_canvas_new (config);

    chafa_symbol_map_unref (symbol_map);
    chafa_canvas_config_unref (config);
    return canvas;
}

void canvas_to_ncurses (WINDOW* win, ChafaCanvas *canvas, int start_y, int start_x, int screen_height, int screen_width) {
    ChafaCanvasMode mode = detect_canvas_mode ();
    int pair = 256;  /* Reserve lower pairs for application in direct-color mode */
    int x, y;

    for (y = start_y; y < screen_height; y++)
    {
        for (x = start_x; x < screen_width; x++)
        {
            wchar_t wc [2];
            cchar_t cch;
            int fg, bg;

            /* wchar_t is 32-bit in glibc, but this may not work on e.g. Windows */
            wc [0] = chafa_canvas_get_char_at (canvas, x, y);
            wc [1] = 0;

            if (mode == CHAFA_CANVAS_MODE_TRUECOLOR)
            {
                chafa_canvas_get_colors_at (canvas, x, y, &fg, &bg);
                init_extended_pair (pair, fg, bg);
            }
            else if (mode == CHAFA_CANVAS_MODE_FGBG)
            {
                pair = 0;
            }
            else
            {
                /* In indexed color mode, we've probably got enough pairs
                 * to just let ncurses allocate and reuse as needed. */
                chafa_canvas_get_raw_colors_at (canvas, x, y, &fg, &bg);
                pair = alloc_pair (fg, bg);
            }

            setcchar (&cch, wc, A_NORMAL, -1, &pair);
            mvwadd_wch (win, y, x, &cch);
            pair++;
        }
    }
}
