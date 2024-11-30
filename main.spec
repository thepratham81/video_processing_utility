# -*- mode: python ; coding: utf-8 -*-
from PyInstaller.utils.hooks import collect_submodules

import platform
if platform.system() != "Linux":
    from kivy_deps import sdl2, glew
from kivymd import hooks_path as kivymd_hooks_path
hiddenimports = []
hiddenimports += collect_submodules('plyer')

a = Analysis(
    ['main.py'],
    pathex=[],
    binaries=[],
    datas=[("ui.kv","."),('asset', 'asset')],
    hiddenimports=hiddenimports,
    hookspath=[kivymd_hooks_path],
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.datas,
    *([Tree(p) for p in (sdl2.dep_bins + glew.dep_bins)] if platform.system() != 'Linux' else []),
    [],
    name='main',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
)
