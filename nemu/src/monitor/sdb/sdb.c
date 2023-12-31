/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();
WP* new_wp(char *expr, word_t val);
void free_wp(WP *wp);
void show_watchpoint();
int delete_watchpoint(int n);

word_t vaddr_read(vaddr_t addr, int len);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
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
  nemu_state.state = NEMU_QUIT;
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args);

static int cmd_info(char *args);

static int cmd_x(char *args);

static int cmd_p(char *args);

static int cmd_w(char *args);

static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "Step through N instructions", cmd_si},
  { "info", "Show the infomation of reg and watch point", cmd_info},
  { "x", "Scan the memory", cmd_x},
  { "p", "Expressions evaluate", cmd_p},
  { "w", "Set the watch point", cmd_w},
  { "d", "delete the watch point", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

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
  int n = 0;
  if (args == NULL)
    n = 1;
  else
    sscanf(args, "%d", &n);
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  if (strcmp(args, "r") == 0) {
    isa_reg_display();
  } else if (strcmp(args, "w") == 0) {
    show_watchpoint();
  } else {
    printf("Unknow parma\n");
  }
  return 0;
}

static int cmd_x(char *args) {
  char *parma1 = strtok(args, " ");
  char *parma2 = strtok(NULL, " ");

  if (parma1 == NULL || parma2 == NULL) {
    printf("Unknow parma\n");
  } else {
    int n = 0;
    sscanf(parma1, "%d", &n);
    bool success = false; 
    vaddr_t addr = expr(parma2, &success);

    for (int i = 0; i < 4 * n; i++) {
      uint8_t val= vaddr_read(addr + i, 1);
      printf("%02x ",val);
    }
    printf("\n");
  }

  return 0;
}

static int cmd_p(char *args) {
    bool success = true;
    sword_t val = expr(args, &success);

    if (success) {
      printf("val = %d\n", val);
    } else {
      printf("expr false, please correctly input again!\n");
    }

    return 0;
}

static int cmd_w(char *args) {
  bool success = true;
  word_t val = expr(args, &success);

  if (success) {
    WP *wp = new_wp(args, val);
    printf("watch point %d set succeed\n", wp->NO);
  }

  return 0;
}

static int cmd_d(char *args) {
  char *param = strtok(args, " ");
  int n = 0;

  if (param == NULL) {
    printf("Unknow parma\n");
  } else {
    sscanf(param, "%d", &n);
    if (delete_watchpoint(n) == 1) {
      printf("delete succeed!\n");
      return 0;
    } else {
      printf("delete failed, please input correct number!\n");
      return -1;
    }
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
