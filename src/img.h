#include <locale.h>
#include <ncurses.h>
#include <chafa.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>

typedef struct
{
    gint width_cells, height_cells;
    gint width_pixels, height_pixels;
}
TermSize;

void get_tty_size(gint* width_cells, gint* height_cells, gint* cell_width, gint* cell_height, gfloat* font_ratio) {
    TermSize term_size;
    *font_ratio = 0.5;
    *cell_width = -1, *cell_height = -1;  /* Size of each character cell, in pixels */

    term_size.width_cells
        = term_size.height_cells
        = term_size.width_pixels
        = term_size.height_pixels
        = -1;

    struct winsize w;
    gboolean have_winsz = FALSE;

    if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &w) >= 0
        || ioctl (STDERR_FILENO, TIOCGWINSZ, &w) >= 0
        || ioctl (STDIN_FILENO, TIOCGWINSZ, &w) >= 0)
        have_winsz = TRUE;

    if (have_winsz)
    {
        term_size.width_cells = w.ws_col;
        term_size.height_cells = w.ws_row;
        term_size.width_pixels = w.ws_xpixel;
        term_size.height_pixels = w.ws_ypixel;
    }

    if (term_size.width_cells <= 0)
        term_size.width_cells = -1;
    if (term_size.height_cells <= 2)
        term_size.height_cells = -1;

    /* If .ws_xpixel and .ws_ypixel are filled out, we can calculate
     * aspect information for the font used. Sixel-capable terminals
     * like mlterm set these fields, but most others do not. */

    if (term_size.width_pixels <= 0 || term_size.height_pixels <= 0)
    {
        term_size.width_pixels = -1;
        term_size.height_pixels = -1;
    }

    if (term_size.width_cells > 0
        && term_size.height_cells > 0
        && term_size.width_pixels > 0
        && term_size.height_pixels > 0)
    {
        *cell_width = term_size.width_pixels / term_size.width_cells;
        *cell_height = term_size.height_pixels / term_size.height_cells;
        *font_ratio = (gdouble) *cell_width / (gdouble) *cell_height;
    }

    *width_cells = term_size.width_cells;
    *height_cells = term_size.height_cells;
}

void detect_terminal(ChafaTermInfo **term_info_out, ChafaCanvasMode *mode_out, ChafaPixelMode *pixel_mode_out) {
    ChafaCanvasMode mode;
    ChafaPixelMode pixel_mode;
    ChafaTermInfo *term_info;
    ChafaTermInfo *fallback_info;
    gchar **envp;

    /* Examine the environment variables and guess what the terminal can do */

    envp = g_get_environ ();
    term_info = chafa_term_db_detect (chafa_term_db_get_default (), envp);

    /* See which control sequences were defined, and use that to pick the most
     * high-quality rendering possible */

    if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_BEGIN_KITTY_IMMEDIATE_IMAGE_V1))
    {
        pixel_mode = CHAFA_PIXEL_MODE_KITTY;
        mode = CHAFA_CANVAS_MODE_TRUECOLOR;
    }
    else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_BEGIN_SIXELS))
    {
        pixel_mode = CHAFA_PIXEL_MODE_SIXELS;
        mode = CHAFA_CANVAS_MODE_TRUECOLOR;
    }
    else
    {
        pixel_mode = CHAFA_PIXEL_MODE_SYMBOLS;

        if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FGBG_DIRECT)
            && chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FG_DIRECT)
            && chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_BG_DIRECT))
            mode = CHAFA_CANVAS_MODE_TRUECOLOR;
        else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FGBG_256)
                 && chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FG_256)
                 && chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_BG_256))
            mode = CHAFA_CANVAS_MODE_INDEXED_240;
        else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FGBG_16)
                 && chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_FG_16)
                 && chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_SET_COLOR_BG_16))
            mode = CHAFA_CANVAS_MODE_INDEXED_16;
        else if (chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_INVERT_COLORS)
                 && chafa_term_info_have_seq (term_info, CHAFA_TERM_SEQ_RESET_ATTRIBUTES))
            mode = CHAFA_CANVAS_MODE_FGBG_BGFG;
        else
            mode = CHAFA_CANVAS_MODE_FGBG;
    }

    /* Hand over the information to caller */

    *term_info_out = term_info;
    *mode_out = mode;
    *pixel_mode_out = pixel_mode;

    /* Cleanup */

    g_strfreev (envp);
}

ChafaCanvas* create_canvas (const gint width_cells, const gint height_cells, const gint cell_width, const gint cell_height,
		ChafaTermInfo* term_info, ChafaCanvasMode mode, ChafaPixelMode pixel_mode) {
    /* Specify the symbols we want */

    ChafaSymbolMap* symbol_map = chafa_symbol_map_new ();
    chafa_symbol_map_add_by_tags (symbol_map, CHAFA_SYMBOL_TAG_BLOCK);

    /* Set up a configuration with the symbols and the canvas size in characters */

    ChafaCanvasConfig* config = chafa_canvas_config_new ();
    chafa_canvas_config_set_canvas_mode (config, mode);
    chafa_canvas_config_set_pixel_mode (config, pixel_mode);
    chafa_canvas_config_set_geometry (config, width_cells, height_cells);
    chafa_canvas_config_set_symbol_map (config, symbol_map);

    if (cell_width > 0 && cell_height > 0)
    {
        /* We know the pixel dimensions of each cell. Store it in the config. */

        chafa_canvas_config_set_cell_geometry (config, cell_width, cell_height);
    }

    /* Create canvas */

    ChafaCanvas* canvas = chafa_canvas_new (config);

    chafa_symbol_map_unref (symbol_map);
    chafa_canvas_config_unref (config);
    return canvas;
}

void canvas_to_ncurses(WINDOW* win, ChafaCanvas *canvas, int start_y, int start_x, int screen_height, int screen_width, ChafaCanvasMode mode) {
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
