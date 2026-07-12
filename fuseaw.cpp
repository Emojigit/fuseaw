#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string>
#include <sstream>
#include <utility>

#include "src/akpk.h"
#include "src/filesystem.h"

namespace fs = std::filesystem;

// g++ -std=c++23 fuseaw.cpp src/*.cpp -o fuseaw $(pkg-config fuse3 --cflags --libs)

/**
 * Directory structure of FUSEAW:
 * /[sector_name]/[language_id]/[file_id].[ext]
 * 
 * sector_name: banks/sounds/externals
 * language_id: raw ID in decimal
 * file_id: raw ID in decimal
 * 
 * If ext = bnk, the bank file is resolved into a directory, where paths:
 * /[sector_name]/[language_id]/[file_id].bnk/[wem_id].wem
 * 
 * The filesystem is read-only. Constructing pck files is a no-goal.
 */

struct FSContext {
    std::span<const std::byte> file_span;
    std::filesystem::path file_path;
    struct stat file_stat;
    std::mutex file_mutex;
    AKPKFilesystemNode file_node;
};

std::vector<std::string> split_path(const std::string& path) {
    std::vector<std::string> components;
    std::stringstream ss(path);
    std::string item;
    while (std::getline(ss, item, '/')) {
        if (!item.empty()) {
            components.push_back(item);
        }
    }
    return components;
}

AKPKFilesystemNode* traverse_node(AKPKFilesystemNode* root, std::vector<std::string> pathcomps) {
    AKPKFilesystemNode* node = root;

    for (const std::string comp : pathcomps) {
        if (!node->children.contains(comp)) {
            return nullptr;
        }

        node = &node->children[comp];
    }

    return node;
}

static void* fuseaw_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
    cfg->kernel_cache = 1;

    return fuse_get_context()->private_data;
}

static int fuseaw_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
    (void) fi;
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));

    auto* ctx = static_cast<FSContext*>(fuse_get_context()->private_data);

    stbuf->st_ctim = ctx->file_stat.st_ctim;
    stbuf->st_atim = ctx->file_stat.st_atim;
    stbuf->st_mtim = ctx->file_stat.st_mtim;

    constexpr mode_t directory_ro = S_IFDIR | S_IRUSR | S_IRGRP | S_IROTH;
    constexpr mode_t file_ro = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH;
    constexpr mode_t symlink_ro = S_IFLNK | 0777;
    std::vector<std::string> pathcomps = split_path(path);

    // Special case: source.pck
    if (pathcomps.size() == 1 && pathcomps[0] == "source.pck") {
        stbuf->st_mode = symlink_ro;
        stbuf->st_nlink = 1;
        stbuf->st_size = ctx->file_path.string().size();

        return 0;
    }

    AKPKFilesystemNode* node = traverse_node(&ctx->file_node, pathcomps);
    if (node == nullptr) return -ENOENT;

    if (node->type == APKPFilesystemType::Directory) {
        stbuf->st_mode = directory_ro;
        stbuf->st_nlink = 2 + node->children.size();

        // source.pck
        if (pathcomps.empty()) stbuf->st_nlink += 1;

        return 0;
    }

    stbuf->st_mode = file_ro;
    stbuf->st_size = node->size;

    return 0;
}

static int fuseaw_readlink(const char *path, char *buf, size_t size) {
    auto* ctx = static_cast<FSContext*>(fuse_get_context()->private_data);
    std::vector<std::string> pathcomps = split_path(path);

    if (pathcomps.size() == 1 && pathcomps[0] == "source.pck") {
        strncpy(buf, ctx->file_path.string().c_str(), size);

        return 0;
    }

    return -ENOENT;
}

static int fuseaw_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    (void) offset; (void) fi; (void) flags;
    auto* ctx = static_cast<FSContext*>(fuse_get_context()->private_data);
    std::vector<std::string> pathcomps = split_path(path);

    AKPKFilesystemNode* node = traverse_node(&ctx->file_node, pathcomps);
    if (node == nullptr) return -ENOENT;
    if (node->type != APKPFilesystemType::Directory) return -ENOTDIR;

    filler(buf, ".", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    filler(buf, "..", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);

    for (auto const& [filename, cnode] : node->children) {
        filler(buf, filename.c_str(), nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    }

    if (pathcomps.size() == 0) {
        filler(buf, "source.pck", nullptr, 0, FUSE_FILL_DIR_DEFAULTS);
    }

    return 0;
}

static int fuseaw_open(const char* path, struct fuse_file_info* fi) {
    auto* ctx = static_cast<FSContext*>(fuse_get_context()->private_data);
    std::vector<std::string> pathcomps = split_path(path);

    AKPKFilesystemNode* node = traverse_node(&ctx->file_node, pathcomps);

    if (node == nullptr) return -ENOENT;
    if (node->type == APKPFilesystemType::Directory) return -EISDIR;
    if ((fi->flags & O_ACCMODE) != O_RDONLY) return -EACCES;

    return 0;
}

static int fuseaw_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi) {
    (void) fi;

    auto* ctx = static_cast<FSContext*>(fuse_get_context()->private_data);
    std::vector<std::string> pathcomps = split_path(path);

    AKPKFilesystemNode* node = traverse_node(&ctx->file_node, pathcomps);

    if (node == nullptr) return -ENOENT;
    if (node->type == APKPFilesystemType::Directory) return -EISDIR;

    if (offset >= static_cast<off_t>(node->size)) return 0;
    size_t to_read = std::min(size, static_cast<size_t>(node->size - offset));

    const size_t target_offset = node->offset + static_cast<size_t>(offset);

    if (target_offset >= ctx->file_span.size()) {
        return 0; 
    }

    if (target_offset + to_read > ctx->file_span.size()) {
        to_read = ctx->file_span.size() - target_offset;
    }

    std::memcpy(buf, ctx->file_span.subspan(target_offset).data(), to_read);
    return static_cast<int>(to_read);
}

static const struct fuse_operations fuseaw_oper = {
    .getattr = fuseaw_getattr,
    .readlink = fuseaw_readlink,
    .open = fuseaw_open,
    .read = fuseaw_read,
    .readdir = fuseaw_readdir,
};

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        std::cerr << "usage: " << argv[0] << " <.pck filename> <mountpoint>" << std::endl;
        return 1;
    }

    fs::path filepath = argv[1];

    struct stat file_stat;
    if (stat(filepath.c_str(), &file_stat)) {
        std::cerr << "Error: Could not stat() .pck file '" << filepath << std::endl;
        return 1;
    }

    size_t filesize = fs::file_size(filepath);

    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        std::cerr << "Error opening file: " << fd << std::endl;
        return 1;
    }

    void* mapped_data = mmap(nullptr, filesize, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);

    if (mapped_data == MAP_FAILED) {
        std::cerr << "Error: Mmap failed" << std::endl;
        return 1;
    }

    std::span<const std::byte> file_span(static_cast<const std::byte*>(mapped_data), filesize);

    AKPKFileData file_data{};
    if (!parse_akpk_span(file_span, file_data)) {
        std::cerr << "Error: Failed to parse pck file\n";
        return 1;
    }

    AKPKFilesystemNode root_node{};
    if (!construct_akpk_filesystem(file_data, root_node)) {
        std::cerr << "Error: Failed to construct root filesystem\n";
        return 1;
    }

    clean_empty_directories(root_node);

    FSContext ctx {
        .file_span = file_span,
        .file_path = std::filesystem::absolute(filepath),
        .file_stat = std::move(file_stat),
        .file_node = std::move(root_node),
    };
    
    strcpy(argv[1], "-r");

    return fuse_main(argc, argv, &fuseaw_oper, &ctx);
}
