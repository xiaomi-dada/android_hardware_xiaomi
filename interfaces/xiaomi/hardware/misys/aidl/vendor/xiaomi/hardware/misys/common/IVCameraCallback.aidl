/*
 * SPDX-FileCopyrightText: The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package vendor.xiaomi.hardware.misys.common;

@VintfStability
interface IVCameraCallback {
    oneway void stateChangeTo(int state);
}
