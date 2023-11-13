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
			noecho();
			keypad(stdscr, TRUE);
			curs_set(0);
			m_running = true;
			m_pos_y = 0;
			m_pos_x = 0;
			m_column_gap = 10;
			m_mode = NORMAL_MODE;
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
		}
		~AetherVortex() {
			endwin();
		}
		void run() {
			while (m_running) {
				getmaxyx(stdscr, m_max_y, m_max_x);
				const Value& a = m_doc["cards"];
				assert(a.IsArray());
				//draw
				clear();
				size_t max_str_len = 0;
				m_rows = 0;
				m_columns = 0;
				int col = 0;
				for (auto& card : a.GetArray()) {
					if (m_rows > m_max_y-1) { //Start a new column
						m_rows = 0;
						col += max_str_len+m_column_gap;
						max_str_len = 0;
						m_columns++;
					}
					if (m_pos_y == m_rows && m_pos_x == m_columns) attron(A_STANDOUT);
					else attroff(A_STANDOUT);
					assert(card.IsObject());
					const Value& name = card["name"];
					assert(name.IsString());
					const char* str = name.GetString();
					mvaddstr(m_rows, col, str);
					m_rows++;
					size_t str_len = strlen(str);
					if (str_len > max_str_len) max_str_len = str_len;
				}
				m_rows -= 1;
				refresh();
				//events
				m_listpos = (m_max_y*m_pos_x) + m_pos_y;
				(this->*m_keymaps[getch()])();
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

		void quit() {
			m_running = false;
		}
		void consider() {
			if (m_doc["cards"].Size() > 0) {
				m_doc["considering"].PushBack(m_doc["cards"][m_listpos], m_doc.GetAllocator());
				m_doc["cards"].Erase(m_doc["cards"].Begin()+m_listpos);
				m_pos_y -= 1;
				if (m_pos_y < 0) {
					m_pos_y = m_max_y-1;
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
					m_pos_y = m_max_y-1;
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
				if (m_pos_y < m_max_y-1) m_pos_y++;
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
