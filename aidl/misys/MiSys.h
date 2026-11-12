/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/vendor/xiaomi/hardware/misys/common/BnMiSysImpl.h>

namespace aidl::vendor::xiaomi::hardware::misys::common {

// Implements the file operations the Xiaomi camera uses to share watermarks,
// fonts and filters with the camera provider. Access is limited to the
// camera's own vendor directories.
class MiSys : public BnMiSysImpl {
  public:
    ndk::ScopedAStatus DirListFiles(const std::string& path,
                                    std::vector<FileInfo>* _aidl_return) override;
    ndk::ScopedAStatus EraseFileOrDirectory(const std::string& path,
                                            const std::string& name) override;
    ndk::ScopedAStatus MiSysReadFile(const std::string& path, const std::string& name,
                                     std::string* _aidl_return) override;
    ndk::ScopedAStatus MiSysWriteFile(const std::string& path, const std::string& name,
                                      const std::string& content, int32_t size,
                                      int8_t append) override;
    ndk::ScopedAStatus IsExists(const std::string& path, const std::string& name,
                                bool* _aidl_return) override;
    ndk::ScopedAStatus MiSysCreateFolder(const std::string& path, const std::string& name) override;
    ndk::ScopedAStatus MiSysReadBuffer(const std::string& path, const std::string& name,
                                       std::vector<uint8_t>* _aidl_return) override;
    ndk::ScopedAStatus MiSysWriteBuffer(const std::string& path, const std::string& name,
                                        const std::vector<uint8_t>& buffer, int64_t size) override;
    ndk::ScopedAStatus SetVirtualCameraConfig(int32_t width, int32_t height, double fps,
                                              bool* _aidl_return) override;
    ndk::ScopedAStatus ConnectVirtualCamera(bool* _aidl_return) override;
    ndk::ScopedAStatus DisconnectVirtualCamera(bool* _aidl_return) override;
    ndk::ScopedAStatus OnFrameData(const std::vector<uint8_t>& data, int32_t size,
                                   bool* _aidl_return) override;
    ndk::ScopedAStatus RegisterVCameraCallback(
            const std::shared_ptr<IVCameraCallback>& callback) override;
    ndk::ScopedAStatus UnregisterVCameraCallback() override;
    ndk::ScopedAStatus GetFileSize(const std::string& path, const std::string& name,
                                   int64_t* _aidl_return) override;
    ndk::ScopedAStatus MiSysSetProp(const std::string& key, const std::string& value) override;
    ndk::ScopedAStatus ReadFromFile(const Ashmem& ashmem, const std::string& path,
                                    const std::string& name) override;
    ndk::ScopedAStatus WriteToFile(const Ashmem& ashmem, const std::string& path,
                                   const std::string& name, int64_t size) override;
    ndk::ScopedAStatus GetPartitionSize(const std::string& path, const std::string& name,
                                        int64_t* _aidl_return) override;
    ndk::ScopedAStatus MiSysCreateFolderMode(const std::string& path, const std::string& name,
                                             int32_t mode) override;
    ndk::ScopedAStatus CopyFile(const std::string& src, const std::string& dst) override;
    ndk::ScopedAStatus DirIntgrFiles(const std::string& path,
                                     std::vector<FileInfo>* _aidl_return) override;
};

}  // namespace aidl::vendor::xiaomi::hardware::misys::common
