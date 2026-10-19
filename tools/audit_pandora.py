#!/usr/bin/env python3
"""Read-only inventory of a notaz StarCraft PND; never execute its contents."""
import argparse
import hashlib
import json
import struct
from pathlib import Path, PurePosixPath


def elf_header(data):
    if len(data) < 20 or data[:4] != b"\x7fELF":
        raise ValueError("missing or truncated ELF header")
    if data[4] not in (1, 2) or data[5] not in (1, 2):
        raise ValueError("invalid ELF class or endianness")
    bits = 32 if data[4] == 1 else 64
    minimum = 52 if bits == 32 else 64
    if len(data) < minimum:
        raise ValueError("truncated ELF header")
    endian = "<" if data[5] == 1 else ">"
    machine = struct.unpack_from(endian + "H", data, 18)[0]
    return {"format": "ELF", "bits": bits, "machine_id": machine,
            "machine": {40: "ARM", 183: "AArch64", 3: "x86", 62: "x86_64"}.get(machine, "unknown")}


def needed_libraries(data):
    """Read DT_NEEDED names through the ELF dynamic section, without readelf."""
    header = elf_header(data)
    endian = "<" if data[5] == 1 else ">"
    is64 = header["bits"] == 64
    phoff = struct.unpack_from(endian + ("Q" if is64 else "I"), data, 32 if is64 else 28)[0]
    phsize, phnum = struct.unpack_from(endian + "HH", data, 54 if is64 else 42)
    expected = 56 if is64 else 32
    if phsize < expected or phoff + phsize * phnum > len(data):
        raise ValueError("invalid ELF program headers")
    loads, dynamic = [], None
    for index in range(phnum):
        p = phoff + index * phsize
        values = struct.unpack_from(endian + ("IIQQQQQQ" if is64 else "IIIIIIII"), data, p)
        kind = values[0]
        off, addr, length = (values[2], values[3], values[5]) if is64 else (values[1], values[2], values[4])
        if off + length > len(data):
            raise ValueError("segment outside ELF file")
        if kind == 1:
            loads.append((addr, off, length))
        elif kind == 2:
            dynamic = (off, length)
    if dynamic is None:
        return []
    stride, fmt = (16, "QQ") if is64 else (8, "II")
    names, strings, string_size = [], None, None
    off, length = dynamic
    if length % stride:
        raise ValueError("invalid dynamic segment size")
    for p in range(off, off + length, stride):
        tag, value = struct.unpack_from(endian + fmt, data, p)
        if tag == 0:
            break
        if tag == 1:
            names.append(value)
        elif tag == 5:
            strings = value
        elif tag == 10:
            string_size = value
    if not names:
        return []
    if strings is None or string_size is None:
        raise ValueError("missing ELF string table")
    for addr, off, length in loads:
        if addr <= strings and strings + string_size <= addr + length:
            start = off + strings - addr
            table = data[start:start + string_size]
            result = []
            for name in names:
                end = table.find(b"\0", name)
                if name >= len(table) or end < 0:
                    raise ValueError("invalid dynamic library name")
                result.append(table[name:end].decode("utf-8", errors="replace"))
            return result
    raise ValueError("ELF string table outside load segments")


def audit(path):
    from dissect.squashfs import SquashFS
    files, source_files, main_module, launcher = [], [], None, None
    with path.open("rb") as handle:
        fs = SquashFS(handle)

        def visit(directory, prefix=""):
            nonlocal main_module, launcher
            for node in directory.iterdir():
                name = prefix + "/" + node.name
                if node.is_dir():
                    visit(node, name)
                elif node.is_file():
                    files.append(name)
                    if PurePosixPath(name).suffix.lower() in {".c", ".cpp", ".h", ".asm", ".s"}:
                        source_files.append(name)
                    if name == "/libscr_sa_arm.exe.so":
                        with node.open() as stream:
                            data = stream.read()
                        main_module = dict(elf_header(data), needed=needed_libraries(data))
                    if name == "/run.sh":
                        with node.open() as stream:
                            launcher = stream.read().decode("utf-8", errors="replace")

        visit(fs.root)
    return {"package_name": path.name, "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "regular_file_count": len(files), "source_files": sorted(source_files),
            "main_module": main_module,
            "launcher_uses_wine": bool(launcher and "/wine/bin/wine" in launcher),
            "files": sorted(files), "native_switch_port_completed": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("package", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        report = audit(args.package)
    except (ImportError, OSError, ValueError) as exc:
        parser.exit(1, f"audit failed: {exc}\n")
    output = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")


if __name__ == "__main__":
    main()
