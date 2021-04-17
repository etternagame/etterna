/*
** Object de/serialization.
** Copyright (C) 2005-2021 Mike Pall. See Copyright Notice in luajit.h
*/

#define lj_serialize_c
#define LUA_CORE

#include "lj_obj.h"

#if LJ_HASBUFFER
#include "lj_err.h"
#include "lj_buf.h"
#include "lj_str.h"
#include "lj_tab.h"
#include "lj_udata.h"
#if LJ_HASFFI
#include "lj_ctype.h"
#include "lj_cdata.h"
#endif
#include "lj_serialize.h"

/* Tags for internal serialization format. */
enum {
  SER_TAG_NIL,		/* 0x00 */
  SER_TAG_FALSE,
  SER_TAG_TRUE,
  SER_TAG_NULL,
  SER_TAG_LIGHTUD32,
  SER_TAG_LIGHTUD64,
  SER_TAG_INT,
  SER_TAG_NUM,
  SER_TAG_TAB,		/* 0x08 */
  SER_TAG_0x0e = SER_TAG_TAB+6,
  SER_TAG_0x0f,
  SER_TAG_INT64,	/* 0x10 */
  SER_TAG_UINT64,
  SER_TAG_COMPLEX,
  SER_TAG_0x13,
  SER_TAG_0x14,
  SER_TAG_0x15,
  SER_TAG_0x16,
  SER_TAG_0x17,
  SER_TAG_0x18,		/* 0x18 */
  SER_TAG_0x19,
  SER_TAG_0x1a,
  SER_TAG_0x1b,
  SER_TAG_0x1c,
  SER_TAG_0x1d,
  SER_TAG_0x1e,
  SER_TAG_0x1f,
  SER_TAG_STR,		/* 0x20 + str->len */
};
LJ_STATIC_ASSERT((SER_TAG_TAB & 7) == 0);

/* -- Helper functions ---------------------------------------------------- */

static LJ_AINLINE char *serialize_more(char *w, StrBuf *sbuf, MSize sz)
{
  if (LJ_UNLIKELY(sz > (MSize)(sbufE(sbuf->sb) - w))) {
    setsbufP(sbuf->sb, w);
    w = lj_buf_more2(sbuf->sb, sz);
  }
  return w;
}

/* Write U124 to buffer. */
static LJ_NOINLINE char *serialize_wu124_(char *w, uint32_t v)
{
  if (v < 0x1fe0) {
    v -= 0xe0;
    *w++ = (char)(0xe0 | (v >> 8)); *w++ = (char)v;
  } else {
    *w++ = (char)0xff;
#if LJ_BE
    v = lj_bswap(v);
#endif
    memcpy(w, &v, 4); w += 4;
  }
  return w;
}

static LJ_AINLINE char *serialize_wu124(char *w, uint32_t v)
{
  if (LJ_LIKELY(v < 0xe0)) {
    *w++ = (char)v;
    return w;
  } else {
    return serialize_wu124_(w, v);
  }
}

static LJ_NOINLINE char *serialize_ru124_(char *r, char *e, uint32_t *pv)
{
  uint32_t v = *pv;
  if (v != 0xff) {
    if (r >= e) return NULL;
    v = ((v & 0x1f) << 8) + *(uint8_t *)r + 0xe0; r++;
  } else {
    if (r + 4 > e) return NULL;
    v = lj_getu32(r); r += 4;
#if LJ_BE
    v = lj_bswap(v);
#endif
  }
  *pv = v;
  return r;
}

static LJ_AINLINE char *serialize_ru124(char *r, char *e, uint32_t *pv)
{
  if (LJ_LIKELY(r < e)) {
    uint32_t v = *(uint8_t *)r; r++;
    *pv = v;
    if (LJ_UNLIKELY(v >= 0xe0)) {
      r = serialize_ru124_(r, e, pv);
    }
    return r;
  }
  return NULL;
}

/* -- Internal serializer ------------------------------------------------- */

/* Put serialized object into buffer. */
static char *serialize_put(char *w, StrBuf *sbuf, cTValue *o)
{
  if (LJ_LIKELY(tvisstr(o))) {
    const GCstr *str = strV(o);
    MSize len = str->len;
    w = serialize_more(w, sbuf, 5+len);
    w = serialize_wu124(w, SER_TAG_STR + len);
    w = lj_buf_wmem(w, strdata(str), len);
  } else if (tvisint(o)) {
    uint32_t x = LJ_BE ? lj_bswap((uint32_t)intV(o)) : (uint32_t)intV(o);
    w = serialize_more(w, sbuf, 1+4);
    *w++ = SER_TAG_INT; memcpy(w, &x, 4); w += 4;
  } else if (tvisnum(o)) {
    uint64_t x = LJ_BE ? lj_bswap64(o->u64) : o->u64;
    w = serialize_more(w, sbuf, 1+sizeof(lua_Number));
    *w++ = SER_TAG_NUM; memcpy(w, &x, 8); w += 8;
  } else if (tvispri(o)) {
    w = serialize_more(w, sbuf, 1);
    *w++ = (char)(SER_TAG_NIL + ~itype(o));
  } else if (tvistab(o)) {
    const GCtab *t = tabV(o);
    uint32_t narray = 0, nhash = 0, one = 2;
    if (sbuf->depth <= 0) lj_err_caller(sbufL(sbuf->sb), LJ_ERR_BUFFER_DEPTH);
    sbuf->depth--;
    if (t->asize > 0) {  /* Determine max. length of array part. */
      ptrdiff_t i;
      TValue *array = tvref(t->array);
      for (i = (ptrdiff_t)t->asize-1; i >= 0; i--)
	if (!tvisnil(&array[i]))
	  break;
      narray = (uint32_t)(i+1);
      if (narray && tvisnil(&array[0])) one = 4;
    }
    if (t->hmask > 0) {  /* Count number of used hash slots. */
      uint32_t i, hmask = t->hmask;
      Node *node = noderef(t->node);
      for (i = 0; i <= hmask; i++)
	nhash += !tvisnil(&node[i].val);
    }
    /* Write number of array slots and hash slots. */
    w = serialize_more(w, sbuf, 1+2*5);
    *w++ = (char)(SER_TAG_TAB + (nhash ? 1 : 0) + (narray ? one : 0));
    if (narray) w = serialize_wu124(w, narray);
    if (nhash) w = serialize_wu124(w, nhash);
    if (narray) {  /* Write array entries. */
      cTValue *oa = tvref(t->array) + (one >> 2);
      cTValue *oe = tvref(t->array) + narray;
      while (oa < oe) w = serialize_put(w, sbuf, oa++);
    }
    if (nhash) {  /* Write hash entries. */
      const Node *node = noderef(t->node) + t->hmask;
      for (;; node--)
	if (!tvisnil(&node->val)) {
	  w = serialize_put(w, sbuf, &node->key);
	  w = serialize_put(w, sbuf, &node->val);
	  if (--nhash == 0) break;
	}
    }
    sbuf->depth++;
#if LJ_HASFFI
  } else if (tviscdata(o)) {
    CTState *cts = ctype_cts(sbufL(sbuf->sb));
    CType *s = ctype_raw(cts, cdataV(o)->ctypeid);
    uint8_t *sp = cdataptr(cdataV(o));
    if (ctype_isinteger(s->info) && s->size == 8) {
      w = serialize_more(w, sbuf, 1+8);
      *w++ = (s->info & CTF_UNSIGNED) ? SER_TAG_UINT64 : SER_TAG_INT64;
#if LJ_BE
      { uint64_t u = lj_bswap64(*(uint64_t *)sp); memcpy(w, &u, 8); }
#else
      memcpy(w, sp, 8);
#endif
      w += 8;
    } else if (ctype_iscomplex(s->info) && s->size == 16) {
      w = serialize_more(w, sbuf, 1+16);
      *w++ = SER_TAG_COMPLEX;
#if LJ_BE
      {  /* Only swap the doubles. The re/im order stays the same. */
	uint64_t u = lj_bswap64(((uint64_t *)sp)[0]); memcpy(w, &u, 8);
	u = lj_bswap64(((uint64_t *)sp)[1]); memcpy(w+8, &u, 8);
      }
#else
      memcpy(w, sp, 16);
#endif
      w += 16;
    } else {
      goto badenc;  /* NYI other cdata */
    }
#endif
  } else if (tvislightud(o)) {
    uintptr_t ud = (uintptr_t)lightudV(G(sbufL(sbuf->sb)), o);
    w = serialize_more(w, sbuf, 1+sizeof(ud));
    if (ud == 0) {
      *w++ = SER_TAG_NULL;
    } else if (LJ_32 || checku32(ud)) {
#if LJ_BE && LJ_64
      ud = lj_bswap64(ud);
#elif LJ_BE
      ud = lj_bswap(ud);
#endif
      *w++ = SER_TAG_LIGHTUD32; memcpy(w, &ud, 4); w += 4;
    } else {
#if LJ_BE
      ud = lj_bswap64(ud);
#endif
      *w++ = SER_TAG_LIGHTUD64; memcpy(w, &ud, 8); w += 8;
    }
  } else {
    /* NYI userdata */
#if LJ_HASFFI
  badenc:
#endif
    lj_err_callerv(sbufL(sbuf->sb), LJ_ERR_BUFFER_BADENC, lj_typename(o));
  }
  return w;
}

/* Get serialized object from buffer. */
static char *serialize_get(char *r, StrBuf *sbuf, TValue *o)
{
  char *e = sbufE(sbuf->sb);
  uint32_t tp;
  r = serialize_ru124(r, e, &tp); if (LJ_UNLIKELY(!r)) goto eob;
  if (LJ_LIKELY(tp >= SER_TAG_STR)) {
    uint32_t len = tp - SER_TAG_STR;
    if (LJ_UNLIKELY(len > (uint32_t)(e - r))) goto eob;
    setstrV(sbufL(sbuf->sb), o, lj_str_new(sbufL(sbuf->sb), r, len));
    r += len;
  } else if (tp == SER_TAG_INT) {
    if (LJ_UNLIKELY(r + 4 > e)) goto eob;
    setintV(o, (int32_t)(LJ_BE ? lj_bswap(lj_getu32(r)) : lj_getu32(r)));
    r += 4;
  } else if (tp == SER_TAG_NUM) {
    if (LJ_UNLIKELY(r + 8 > e)) goto eob;
    memcpy(o, r, 8); r += 8;
#if LJ_BE
    o->u64 = lj_bswap64(o->u64);
#endif
    if (!tvisnum(o)) setnanV(o);
  } else if (tp <= SER_TAG_TRUE) {
    setpriV(o, ~tp);
  } else if (tp >= SER_TAG_TAB && tp < SER_TAG_TAB+6) {
    uint32_t narray = 0, nhash = 0;
    GCtab *t;
    if (tp >= SER_TAG_TAB+2) {
      r = serialize_ru124(r, e, &narray); if (LJ_UNLIKELY(!r)) goto eob;
    }
    if ((tp & 1)) {
      r = serialize_ru124(r, e, &nhash); if (LJ_UNLIKELY(!r)) goto eob;
    }
    t = lj_tab_new(sbufL(sbuf->sb), narray, hsize2hbits(nhash));
    settabV(sbufL(sbuf->sb), o, t);
    if (narray) {
      TValue *oa = tvref(t->array) + (tp >= SER_TAG_TAB+4);
      TValue *oe = tvref(t->array) + narray;
      while (oa < oe) r = serialize_get(r, sbuf, oa++);
    }
    if (nhash) {
      do {
	TValue k, *v;
	r = serialize_get(r, sbuf, &k);
	v = lj_tab_set(sbufL(sbuf->sb), t, &k);
	if (LJ_UNLIKELY(!tvisnil(v)))
	  lj_err_caller(sbufL(sbuf->sb), LJ_ERR_BUFFER_DUPKEY);
	r = serialize_get(r, sbuf, v);
      } while (--nhash);
    }
#if LJ_HASFFI
  } else if (tp >= SER_TAG_INT64 &&  tp <= SER_TAG_COMPLEX) {
    uint32_t sz = tp == SER_TAG_COMPLEX ? 16 : 8;
    GCcdata *cd;
    if (LJ_UNLIKELY(r + sz > e)) goto eob;
    cd = lj_cdata_new_(sbufL(sbuf->sb),
	   tp == SER_TAG_INT64 ? CTID_INT64 :
	   tp == SER_TAG_UINT64 ? CTID_UINT64 : CTID_COMPLEX_DOUBLE,
	   sz);
    memcpy(cdataptr(cd), r, sz); r += sz;
#if LJ_BE
    *(uint64_t *)cdataptr(cd) = lj_bswap64(*(uint64_t *)cdataptr(cd));
    if (sz == 16)
      ((uint64_t *)cdataptr(cd))[1] = lj_bswap64(((uint64_t *)cdataptr(cd))[1]);
#endif
    setcdataV(sbufL(sbuf->sb), o, cd);
#endif
  } else if (tp <= (LJ_64 ? SER_TAG_LIGHTUD64 : SER_TAG_LIGHTUD32)) {
    uintptr_t ud = 0;
    if (tp == SER_TAG_LIGHTUD32) {
      if (LJ_UNLIKELY(r + 4 > e)) goto eob;
      ud = (uintptr_t)(LJ_BE ? lj_bswap(lj_getu32(r)) : lj_getu32(r));
      r += 4;
    }
#if LJ_64
    else if (tp == SER_TAG_LIGHTUD64) {
      if (LJ_UNLIKELY(r + 8 > e)) goto eob;
      memcpy(&ud, r, 8); r += 8;
#if LJ_BE
      ud = lj_bswap64(ud);
#endif
    }
    setrawlightudV(o, lj_lightud_intern(sbufL(sbuf->sb), (void *)ud));
#else
    setrawlightudV(o, (void *)ud);
#endif
  } else {
    lj_err_callerv(sbufL(sbuf->sb), LJ_ERR_BUFFER_BADDEC, tp);
  }
  return r;
eob:
  lj_err_caller(sbufL(sbuf->sb), LJ_ERR_BUFFER_EOB);
  return NULL;
}

StrBuf * LJ_FASTCALL lj_serialize_put(StrBuf *sbuf, cTValue *o)
{
  sbuf->depth = LJ_SERIALIZE_DEPTH;
  setsbufP(sbuf->sb, serialize_put(sbufP(sbuf->sb), sbuf, o));
  return sbuf;
}

StrBuf * LJ_FASTCALL lj_serialize_get(StrBuf *sbuf, TValue *o)
{
  char *r = serialize_get(sbuf->r, sbuf, o);
  if (r != sbufP(sbuf->sb))
    lj_err_caller(sbufL(sbuf->sb), LJ_ERR_BUFFER_LEFTOV);
  sbuf->r = r;
  return sbuf;
}

#endif
