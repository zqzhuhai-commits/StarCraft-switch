#include "../openbw-switch/src/map_files.h"
#include <cassert>
#include <filesystem>
#include <fstream>
int main(int argc, char** argv) {
    assert(argc == 2);
    std::filesystem::path root = argv[1];
    std::filesystem::create_directories(root / "campaign");
    std::ofstream(root / "b.SCX").put('x');
    std::ofstream(root / "a.sCm").put('x');
    std::ofstream(root / "campaign/c.scc").put('x');
    std::ofstream(root / "ignore.mpq").put('x');
    std::filesystem::create_directory_symlink(root, root / "loop");
    auto maps = starcraft::find_maps(root.string());
    assert(maps.size() == 3);
    assert(maps[0] == (root / "a.sCm").string());
    assert(maps[1] == (root / "b.SCX").string());
    assert(maps[2] == (root / "campaign/c.scc").string());
    assert(starcraft::find_maps((root / "missing").string()).empty());
}
