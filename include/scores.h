#ifndef ARCADE_SCORES_H
#define ARCADE_SCORES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARCADE_SCORES_FILE "arcade_scores.ini"
#define ARCADE_MAX_SECTIONS 32
#define ARCADE_MAX_KEYS_PER_SECTION 16

typedef struct {
  char key[64];
  int value;
} ArcadeScoreKey;

typedef struct {
  char section[64];
  int key_count;
  ArcadeScoreKey keys[ARCADE_MAX_KEYS_PER_SECTION];
} ArcadeScoreSection;

typedef struct {
  int section_count;
  ArcadeScoreSection sections[ARCADE_MAX_SECTIONS];
} ArcadeScoreStore;

static inline const char *arcade_scores_path(void) {
  // 1. Check if arcade_scores.ini exists in current working directory
  FILE *f = fopen(ARCADE_SCORES_FILE, "r");
  if (f) {
    fclose(f);
    return ARCADE_SCORES_FILE;
  }

  // 2. Check 2 directories up (for binaries/tests run from games/<game_name>)
  f = fopen("../../" ARCADE_SCORES_FILE, "r");
  if (f) {
    fclose(f);
    return "../../" ARCADE_SCORES_FILE;
  }

  // 3. Check 1 directory up
  f = fopen("../" ARCADE_SCORES_FILE, "r");
  if (f) {
    fclose(f);
    return "../" ARCADE_SCORES_FILE;
  }

  // 4. File doesn't exist yet: locate root from Makefile presence
  f = fopen("../../Makefile", "r");
  if (f) {
    fclose(f);
    return "../../" ARCADE_SCORES_FILE;
  }

  f = fopen("../Makefile", "r");
  if (f) {
    fclose(f);
    return "../" ARCADE_SCORES_FILE;
  }

  return ARCADE_SCORES_FILE;
}

static inline void arcade_scores_trim(char *s) {
  if (!s) return;
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n')) {
    s[--len] = '\0';
  }
}

static inline void arcade_safe_copy(char *dest, const char *src, size_t dest_size) {
  if (!dest || dest_size == 0) return;
  if (!src) {
    dest[0] = '\0';
    return;
  }
  size_t i = 0;
  while (i + 1 < dest_size && src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
}

static inline void arcade_scores_load_store(ArcadeScoreStore *store) {
  memset(store, 0, sizeof(ArcadeScoreStore));
  const char *path = arcade_scores_path();
  FILE *f = fopen(path, "r");
  if (!f) return;

  char line[256];
  int current_sec = -1;

  while (fgets(line, sizeof(line), f)) {
    arcade_scores_trim(line);
    if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
      continue;
    }

    if (line[0] == '[') {
      char *end = strchr(line, ']');
      if (end) {
        *end = '\0';
        char *sec_name = line + 1;
        arcade_scores_trim(sec_name);
        if (store->section_count < ARCADE_MAX_SECTIONS) {
          current_sec = store->section_count++;
          arcade_safe_copy(store->sections[current_sec].section, sec_name, sizeof(store->sections[current_sec].section));
          store->sections[current_sec].key_count = 0;
        }
      }
      continue;
    }

    char *eq = strchr(line, '=');
    if (eq && current_sec >= 0) {
      *eq = '\0';
      char *key = line;
      char *val_str = eq + 1;
      arcade_scores_trim(key);
      arcade_scores_trim(val_str);

      ArcadeScoreSection *sec = &store->sections[current_sec];
      if (sec->key_count < ARCADE_MAX_KEYS_PER_SECTION) {
        arcade_safe_copy(sec->keys[sec->key_count].key, key, sizeof(sec->keys[sec->key_count].key));
        sec->keys[sec->key_count].value = atoi(val_str);
        sec->key_count++;
      }
    }
  }

  fclose(f);
}

static inline void arcade_scores_save_store(const ArcadeScoreStore *store) {
  const char *path = arcade_scores_path();
  FILE *f = fopen(path, "w");
  if (!f) return;

  for (int s = 0; s < store->section_count; s++) {
    const ArcadeScoreSection *sec = &store->sections[s];
    fprintf(f, "[%s]\n", sec->section);
    for (int k = 0; k < sec->key_count; k++) {
      fprintf(f, "%s=%d\n", sec->keys[k].key, sec->keys[k].value);
    }
    fprintf(f, "\n");
  }

  fclose(f);
}

static inline int arcade_score_get(const char *section_name, const char *key_name) {
  if (!section_name || !key_name) return 0;
  ArcadeScoreStore store;
  arcade_scores_load_store(&store);

  for (int s = 0; s < store.section_count; s++) {
    if (strcmp(store.sections[s].section, section_name) == 0) {
      for (int k = 0; k < store.sections[s].key_count; k++) {
        if (strcmp(store.sections[s].keys[k].key, key_name) == 0) {
          return store.sections[s].keys[k].value;
        }
      }
      return 0;
    }
  }
  return 0;
}

static inline void arcade_score_set(const char *section_name, const char *key_name, int value) {
  if (!section_name || !key_name) return;
  ArcadeScoreStore store;
  arcade_scores_load_store(&store);

  int sec_idx = -1;
  for (int s = 0; s < store.section_count; s++) {
    if (strcmp(store.sections[s].section, section_name) == 0) {
      sec_idx = s;
      break;
    }
  }

  if (sec_idx < 0) {
    if (store.section_count >= ARCADE_MAX_SECTIONS) return;
    sec_idx = store.section_count++;
    arcade_safe_copy(store.sections[sec_idx].section, section_name, sizeof(store.sections[sec_idx].section));
    store.sections[sec_idx].key_count = 0;
  }

  ArcadeScoreSection *sec = &store.sections[sec_idx];
  int key_idx = -1;
  for (int k = 0; k < sec->key_count; k++) {
    if (strcmp(sec->keys[k].key, key_name) == 0) {
      key_idx = k;
      break;
    }
  }

  if (key_idx >= 0) {
    sec->keys[key_idx].value = value;
  } else {
    if (sec->key_count >= ARCADE_MAX_KEYS_PER_SECTION) return;
    arcade_safe_copy(sec->keys[sec->key_count].key, key_name, sizeof(sec->keys[sec->key_count].key));
    sec->keys[sec->key_count].value = value;
    sec->key_count++;
  }

  arcade_scores_save_store(&store);
}

#endif // ARCADE_SCORES_H
