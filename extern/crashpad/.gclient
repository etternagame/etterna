target_os = ["mac", "win", "unix"];
solutions = [
  {
    "custom_vars": { "pull_linux_clang": True },
    "name": "crashpad",
    "url": "https://chromium.googlesource.com/crashpad/crashpad.git",
    "managed": False,
  },
];
