#define _XOPEN_SOURCE_EXTENDED

#include <stdbool.h>
#include <ncurses.h>

class AetherVortex {
	public:
		AetherVortex() {
			initscr();
			noecho();
			keypad(stdscr, TRUE);
			curs_set(0);
			m_running = true;
			m_pos_y = 0;
			m_pos_x = 0;
			m_menu_num = 3;
		}
		~AetherVortex() {
			endwin();
		}
		void run() {
			while (m_running) {
				draw();
				events();
			}
		}
	private:
		bool m_running;
		int m_pos_y;
		int m_pos_x;
		size_t m_menu_num;
		void events() {
			char c = getch();
			switch (c) {
				case 'q':
					m_running = false;
					break;
				case 'j':
					if (m_pos_y+1 != m_menu_num) m_pos_y++;
					break;
				case 'k':
					if (m_pos_y != 0) m_pos_y -= 1;
					break;
				default:
					break;
			}
		}
		void draw() {
			clear();
			if (m_pos_y == 0) attron(A_STANDOUT);
			else attroff(A_STANDOUT);
			printw("Option 1\n");
			if (m_pos_y == 1) attron(A_STANDOUT);
			else attroff(A_STANDOUT);
			printw("Option 2\n");
			if (m_pos_y == 2) attron(A_STANDOUT);
			else attroff(A_STANDOUT);
			printw("Option 3");
			refresh();
		}
};

int main(int argc, char** argv) {
	AetherVortex av;
	av.run();
	return 0;
}
