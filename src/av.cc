#define _XOPEN_SOURCE_EXTENDED
#include <ncurses.h>

#include <stdbool.h>
#include <algorithm>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
using namespace rapidjson;

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
		AetherVortex(Document& doc) : m_doc(doc) {
			initscr();
			noecho();
			keypad(stdscr, TRUE);
			curs_set(0);
			m_running = true;
			m_pos_y = 0;
			m_pos_x = 0;
			m_column_gap = 10;
			m_mode = NORMAL_MODE;
		}
		~AetherVortex() {
			endwin();
		}
		void run() {
			while (m_running) {
				int max_y, max_x;
				getmaxyx(stdscr, max_y, max_x);
				const Value& a = m_doc["cards"];
				assert(a.IsArray());
				draw(a, max_y, max_x);
				events(max_y, max_x);
			}
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

		void events(const int max_y, const int max_x) {
			char c = getch();
			int i = (max_y*m_pos_x) + m_pos_y;
			switch (c) {
				case 'q':
					{
					m_running = false;
					break;
					}
				case 'i':
					{
					if (m_doc["cards"].Size() > 0) {
						m_doc["considering"].PushBack(m_doc["cards"][i], m_doc.GetAllocator());
						m_doc["cards"].Erase(m_doc["cards"].Begin()+i);
						m_pos_y -= 1;
						if (m_pos_y < 0) {
							m_pos_y = max_y-1;
							m_pos_x -= 1;
						}
						m_mode = CONSIDERING_MODE;
					}
					break;
					}
				case 'o':
					{
					m_mode = SCRYFALL_MODE;
					break;
					}
				case 'a':
					{
					m_mode = CONSIDERING_MODE;
					break;
					}
				case 'd':
					{
					if (m_doc["cards"].Size() > 0) {
						m_doc["cuts"].PushBack(m_doc["cards"][i], m_doc.GetAllocator());
						m_doc["cards"].Erase(m_doc["cards"].Begin()+i);
						m_pos_y -= 1;
						if (m_pos_y < 0) {
							m_pos_y = max_y-1;
							m_pos_x -= 1;
						}
					}
					break;
					}
				case 'g':
					{
					m_pos_y = 0;
					m_pos_x = 0;
					break;
					}
				case 'h':
					{
					if (m_pos_x > 0) m_pos_x -= 1;
					break;
					}
				case 'j':
					{
					if (m_pos_x == m_columns) {
						if (m_pos_y < m_rows) m_pos_y++;
					} else {
						if (m_pos_y < max_y-1) m_pos_y++;
					}
					break;
					}
				case 'k':
					{
					if (m_pos_y > 0) m_pos_y -= 1;
					break;
					}
				case 'l':
					{
					if (m_pos_x < m_columns) {
						m_pos_x++;
						if (m_pos_y > m_rows) m_pos_y = m_rows;
					}
					break;
					}
				case 'z':
					{
					//temp
					StringBuffer buffer;
					buffer.Clear();
					Writer<StringBuffer> writer(buffer);
					m_doc.Accept(writer);
					fprintf(stderr, "%s", buffer.GetString());
					break;
					}
				case 'x':
					{
					if (m_doc["cards"].Size() > 0) {
						m_doc["considering"].PushBack(m_doc["cards"][i], m_doc.GetAllocator());
						m_doc["cards"].Erase(m_doc["cards"].Begin()+i);
						m_pos_y -= 1;
						if (m_pos_y < 0) {
							m_pos_y = max_y-1;
							m_pos_x -= 1;
						}
					}
					break;
					}
				case 'c':
					{
					if (m_doc["cards"].Size() > 0) {
						m_doc["considering"].PushBack(m_doc["cards"][i], m_doc.GetAllocator());
						m_doc["cards"].Erase(m_doc["cards"].Begin()+i);
						m_pos_y -= 1;
						if (m_pos_y < 0) {
							m_pos_y = max_y-1;
							m_pos_x -= 1;
						}
						m_mode = SCRYFALL_MODE;
					}
					break;
					}
				case 27: //Esc
					{
					m_mode = NORMAL_MODE;
					break;
					}
				default:
					break;
			}
		}

		void draw(const Value& a, const int max_y, const int max_x) {
			clear();
			size_t max_str_len = 0;
			m_rows = 0;
			m_columns = 0;
			int col = 0;
			for (auto& card : a.GetArray()) {
				if (m_rows > max_y-1) { //Start a new column
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
		}
};

int main(int argc, char** argv) {
	Document doc;
	doc.Parse(example_json); //temp
	AetherVortex av(doc);
	av.run();
	return 0;
}
