#include "lj_fopen.h"

/*
 * Forces to look for UTF-8 paths on Windows
 * (Non-standard optional feature intended for convenience to embed LuaJIT into cross-platform applications)
 */
#if defined(_WIN32) && defined(LUAJIT_FORCE_UTF8_FOPEN)
#include <windows.h>

FILE *_lua_fopen(const char *filename, const char *mode)
{
  int fn_len_s = strlen(filename), m_len_s = strlen(mode), fn_len_w = 0, m_len_w = 0;
  wchar_t path[MAX_PATH], wmode[MAX_PATH];

  if (fn_len_s == 0 || m_len_s == 0)
    return NULL;

  fn_len_w = MultiByteToWideChar(CP_UTF8, 0, filename, fn_len_s, path, fn_len_s);
  if (fn_len_w >= MAX_PATH) return NULL;
  path[fn_len_w] = L'\0';

  m_len_w = MultiByteToWideChar(CP_UTF8, 0, mode, m_len_s, wmode, m_len_s);
  if (m_len_w >= MAX_PATH) return NULL;
  wmode[m_len_w] = L'\0';

  return _wfopen(path, wmode);
}

FILE *_lua_freopen(const char *filename, const char *mode, FILE * oldfile)
{
  int fn_len_s = strlen(filename), m_len_s = strlen(mode), fn_len_w = 0, m_len_w = 0;
  wchar_t path[MAX_PATH], wmode[MAX_PATH];

  if (fn_len_s == 0 || m_len_s == 0)
    return NULL;

  fn_len_w = MultiByteToWideChar(CP_UTF8, 0, filename, fn_len_s, path, fn_len_s);
  if (fn_len_w >= MAX_PATH) return NULL;
  path[fn_len_w] = L'\0';

  m_len_w = MultiByteToWideChar(CP_UTF8, 0, mode, m_len_s, wmode, m_len_s);
  if (m_len_w >= MAX_PATH) return NULL;
  wmode[m_len_w] = L'\0';

  return _wfreopen(path, wmode, oldfile);
}

#endif
