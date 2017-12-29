#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);

/* We use the `readline' library to provide more flexibility to read from stdin. */
char* rl_gets() {
    static char *line_read = NULL;

    if (line_read) {
        free(line_read);
        line_read = NULL;
    }

    line_read = readline("(nemu) ");

    if (line_read && *line_read) {
        add_history(line_read);
    }

    return line_read;
}

static int cmd_c(char *args) {
    cpu_exec(-1);
    return 0;
}

static int cmd_q(char *args) {
    return -1;
}

static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);

static struct {
    char *name;
    char *description;
    int (*handler) (char *);
} cmd_table [] = {
    { "help", "Display informations about all supported commands", cmd_help },
    { "c", "Continue the execution of the program", cmd_c },
    { "q", "Exit NEMU", cmd_q },
	{ "si", "Setp one instruction exactly", cmd_si },
	{ "info", "List of integer registers and their contents", cmd_info },
	{ "x", "Examine memory", cmd_x },

    /* TODO: Add more commands */

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int i;

    if (arg == NULL) {
        /* no argument given */
        for (i = 0; i < NR_CMD; i ++) {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    }
    else {
        for (i = 0; i < NR_CMD; i ++) {
            if (strcmp(arg, cmd_table[i].name) == 0) {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

static int cmd_si(char *args) {
	int N;
	char *arg = strtok(NULL, " ");
	if (arg == NULL) {
		N = 1;
	} else {
		N = atoi(arg);
	}
	if (N < 0) {
		printf("N is invalid!\n");
		return 0;
	}
	cpu_exec(N);
	return 0;
}

static int cmd_info(char *args) {
	char *arg = strtok(NULL, " ");
	if (arg == NULL) {
		printf("args are invalid!\n");
		printf("Usage: info { r | w }\n");
		return 0;
	}

	if (!strcmp(arg, "r")) {
		for (int i = 0; i < 8; i++) {
			printf("%s\t0x%x\t%u\n", regsl[i], cpu.gpr[i]._32, cpu.gpr[i]._32);
		}
		printf("%s\t0x%x\t%u\n", "eip", cpu.eip, cpu.eip);
	}

	return 0;
}

static int cmd_x(char *args) {
	int LEN = 4;
	char *arg = strtok(NULL, " ");
	if (arg == NULL) {
		printf("args are invalid!\n");
		printf("Usage: x N EXPR\n");
		return 0;
	}
	int N = atoi(arg);
	if (N < 1) {
		printf("N must bigger than 0!\n");
		return 0;
	}

	arg = strtok(NULL, " ");
	if (arg == NULL) {
		printf("args are invalid!\n");
		printf("Usage: x N EXPR\n");
		return 0;
	}
	vaddr_t addr;
	sscanf(arg, "%x", &addr);
	if (addr < 0) {
		printf("addr must bigger than or equal to 0x0!\n");
		return 0;
	}

	for (int i = 0; i < N; i++) {
		if (i % 4 == 0) {
			printf("0x%x:", addr);
		}
		printf("\t0x%08x", vaddr_read(addr, LEN));
		addr += LEN;
		if ((i + 1) % 4 == 0 || i == N - 1) {
			printf("\n");
		}
	}

	return 0;
}

void ui_mainloop(int is_batch_mode) {
    if (is_batch_mode) {
        cmd_c(NULL);
        return;
    }

    while (1) {
        char *str = rl_gets();
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL) {
            continue;
        }

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end) {
            args = NULL;
        }

#ifdef HAS_IOE
        extern void sdl_clear_event_queue(void);
        sdl_clear_event_queue();
#endif

        int i;
        for (i = 0; i < NR_CMD; i ++) {
            if (strcmp(cmd, cmd_table[i].name) == 0) {
                if (cmd_table[i].handler(args) < 0) {
                    return;
                }
                break;
            }
        }

        if (i == NR_CMD) {
            printf("Unknown command '%s'\n", cmd);
        }
    }
}
