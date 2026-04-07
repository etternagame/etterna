# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'conditions': [
      [ 'OS=="none"', {
        # Because we have a patched zlib, we cannot use the system libz.
        # TODO(pvalchev): OpenBSD is purposefully left out, as the system
        # zlib brings up an incompatibility that breaks rendering.
        'use_system_zlib%': 1,
      }, {
        'use_system_zlib%': 0,
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'zlib',
      'type': 'static_library',
      'conditions': [
        ['use_system_zlib==0', {
          'sources': [
            'contrib/minizip/ioapi.c',
            'contrib/minizip/ioapi.h',
            'contrib/minizip/iowin32.c',
            'contrib/minizip/iowin32.h',
            'contrib/minizip/unzip.c',
            'contrib/minizip/unzip.h',
            'contrib/minizip/zip.c',
            'contrib/minizip/zip.h',
            'adler32.c',
            'compress.c',
            'crc32.c',
            'crc32.h',
            'deflate.c',
            'deflate.h',
            'gzio.c',
            'infback.c',
            'inffast.c',
            'inffast.h',
            'inffixed.h',
            'inflate.c',
            'inflate.h',
            'inftrees.c',
            'inftrees.h',
            'mozzconf.h',
            'trees.c',
            'trees.h',
            'uncompr.c',
            'zconf.h',
            'zlib.h',
            'zutil.c',
            'zutil.h',
          ],
          'include_dirs': [
            '.',
            # For contrib/minizip
            '../..',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '.',
            ],
          },
          'conditions': [
            ['OS!="win"', {
              'product_name': 'chrome_zlib',
              'sources!': [
                'contrib/minizip/iowin32.c'
              ],
            }], ['OS=="android"', {
              'toolsets': ['target', 'host'],
            }],
          ],
        }, {
          'direct_dependent_settings': {
            'defines': [
              'USE_SYSTEM_ZLIB',
            ],
          },
          'defines': [
            'USE_SYSTEM_ZLIB',
          ],
          'sources': [
            'contrib/minizip/ioapi.c',
            'contrib/minizip/ioapi.h',
            'contrib/minizip/unzip.c',
            'contrib/minizip/unzip.h',
            'contrib/minizip/zip.c',
            'contrib/minizip/zip.h',
          ],
          'conditions': [
            ['OS=="android"', {
              'toolsets': ['target', 'host'],
            }],
          ],
          'link_settings': {
            'libraries': [
              '-lz',
            ],
          },
        }],
        ['OS=="mac" or OS=="ios" or os_bsd==1 or OS=="android"', {
          # Mac, Android and the BSDs don't have fopen64, ftello64, or
          # fseeko64. We use fopen, ftell, and fseek instead on these
          # systems.
          'defines': [
            'USE_FILE32API'
          ],
        }],
        ['clang==1', {
          'xcode_settings': {
            'WARNING_CFLAGS': [
              # zlib uses `if ((a == b))` for some reason.
              '-Wno-parentheses-equality',
            ],
          },
          'cflags': [
            '-Wno-parentheses-equality',
          ],
        }],
      ],
    }
  ],
}
