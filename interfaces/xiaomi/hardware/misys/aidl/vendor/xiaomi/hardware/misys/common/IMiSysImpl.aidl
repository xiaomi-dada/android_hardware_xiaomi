/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.xiaomi.hardware.misys.common;

import vendor.xiaomi.hardware.misys.common.Ashmem;
import vendor.xiaomi.hardware.misys.common.FileInfo;
import vendor.xiaomi.hardware.misys.common.IVCameraCallback;

/**
 * File and system helper used by Xiaomi system apps to reach vendor
 * locations. Every method that takes a path and a name operates on
 * path + "/" + name.
 */
@VintfStability
interface IMiSysImpl {
    FileInfo[] DirListFiles(in String path);
    void EraseFileOrDirectory(in String path, in String name);
    String MiSysReadFile(in String path, in String name);
    void MiSysWriteFile(in String path, in String name, in String content, in int size, in byte append);
    boolean IsExists(in String path, in String name);
    void MiSysCreateFolder(in String path, in String name);
    byte[] MiSysReadBuffer(in String path, in String name);
    void MiSysWriteBuffer(in String path, in String name, in byte[] buffer, in long size);
    boolean SetVirtualCameraConfig(in int width, in int height, in double fps);
    boolean ConnectVirtualCamera();
    boolean DisconnectVirtualCamera();
    boolean OnFrameData(in byte[] data, in int size);
    void RegisterVCameraCallback(in IVCameraCallback callback);
    void UnregisterVCameraCallback();
    long GetFileSize(in String path, in String name);
    void MiSysSetProp(in String key, in String value);
    void ReadFromFile(in Ashmem ashmem, in String path, in String name);
    void WriteToFile(in Ashmem ashmem, in String path, in String name, in long size);
    long GetPartitionSize(in String path, in String name);
    void MiSysCreateFolderMode(in String path, in String name, in int mode);
    void CopyFile(in String src, in String dst);
    FileInfo[] DirIntgrFiles(in String path);
}
