#!/usr/bin/env python3
import os
import sys
import re
import shutil
import subprocess
from pathlib import Path
from datetime import datetime, timezone
from collections import Counter

ROOT = Path('/home/deck/pokopia-upstream')

def valid_name(name):
    return bool(name.strip()) and '\n' not in name and '\r' not in name and 1 <= len(name.encode('utf-16-le')) // 2 <= 12

def capture_session(start_time_utc: datetime, end_time_utc: datetime, exit_code: int, game_path: str):
    session_ts = start_time_utc.strftime('%Y%m%d-%H%M%S-%f')[:19]
    session_dir = ROOT / 'logs' / 'eglantine-sessions' / f'session-{session_ts}'
    session_dir.mkdir(parents=True, exist_ok=True)

    candidate_logs = [
        ROOT / 'test' / 'user' / 'log' / 'eden_log.txt',
        ROOT / 'test' / 'user' / 'log' / 'emulator.log',
        ROOT / 'test' / 'user' / 'log' / 'eglantine_log.txt',
        Path.home() / '.local' / 'share' / 'eden' / 'log' / 'eden_log.txt',
        Path.home() / '.local' / 'share' / 'eglantine' / 'log' / 'eglantine_log.txt',
    ]

    source_log = None
    for cand in candidate_logs:
        if cand.is_file() and cand.stat().st_size > 0:
            if source_log is None or cand.stat().st_mtime > source_log.stat().st_mtime:
                source_log = cand

    dest_log = session_dir / 'emulator.log'
    if source_log:
        try:
            shutil.copy2(source_log, dest_log)
        except Exception as e:
            print(f"Warning: failed to copy log {source_log}: {e}", file=sys.stderr)

    total_lines = 0
    severities = Counter()
    subsystems = Counter()
    signatures = Counter()

    log_pattern = re.compile(r'^(?:\[\s*\d+(?:\.\d+)?\]\s*)?([A-Za-z0-9_.]+)\s+<(Warning|Error|Critical)>\s+(.*)$')

    if dest_log.exists():
        with open(dest_log, 'r', encoding='utf-8', errors='replace') as f:
            for line in f:
                total_lines += 1
                m = log_pattern.match(line)
                if not m:
                    continue
                subsys, sev, msg = m.groups()
                severities[sev] += 1
                subsystems[f'{subsys} <{sev}>'] += 1

                norm_msg = re.sub(r'0x[0-9a-fA-F]+', '0xHEX', msg)
                norm_msg = re.sub(r'\b\d+(?:\.\d+)?\b', 'N', norm_msg)
                norm_msg = ' '.join(norm_msg.split())
                signatures[f'{subsys} <{sev}> {norm_msg}'] += 1

    summary_lines = [
        "# Eglantine / Eden Gameplay Log Summary (Steam Deck)",
        f"GeneratedAtUTC: {datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')}",
        f"StartedAtUTC: {start_time_utc.strftime('%Y-%m-%dT%H:%M:%SZ')}",
        f"EndedAtUTC: {end_time_utc.strftime('%Y-%m-%dT%H:%M:%SZ')}",
        f"ExitCode: {exit_code}",
        f"GamePath: {game_path}",
        f"SourceLog: {source_log if source_log else 'NONE'}",
        f"CapturedSnapshot: {dest_log}",
        f"TotalLines: {total_lines}",
        f"WarningLines: {severities['Warning']}",
        f"ErrorLines: {severities['Error']}",
        f"CriticalLines: {severities['Critical']}",
        f"UniqueSignatures: {len(signatures)}",
        "",
        "## Subsystem Breakdown"
    ]
    for subsys_tag, count in subsystems.most_common():
        summary_lines.append(f"{count:8d} | {subsys_tag}")

    summary_lines.extend(["", "## Top 50 Normalized Warning/Error Frequencies"])
    for sig, count in signatures.most_common(50):
        summary_lines.append(f"{count:8d} | {sig}")

    summary_text = '\n'.join(summary_lines) + '\n'

    try:
        (session_dir / 'summary.log').write_text(summary_text, encoding='utf-8')
        (ROOT / 'summary.log').write_text(summary_text, encoding='utf-8')
    except Exception as e:
        print(f"Warning: failed to write summary.log: {e}", file=sys.stderr)

def get_character_name(saved_file: Path) -> str:
    # 1. Command-line override: --name <name>
    if '--name' in sys.argv:
        idx = sys.argv.index('--name')
        if idx + 1 < len(sys.argv) and valid_name(sys.argv[idx + 1]):
            name = sys.argv[idx + 1]
            try:
                saved_file.write_text(name + '\n', encoding='utf-8')
            except Exception:
                pass
            return name

    # 2. Existing valid name: use immediately without opening any dialog
    if saved_file.is_file() and '--rename' not in sys.argv:
        try:
            name = saved_file.read_text(encoding='utf-8').strip()
            if valid_name(name):
                return name
        except Exception:
            pass

    # 3. Prompt user via kdialog on first run or explicit --rename
    default = 'Hero'
    if saved_file.is_file():
        try:
            default = saved_file.read_text(encoding='utf-8').strip() or 'Hero'
        except Exception:
            pass

    kdialog_bin = shutil.which('kdialog') or '/usr/bin/kdialog'
    if not os.environ.get('DISPLAY') or not Path(kdialog_bin).exists():
        return default if valid_name(default) else 'Hero'

    while True:
        try:
            result = subprocess.run([
                kdialog_bin, '--title', 'Pokopia — Character name',
                '--inputbox', 'Character name (1–12 characters).\nUsed when the game asks for your name.\nThis does not rename an existing character.',
                default
            ], text=True, capture_output=True, timeout=30)
            if result.returncode != 0:
                return default if valid_name(default) else 'Hero'
            name = result.stdout.rstrip('\r\n')
            if valid_name(name):
                try:
                    saved_file.write_text(name + '\n', encoding='utf-8')
                except Exception:
                    pass
                return name
            subprocess.run([kdialog_bin, '--error', 'Enter a nonblank name of 1–12 characters.'], timeout=10)
            default = name
        except Exception:
            return default if valid_name(default) else 'Hero'

def sync_profile_name(name: str):
    profiles_paths = [
        ROOT / 'test/user/nand/system/save/8000000000000010/su/avators/profiles.dat',
        ROOT / 'user/nand/system/save/8000000000000010/su/avators/profiles.dat',
        Path.home() / '.local/share/eden/nand/system/save/8000000000000010/su/avators/profiles.dat',
    ]
    name_bytes = name.encode('utf-8')[:31] + b'\0'
    for p in profiles_paths:
        if p.is_file():
            try:
                data = bytearray(p.read_bytes())
                if len(data) >= 0x58:
                    data[0x38:0x38 + len(name_bytes)] = name_bytes
                    for i in range(0x38 + len(name_bytes), 0x58):
                        data[i] = 0
                    p.write_bytes(data)
            except Exception:
                pass

def main():
    saved = ROOT / 'test/character-name.txt'
    name = get_character_name(saved)
    sync_profile_name(name)

    start_time = datetime.now(timezone.utc)
    shader_dump_dir = ROOT / 'logs' / 'eglantine-shader-dumps' / start_time.strftime('%Y%m%dTHHmmssfffZ')
    shader_dump_dir.mkdir(parents=True, exist_ok=True)

    environment = dict(os.environ)
    environment.update(
        LD_LIBRARY_PATH=str(ROOT / 'runtime/lib'),
        EDEN_NX2_AUTO_STUB='1',
        EDEN_SDL_KEYBOARD_TEXT=name,
        EDEN_NX2_SHADER_DUMP=str(shader_dump_dir),
        EDEN_NX2_FRAMES_AHEAD='a',
    )

    os.chdir(ROOT / 'test')
    config = 'steam-controls.ini' if os.environ.get('SteamAppId', '0') not in ('', '0') else 'deck-controls.ini'
    binary = str(ROOT / 'source/build/deck/bin/eden-cli-name-entry')
    game_path = '/home/deck/Downloads/Pokopia/updates/655360/exefs/main'
    cmd = [binary, '-f', '-c', str(ROOT / 'test' / config), '-g', game_path]

    exit_code = 0
    try:
        proc = subprocess.run(cmd, env=environment)
        exit_code = proc.returncode
    except Exception as e:
        print(f"Error running emulator: {e}", file=sys.stderr)
        exit_code = -1
    finally:
        end_time = datetime.now(timezone.utc)
        capture_session(start_time, end_time, exit_code, game_path)

if __name__ == '__main__':
    main()
