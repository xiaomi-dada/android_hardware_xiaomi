/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */
///////////////////////////////////////////////////////////////////////////////
// THIS FILE IS IMMUTABLE. DO NOT EDIT IN ANY CASE.                          //
///////////////////////////////////////////////////////////////////////////////

// This file is a snapshot of an AIDL file. Do not edit it manually. There are
// two cases:
// 1). this is a frozen version file - do not edit this in any case.
// 2). this is a 'current' file. If you make a backwards compatible change to
//     the interface (from the latest frozen version), the build system will
//     prompt you to update this file with `m <name>-update-api`.
//
// You must not make a backward incompatible change to any AIDL file built
// with the aidl_interface module type with versions property set. The module
// type is used to build AIDL files in a way that they can be used across
// independently updatable components of the system. If a device is shipped
// with such a backward incompatible change, it has a high risk of breaking
// later when a module using the interface is updated, e.g., Mainline modules.

package vendor.xiaomi.hardware.misys.common;
@VintfStability
interface IMiSysImpl {
  vendor.xiaomi.hardware.misys.common.FileInfo[] DirListFiles(in String path);
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
  void RegisterVCameraCallback(in vendor.xiaomi.hardware.misys.common.IVCameraCallback callback);
  void UnregisterVCameraCallback();
  long GetFileSize(in String path, in String name);
  void MiSysSetProp(in String key, in String value);
  void ReadFromFile(in vendor.xiaomi.hardware.misys.common.Ashmem ashmem, in String path, in String name);
  void WriteToFile(in vendor.xiaomi.hardware.misys.common.Ashmem ashmem, in String path, in String name, in long size);
  long GetPartitionSize(in String path, in String name);
  void MiSysCreateFolderMode(in String path, in String name, in int mode);
  void CopyFile(in String src, in String dst);
  vendor.xiaomi.hardware.misys.common.FileInfo[] DirIntgrFiles(in String path);
}
