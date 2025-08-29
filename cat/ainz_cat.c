#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int b, e, n, s, t, T, E, v;
} Maskflags;

int is_flag_valid(char arg[]) {
  char *allowed_flags[] = {"-b",
                           "-e",
                           "-n",
                           "-s",
                           "-t",
                           "-E",
                           "-T",
                           "-v",
                           "--squeeze-blank",
                           "--number-nonblank",
                           "--number"};
  int flag = 0, len = sizeof(allowed_flags) / sizeof(allowed_flags[0]);
  for (int i = 0; i < len; ++i) {
    if (strcmp(arg, allowed_flags[i]) == 0) {
      flag = 1;
      break;
    }
  }
  return flag;
}

int is_file_exist(char name[]) {
  int flag = 0;
  FILE *file = fopen(name, "r");
  if (file != NULL) {
    flag = 1;
    fclose(file);
  }
  return flag;
}

void process_maskflags(Maskflags *flags, int argc, char *argv[]) {
  flags->b = 0;
  flags->e = 0;
  flags->n = 0;
  flags->s = 0;
  flags->t = 0;
  flags->T = 0;
  flags->E = 0;
  flags->v = 0;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-b") == 0 ||
        strcmp(argv[i], "--number-nonblank") == 0) {
      flags->b = 1;
    } else if (strcmp(argv[i], "-e") == 0) {
      flags->e = 1;
    } else if (strcmp(argv[i], "-E") == 0) {
      flags->E = 1;
    } else if (strcmp(argv[i], "-T") == 0) {
      flags->T = 1;
    } else if (strcmp(argv[i], "-t") == 0) {
      flags->t = 1;
    } else if (strcmp(argv[i], "-s") == 0 ||
               strcmp(argv[i], "--squeeze-blank") == 0) {
      flags->s = 1;
    } else if (strcmp(argv[i], "-v") == 0) {
      flags->v = 1;
    } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--number") == 0) {
      flags->n = 1;
    }
  }
}

void put_v(int c) {
  if (c == '\n' || c == '\t') {
    putchar(c);
    return;
  }
  unsigned char uc = (unsigned char)c;
  if (uc < 32) {
    putchar('^');
    putchar(uc + 64);
  } else if (uc == 127) {
    putchar('^');
    putchar('?');
  } else if (uc >= 128) {
    printf("M-");
    uc -= 128;
    if (uc < 32) {
      putchar('^');
      putchar(uc + 64);
    } else if (uc == 127) {
      putchar('^');
      putchar('?');
    } else {
      putchar(uc);
    }
  } else {
    putchar(uc);
  }
}

char **check_files(int n, char *file_list[], int *len) {
  int count = 0;
  for (int i = 0; i < n; ++i)
    if (file_list[i] && file_list[i][0] != '\0') count++;
  char **non_empty = malloc(sizeof(char *) * (count > 0 ? count : 1));
  int idx = 0;
  for (int i = 0; i < n; ++i)
    if (file_list[i] && file_list[i][0] != '\0')
      non_empty[idx++] = file_list[i];
  *len = count;
  return non_empty;
}

void process(int argc, char *file_list[], Maskflags flags) {
  int consecutive_blank = 0, at_line_start = 1, files_len = 0;
  unsigned long long number = 1;
  char **files = check_files(argc - 1, file_list, &files_len);
  for (int f = 0; f < files_len; ++f) {
    FILE *fp = fopen(files[f], "r");
    if (!fp) {
      printf("ERROR\n");
      continue;
    }
    int ch, line_blank = 1;
    while ((ch = fgetc(fp)) != EOF) {
      if (flags.s && at_line_start && ch == '\n' && consecutive_blank >= 1) {
        continue;
      }
      if (at_line_start) {
        int is_blank_line = (ch == '\n');
        int need_number = (flags.b ? !is_blank_line : flags.n);
        if (need_number) printf("%6llu\t", number++);
        at_line_start = 0;
        line_blank = 1;
      }
      if (ch == '\n') {
        if (flags.E || flags.e) putchar('$');
        putchar('\n');
        at_line_start = 1;
        if (line_blank)
          consecutive_blank++;
        else
          consecutive_blank = 0;
        line_blank = 1;
        continue;
      }
      line_blank = 0;
      consecutive_blank = 0;
      if ((flags.T || flags.t) && ch == '\t') {
        printf("^I");
      } else if (flags.v || flags.e || flags.t) {
        put_v(ch);
      } else {
        putchar(ch);
      }
    }
    fclose(fp);
  }
  free(files);
}

int main(int argc, char *argv[]) {
  char *file_list[argc - 1];
  for (int i = 0; i < argc - 1; ++i) {
    file_list[i] = "";
  }
  int c = 0;
  for (int i = 0; i < argc - 1; ++i) {
    if (!is_flag_valid(argv[i + 1])) {
      file_list[c++] = argv[i + 1];
    }
  }
  Maskflags flags;
  process_maskflags(&flags, argc, argv);
  process(argc, file_list, flags);
  return 0;
}
