#include <assert.h>
#include <string.h>

#include <tcutil.h>

#include "tcdb.h"

/** Private function prototypes */
static int64_t rk_tcdb_add_int(rk_tcdb_t *db, const char *kbuf, int ksiz, int64_t num);
char *rk_tcdb_hash_get(TCTDB *db, const char *kbuf, int ksiz,
                       const char *fbuf, int fsiz, int *sp);
static int rk_tcdb_hash_put(TCTDB *db, const char *kbuf, int ksiz,
                            const char *fbuf, int fsiz, const char *vbuf, int vsiz);
static int rk_tcdb_hash_out(TCTDB *db, const char *kbuf, int ksiz,
                            const char *fbuf, int fsiz);
static int rk_tcdb_hash_put_keep(TCTDB *db, const char *kbuf, int ksiz,
                                 const char *fbuf, int fsiz, const char *vbuf, int vsiz);
static int rk_tcdb_hash_exists(TCTDB *db, const char *kbuf, int ksiz,
                               const char *fbuf, int fsiz);
static int rk_tcdb_hash_rnum(TCTDB *db, const char *kbuf, int ksiz);

/** 
 * NOTES
 * --
 *   So far the `str` table will *always* store values as raw data (i.e. pure
 *   byte array). In other words even if INCR and DECR commands give integers
 *   values, we won't store these values as serialized int-s (this is what
 *   Tokyo Cabinet does with `tchdbaddint` function - see HDBPDADDINT flag).
 *   We could think of optimizing this by maintaining one Hash DB per kind of
 *   encoding (see REDIS_ENCODING_RAW, REDIS_ENCODING_INT, etc).
 */

void rk_tcdb_skel_init(rk_skel_t *skel) {
  assert(skel);
  memset(skel, 0, sizeof(*skel));
  skel->opq = rk_tcdb_new();
  skel->free = (void (*)(void *))rk_tcdb_free;
  skel->open = (bool (*)(void *, const char *))rk_tcdb_open;
  skel->close = (bool (*)(void *))rk_tcdb_close;
  skel->get = (char *(*)(void *, const char *, int, int *))rk_tcdb_get;
  skel->set = (bool (*)(void *, const char *, int, const char *, int))rk_tcdb_set;
}

rk_tcdb_t *rk_tcdb_new(void) {
  rk_tcdb_t *db = malloc(sizeof(*db));
  db->open = false;
  db->str = NULL;
  db->hsh = NULL;
  db->set = NULL;
  return db;
}

void rk_tcdb_free(rk_tcdb_t *db) {
  assert(db);
  if (db->open) rk_tcdb_close(db);
  free(db);  
}

bool rk_tcdb_open(rk_tcdb_t *db, const char *path) {
  assert(db && path);
  int omode = HDBOWRITER | HDBOCREAT | HDBOLCKNB;
  TCHDB *str = tchdbnew();
  if (!tchdbopen(str, path, omode)) {
    tchdbdel(str);
    return false;
  }
  char *hpath = tcsprintf("%s.hash", path);
  omode = TDBOWRITER | TDBOCREAT | TDBOLCKNB;
  TCTDB *hsh = tctdbnew();
  if (!tctdbopen(hsh, hpath, omode)) {
      tchdbdel(str);
      tctdbdel(hsh);
      free(hpath);
      return false;
  }
  char *spath = tcsprintf("%s.set", path);
  TCTDB *set = tctdbnew();
  if (!tctdbopen(set, spath, omode)) {
      tchdbdel(str);
      tctdbdel(hsh);
      tctdbdel(set);
      free(hpath);
      free(spath);
      return false;
  }
  db->str = str;
  db->hsh = hsh;
  db->set = set;
  db->open = true;
  free(hpath);
  free(spath);
  return true;
}

bool rk_tcdb_close(rk_tcdb_t *db) {
  assert(db);
  if (!db->open) return false;
  bool err = false;
  if (!tchdbclose(db->str)) err = true;
  tchdbdel(db->str);
  if (!tctdbclose(db->hsh)) err = true;
  tctdbdel(db->hsh);
  if (!tctdbclose(db->set)) err = true;
  tctdbdel(db->set);
  db->str = NULL;
  db->hsh = NULL;
  db->set = NULL;
  db->open = false;
  return !err;
}

char *rk_tcdb_get(rk_tcdb_t *db, const char *kbuf, int ksiz, int *sp) {
  assert(db && kbuf && ksiz >= 0 && sp);
  if (!db->open) return NULL;
  return tchdbget(db->str, kbuf, ksiz, sp);
}

bool rk_tcdb_set(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return NULL;
  return tchdbput(db->str, kbuf, ksiz, vbuf, vsiz);
}

int rk_tcdb_setnx(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return -1;
  int rv = 1;
  if (!tchdbputkeep(db->str, kbuf, ksiz, vbuf, vsiz)) {
    rv = (tchdbecode(db->str) == TCEKEEP) ? 0 : -1;
  }
  return rv;
}

int64_t rk_tcdb_incr(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return INT_MIN;
  /* TODO: see above NOTES */
  /* return tchdbaddint(db->str, kbuf, ksiz, 1); */
  return rk_tcdb_add_int(db, kbuf, ksiz, 1);
}

int64_t rk_tcdb_decr(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return INT_MIN;
  return rk_tcdb_add_int(db, kbuf, ksiz, -1);
}

int64_t rk_tcdb_incrby(rk_tcdb_t *db, const char *kbuf, int ksiz, int64_t inc) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return INT_MIN;
  return rk_tcdb_add_int(db, kbuf, ksiz, inc);
}

int64_t rk_tcdb_decrby(rk_tcdb_t *db, const char *kbuf, int ksiz, int dec) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return INT_MIN;
  return rk_tcdb_add_int(db, kbuf, ksiz, -dec);
}

char *rk_tcdb_hget(rk_tcdb_t *db, const char *kbuf, int ksiz,
                   const char *fbuf, int fsiz, int *sp) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && sp);
  if (!db->open) return NULL;
  return rk_tcdb_hash_get(db->hsh, kbuf, ksiz, fbuf, fsiz, sp);
}

int rk_tcdb_hset(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *fbuf, int fsiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_put(db->hsh, kbuf, ksiz, fbuf, fsiz, vbuf, vsiz);
}

int rk_tcdb_hdel(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *fbuf, int fsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_out(db->hsh, kbuf, ksiz, fbuf, fsiz);
}

int rk_tcdb_hsetnx(rk_tcdb_t *db, const char *kbuf, int ksiz,
                   const char *fbuf, int fsiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_put_keep(db->hsh, kbuf, ksiz, fbuf, fsiz, vbuf, vsiz);  
}

int rk_tcdb_hexists(rk_tcdb_t *db, const char *kbuf, int ksiz,
                    const char *fbuf, int fsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_exists(db->hsh, kbuf, ksiz, fbuf, fsiz);
}

int rk_tcdb_hlen(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_rnum(db->hsh, kbuf, ksiz);  
}

int rk_tcdb_sadd(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *mbuf, int msiz) {
  assert(db && kbuf && ksiz >= 0 && mbuf && msiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_put(db->set, kbuf, ksiz, mbuf, msiz, "", 0);              
}

int rk_tcdb_srem(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *mbuf, int msiz) {
  assert(db && kbuf && ksiz >= 0 && mbuf && msiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_out(db->set, kbuf, ksiz, mbuf, msiz);
}

int rk_tcdb_scard(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_rnum(db->set, kbuf, ksiz);
}

int rk_tcdb_sismember(rk_tcdb_t *db, const char *kbuf, int ksiz,
                      const char *mbuf, int msiz) {
  assert(db && kbuf && ksiz >= 0 && mbuf && msiz >= 0);
  if (!db->open) return -1;
  return rk_tcdb_hash_exists(db->set, kbuf, ksiz, mbuf, msiz);
}

static int64_t rk_tcdb_add_int(rk_tcdb_t *db, const char *kbuf, int ksiz, int64_t num) {
  assert(db && kbuf && ksiz >= 0);
  bool err = false;
  int64_t rv;
  int vsiz;
  char buf[32];
  int siz;
  const char *vbuf = tchdbget(db->str, kbuf, ksiz, &vsiz);
  if (vbuf == NULL) {
    if (tchdbecode(db->str) == TCENOREC) rv = num;
    else err = true;
  }
  else {
    if (tcstrisnum(vbuf)) rv = tcatoi(vbuf) + num;
    else err = true;
  }
  if (!err) {
    siz = sprintf(buf, "%lld", (long long) rv);
    if (!tchdbput(db->str, kbuf, ksiz, buf, siz)) err = true;
  }
  return (err ? INT_MIN : rv);
}

char *rk_tcdb_hash_get(TCTDB *db, const char *kbuf, int ksiz,
                       const char *fbuf, int fsiz, int *sp) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && sp);
  return tctdbget4(db, kbuf, ksiz, fbuf, fsiz, sp);
}

static int rk_tcdb_hash_put(TCTDB *db, const char *kbuf, int ksiz,
                            const char *fbuf, int fsiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && vbuf && vsiz >= 0);
  TCMAP *cols = NULL;
  if (!tctdbiterinit2(db, kbuf, ksiz)) {
    if (tctdbecode(db) != TCENOREC) return -1;
    else cols = tcmapnew();
  }
  else {
    cols = tctdbiternext3(db);
    tcmapout2(cols, "");
  }
  if (!cols) return -1;
  int rsiz;
  const void *rbuf = tcmapget(cols, fbuf, fsiz, &rsiz);
  int rv = (rbuf == NULL ? 1 : 0);
  tcmapput(cols, fbuf, fsiz, vbuf, vsiz);
  bool err = !tctdbput(db, kbuf, ksiz, cols);
  if (cols) tcmapdel(cols);
  return err ? -1 : rv;
}

static int rk_tcdb_hash_out(TCTDB *db, const char *kbuf, int ksiz,
                            const char *fbuf, int fsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0);
  TCMAP *cols = NULL;
  if (!tctdbiterinit2(db, kbuf, ksiz)) {
    if (tctdbecode(db) != TCENOREC) return -1;
    else return 0;
  }
  else {
    cols = tctdbiternext3(db);
    tcmapout2(cols, "");
  }
  if (!cols) return -1;
  int rv = 0;
  bool err = false;
  if (tcmapout(cols, fbuf, fsiz)) {
    rv = 1;
    err = !tctdbput(db, kbuf, ksiz, cols);
  }
  if (cols) tcmapdel(cols);
  return err ? -1 : rv;  
}

static int rk_tcdb_hash_put_keep(TCTDB *db, const char *kbuf, int ksiz,
                                 const char *fbuf, int fsiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && vbuf && vsiz >= 0);
  TCMAP *cols = NULL;
  if (!tctdbiterinit2(db, kbuf, ksiz)) {
    if (tctdbecode(db) != TCENOREC) return -1;
    else cols = tcmapnew();
  }
  else {
    cols = tctdbiternext3(db);
    tcmapout2(cols, "");
  }
  if (!cols) return -1;
  int rv = 0;
  bool err = false;
  if (tcmapputkeep(cols, fbuf, fsiz, vbuf, vsiz)) {
    rv = 1;
    err = !tctdbput(db, kbuf, ksiz, cols);
  }
  if (cols) tcmapdel(cols);
  return err ? -1 : rv;                                       
}

static int rk_tcdb_hash_exists(TCTDB *db, const char *kbuf, int ksiz,
                               const char *fbuf, int fsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0);
  TCMAP *cols = NULL;
  if (!tctdbiterinit2(db, kbuf, ksiz)) {
    if (tctdbecode(db) != TCENOREC) return -1;
    else return 0;
  }
  else {
    cols = tctdbiternext3(db);
  }
  if (!cols) return -1;
  int rsiz;
  const void *rbuf = tcmapget(cols, fbuf, fsiz, &rsiz);
  int rv = (rbuf == NULL ? 0 : 1);
  if (cols) tcmapdel(cols);
  return rv;                                     
}

static int rk_tcdb_hash_rnum(TCTDB *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  TCMAP *cols = NULL;
  if (!tctdbiterinit2(db, kbuf, ksiz)) {
    if (tctdbecode(db) != TCENOREC) return -1;
    else return 0;
  }
  else {
    cols = tctdbiternext3(db);
  }
  if (!cols) return -1;
  int rv = tcmaprnum(cols) - 1;
  if (cols) tcmapdel(cols);
  return rv;  
}
