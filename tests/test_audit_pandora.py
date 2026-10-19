import importlib.util
import struct
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("audit_pandora", Path(__file__).parents[1] / "tools/audit_pandora.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def elf32():
    data = bytearray(512)
    data[:7] = b"\x7fELF\x01\x01\x01"
    struct.pack_into("<H", data, 18, 40)
    struct.pack_into("<I", data, 28, 52)
    struct.pack_into("<HH", data, 42, 32, 2)
    struct.pack_into("<IIIIIIII", data, 52, 1, 0, 0x1000, 0, len(data), len(data), 4, 4)
    struct.pack_into("<IIIIIIII", data, 84, 2, 128, 0x1080, 0, 40, 40, 4, 4)
    names = b"libwine.so.1\0libc.so.6\0"
    entries = [(1, 0), (1, len(b"libwine.so.1\0")), (5, 0x10F0), (10, len(names)), (0, 0)]
    for n, (tag, value) in enumerate(entries):
        struct.pack_into("<II", data, 128 + n * 8, tag, value)
    data[240:240+len(names)] = names
    return data


class AuditTests(unittest.TestCase):
    def test_arm32_header(self):
        self.assertEqual(audit.elf_header(elf32())["machine"], "ARM")
        self.assertEqual(audit.elf_header(elf32())["bits"], 32)

    def test_needed_libraries(self):
        self.assertEqual(audit.needed_libraries(elf32()), ["libwine.so.1", "libc.so.6"])

    def test_non_elf(self):
        with self.assertRaises(ValueError):
            audit.elf_header(b"MZ" + bytes(80))

    def test_truncated(self):
        with self.assertRaises(ValueError):
            audit.elf_header(elf32()[:30])

    def test_arm64_big_endian(self):
        data = bytearray(64)
        data[:7] = b"\x7fELF\x02\x02\x01"
        struct.pack_into(">H", data, 18, 183)
        self.assertEqual(audit.elf_header(data)["machine"], "AArch64")

    def test_bad_program_headers(self):
        data = elf32()
        struct.pack_into("<I", data, 28, 500)
        with self.assertRaises(ValueError):
            audit.needed_libraries(data)

    def test_bad_string_reference(self):
        data = elf32()
        struct.pack_into("<I", data, 132, 10000)
        with self.assertRaises(ValueError):
            audit.needed_libraries(data)


if __name__ == "__main__":
    unittest.main()
