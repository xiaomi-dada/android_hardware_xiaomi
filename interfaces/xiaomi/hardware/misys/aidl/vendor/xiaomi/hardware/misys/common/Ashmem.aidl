/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.xiaomi.hardware.misys.common;

@VintfStability
parcelable Ashmem {
    ParcelFileDescriptor fd;
    long size;
}
