//AetherVortex by Sebastien MacDougall-Landry
//License is available at
//https://github.com/EmperorPenguin18/aethervortex/blob/main/LICENSE

#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>

#include <stdbool.h>
#include <algorithm>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
using namespace rapidjson;

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <scry/scry.h>

#include <chafa.h>
#include "img.h"

#include <git2.h>

//temp
const char* example_json =
"{\
	\"name\": \"Mechanism\",\
	\"format\": \"Commander\",\
	\"search\": \"legal:commander commander:ur otag:synergy-artifact-creature\",\
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

static int create_commit(git_repository *repo, git_index* index, const char* message) {
	// Create a tree from the index
	git_oid tree_id;
	int error = git_index_write_tree(&tree_id, index);
	if (error < 0) {
		fprintf(stderr, "Error creating tree from index\n");
		return -1;
	}
	git_tree* tree = NULL;
	error = git_tree_lookup(&tree, repo, &tree_id);
	if (error < 0) {
		fprintf(stderr, "Error looking up tree\n");
		return -1;
	}

	// Get the HEAD reference
	git_reference *head = NULL;
	error = git_repository_head(&head, repo);

	// Get the commit that HEAD points to
	git_commit *parent_commit = NULL;
	error = git_commit_lookup(&parent_commit, repo, git_reference_target(head));
	git_reference_free(head);

	const git_commit *parents[1];
	size_t parents_count = 0;
	if (parent_commit != NULL) {
		parents[0] = parent_commit;
		parents_count = 1;
	}

	// Prepare the commit
	git_signature *signature;
	git_signature_default(&signature, repo);

	git_oid commit_id;
	error = git_commit_create(&commit_id, repo, "HEAD", signature, signature, NULL, message, tree, 1, parents);
	if (error < 0) {
		fprintf(stderr, "Error creating commit\n");
		return -1;
	}

	// Free resources
	git_signature_free(signature);
	if (parent_commit != NULL) {
		git_commit_free(parent_commit);
	}
	git_commit_free(parent_commit);
	return 0;
}

class AetherVortex {
	public:
		AetherVortex(Document& doc, char* buffer, const char* git_path) : m_doc(doc), m_git_path(git_path) {
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
			m_scry = new Scry;
			getmaxyx(stdscr, m_max_y, m_max_x);
			m_win_height = m_max_y-5;
			m_win_info = newwin(m_max_y-2, m_max_x/5, 1, 0);
			m_win_list = newwin(m_max_y-2, (m_max_x*4)/5, 1, m_max_x/5);

			m_repo = NULL;
			git_libgit2_init();
			git_repository_init_options initopts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
			initopts.flags = GIT_REPOSITORY_INIT_MKPATH;
			git_repository_init_ext(&m_repo, git_path, &initopts);

			memset(m_keymaps_n, 0, sizeof(m_keymaps_n));
			m_keymaps_n['q'] = &AetherVortex::quit;
			m_keymaps_n['i'] = &AetherVortex::insert;
			m_keymaps_n['o'] = &AetherVortex::s_mode;
			m_keymaps_n['a'] = &AetherVortex::c_mode;
			m_keymaps_n['d'] = &AetherVortex::cut;
			m_keymaps_n['g'] = &AetherVortex::begin;
			m_keymaps_n['h'] = &AetherVortex::move_left;
			m_keymaps_n['j'] = &AetherVortex::move_down;
			m_keymaps_n['k'] = &AetherVortex::move_up;
			m_keymaps_n['l'] = &AetherVortex::move_right;
			m_keymaps_n['z'] = &AetherVortex::write;
			m_keymaps_n['x'] = &AetherVortex::consider;
			m_keymaps_n['c'] = &AetherVortex::change;
			m_keymaps_n[27] = &AetherVortex::n_mode; //Esc

			memset(m_keymaps_c, 0, sizeof(m_keymaps_c));
			m_keymaps_c['q'] = &AetherVortex::quit;
			m_keymaps_c['h'] = &AetherVortex::move_left;
			m_keymaps_c['j'] = &AetherVortex::move_down;
			m_keymaps_c['k'] = &AetherVortex::move_up;
			m_keymaps_c['l'] = &AetherVortex::move_right;
			m_keymaps_c[27] = &AetherVortex::n_mode; //Esc

			memset(m_keymaps_s, 0, sizeof(m_keymaps_s));
			m_keymaps_s['q'] = &AetherVortex::quit;
			m_keymaps_s['h'] = &AetherVortex::move_left;
			m_keymaps_s['j'] = &AetherVortex::move_down;
			m_keymaps_s['k'] = &AetherVortex::move_up;
			m_keymaps_s['l'] = &AetherVortex::move_right;
			m_keymaps_s[27] = &AetherVortex::n_mode; //Esc

			if (buffer) {
				lua_State* L = luaL_newstate();
				luaL_openlibs(L);
				//lua_register(L, "keymap", keymap_api);
				luaL_dostring(L, buffer);
				lua_close(L);
			}
		}
		~AetherVortex() {
			delwin(m_win_info);
			delwin(m_win_list);
			git_repository_free(m_repo);
			git_libgit2_shutdown();
			endwin();
			delete m_scry;
		}
		void run() {
			while (m_running) {
				clear();
				printw("Card info");
				mvprintw(0, m_max_x/5, (m_mode == CONSIDERING_MODE) ? "Considering" : (m_mode == SCRYFALL_MODE) ? "Scryfall search" : "Decklist");
				refresh();
				Document doc;
				const Value& a = (m_mode == CONSIDERING_MODE) ? m_doc["considering"] : (m_mode == SCRYFALL_MODE) ? get_list(doc) : m_doc["cards"];
				assert(a.IsArray());
				//draw
				wclear(m_win_list);
				box(m_win_list, 0, 0);
				wclear(m_win_info);
				box(m_win_info, 0, 0);
				size_t max_str_len = 0;
				m_rows = 0;
				m_columns = 0;
				int col = 0;
				for (auto& card : a.GetArray()) {
					if (m_rows > m_win_height) { //Start a new column
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
				wrefresh(m_win_info);
				//events
				m_listpos = ((m_win_height+1)*m_pos_x) + m_pos_y;
				char ch = getch();
				if (m_mode == NORMAL_MODE) {
					if (m_keymaps_n[ch] != NULL)
						(this->*m_keymaps_n[ch])();
				} else if (m_mode == CONSIDERING_MODE) {
					if (m_keymaps_c[ch] != NULL)
						(this->*m_keymaps_c[ch])();
				} else if (m_mode == SCRYFALL_MODE) {
					if (m_keymaps_s[ch] != NULL)
						(this->*m_keymaps_s[ch])();
				}
			}
		}
		int keymap_api(lua_State* L) {
			char c = luaL_checknumber(L, 1);
			const char* str = luaL_checkstring(L, 2);
			if (strcmp(str, "quit") == 0) {
				m_keymaps_n[c] = &AetherVortex::quit;
			} else if (strcmp(str, "consider") == 0) {
				m_keymaps_n[c] = &AetherVortex::consider;
			} else if (strcmp(str, "insert") == 0) {
				m_keymaps_n[c] = &AetherVortex::insert;
			} else if (strcmp(str, "cut") == 0) {
				m_keymaps_n[c] = &AetherVortex::cut;
			} else if (strcmp(str, "c_mode") == 0) {
				m_keymaps_n[c] = &AetherVortex::c_mode;
			} else if (strcmp(str, "s_mode") == 0) {
				m_keymaps_n[c] = &AetherVortex::s_mode;
			} else if (strcmp(str, "n_mode") == 0) {
				m_keymaps_n[c] = &AetherVortex::n_mode;
			} else if (strcmp(str, "change") == 0) {
				m_keymaps_n[c] = &AetherVortex::change;
			} else if (strcmp(str, "begin") == 0) {
				m_keymaps_n[c] = &AetherVortex::begin;
			} else if (strcmp(str, "move_left") == 0) {
				m_keymaps_n[c] = &AetherVortex::move_left;
			} else if (strcmp(str, "move_down") == 0) {
				m_keymaps_n[c] = &AetherVortex::move_down;
			} else if (strcmp(str, "move_up") == 0) {
				m_keymaps_n[c] = &AetherVortex::move_up;
			} else if (strcmp(str, "move_right") == 0) {
				m_keymaps_n[c] = &AetherVortex::move_right;
			} else if (strcmp(str, "write") == 0) {
				m_keymaps_n[c] = &AetherVortex::write;
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
		void (AetherVortex::*m_keymaps_n[255])();
		void (AetherVortex::*m_keymaps_c[255])();
		void (AetherVortex::*m_keymaps_s[255])();
		int m_listpos;
		int m_max_x;
		int m_max_y;
		Scry* m_scry;
		WINDOW* m_win_info;
		WINDOW* m_win_list;
		int m_win_height;
		git_repository* m_repo;
		std::string m_git_path;

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

			gint width_cells = m_max_x/5, height_cells = m_max_y;
			chafa_calc_canvas_geometry(480, 680, &width_cells, &height_cells, 0.5, FALSE, FALSE);
			ChafaCanvas* canvas = create_canvas(height_cells, width_cells-1);
			chafa_canvas_draw_all_pixels(canvas, CHAFA_PIXEL_RGBA8_UNASSOCIATED, pixels, 480, 680, 480*4);

			canvas_to_ncurses(m_win_info, canvas, 1, 1, height_cells, width_cells-1);
			chafa_canvas_unref(canvas);
			free(pixels);
		}

		Value& get_list(Document& doc) {
			const Value& search = m_doc["search"];
			assert(search.IsString());
			const char* str = search.GetString();
			List* list = m_scry->cards_search_cache(str);
			doc.Parse(list->json().c_str());
			return doc["data"];
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
					m_pos_y = m_win_height;
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
					m_pos_y = m_win_height;
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
			begin();
		}
		void s_mode() {
			m_mode = SCRYFALL_MODE;
			begin();
		}
		void n_mode() {
			m_mode = NORMAL_MODE;
			begin();
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
				if (m_pos_y < m_win_height) m_pos_y++;
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
			StringBuffer buffer;
			buffer.Clear();
			PrettyWriter<StringBuffer> writer(buffer);
			m_doc.Accept(writer);

			const Value& name = m_doc["name"];
			assert(name.IsString());
			const char* str = name.GetString();
			char* filename = new char[strlen(str)+6];
			sprintf(filename, "%s.json", str);

			char* filepath = new char[m_git_path.size()+strlen(filename)+2];
			sprintf(filepath, "%s/%s", m_git_path.c_str(), filename);
			std::ofstream ofs(filepath, std::ofstream::out);
			delete[] filepath;

			ofs << buffer.GetString() << std::endl;
			ofs.close();

			git_index *idx = NULL;
			int error = git_repository_index(&idx, m_repo);
			error = git_index_add_bypath(idx, filename);
			git_index_write(idx);
			char* message = new char[strlen(filename)+12];
			sprintf(message, "Changes to %s", filename);
			error = create_commit(m_repo, idx, message);
			delete[] message;
			git_index_free(idx);
			delete[] filename;
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
	char* config_path = new char[strlen(home)+23];
	char* git_path = new char[strlen(home)+17];
	sprintf(config_path, "%s/.config/av/config.lua", home);
	sprintf(git_path, "%s/.local/share/av", home);
	char* buffer = read_file(config_path);
	AetherVortex av(doc, buffer, git_path);
	delete[] buffer;
	delete[] config_path;
	delete[] git_path;
	av.run();
	return 0;
}
