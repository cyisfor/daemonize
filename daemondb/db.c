#include <stdlib.h>
#include <gdbm.h>

GDBM_FILE c = NULL;

inline void setup(void) {
  if(c == NULL) {
    c = gdbm_open(path,512,GDBM_WRCREAT,0600,NULL);
    assert(c);
  }
}

char** db_lookup(const char* name) {
  setup();
  datum key = {
    .dptr = name,
    .dsize = strlen(name)
  };
  datum result = gdbm_fetch(c, key);

  if(result.dptr == NULL) return NULL;
  
  uint8_t* data = (uint8_t*)result.dptr;
  uint8_t nargs = data[0];
  char** ret = malloc(sizeof(char*) * nargs + 1);

  ret[nargs] = NULL;
  uint8_t* cur = data + nargs + 1;
  for(i=0;i<nargs;++i) {
    uint8_t len = data[i+1];
    ret[i] = cur;
    cur += len+1; // +null
  }
  return ret;
}   

void db_remember(char** argv) {
  /blurph
