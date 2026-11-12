/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "vendor.xiaomi.hardware.misys.common-service.xiaomi"

#include "MiSys.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/strings.h>
#include <android-base/unique_fd.h>
#include <cutils/ashmem.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <optional>

using android::base::Dirname;
using android::base::ReadFully;
using android::base::Realpath;
using android::base::Split;
using android::base::unique_fd;
using android::base::WriteFully;

namespace aidl::vendor::xiaomi::hardware::misys::common {

namespace {

// Directories the camera shares with the camera provider
struct Area {
    std::string_view root;
    // Whether folders can be created and erased below root. If not, only files
    // directly in root with one of the extensions can be accessed.
    bool folders;
    std::vector<std::string_view> extensions;
};

const Area kAreas[] = {
        // Watermark templates, fonts and AI watermarks
        {"/data/vendor/camera", true, {}},
        // Filter LUTs, next to the camera calibration data
        {"/mnt/vendor/persist/camera", false, {".png", ".txt", ".webp"}},
};

// Larger buffers don't fit in a binder transaction
constexpr size_t kMaxBufferSize = 1024 * 1024;

struct Target {
    std::string path;
    const Area* area;
    bool isRoot;
};

std::optional<std::string> Normalize(const std::string& path) {
    if (!path.starts_with('/')) return std::nullopt;

    std::string normalized;
    for (const auto& component : Split(path, "/")) {
        if (component.empty()) continue;
        if (component == "." || component == "..") return std::nullopt;
        normalized += "/" + component;
    }
    return normalized;
}

// Resolves path + "/" + name to a location in one of kAreas
std::optional<Target> Resolve(const std::string& path, const std::string& name = "") {
    if (name.find('/') != std::string::npos) return std::nullopt;

    auto full = Normalize(name.empty() ? path : path + "/" + name);
    if (!full) return std::nullopt;

    for (const auto& area : kAreas) {
        if (*full == area.root) return Target{*full, &area, true};
        if (!full->starts_with(area.root) || (*full)[area.root.size()] != '/') continue;

        if (!area.folders) {
            if (full->find('/', area.root.size() + 1) != std::string::npos) return std::nullopt;
            if (std::none_of(area.extensions.begin(), area.extensions.end(),
                             [&](auto ext) { return full->ends_with(ext); })) {
                return std::nullopt;
            }
        }
        return Target{*full, &area, false};
    }
    return std::nullopt;
}

// Rejects locations reached through symbolic links
bool HasRealParent(const Target& target) {
    std::string parent = Dirname(target.path);
    std::string real;
    return Realpath(parent, &real) && real == parent;
}

std::optional<Target> ResolveExisting(const std::string& path, const std::string& name = "") {
    auto target = Resolve(path, name);
    if (!target || !HasRealParent(*target)) return std::nullopt;
    return target;
}

ndk::ScopedAStatus Denied(const std::string& path, const std::string& name = "") {
    LOG(WARNING) << "Denied access to " << path << (name.empty() ? "" : "/") << name;
    return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_SECURITY, "Location not allowed");
}

ndk::ScopedAStatus Failed(const char* what, const std::string& path) {
    int error = errno;
    PLOG(ERROR) << what << " " << path;
    return ndk::ScopedAStatus::fromServiceSpecificErrorWithMessage(error, strerror(error));
}

ndk::ScopedAStatus Unsupported() {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus WriteFile(const Target& target, const void* data, size_t size) {
    unique_fd fd(open(target.path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC,
                      0644));
    if (fd < 0) return Failed("Failed to open", target.path);
    if (!WriteFully(fd, data, size)) return Failed("Failed to write", target.path);
    if (fsync(fd) < 0) return Failed("Failed to sync", target.path);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus OpenFile(const Target& target, unique_fd* fd, size_t* size) {
    fd->reset(open(target.path.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));
    if (*fd < 0) return Failed("Failed to open", target.path);

    struct stat st;
    if (fstat(*fd, &st) < 0) return Failed("Failed to stat", target.path);
    if (!S_ISREG(st.st_mode)) {
        errno = EISDIR;
        return Failed("Not a file", target.path);
    }
    *size = st.st_size;
    return ndk::ScopedAStatus::ok();
}

class Mapping {
  public:
    Mapping(const Ashmem& ashmem, size_t size, int prot) : mSize(size) {
        int regionSize = ashmem_get_size_region(ashmem.fd.get());
        if (regionSize < 0 || static_cast<size_t>(regionSize) < size) {
            errno = EINVAL;
            return;
        }
        if (size == 0) {
            mValid = true;
            return;
        }
        mAddr = mmap(nullptr, size, prot, MAP_SHARED, ashmem.fd.get(), 0);
        mValid = mAddr != MAP_FAILED;
    }

    ~Mapping() {
        if (mValid && mSize > 0) munmap(mAddr, mSize);
    }

    bool valid() const { return mValid; }
    void* data() const { return mAddr; }

  private:
    void* mAddr = MAP_FAILED;
    size_t mSize;
    bool mValid = false;
};

}  // namespace

ndk::ScopedAStatus MiSys::DirListFiles(const std::string& path,
                                       std::vector<FileInfo>* _aidl_return) {
    auto target = ResolveExisting(path);
    if (!target || (!target->isRoot && !target->area->folders)) return Denied(path);

    std::unique_ptr<DIR, decltype(&closedir)> dir(opendir(target->path.c_str()), closedir);
    if (!dir) return Failed("Failed to open", target->path);

    _aidl_return->clear();
    while (dirent* entry = readdir(dir.get())) {
        struct stat st;
        if (fstatat(dirfd(dir.get()), entry->d_name, &st, AT_SYMLINK_NOFOLLOW) < 0 ||
            !S_ISREG(st.st_mode) || !Resolve(target->path, entry->d_name)) {
            continue;
        }
        _aidl_return->push_back({
                .name = entry->d_name,
                .size = st.st_size,
                .mtime = st.st_mtim.tv_sec,
        });
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MiSys::EraseFileOrDirectory(const std::string& path, const std::string& name) {
    auto target = ResolveExisting(path, name);
    if (!target || target->isRoot) return Denied(path, name);

    struct stat st;
    if (lstat(target->path.c_str(), &st) < 0) {
        if (errno == ENOENT) return ndk::ScopedAStatus::ok();
        return Failed("Failed to stat", target->path);
    }

    if (S_ISDIR(st.st_mode)) {
        if (!target->area->folders) return Denied(path, name);

        std::error_code ec;
        std::filesystem::remove_all(target->path, ec);
        if (ec) {
            errno = ec.value();
            return Failed("Failed to erase", target->path);
        }
    } else if (unlink(target->path.c_str()) < 0) {
        return Failed("Failed to erase", target->path);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MiSys::MiSysReadFile(const std::string& /*path*/, const std::string& /*name*/,
                                        std::string* /*_aidl_return*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::MiSysWriteFile(const std::string& /*path*/, const std::string& /*name*/,
                                         const std::string& /*content*/, int32_t /*size*/,
                                         int8_t /*append*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::IsExists(const std::string& path, const std::string& name,
                                   bool* _aidl_return) {
    auto target = ResolveExisting(path, name);
    if (!target) {
        // Also covers missing parent folders
        if (Resolve(path, name)) {
            *_aidl_return = false;
            return ndk::ScopedAStatus::ok();
        }
        return Denied(path, name);
    }

    struct stat st;
    if (lstat(target->path.c_str(), &st) < 0) {
        if (errno != ENOENT) return Failed("Failed to stat", target->path);
        *_aidl_return = false;
    } else {
        *_aidl_return = true;
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MiSys::MiSysCreateFolder(const std::string& path, const std::string& name) {
    auto target = Resolve(path, name);
    if (!target || target->isRoot || !target->area->folders) return Denied(path, name);

    // Create the missing folders one by one so none of them can be a symbolic link
    std::string folder(target->area->root);
    for (const auto& component : Split(target->path.substr(folder.size() + 1), "/")) {
        folder += "/" + component;
        if (mkdir(folder.c_str(), 0755) < 0 && errno != EEXIST) {
            return Failed("Failed to create", folder);
        }

        struct stat st;
        if (lstat(folder.c_str(), &st) < 0) return Failed("Failed to stat", folder);
        if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            return Failed("Failed to create", folder);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MiSys::MiSysReadBuffer(const std::string& path, const std::string& name,
                                          std::vector<uint8_t>* _aidl_return) {
    auto target = ResolveExisting(path, name);
    if (!target || target->isRoot) return Denied(path, name);

    unique_fd fd;
    size_t size;
    if (auto status = OpenFile(*target, &fd, &size); !status.isOk()) return status;
    if (size > kMaxBufferSize) {
        return ndk::ScopedAStatus::fromExceptionCodeWithMessage(EX_ILLEGAL_ARGUMENT,
                                                                "File too large");
    }

    _aidl_return->resize(size);
    if (!ReadFully(fd, _aidl_return->data(), size)) return Failed("Failed to read", target->path);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MiSys::MiSysWriteBuffer(const std::string& path, const std::string& name,
                                           const std::vector<uint8_t>& buffer, int64_t size) {
    auto target = ResolveExisting(path, name);
    if (!target || target->isRoot) return Denied(path, name);
    if (size < 0 || static_cast<uint64_t>(size) > buffer.size()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    return WriteFile(*target, buffer.data(), size);
}

ndk::ScopedAStatus MiSys::SetVirtualCameraConfig(int32_t /*width*/, int32_t /*height*/,
                                                 double /*fps*/, bool* /*_aidl_return*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::ConnectVirtualCamera(bool* /*_aidl_return*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::DisconnectVirtualCamera(bool* /*_aidl_return*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::OnFrameData(const std::vector<uint8_t>& /*data*/, int32_t /*size*/,
                                      bool* /*_aidl_return*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::RegisterVCameraCallback(
        const std::shared_ptr<IVCameraCallback>& /*callback*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::UnregisterVCameraCallback() {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::GetFileSize(const std::string& path, const std::string& name,
                                      int64_t* _aidl_return) {
    auto target = ResolveExisting(path, name);
    if (!target || target->isRoot) return Denied(path, name);

    unique_fd fd;
    size_t size;
    if (auto status = OpenFile(*target, &fd, &size); !status.isOk()) return status;

    *_aidl_return = size;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MiSys::MiSysSetProp(const std::string& /*key*/, const std::string& /*value*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::ReadFromFile(const Ashmem& ashmem, const std::string& path,
                                       const std::string& name) {
    auto target = ResolveExisting(path, name);
    if (!target || target->isRoot) return Denied(path, name);
    if (ashmem.size < 0) return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);

    unique_fd fd;
    size_t size;
    if (auto status = OpenFile(*target, &fd, &size); !status.isOk()) return status;
    size = std::min(size, static_cast<size_t>(ashmem.size));

    Mapping mapping(ashmem, size, PROT_READ | PROT_WRITE);
    if (!mapping.valid()) return Failed("Failed to map shared memory for", target->path);
    if (!ReadFully(fd, mapping.data(), size)) return Failed("Failed to read", target->path);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus MiSys::WriteToFile(const Ashmem& ashmem, const std::string& path,
                                      const std::string& name, int64_t size) {
    auto target = ResolveExisting(path, name);
    if (!target || target->isRoot) return Denied(path, name);
    if (size < 0 || size > ashmem.size) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    Mapping mapping(ashmem, size, PROT_READ);
    if (!mapping.valid()) return Failed("Failed to map shared memory for", target->path);
    return WriteFile(*target, mapping.data(), size);
}

ndk::ScopedAStatus MiSys::GetPartitionSize(const std::string& /*path*/,
                                           const std::string& /*name*/,
                                           int64_t* /*_aidl_return*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::MiSysCreateFolderMode(const std::string& /*path*/,
                                                const std::string& /*name*/, int32_t /*mode*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::CopyFile(const std::string& /*src*/, const std::string& /*dst*/) {
    return Unsupported();
}

ndk::ScopedAStatus MiSys::DirIntgrFiles(const std::string& /*path*/,
                                        std::vector<FileInfo>* /*_aidl_return*/) {
    return Unsupported();
}

}  // namespace aidl::vendor::xiaomi::hardware::misys::common
