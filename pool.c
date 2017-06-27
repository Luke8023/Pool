#include "pool.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct pool{
  char *mem;
  int cap;
  struct memory *m;
};

struct memory{
  int *poi;
  int *alsize;
  int len;
};

struct pool *pool_create(int size){
  assert(size > 0);
  struct pool *p = malloc(sizeof(struct pool));
  p->cap = size;
  p->mem = malloc(sizeof(char) * size);
  p->m = malloc(sizeof(struct memory));
  p->m->len = 0;
  p->m->poi = NULL;
  p->m->alsize = NULL;
  return p;
}

bool pool_destroy(struct pool *p){
  assert(p);
  if(p->m->len != 0){
    return false;
  } else {
    free(p->m);
    free(p->mem);
    free(p);
    return true;
  }
}

static int add(int *poi, int *alsize, int len, int k, int size){
  for(int i = len - 1; i > k; --i){
    *(poi + i) = *(poi + i - 1);
    *(alsize + i) = *(alsize + i - 1);
  }
  *(poi + k) = *(poi + k - 1) + *(alsize + k - 1);
  *(alsize + k) = size;
  return *(poi + k);
}

char *pool_alloc(struct pool *p, int size){
  assert(p);
  assert(size > 0);
  if(size > p->cap) return NULL;
  if(p->m->len == 0){
    p->m->len++;
    p->m->poi = malloc(sizeof(int) * p->m->len);
    p->m->alsize = malloc(sizeof(int) * p->m->len);
    *(p->m->poi) = 0;
    *(p->m->alsize) = size;
    return (p->mem);
  } else {
    int used = 0;
    for(int i = 0; i < p->m->len; ++i){
      used += *(p->m->alsize + i);
    }
    if(size > p->cap - used) return NULL;
    if(p->m->len == 1){
      p->m->len++;
      p->m->poi = realloc(p->m->poi, sizeof(int) * p->m->len);
      p->m->alsize = realloc(p->m->alsize, sizeof(int) * p->m->len); 
      *(p->m->poi + 1) = *(p->m->poi) + *(p->m->alsize);
      *(p->m->alsize + 1) = size;
      return (p->mem + *(p->m->poi) + *(p->m->alsize));
    } else {
      for(int k = 0; k < p->m->len - 1; ++k){
        if(*(p->m->poi + k + 1) - *(p->m->poi + k)
           - *(p->m->alsize + k) >= size){
          p->m->len++;
          p->m->poi = realloc(p->m->poi, sizeof(int) * p->m->len);
          p->m->alsize = realloc(p->m->alsize, sizeof(int) * p->m->len); 
          return (p->mem + add(p->m->poi, p->m->alsize,
                               p->m->len, k + 1, size));
        }
      }
      if(p->cap - *(p->m->poi + p->m->len - 1) -
         *(p->m->alsize + p->m->len - 1) >= size){
        p->m->len++;
        p->m->poi = realloc(p->m->poi, sizeof(int) * p->m->len);
        p->m->alsize = realloc(p->m->alsize, sizeof(int) * p->m->len);
        *(p->m->poi + p->m->len - 1) = *(p->m->poi + p->m->len - 2) +
          *(p->m->alsize + p->m->len - 2);
        *(p->m->alsize + p->m->len - 1) = size;
        return (p->mem + *(p->m->poi + p->m->len - 1));
      }
    }
  }
  return NULL;
}

bool pool_free(struct pool *p, char *addr){
  assert(p);
  int target = 0;
  for(int i = 0; i < p->m->len; ++i){
    if((addr - p->mem) == *(p->m->poi + i)){
      target = i;
      break;
    }
    if((i == p->m->len - 1) && ((addr - p->mem) != *(p->m->poi + i))){
      return false;
    }
  }
  for(int k = target; k < p->m->len - 1; ++k){
    *(p->m->poi + k) = *(p->m->poi + k + 1);
    *(p->m->alsize + k) = *(p->m->alsize + k + 1);
  }
  p->m->len--;
  p->m->poi = realloc(p->m->poi, sizeof(int) * p->m->len);
  p->m->alsize = realloc(p->m->alsize, sizeof(int) * p->m->len);
  return true;
}

char *pool_realloc(struct pool *p, char *addr, int size){
  assert(p);
  assert(p->m->len > 0);
  int target = 0;

  for(int i = 0; i < p->m->len; ++i){
    if((addr - p->mem) == *(p->m->poi + i)){
      target = i;
      break;
    }
    if((i == p->m->len - 1) && ((addr - p->mem) != *(p->m->poi + i))){
      return NULL;
    }
  }
  if(size <= *(p->m->alsize + target)){
    *(p->m->alsize + target) = size;
    return addr;
  }
  if(target == p->m->len - 1){
    if(p->cap - *(p->m->poi + target) >= size){
      *(p->m->alsize + target) = size;
      return addr;
    } else {
      int prev_poi = *(p->m->poi + target);
      int prev_size = *(p->m->alsize + target);
      p->m->len--;
      p->m->poi = realloc(p->m->poi, sizeof(int) * p->m->len);
      p->m->alsize = realloc(p->m->alsize, sizeof(int) * p->m->len);
      char *naddr = pool_alloc(p, size);
      for(int c = 0; c < prev_size; ++c){
        naddr[c] = *(p->mem + prev_poi + c);
      }
      return naddr;
    }
  } else {
    if(*(p->m->poi + target + 1) - *(p->m->poi + target) >= size){
      *(p->m->alsize + target) = size;
      return (p->mem + *(p->m->poi + target));
    } else {
      int prev_poi = *(p->m->poi + target);
      int prev_size = *(p->m->alsize + target);
      for(int k = 0; k < p->m->len - 1; ++k){
        if(*(p->m->poi + k + 1) - 
           *(p->m->poi + k) - *(p->m->alsize + k) >= size){
          int ret = add(p->m->poi, p->m->alsize, p->m->len, k + 1, size);
          if(target > k){
            target++;
          } 
          for(int j = target; j < p->m->len - 1; ++j){
            *(p->m->poi + j) = *(p->m->poi + j + 1);
            *(p->m->alsize + j) = *(p->m->alsize + j + 1);
          }
          p->m->len--;
          p->m->poi = realloc(p->m->poi, sizeof(int) * p->m->len);
          p->m->alsize = realloc(p->m->alsize, sizeof(int) * p->m->len);
          for(int c = 0; c < prev_size; ++c){
            *(p->mem + ret + c) = *(p->mem + prev_poi + c);
          }
          return (p->mem + ret);
        }
      }
      return NULL;
    }
  }
  return NULL;
}

void pool_print_active(struct pool *p){
  assert(p);
  if(p->m->len == 0){
    printf("active: none\n");
    return;
  } else {
    printf("active:");
    for(int i = 0; i < p->m->len; ++i){
      if(i == p->m->len - 1){
        printf(" %d [%d]\n", *(p->m->poi + i), *(p->m->alsize+ i));
      } else {
        printf(" %d [%d],", *(p->m->poi + i), *(p->m->alsize+ i));
      }
    }
  }
}

void pool_print_available(struct pool *p){
  assert(p);
  int used = 0;
  if(p->m->len == 0){
    printf("available: %d [%d]\n", 0, p->cap);
    return;
  }
  for(int i = 0; i < p->m->len; ++i){
    used += *(p->m->alsize + i);
  }
  if(used == p->cap){
    printf("available: none\n");
    return;
  }

  if(*(p->m->poi) == 0){
    if(p->m->len == 1){
      printf("available: %d [%d]\n", *p->m->alsize, p->cap - *p->m->alsize);
      return;
    } else {
      printf("available:");
      for(int k = 0; k < p->m->len - 1; ++k){
        int ava = *(p->m->poi + k + 1) - *(p->m->poi + k)
          - *(p->m->alsize + k);
        if(k == p->m->len - 2){
          int lava = p->cap - *(p->m->poi + k + 1) - *(p->m->alsize + k + 1);
          if(lava > 0){
            if(ava > 0){
              printf(" %d [%d],", *(p->m->poi + k) 
                     + *(p->m->alsize + k), ava);
              printf(" %d [%d]\n", *(p->m->poi + k + 1) 
                     + *(p->m->alsize + k + 1), lava);
              return;
            } else {
              printf(" %d [%d]\n", *(p->m->poi + k + 1) 
                     + *(p->m->alsize + k + 1), lava);
              return;
            }
          }
        }

        if(ava > 0){
          printf(" %d [%d],", *(p->m->poi + k) 
                 + *(p->m->alsize + k), ava);
        }
      }
    }
  }
}

