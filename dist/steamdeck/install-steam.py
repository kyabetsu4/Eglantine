#!/usr/bin/env python3
import sys
import os
import shutil
import struct
import zlib
import time
import hashlib
from pathlib import Path

def decode_vdf(data: bytes) -> dict:
    offset = 0
    def read_string():
        nonlocal offset
        end = data.index(b'\0', offset)
        value = data[offset:end].decode('utf-8', errors='replace')
        offset = end + 1
        return value

    def read_obj():
        nonlocal offset
        result = {}
        while True:
            kind = data[offset]
            offset += 1
            if kind == 8:
                return result
            key = read_string()
            if kind == 0:
                result[key] = read_obj()
            elif kind == 1:
                result[key] = read_string()
            elif kind == 2:
                result[key] = struct.unpack_from('<I', data, offset)[0]
                offset += 4
            else:
                raise ValueError(f'Unsupported VDF type {kind} at offset {offset}')
    obj = read_obj()
    assert offset == len(data), f'Trailing bytes in VDF: {len(data) - offset}'
    return obj

def encode_vdf(value: dict) -> bytes:
    out = bytearray()
    for key, item in value.items():
        kind = 0 if isinstance(item, dict) else 1 if isinstance(item, str) else 2
        out.extend(bytes([kind]) + key.encode('utf-8') + b'\0')
        if kind == 0:
            out.extend(encode_vdf(item))
        elif kind == 1:
            out.extend(item.encode('utf-8') + b'\0')
        else:
            out.extend(struct.pack('<I', item))
    return bytes(out) + b'\x08'

def main():
    root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parent
    launch_sh = root / 'launch.sh'
    if not launch_sh.is_file():
        launch_sh = root / 'launch-pokopia.py'

    quoted_exe = f'"{launch_sh}"'
    appid = zlib.crc32((quoted_exe + 'Pokopia').encode('utf-8')) | 0x80000000
    print(f"Target directory: {root}")
    print(f"Generated AppID: {appid}")

    steam_roots = [
        Path.home() / '.local' / 'share' / 'Steam',
        Path.home() / '.steam' / 'steam',
        Path.home() / '.var' / 'app' / 'com.valvesoftware.Steam' / '.local' / 'share' / 'Steam',
    ]
    steam_dir = None
    for sr in steam_roots:
        if (sr / 'userdata').is_dir():
            steam_dir = sr
            break

    if not steam_dir:
        print("Notice: Steam installation not found in standard user paths. Skipping shortcuts.vdf injection.")
    else:
        userdata = steam_dir / 'userdata'
        user_dirs = [d for d in userdata.iterdir() if d.is_dir() and (d / 'config').is_dir()]
        if not user_dirs:
            print("Notice: No Steam user profiles found in userdata. Skipping shortcuts.vdf injection.")
        else:
            for udir in user_dirs:
                cfg = udir / 'config'
                grid = cfg / 'grid'
                grid.mkdir(parents=True, exist_ok=True)
                target_vdf = cfg / 'shortcuts.vdf'

                art_map = {
                    f'{appid}p.png': 'cover-full.png',
                    f'{appid}.jpg': 'wide.jpg',
                    f'{appid}_hero.jpg': 'hero.jpg',
                    f'{appid}_logo.png': 'logo.png',
                }
                art_src = root / 'artwork'
                if art_src.is_dir():
                    for dst_name, src_name in art_map.items():
                        src_path = art_src / src_name
                        if src_path.is_file():
                            shutil.copy2(src_path, grid / dst_name)

                shortcuts_data = {'shortcuts': {}}
                if target_vdf.is_file() and target_vdf.stat().st_size > 0:
                    raw = target_vdf.read_bytes()
                    try:
                        shortcuts_data = decode_vdf(raw)
                        if encode_vdf(shortcuts_data) == raw:
                            bak_path = cfg / f'shortcuts.vdf.bak-{time.strftime("%Y%m%d-%H%M%S")}'
                            shutil.copy2(target_vdf, bak_path)
                    except Exception as e:
                        print(f"Warning: Failed to decode existing shortcuts.vdf ({e}). Creating backup.")
                        shutil.copy2(target_vdf, cfg / 'shortcuts.vdf.corrupt.bak')
                        shortcuts_data = {'shortcuts': {}}

                shortcuts = shortcuts_data.setdefault('shortcuts', {})
                existing_key = None
                for k, v in shortcuts.items():
                    if isinstance(v, dict) and v.get('appname', '').lower() == 'pokopia':
                        existing_key = k
                        break

                new_entry = {
                    'appid': appid,
                    'appname': 'Pokopia',
                    'exe': quoted_exe,
                    'StartDir': f'"{root}"',
                    'icon': str(grid / f'{appid}p.png'),
                    'ShortcutPath': '',
                    'LaunchOptions': '',
                    'IsHidden': 0,
                    'AllowDesktopConfig': 1,
                    'AllowOverlay': 1,
                    'OpenVR': 0,
                    'Devkit': 0,
                    'DevkitGameID': '',
                    'DevkitOverrideAppID': 0,
                    'LastPlayTime': 0,
                    'FlatpakAppID': '',
                    'tags': {}
                }

                if existing_key is not None:
                    shortcuts[existing_key] = new_entry
                    print(f"Updated existing Pokopia entry for Steam profile {udir.name}")
                else:
                    next_idx = str(max([int(k) for k in shortcuts.keys() if k.isdigit()], default=-1) + 1)
                    shortcuts[next_idx] = new_entry
                    print(f"Added Pokopia shortcut (index {next_idx}) to Steam profile {udir.name}")

                encoded = encode_vdf(shortcuts_data)
                tmp_file = target_vdf.with_name('shortcuts.pokopia.tmp')
                tmp_file.write_bytes(encoded)
                tmp_file.replace(target_vdf)
                print(f"Steam shortcut and artwork successfully applied for user {udir.name}!")

    desktop_dir = Path.home() / 'Desktop'
    apps_dir = Path.home() / '.local' / 'share' / 'applications'
    icon_path = root / 'artwork' / 'logo.png'

    desktop_content = f"""[Desktop Entry]
Version=1.0
Type=Application
Name=Pokopia (60 FPS)
Comment=Play Pokopia with native 60 FPS LSFG on Steam Deck
Exec="{launch_sh}" %u
Icon={icon_path}
Terminal=false
Categories=Game;Emulator;
StartupNotify=true
"""

    for out_dir in [desktop_dir, apps_dir]:
        if out_dir.is_dir():
            shortcut_file = out_dir / 'Pokopia.desktop'
            shortcut_file.write_text(desktop_content, encoding='utf-8')
            shortcut_file.chmod(0o755)
            print(f"Created desktop shortcut at: {shortcut_file}")

if __name__ == '__main__':
    main()
