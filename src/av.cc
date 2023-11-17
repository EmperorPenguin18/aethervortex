//AetherVortex by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/aethervortex/blob/main/LICENSE

#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>

#include <stdbool.h>
#include <algorithm>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
using namespace rapidjson;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <scry/scry.h>

#include <chafa.h>

#include "img.h"

//temp
const char* example_json =
"{\
	\"name\": \"G/W Humans\",\
	\"format\": \"Commander\",\
	\"search\": \"set:lci\",\
	\"considering\": [],\
	\"cards\": [\
		{\
			\"name\": \"Brudiclad, Telchor Engineer\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Steel Overseer\"\
		},\
		{\
			\"name\": \"Brudiclad, Telchor Engineer\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Steel Overseer\"\
		},\
		{\
			\"name\": \"Brudiclad, Telchor Engineer\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Sai, Master Thopterist\"\
		},\
		{\
			\"name\": \"Steel Overseer\"\
		}\
	],\
	\"cuts\": []\
}";

enum Mode {
	NORMAL_MODE,
	CONSIDERING_MODE,
	SCRYFALL_MODE,
	NUM_MODES,
	INVALID_MODE
};

class AetherVortex {
	public:
		AetherVortex(Document& doc, char* buffer) : m_doc(doc) {
			initscr();
			start_color();
			use_default_colors();
			noecho();
			curs_set(0);
			keypad(stdscr, TRUE);

			m_running = true;
			m_pos_y = 0;
			m_pos_x = 0;
			m_column_gap = 5;
			m_mode = NORMAL_MODE;
			memset(m_keymaps, 0, sizeof(m_keymaps));
			m_keymaps['q'] = &AetherVortex::quit;
			m_keymaps['i'] = &AetherVortex::insert;
			m_keymaps['o'] = &AetherVortex::s_mode;
			m_keymaps['a'] = &AetherVortex::c_mode;
			m_keymaps['d'] = &AetherVortex::cut;
			m_keymaps['g'] = &AetherVortex::begin;
			m_keymaps['h'] = &AetherVortex::move_left;
			m_keymaps['j'] = &AetherVortex::move_down;
			m_keymaps['k'] = &AetherVortex::move_up;
			m_keymaps['l'] = &AetherVortex::move_right;
			m_keymaps['z'] = &AetherVortex::write;
			m_keymaps['x'] = &AetherVortex::consider;
			m_keymaps['c'] = &AetherVortex::change;
			m_keymaps[27] = &AetherVortex::n_mode; //Esc
			if (buffer) {
				lua_State* L = luaL_newstate();
				luaL_openlibs(L);
				//lua_register(L, "keymap", keymap_api);
				luaL_dostring(L, buffer);
				lua_close(L);
			}
			m_scry = new Scry;
			getmaxyx(stdscr, m_max_y, m_max_x);
			m_win_info = newwin(m_max_y, m_max_x/5, 0, 0);
			m_win_list = newwin(m_max_y, (m_max_x*4)/5, 0, m_max_x/5);
			refresh();
		}
		~AetherVortex() {
			delwin(m_win_info);
			delwin(m_win_list);
			endwin();
			delete m_scry;
		}
		void run() {
			while (m_running) {
				m_listpos = (m_max_y*m_pos_x) + m_pos_y;
				const Value& a = m_doc["cards"];
				assert(a.IsArray());
				//draw
				wclear(m_win_list);
				box(m_win_list, 0, 0);
				size_t max_str_len = 0;
				m_rows = 0;
				m_columns = 0;
				int col = 0;
				for (auto& card : a.GetArray()) {
					if (m_rows+2 > m_max_y-1) { //Start a new column
						m_rows = 0;
						col += max_str_len+m_column_gap;
						max_str_len = 0;
						m_columns++;
					}
					assert(card.IsObject());
					const Value& name = card["name"];
					assert(name.IsString());
					const char* str = name.GetString();
					if (m_pos_y == m_rows && m_pos_x == m_columns) {
						set_image(str);
						wattron(m_win_list, A_STANDOUT);
					}
					mvwaddstr(m_win_list, m_rows+1, col+1, str);
					wattroff(m_win_list, A_STANDOUT);
					m_rows++;
					size_t str_len = strlen(str);
					if (str_len > max_str_len) max_str_len = str_len;
				}
				m_rows -= 1;
				wrefresh(m_win_list);
				//events
				char c = getch();
				if (m_keymaps[c] != NULL)
					(this->*m_keymaps[c])();
			}
		}
		int keymap_api(lua_State* L) {
			char c = luaL_checknumber(L, 1);
			const char* str = luaL_checkstring(L, 2);
			if (strcmp(str, "quit") == 0) {
				m_keymaps[c] = &AetherVortex::quit;
			} else if (strcmp(str, "consider") == 0) {
				m_keymaps[c] = &AetherVortex::consider;
			} else if (strcmp(str, "insert") == 0) {
				m_keymaps[c] = &AetherVortex::insert;
			} else if (strcmp(str, "cut") == 0) {
				m_keymaps[c] = &AetherVortex::cut;
			} else if (strcmp(str, "c_mode") == 0) {
				m_keymaps[c] = &AetherVortex::c_mode;
			} else if (strcmp(str, "s_mode") == 0) {
				m_keymaps[c] = &AetherVortex::s_mode;
			} else if (strcmp(str, "n_mode") == 0) {
				m_keymaps[c] = &AetherVortex::n_mode;
			} else if (strcmp(str, "change") == 0) {
				m_keymaps[c] = &AetherVortex::change;
			} else if (strcmp(str, "begin") == 0) {
				m_keymaps[c] = &AetherVortex::begin;
			} else if (strcmp(str, "move_left") == 0) {
				m_keymaps[c] = &AetherVortex::move_left;
			} else if (strcmp(str, "move_down") == 0) {
				m_keymaps[c] = &AetherVortex::move_down;
			} else if (strcmp(str, "move_up") == 0) {
				m_keymaps[c] = &AetherVortex::move_up;
			} else if (strcmp(str, "move_right") == 0) {
				m_keymaps[c] = &AetherVortex::move_right;
			} else if (strcmp(str, "write") == 0) {
				m_keymaps[c] = &AetherVortex::write;
			}
			return 0;
		}
	private:
		bool m_running;
		int m_pos_y;
		int m_pos_x;
		Document& m_doc;
		int m_rows;
		int m_columns;
		int m_column_gap;
		Mode m_mode;
		void (AetherVortex::*m_keymaps[255])();
		int m_listpos;
		int m_max_x;
		int m_max_y;
		Scry* m_scry;
		WINDOW* m_win_info;
		WINDOW* m_win_list;

		void set_image(const char* name) {
			size_t img_size = 0;
			byte* image = m_scry->cards_named_cache(name, &img_size);

			FILE* fp = fopen("/tmp/av.jpg", "wb");
			fwrite((FILE*)image, sizeof(byte), img_size/sizeof(byte), fp);
			fclose(fp);
			system("convert /tmp/av.jpg RGBA:/tmp/av.raw");
			unsigned char* pixels = (unsigned char*)malloc(sizeof(unsigned char)*480*680*4);
			fp = fopen("/tmp/av.raw", "rb");
			fread(pixels, sizeof(unsigned char), 480*680*4, fp);
			fclose(fp);

			wclear(m_win_info);
			box(m_win_info, 0, 0);

			gint width_cells, height_cells, cell_width, cell_height;
			gfloat font_ratio;
			get_tty_size(&width_cells, &height_cells, &cell_width, &cell_height, &font_ratio);
			chafa_calc_canvas_geometry(480, 680, &width_cells, &height_cells, font_ratio, TRUE, FALSE);

			ChafaTermInfo* term_info;
			ChafaCanvasMode mode;
			ChafaPixelMode pixel_mode;
			detect_terminal(&term_info, &mode, &pixel_mode);
			ChafaCanvas* canvas = create_canvas(width_cells, height_cells, cell_width, cell_height, term_info, mode, pixel_mode);
			chafa_canvas_draw_all_pixels(canvas, CHAFA_PIXEL_RGBA8_UNASSOCIATED, pixels, 480, 680, 480*4);

			//canvas_to_ncurses(m_win_info, canvas, 1, 1, height_cells, width_cells, mode);
			//
			GString* printable = chafa_canvas_print(canvas, term_info);
			waddwstr(m_win_info, (wchar_t*)printable->str);
			g_string_free(printable, TRUE);
			//
			chafa_canvas_unref(canvas);
			chafa_term_info_unref(term_info);

			wrefresh(m_win_info);
			free(pixels);
		}

		void quit() {
			m_running = false;
		}
		void consider() {
			if (m_doc["cards"].Size() > 0) {
				m_doc["considering"].PushBack(m_doc["cards"][m_listpos], m_doc.GetAllocator());
				m_doc["cards"].Erase(m_doc["cards"].Begin()+m_listpos);
				m_pos_y -= 1;
				if (m_pos_y < 0) {
					m_pos_y = m_max_y-3;
					m_pos_x -= 1;
					if (m_pos_x < 0) {
						m_pos_y = 0;
						m_pos_x = 0;
					}
				}
			}
		}
		void insert() {
			consider();
			c_mode();
		}
		void cut() {
			if (m_doc["cards"].Size() > 0) {
				m_doc["cuts"].PushBack(m_doc["cards"][m_listpos], m_doc.GetAllocator());
				m_doc["cards"].Erase(m_doc["cards"].Begin()+m_listpos);
				m_pos_y -= 1;
				if (m_pos_y < 0) {
					m_pos_y = m_max_y-3;
					m_pos_x -= 1;
					if (m_pos_x < 0) {
						m_pos_y = 0;
						m_pos_x = 0;
					}
				}
			}
		}
		void c_mode() {
			m_mode = CONSIDERING_MODE;
		}
		void s_mode() {
			m_mode = SCRYFALL_MODE;
		}
		void n_mode() {
			m_mode = NORMAL_MODE;
		}
		void change() {
			consider();
			s_mode();
		}
		void begin() {
			m_pos_y = 0;
			m_pos_x = 0;
		}
		void move_left() {
			if (m_pos_x > 0) m_pos_x -= 1;
		}
		void move_down() {
			if (m_pos_x == m_columns) {
				if (m_pos_y < m_rows) m_pos_y++;
			} else {
				if (m_pos_y+2 < m_max_y-1) m_pos_y++;
			}
		}
		void move_up() {
			if (m_pos_y > 0) m_pos_y -= 1;
		}
		void move_right() {
			if (m_pos_x < m_columns) {
				m_pos_x++;
				if (m_pos_y > m_rows) m_pos_y = m_rows;
			}
		}
		void write() {
			//temp
			StringBuffer buffer;
			buffer.Clear();
			Writer<StringBuffer> writer(buffer);
			m_doc.Accept(writer);
			fprintf(stderr, "%s", buffer.GetString());
		}
};

static char* read_file(const char* name) {
	std::ifstream is(name, std::ifstream::binary);
	if (is) {
		is.seekg (0, is.end);
		int length = is.tellg();
		is.seekg (0, is.beg);
		char* buffer = new char[length+1];
		is.read(buffer, length);
		is.close();
		buffer[length] = '\0';
		return buffer;
	}
	return NULL;
}

int main(int argc, char** argv) {
	Document doc;
	doc.Parse(example_json); //temp
	const char* home = getenv("HOME");
	char* path = new char[strlen(home)+23];
	sprintf(path, "%s/.config/av/config.lua", home);
	char* buffer = read_file(path);
	AetherVortex av(doc, buffer);
	delete[] buffer;
	delete[] path;
	av.run();
	return 0;
}
