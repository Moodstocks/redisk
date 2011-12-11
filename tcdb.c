#include <assert.h>
#include <string.h>

#include <tcutil.h>

#include "tcdb.h"

/** Redis object types */
enum {
  RK_TCDB_NONE      = -1,
  RK_TCDB_STRING    = 0,
  RK_TCDB_LIST      = 1,
  RK_TCDB_SET       = 2,
  RK_TCDB_ZSET      = 3,
  RK_TCDB_HASH      = 4
};

/* Private macro */
#define RKTCDBOCHECK(RK_type, RK_exp)                            \
  (((RK_type) >= 0 && (RK_type) != (RK_exp)) ? false : true)

/** Private function prototypes */
static int rk_tcdb_obj_search(rk_tcdb_t *db, const char *kbuf, int ksiz, int *type);
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
static TCLIST *rk_tcdb_hash_keys(TCTDB *db, const char *kbuf, int ksiz);

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
  skel->del = (int (*)(void *, const char *, int))rk_tcdb_del;
  skel->exists = (int (*)(void *, const char *, int))rk_tcdb_exists;
  skel->type = (char *(*)(void *, const char *, int))rk_tcdb_type;
  skel->get = (char *(*)(void *, const char *, int, int *))rk_tcdb_get;
  skel->set = (bool (*)(void *, const char *, int, const char *, int))rk_tcdb_set;
  skel->setnx = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_setnx;
  skel->incr = (int64_t (*)(void *, const char *, int))rk_tcdb_incr;
  skel->decr = (int64_t (*)(void *, const char *, int))rk_tcdb_decr;
  skel->incrby = (int64_t (*)(void *, const char *, int, int64_t))rk_tcdb_incrby;
  skel->decrby = (int64_t (*)(void *, const char *, int, int64_t))rk_tcdb_decrby;
  skel->getset = (char *(*)(void *, const char *, int, const char *, int, int *))rk_tcdb_getset;
  skel->hget = (char *(*)(void *, const char *, int, const char *, int, int *))rk_tcdb_hget;
  skel->hset = (int (*)(void *, const char *, int, const char *, int, const char *, int))rk_tcdb_hset;
  skel->hdel = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_hdel;
  skel->hsetnx = (int (*)(void *, const char *, int, const char *, int, const char *, int))rk_tcdb_hsetnx;
  skel->hexists = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_hexists;
  skel->hlen = (int (*)(void *, const char *, int))rk_tcdb_hlen;
  skel->sadd = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_sadd;
  skel->srem = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_srem;
  skel->scard = (int (*)(void *, const char *, int))rk_tcdb_scard;
  skel->sismember = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_sismember;
  skel->smembers = (rk_listval_t *(*)(void *, const char *, int))rk_tcdb_smembers;
  skel->llen = (int (*)(void *, const char *, int))rk_tcdb_llen;
  skel->lpush = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_lpush;
  skel->rpush = (int (*)(void *, const char *, int, const char *, int))rk_tcdb_rpush;
  skel->lpop = (char *(*)(void *, const char *, int, int *))rk_tcdb_lpop;
  skel->rpop = (char *(*)(void *, const char *, int, int *))rk_tcdb_rpop;
  skel->lrange = (rk_listval_t *(*)(void *, const char *, int, int, int))rk_tcdb_lrange;
}

rk_tcdb_t *rk_tcdb_new(void) {
  rk_tcdb_t *db = malloc(sizeof(*db));
  db->open = false;
  db->str = NULL;
  db->hsh = NULL;
  db->set = NULL;
  db->lst = NULL;
  return db;
}

void rk_tcdb_free(rk_tcdb_t *db) {
  assert(db);
  if (db->open) rk_tcdb_close(db);
  free(db);  
}

bool rk_tcdb_open(rk_tcdb_t *db, const char *path) {
  assert(db && path);
  bool err = false;
  int omode;
  TCHDB *str = NULL;
  TCTDB *hsh = NULL;
  TCTDB *set = NULL;
  TCBDB *lst = NULL;
  omode = HDBOWRITER | HDBOCREAT | HDBOLCKNB;
  str = tchdbnew();
  if (!tchdbopen(str, path, omode)) err = true;
  if (!err) {
    char *hpath = tcsprintf("%s.hash", path);
    omode = TDBOWRITER | TDBOCREAT | TDBOLCKNB;
    hsh = tctdbnew();
    if (!tctdbopen(hsh, hpath, omode)) err = true;
    free(hpath);
  }
  if (!err) {
    char *spath = tcsprintf("%s.set", path);
    omode = TDBOWRITER | TDBOCREAT | TDBOLCKNB;
    set = tctdbnew();
    if (!tctdbopen(set, spath, omode)) err = true;
    free(spath);
  }
  if (!err) {
    char *lpath = tcsprintf("%s.list", path);
    omode = BDBOWRITER | BDBOCREAT | BDBOLCKNB;
    lst = tcbdbnew();
    if (!tcbdbopen(lst, lpath, omode)) err = true;
    free(lpath);
  }
  if (!err) {
    db->str = str;
    db->hsh = hsh;
    db->set = set;
    db->lst = lst;
    db->open = true;
  }
  else {
    if (str) tchdbdel(str);
    if (hsh) tctdbdel(hsh);
    if (set) tctdbdel(set);
    if (lst) tcbdbdel(lst); 
  }
  return !err;
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
  if (!tcbdbclose(db->lst)) err = true;
  tcbdbdel(db->lst);
  db->str = NULL;
  db->hsh = NULL;
  db->set = NULL;
  db->lst = NULL;
  db->open = false;
  return !err;
}

int rk_tcdb_del(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz);
  if (!db->open) return -1;
  if (!tchdbout(db->str, kbuf, ksiz)) {
    if (tchdbecode(db->str) != TCENOREC) return -1;
  }
  else return 1;
  if (!tctdbout(db->hsh, kbuf, ksiz)) {
    if (tctdbecode(db->hsh) != TCENOREC) return -1;
  }
  else return 1;
  if (!tctdbout(db->set, kbuf, ksiz)) {
    if (tctdbecode(db->set) != TCENOREC) return -1;
  }
  else return 1;
  if (!tcbdbout3(db->lst, kbuf, ksiz)) {
    if (tcbdbecode(db->lst) != TCENOREC) return -1;
  }
  else return 1;
  return 0;
}

int rk_tcdb_exists(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  int type;
  return rk_tcdb_obj_search(db, kbuf, ksiz, &type);
}

char *rk_tcdb_type(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  switch (type) {
    case RK_TCDB_STRING: return tcsprintf("%s", "string");
    case RK_TCDB_LIST: return tcsprintf("%s", "list");
    case RK_TCDB_SET: return tcsprintf("%s", "set");
    case RK_TCDB_ZSET: return tcsprintf("%s", "zset");
    case RK_TCDB_HASH: return tcsprintf("%s", "hash");
  }
  return tcsprintf("%s", "none");
}

char *rk_tcdb_get(rk_tcdb_t *db, const char *kbuf, int ksiz, int *sp) {
  assert(db && kbuf && ksiz >= 0 && sp);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) return NULL;
  return tchdbget(db->str, kbuf, ksiz, sp);
}

bool rk_tcdb_set(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return false;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) {
    /* According to Redis spec "if key already holds a value, it is
       overwritten, regardless of its type" */
    if (!rk_tcdb_del(db, kbuf, ksiz)) return false;
  }
  return tchdbput(db->str, kbuf, ksiz, vbuf, vsiz);
}

int rk_tcdb_setnx(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) {
    /* SETNX behaves the same than SET regarding overwriting */
    if (!rk_tcdb_del(db, kbuf, ksiz)) return -1;
  }
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
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return INT_MIN;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) return INT_MIN;
  return rk_tcdb_add_int(db, kbuf, ksiz, 1);
}

int64_t rk_tcdb_decr(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return INT_MIN;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return INT_MIN;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) return INT_MIN;
  return rk_tcdb_add_int(db, kbuf, ksiz, -1);
}

int64_t rk_tcdb_incrby(rk_tcdb_t *db, const char *kbuf, int ksiz, int64_t inc) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return INT_MIN;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return INT_MIN;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) return INT_MIN;
  return rk_tcdb_add_int(db, kbuf, ksiz, inc);
}

int64_t rk_tcdb_decrby(rk_tcdb_t *db, const char *kbuf, int ksiz, int64_t dec) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return INT_MIN;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return INT_MIN;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) return INT_MIN;
  return rk_tcdb_add_int(db, kbuf, ksiz, -dec);
}

char *rk_tcdb_getset(rk_tcdb_t *db, const char *kbuf, int ksiz,
                     const char *vbuf, int vsiz, int *sp) {
  assert(db && kbuf && ksiz >= 0 && vbuf && vsiz >= 0 && sp);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  if (!RKTCDBOCHECK(type, RK_TCDB_STRING)) return NULL;
  int siz;
  char *buf = tchdbget(db->str, kbuf, ksiz, &siz);
  if (!tchdbput(db->str, kbuf, ksiz, vbuf, vsiz)) {
    if (buf) free(buf);
    return NULL;
  }
  if (buf) *sp = siz;
  return buf;
}

char *rk_tcdb_hget(rk_tcdb_t *db, const char *kbuf, int ksiz,
                   const char *fbuf, int fsiz, int *sp) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && sp);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  if (!RKTCDBOCHECK(type, RK_TCDB_HASH)) return NULL;
  return rk_tcdb_hash_get(db->hsh, kbuf, ksiz, fbuf, fsiz, sp);
}

int rk_tcdb_hset(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *fbuf, int fsiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_HASH)) return -1;
  return rk_tcdb_hash_put(db->hsh, kbuf, ksiz, fbuf, fsiz, vbuf, vsiz);
}

int rk_tcdb_hdel(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *fbuf, int fsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_HASH)) return -1;
  return rk_tcdb_hash_out(db->hsh, kbuf, ksiz, fbuf, fsiz);
}

int rk_tcdb_hsetnx(rk_tcdb_t *db, const char *kbuf, int ksiz,
                   const char *fbuf, int fsiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0 && vbuf && vsiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_HASH)) return -1;
  return rk_tcdb_hash_put_keep(db->hsh, kbuf, ksiz, fbuf, fsiz, vbuf, vsiz);  
}

int rk_tcdb_hexists(rk_tcdb_t *db, const char *kbuf, int ksiz,
                    const char *fbuf, int fsiz) {
  assert(db && kbuf && ksiz >= 0 && fbuf && fsiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_HASH)) return -1;
  return rk_tcdb_hash_exists(db->hsh, kbuf, ksiz, fbuf, fsiz);
}

int rk_tcdb_hlen(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_HASH)) return -1;
  return rk_tcdb_hash_rnum(db->hsh, kbuf, ksiz);  
}

int rk_tcdb_sadd(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *mbuf, int msiz) {
  assert(db && kbuf && ksiz >= 0 && mbuf && msiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_SET)) return -1;
  return rk_tcdb_hash_put(db->set, kbuf, ksiz, mbuf, msiz, "", 0);              
}

int rk_tcdb_srem(rk_tcdb_t *db, const char *kbuf, int ksiz,
                 const char *mbuf, int msiz) {
  assert(db && kbuf && ksiz >= 0 && mbuf && msiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_SET)) return -1;
  return rk_tcdb_hash_out(db->set, kbuf, ksiz, mbuf, msiz);
}

int rk_tcdb_scard(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_SET)) return -1;
  return rk_tcdb_hash_rnum(db->set, kbuf, ksiz);
}

int rk_tcdb_sismember(rk_tcdb_t *db, const char *kbuf, int ksiz,
                      const char *mbuf, int msiz) {
  assert(db && kbuf && ksiz >= 0 && mbuf && msiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_SET)) return -1;
  return rk_tcdb_hash_exists(db->set, kbuf, ksiz, mbuf, msiz);
}

rk_listval_t *rk_tcdb_smembers(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  if (!RKTCDBOCHECK(type, RK_TCDB_SET)) return NULL;
  rk_val_t *ary = NULL;
  TCLIST *keys = rk_tcdb_hash_keys(db->set, kbuf, ksiz);
  int num = 0;
  if (keys != NULL) {
    int siz = tclistnum(keys);
    if (siz > 0) {
      ary = malloc(siz*sizeof(rk_val_t));
      int i;
      for (i = 0; i < siz; i++) {
        int vsiz;
        const char *vbuf = tclistval(keys, i, &vsiz);
        ary[i].buf = tcmemdup(vbuf, vsiz);
        ary[i].siz = vsiz;
      }
      num = siz;
    }
    tclistdel(keys);
  }
  rk_listval_t *lval = NULL;
  if (ary != NULL) {
    lval = malloc(sizeof(*lval));
    lval->ary = ary;
    lval->num = num;
  }
  return lval;
}

int rk_tcdb_llen(rk_tcdb_t *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_LIST)) return -1;
  return tcbdbvnum(db->lst, kbuf, ksiz);
}

int rk_tcdb_lpush(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_LIST)) return -1;
  if (!tcbdbputdupback(db->lst, kbuf, ksiz, vbuf, vsiz)) return -1;
  return tcbdbvnum(db->lst, kbuf, ksiz);
}

int rk_tcdb_rpush(rk_tcdb_t *db, const char *kbuf, int ksiz, const char *vbuf, int vsiz) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return -1;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return -1;
  if (!RKTCDBOCHECK(type, RK_TCDB_LIST)) return -1;
  if (!tcbdbputdup(db->lst, kbuf, ksiz, vbuf, vsiz)) return -1;
  return tcbdbvnum(db->lst, kbuf, ksiz);
}

char *rk_tcdb_lpop(rk_tcdb_t *db, const char *kbuf, int ksiz, int *sp) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  if (!RKTCDBOCHECK(type, RK_TCDB_LIST)) return NULL;
  int vsiz;
  char *vbuf = NULL;
  BDBCUR *cur = tcbdbcurnew(db->lst);
  if (tcbdbcurjump(cur, kbuf, ksiz)) {
    int siz;
    const void *buf = tcbdbcurkey3(cur, &siz);
    if (siz == ksiz && !memcmp(buf, kbuf, ksiz)) {
      vbuf = tcbdbcurval(cur, &vsiz);
      if (!tcbdbcurout(cur)) {
        if (vbuf) free(vbuf);
        vbuf = NULL;
      }
    }
  }
  tcbdbcurdel(cur);
  if (vbuf != NULL) *sp = vsiz;
  return vbuf;
}

char *rk_tcdb_rpop(rk_tcdb_t *db, const char *kbuf, int ksiz, int *sp) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  if (!RKTCDBOCHECK(type, RK_TCDB_LIST)) return NULL;
  int vsiz;
  char *vbuf = NULL;
  BDBCUR *cur = tcbdbcurnew(db->lst);
  if (tcbdbcurjumpback(cur, kbuf, ksiz)) {
    int siz;
    const void *buf = tcbdbcurkey3(cur, &siz);
    if (siz == ksiz && !memcmp(buf, kbuf, ksiz)) {
      vbuf = tcbdbcurval(cur, &vsiz);
      if (!tcbdbcurout(cur)) {
        if (vbuf) free(vbuf);
        vbuf = NULL;
      }
    }
  }
  tcbdbcurdel(cur);
  if (vbuf != NULL) *sp = vsiz;
  return vbuf;
}

rk_listval_t *rk_tcdb_lrange(rk_tcdb_t *db, const char *kbuf, int ksiz,
                             int start, int stop) {
  assert(db && kbuf && ksiz >= 0);
  if (!db->open) return NULL;
  int type;
  if (rk_tcdb_obj_search(db, kbuf, ksiz, &type) < 0) return NULL;
  if (!RKTCDBOCHECK(type, RK_TCDB_LIST)) return NULL;
  rk_val_t *ary = NULL;
  int num = 0;
  TCLIST *vals = tcbdbget4(db->lst, kbuf, ksiz);
  int llen = (vals != NULL) ? tclistnum(vals) : 0;
  if (llen > 0 && start < llen && stop >= -llen) {
    int low = start < 0 ? tclmax(start, -llen) + llen : start;
    int high = stop < 0 ? stop + llen : tclmin(stop, llen-1);
    if (high >= low) {
      int siz = high - low + 1;
      ary = malloc(siz*sizeof(rk_val_t));
      int i;
      for (i = 0; i < siz; i++) {
        int vsiz;
        const char *vbuf = tclistval(vals, i + low, &vsiz);
        ary[i].buf = tcmemdup(vbuf, vsiz);
        ary[i].siz = vsiz;
      }
      num = siz;
    }
  }
  if (vals) tclistdel(vals);
  rk_listval_t *lval = NULL;
  if (ary != NULL) {
    lval = malloc(sizeof(*lval));
    lval->ary = ary;
    lval->num = num;
  }
  return lval;
}

static int rk_tcdb_obj_search(rk_tcdb_t *db, const char *kbuf, int ksiz, int *type) {
  assert(db && kbuf && ksiz >= 0 && type);
  if (!tchdbiterinit2(db->str, kbuf, ksiz)) {
    if (tchdbecode(db->str) != TCENOREC) return -1;
  }
  else {
    *type = RK_TCDB_STRING;
    return 1;
  }
  if (!tctdbiterinit2(db->hsh, kbuf, ksiz)) {
    if (tctdbecode(db->hsh) != TCENOREC) return -1;
  }
  else {
    *type = RK_TCDB_HASH;
    return 1;
  }
  if (!tctdbiterinit2(db->set, kbuf, ksiz)) {
    if (tctdbecode(db->set) != TCENOREC) return -1;
  }
  else {
    *type = RK_TCDB_SET;
    return 1;
  }
  BDBCUR *cur = tcbdbcurnew(db->lst);
  if (tcbdbcurjump(cur, kbuf, ksiz)) {
    int siz;
    const void *buf = tcbdbcurkey3(cur, &siz);
    if (siz == ksiz && !memcmp(buf, kbuf, ksiz)) {
      tcbdbcurdel(cur);
      *type = RK_TCDB_LIST;
      return 1;
    }
  }
  tcbdbcurdel(cur);
  *type = RK_TCDB_NONE;
  return 0;  
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

static TCLIST *rk_tcdb_hash_keys(TCTDB *db, const char *kbuf, int ksiz) {
  assert(db && kbuf && ksiz >= 0);
  TCMAP *cols = NULL;
  if (!tctdbiterinit2(db, kbuf, ksiz)) return NULL;
  else {
    cols = tctdbiternext3(db);
    tcmapout2(cols, "");
  }
  if (!cols) return NULL;
  TCLIST *keys = tcmapkeys(cols);
  tcmapdel(cols);
  return keys;
}
