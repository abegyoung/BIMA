#define _XOPEN_SOURCE 600
#include <sys/wait.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <libgen.h>
#include <stdio.h>
#include <fcntl.h>

#define MAX_PROCS 128
#define LIST_FILE "executables.list"

typedef struct {
    char name[64];
    char path[256];
    char exec[128];
    pid_t pid;
} Proc;

Proc procs[MAX_PROCS];
int nprocs = 0;

/* ---------- helpers ---------- */

int is_running(pid_t pid) {
    if (pid <= 0) return 0;
    return (kill(pid, 0) == 0);
}

void save_list() {
    FILE *f;
    int i;

    f = fopen(LIST_FILE, "w");
    if (!f) return;

    for (i = 0; i < nprocs; i++) {
        fprintf(f, "%s %s %d\n",
                procs[i].name,
                procs[i].path,
                (int)procs[i].pid);
    }

    fclose(f);
}

void kill_proc(Proc *p) {
    int i;

    if (p->pid <= 0) return;

    if (is_running(p->pid)) {
        kill(p->pid, SIGTERM);

        /* wait up to ~1 sec */
        for (i = 0; i < 10; i++) {
            if (!is_running(p->pid)) break;
            usleep(100000);
        }

        if (is_running(p->pid)) {
            kill(p->pid, SIGKILL);
        }
    }

    p->pid = -1;
}

void kill_all_procs() {
    int i;

    for (i = 0; i < nprocs; i++) {
        if (procs[i].pid > 0) {

            /* kill entire process group (important!) */
            kill(-procs[i].pid, SIGTERM);

            usleep(200000);

            if (is_running(procs[i].pid)) {
                kill(-procs[i].pid, SIGKILL);
            }
        }

        procs[i].pid = -1;
    }

    save_list();
}


void start_proc(Proc *p)
{
    pid_t pid;

    pid = fork();

    if (pid == 0) {
        int fd;
        char *prog;

        if (setsid() < 0)
            exit(1);

        fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO);
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);

            if (fd > 2)
                close(fd);
        }

        prog = strrchr(p->path, '/');
        if (prog)
            prog++;
        else
            prog = p->path;

        execl(p->path, prog, (char *)NULL);

        exit(1);
    }
    else if (pid > 0) {
        p->pid = pid;
    }
}

void restart_proc(int idx) {
    if (idx < 0 || idx >= nprocs) return;
    kill_proc(&procs[idx]);
    start_proc(&procs[idx]);
    save_list();
}

void kill_only_proc(int idx) {
    if (idx < 0 || idx >= nprocs) return;
    kill_proc(&procs[idx]);
    save_list();
}


void reap_children() {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0) {
        /* reaping all dead children */
    }
}

/* ---------- load ---------- */

void load_list(const char *filename) {
    FILE *f;
    int pid_tmp;

    f = fopen(filename, "r");
    if (!f) {
        perror("open list");
        exit(1);
    }

    while (nprocs < MAX_PROCS &&
           fscanf(f, "%63s %255s %d",
                  procs[nprocs].name,
                  procs[nprocs].path,
                  &pid_tmp) == 3) {

        procs[nprocs].pid = (pid_t)pid_tmp;

        strncpy(procs[nprocs].exec,
                basename(procs[nprocs].path),
                sizeof(procs[nprocs].exec));

        nprocs++;
    }

    fclose(f);
}

/* ---------- init ---------- */

void init_processes() {
    int i;

    for (i = 0; i < nprocs; i++) {

        if (procs[i].pid == -1) {
            /* Only start missing processes */
            start_proc(&procs[i]);
            save_list();
            sleep(1);
        }

        /* else: DO NOTHING — trust existing PID */
    }
}

/* ---------- UI ---------- */

void draw_ui() {
    int i, row;

    clear();
    mvprintw(0, 0, "#  Name        Executable        PID     Status");

    for (i = 0; i < nprocs; i++) {
        row = i + 2;

        mvprintw(row, 0, "%-2d %-10s %-16s %-7d",
                 i + 1,
                 procs[i].name,
                 procs[i].exec,
                 (int)procs[i].pid);

        if (is_running(procs[i].pid)) {
            attron(COLOR_PAIR(1));
            mvprintw(row, 45, "[ RUN ]");
            attroff(COLOR_PAIR(1));
        } else {
            attron(COLOR_PAIR(2));
            mvprintw(row, 45, "[ STOP ]");
            attroff(COLOR_PAIR(2));
        }
    }

    mvprintw(22, 0, "Commands: :r #  :k #  :K (kill all + exit)  q (quit)");

    refresh();
}

/* ---------- command mode ---------- */

void command_mode(int cmd_type) {
    int idx = -1;

    move(23, 0);
    clrtoeol();

    if (cmd_type == 'r')
        mvprintw(23, 0, ":r [restart process] ");
    else
        mvprintw(23, 0, ":k [kill process] ");

    echo();
    curs_set(1);

    scanw("%d", &idx);

    noecho();
    curs_set(0);

    if (idx >= 1 && idx <= nprocs) {
        if (cmd_type == 'r')
            restart_proc(idx - 1);
        else
            kill_only_proc(idx - 1);
    }
}

/* ---------- main ---------- */

int main() {
    int ch;
    int i;

    load_list(LIST_FILE);
    init_processes();

    initscr();
    start_color();
    use_default_colors();

    init_pair(1, COLOR_GREEN, -1);
    init_pair(2, COLOR_RED, -1);

    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);

    while (1) {
        reap_children();

        draw_ui();

        ch = getch();

        if (ch == ':') {
            nodelay(stdscr, FALSE);
        
            ch = getch();
        
            if (ch == 'r' || ch == 'k') {
                command_mode(ch);
            }
            else if (ch == 'K') {
                /* show confirmation line */
                move(23, 0);
                clrtoeol();
                mvprintw(23, 0, ":K [kill ALL and exit] (y/n): ");
                refresh();

                echo();
                ch = getch();
                noecho();

                if (ch == 'y' || ch == 'Y') {
                    endwin();
                    kill_all_procs();
                    exit(0);
                }
            }

            nodelay(stdscr, TRUE);
        }

        else if (ch == 'q') {
            break;
        }

        usleep(200000);
    }

    endwin();

    save_list();

    return 0;
}
