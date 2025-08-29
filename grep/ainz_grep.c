#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  int e, i, v, c, l, n;
  int h, s, o, f;
} GrepFlags;

void add_pattern(char ***pattern_list, int *pattern_count, const char *pat) {
  char **tmp = realloc(*pattern_list, sizeof(char *) * (*pattern_count + 2));
  if (!tmp) {
    perror("realloc");
    exit(1);
  }
  *pattern_list = tmp;
  (*pattern_list)[*pattern_count] = strdup(pat);
  if (!(*pattern_list)[*pattern_count]) {
    perror("strdup");
    exit(1);
  }
  (*pattern_count)++;
  (*pattern_list)[*pattern_count] = NULL;
}

void read_patterns_from_file(char ***pattern_list, int *pattern_count,
                             const char *filename, int silent) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    if (!silent) printf("Failed to open pattern file '%s': \n", filename);
    return;
  }
  char *line = NULL;
  size_t len = 0;
  ssize_t r;
  while ((r = getline(&line, &len, f)) != -1) {
    while (r > 0 && (line[r - 1] == '\n' || line[r - 1] == '\r')) {
      line[--r] = '\0';
    }
    add_pattern(pattern_list, pattern_count, line);
  }
  free(line);
  fclose(f);
}

GrepFlags parse(int argc, char *argv[], char ***pattern_list,
                int *pattern_count, char ***file_list, int *file_count) {
  *pattern_list = NULL;
  *pattern_count = 0;
  *file_list = NULL;
  *file_count = 0;
  GrepFlags flags = {0};
  int opt;
  while ((opt = getopt(argc, argv, "e:f:ivclnhso")) != -1) {
    switch (opt) {
      case 'f': {
        flags.f = 1;
        read_patterns_from_file(pattern_list, pattern_count, optarg, flags.s);
        break;
      }
      case 'e':
        flags.e = 1;
        add_pattern(pattern_list, pattern_count, optarg);
        break;
      case 'i':
        flags.i = 1;
        break;
      case 'v':
        flags.v = 1;
        break;
      case 'c':
        flags.c = 1;
        break;
      case 'l':
        flags.l = 1;
        break;
      case 'n':
        flags.n = 1;
        break;
      case 'h':
        flags.h = 1;
        break;
      case 's':
        flags.s = 1;
        break;
      case 'o':
        flags.o = 1;
        break;
      default:
        break;
    }
  }
  if (*pattern_count == 0) {
    if (optind < argc) {
      add_pattern(pattern_list, pattern_count, argv[optind]);
      optind++;
    } else {
      printf("grep: missing pattern\n");
      exit(2);
    }
  }
  int j = 0;
  if (optind < argc) {
    int rem = argc - optind;
    *file_list = malloc(sizeof(char *) * (rem + 1));
    if (!*file_list) {
      perror("malloc");
      exit(1);
    }
    for (int i = optind; i < argc; ++i) {
      (*file_list)[j++] = strdup(argv[i]);
    }
  }
  (*file_list)[j] = NULL;
  *file_count = j;
  return flags;
}

void free_str_array(char **arr) {
  if (!arr) return;
  char **p = arr;
  while (*p) {
    free(*p);
    p++;
  }
  free(arr);
}

int find_earliest_match(regex_t *regexes, int pattern_count, const char *line,
                        int start_offset, int *which, regmatch_t *rm) {
  int found = 0;
  int best_so = -1;
  regmatch_t cur;
  for (int j = 0; j < pattern_count; ++j) {
    int ret = regexec(&regexes[j], line + start_offset, 1, &cur, 0);
    if (ret == 0) {
      int so = start_offset + cur.rm_so;
      if (!found || so < best_so) {
        found = 1;
        best_so = so;
        *which = j;
        *rm = cur;
      }
    }
  }
  return found;
}

void process(char **pattern_list, char **file_list, int pattern_count,
             int file_count, GrepFlags flags) {
  int multi_file = file_count > 1;
  regex_t *regexes = malloc(sizeof(regex_t) * pattern_count);
  if (!regexes) {
    perror("malloc");
    exit(1);
  }
  int cflags = flags.i ? REG_ICASE : 0;
  for (int j = 0; j < pattern_count; j++) {
    int rc = regcomp(&regexes[j], pattern_list[j], cflags | REG_EXTENDED);
    if (rc != 0) {
      char errbuf[256];
      regerror(rc, &regexes[j], errbuf, sizeof(errbuf));
      printf("Invalid regex '%s': \n", pattern_list[j]);
      for (int k = 0; k < j; ++k) regfree(&regexes[k]);
      free(regexes);
      exit(2);
    }
  }

  for (int i = 0; i < file_count; ++i) {
    const char *fname = file_list[i];
    FILE *current_file = NULL;
    int is_stdin = 0;
    if (strcmp(fname, "-") == 0) {
      current_file = stdin;
      is_stdin = 1;
    } else {
      current_file = fopen(fname, "r");
      if (!current_file) {
        if (!flags.s) printf("Failed to open '%s': \n", fname);
        continue;
      }
    }

    int count = 0;
    char *line = NULL;
    size_t cap = 0;
    ssize_t nread;
    int line_number = 0;

    while ((nread = getline(&line, &cap, current_file)) != -1) {
      line_number++;
      if (flags.o && flags.v) {
        continue;
      }
      int matched_any = 0;
      for (int j = 0; j < pattern_count; ++j) {
        if (regexec(&regexes[j], line, 0, NULL, 0) == 0) {
          matched_any = 1;
          break;
        }
      }
      int matched = flags.v ? !matched_any : matched_any;

      if (flags.l && matched) {
        printf("%s\n", fname);
        break;
      }
      if (flags.c && !flags.l) {
        if (matched) count++;
        continue;
      }

      if (flags.o && !flags.v) {
        if (!matched_any) continue;
        int offset = 0;
        int line_len = (int)nread;
        while (offset <= line_len) {
          int which = -1;
          regmatch_t rm;
          if (!find_earliest_match(regexes, pattern_count, line, offset, &which,
                                   &rm))
            break;
          int so = offset + rm.rm_so;
          int eo = offset + rm.rm_eo;
          if (so < 0 || eo < 0 || eo <= so) {
            offset = offset + 1;
            continue;
          }
          if (multi_file && !flags.h && !is_stdin) printf("%s:", fname);
          if (flags.n) printf("%d:", line_number);
          fwrite(line + so, 1, (size_t)(eo - so), stdout);
          putchar('\n');
          offset = eo;
        }
        continue;
      }

      if (matched && !flags.c && !flags.l) {
        if (multi_file && !flags.h && !is_stdin) printf("%s:", fname);
        if (flags.n) printf("%d:", line_number);
        fwrite(line, 1, (size_t)nread, stdout);
        if (nread == 0 || line[nread - 1] != '\n') {
          putchar('\n');
        }
      }
    }

    if (flags.c && !flags.l) {
      if (multi_file && !flags.h && !is_stdin) printf("%s:", fname);
      printf("%d\n", count);
    }

    free(line);
    if (!is_stdin) fclose(current_file);
  }

  for (int j = 0; j < pattern_count; ++j) regfree(&regexes[j]);
  free(regexes);
}

int main(int argc, char *argv[]) {
  char **pattern_list = NULL, **file_list = NULL;
  int pattern_count = 0, file_count = 0;
  GrepFlags flags =
      parse(argc, argv, &pattern_list, &pattern_count, &file_list, &file_count);
  process(pattern_list, file_list, pattern_count, file_count, flags);
  free_str_array(pattern_list);
  free_str_array(file_list);
  return 0;
}
