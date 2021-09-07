/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <exosphere.hpp>
#include "fusee_key_derivation.hpp"
#include "fusee_fatal.hpp"

namespace ams::nxboot {

    namespace {

        alignas(se::AesBlockSize) constexpr inline const u8 MarikoMasterKekSource[se::AesBlockSize] = {
            /* TODO: Update on next change of keys. */
            0xE5, 0x41, 0xAC, 0xEC, 0xD1, 0xA7, 0xD1, 0xAB, 0xED, 0x03, 0x77, 0xF1, 0x27, 0xCA, 0xF8, 0xF1
        };

        alignas(se::AesBlockSize) constexpr inline const u8 MarikoMasterKekSourceDev[se::AesBlockSize] = {
            /* TODO: Update on next change of keys. */
            0x75, 0x2D, 0x2E, 0xF3, 0x2F, 0x3F, 0xFE, 0x65, 0xF4, 0xA9, 0x83, 0xB4, 0xED, 0x42, 0x63, 0xBA
        };

        alignas(se::AesBlockSize) constexpr inline const u8 EristaMasterKekSource[se::AesBlockSize] = {
            /* TODO: Update on next change of keys. */
            0x84, 0x67, 0xB6, 0x7F, 0x13, 0x11, 0xAE, 0xE6, 0x58, 0x9B, 0x19, 0xAF, 0x13, 0x6C, 0x80, 0x7A
        };

        alignas(se::AesBlockSize) constexpr inline const u8 KeyblobKeySource[se::AesBlockSize] = {
            0xDF, 0x20, 0x6F, 0x59, 0x44, 0x54, 0xEF, 0xDC, 0x70, 0x74, 0x48, 0x3B, 0x0D, 0xED, 0x9F, 0xD3
        };

        alignas(se::AesBlockSize) constexpr inline const u8 MasterKeySource[se::AesBlockSize] = {
            0xD8, 0xA2, 0x41, 0x0A, 0xC6, 0xC5, 0x90, 0x01, 0xC6, 0x1D, 0x6A, 0x26, 0x7C, 0x51, 0x3F, 0x3C
        };

        alignas(se::AesBlockSize) constexpr inline const u8 DeviceKeySource[se::AesBlockSize] = {
            0x4F, 0x02, 0x5F, 0x0E, 0xB6, 0x6D, 0x11, 0x0E, 0xDC, 0x32, 0x7D, 0x41, 0x86, 0xC2, 0xF4, 0x78
        };

        alignas(se::AesBlockSize) constexpr inline const u8 DeviceMasterKeySourceKekSource[se::AesBlockSize] = {
            0x0C, 0x91, 0x09, 0xDB, 0x93, 0x93, 0x07, 0x81, 0x07, 0x3C, 0xC4, 0x16, 0x22, 0x7C, 0x6C, 0x28
        };

        alignas(se::AesBlockSize) constexpr inline const u8 DeviceMasterKekSource[se::AesBlockSize] = {
            0x2D, 0xC1, 0xF4, 0x8D, 0xF3, 0x5B, 0x69, 0x33, 0x42, 0x10, 0xAC, 0x65, 0xDA, 0x90, 0x46, 0x66
        };

        alignas(se::AesBlockSize) constexpr inline const u8 DeviceMasterKeySourceSources[pkg1::OldDeviceMasterKeyCount][se::AesBlockSize] = {
            { 0x8B, 0x4E, 0x1C, 0x22, 0x42, 0x07, 0xC8, 0x73, 0x56, 0x94, 0x08, 0x8B, 0xCC, 0x47, 0x0F, 0x5D }, /* 4.x    Device Master Key Source Source. */
            { 0x6C, 0xEF, 0xC6, 0x27, 0x8B, 0xEC, 0x8A, 0x91, 0x99, 0xAB, 0x24, 0xAC, 0x4F, 0x1C, 0x8F, 0x1C }, /* 5.x    Device Master Key Source Source. */
            { 0x70, 0x08, 0x1B, 0x97, 0x44, 0x64, 0xF8, 0x91, 0x54, 0x9D, 0xC6, 0x84, 0x8F, 0x1A, 0xB2, 0xE4 }, /* 6.x    Device Master Key Source Source. */
            { 0x8E, 0x09, 0x1F, 0x7A, 0xBB, 0xCA, 0x6A, 0xFB, 0xB8, 0x9B, 0xD5, 0xC1, 0x25, 0x9C, 0xA9, 0x17 }, /* 6.2.0  Device Master Key Source Source. */
            { 0x8F, 0x77, 0x5A, 0x96, 0xB0, 0x94, 0xFD, 0x8D, 0x28, 0xE4, 0x19, 0xC8, 0x16, 0x1C, 0xDB, 0x3D }, /* 7.0.0  Device Master Key Source Source. */
            { 0x67, 0x62, 0xD4, 0x8E, 0x55, 0xCF, 0xFF, 0x41, 0x31, 0x15, 0x3B, 0x24, 0x0C, 0x7C, 0x07, 0xAE }, /* 8.1.0  Device Master Key Source Source. */
            { 0x4A, 0xC3, 0x4E, 0x14, 0x8B, 0x96, 0x4A, 0xD5, 0xD4, 0x99, 0x73, 0xC4, 0x45, 0xAB, 0x8B, 0x49 }, /* 9.0.0  Device Master Key Source Source. */
            { 0x14, 0xB8, 0x74, 0x12, 0xCB, 0xBD, 0x0B, 0x8F, 0x20, 0xFB, 0x30, 0xDA, 0x27, 0xE4, 0x58, 0x94 }, /* 9.1.0  Device Master Key Source Source. */
            { 0xAA, 0xFD, 0xBC, 0xBB, 0x25, 0xC3, 0xA4, 0xEF, 0xE3, 0xEE, 0x58, 0x53, 0xB7, 0xF8, 0xDD, 0xD6 }, /* 12.1.0 Device Master Key Source Source. */
        };

        alignas(se::AesBlockSize) constexpr inline const u8 DeviceMasterKekSources[pkg1::OldDeviceMasterKeyCount][se::AesBlockSize] = {
            { 0x88, 0x62, 0x34, 0x6E, 0xFA, 0xF7, 0xD8, 0x3F, 0xE1, 0x30, 0x39, 0x50, 0xF0, 0xB7, 0x5D, 0x5D }, /* 4.x    Device Master Kek Source. */
            { 0x06, 0x1E, 0x7B, 0xE9, 0x6D, 0x47, 0x8C, 0x77, 0xC5, 0xC8, 0xE7, 0x94, 0x9A, 0xA8, 0x5F, 0x2E }, /* 5.x    Device Master Kek Source. */
            { 0x99, 0xFA, 0x98, 0xBD, 0x15, 0x1C, 0x72, 0xFD, 0x7D, 0x9A, 0xD5, 0x41, 0x00, 0xFD, 0xB2, 0xEF }, /* 6.x    Device Master Kek Source. */
            { 0x81, 0x3C, 0x6C, 0xBF, 0x5D, 0x21, 0xDE, 0x77, 0x20, 0xD9, 0x6C, 0xE3, 0x22, 0x06, 0xAE, 0xBB }, /* 6.2.0  Device Master Kek Source. */
            { 0x86, 0x61, 0xB0, 0x16, 0xFA, 0x7A, 0x9A, 0xEA, 0xF6, 0xF5, 0xBE, 0x1A, 0x13, 0x5B, 0x6D, 0x9E }, /* 7.0.0  Device Master Kek Source. */
            { 0xA6, 0x81, 0x71, 0xE7, 0xB5, 0x23, 0x74, 0xB0, 0x39, 0x8C, 0xB7, 0xFF, 0xA0, 0x62, 0x9F, 0x8D }, /* 8.1.0  Device Master Kek Source. */
            { 0x03, 0xE7, 0xEB, 0x43, 0x1B, 0xCF, 0x5F, 0xB5, 0xED, 0xDC, 0x97, 0xAE, 0x21, 0x8D, 0x19, 0xED }, /* 9.0.0  Device Master Kek Source. */
            { 0xCE, 0xFE, 0x41, 0x0F, 0x46, 0x9A, 0x30, 0xD6, 0xF2, 0xE9, 0x0C, 0x6B, 0xB7, 0x15, 0x91, 0x36 }, /* 9.1.0  Device Master Kek Source. */
            { 0xC2, 0x65, 0x34, 0x6E, 0xC7, 0xC6, 0x5D, 0x97, 0x3E, 0x34, 0x5C, 0x6B, 0xB3, 0x7E, 0xC6, 0xE3 }, /* 12.1.0 Device Master Kek Source. */
        };

        alignas(se::AesBlockSize) constexpr inline const u8 DeviceMasterKekSourcesDev[pkg1::OldDeviceMasterKeyCount][se::AesBlockSize] = {
            { 0xD6, 0xBD, 0x9F, 0xC6, 0x18, 0x09, 0xE1, 0x96, 0x20, 0x39, 0x60, 0xD2, 0x89, 0x83, 0x31, 0x34 }, /* 4.x    Device Master Kek Source. */
            { 0x59, 0x2D, 0x20, 0x69, 0x33, 0xB5, 0x17, 0xBA, 0xCF, 0xB1, 0x4E, 0xFD, 0xE4, 0xC2, 0x7B, 0xA8 }, /* 5.x    Device Master Kek Source. */
            { 0xF6, 0xD8, 0x59, 0x63, 0x8F, 0x47, 0xCB, 0x4A, 0xD8, 0x74, 0x05, 0x7F, 0x88, 0x92, 0x33, 0xA5 }, /* 6.x    Device Master Kek Source. */
            { 0x20, 0xAB, 0xF2, 0x0F, 0x05, 0xE3, 0xDE, 0x2E, 0xA1, 0xFB, 0x37, 0x5E, 0x8B, 0x22, 0x1A, 0x38 }, /* 6.2.0  Device Master Kek Source. */
            { 0x60, 0xAE, 0x56, 0x68, 0x11, 0xE2, 0x0C, 0x99, 0xDE, 0x05, 0xAE, 0x68, 0x78, 0x85, 0x04, 0xAE }, /* 7.0.0  Device Master Kek Source. */
            { 0x94, 0xD6, 0xA8, 0xC0, 0x95, 0xAF, 0xD0, 0xA6, 0x27, 0x53, 0x5E, 0xE5, 0x8E, 0x70, 0x1F, 0x87 }, /* 8.1.0  Device Master Kek Source. */
            { 0x61, 0x6A, 0x88, 0x21, 0xA3, 0x52, 0xB0, 0x19, 0x16, 0x25, 0xA4, 0xE3, 0x4C, 0x54, 0x02, 0x0F }, /* 9.0.0  Device Master Kek Source. */
            { 0x9D, 0xB1, 0xAE, 0xCB, 0xF6, 0xF6, 0xE3, 0xFE, 0xAB, 0x6F, 0xCB, 0xAF, 0x38, 0x03, 0xFC, 0x7B }, /* 9.1.0  Device Master Kek Source. */
            { 0xC4, 0xBB, 0xF3, 0x9F, 0xA3, 0xAA, 0x00, 0x99, 0x7C, 0x97, 0xAD, 0x91, 0x8F, 0xE8, 0x45, 0xCB }, /* 12.1.0 Device Master Kek Source. */
        };

        alignas(se::AesBlockSize) constexpr inline const u8 MasterKeySources[pkg1::KeyGeneration_Count][se::AesBlockSize] = {
            { 0x0C, 0xF0, 0x59, 0xAC, 0x85, 0xF6, 0x26, 0x65, 0xE1, 0xE9, 0x19, 0x55, 0xE6, 0xF2, 0x67, 0x3D }, /* Zeroes encrypted with Master Key 00. */
            { 0x29, 0x4C, 0x04, 0xC8, 0xEB, 0x10, 0xED, 0x9D, 0x51, 0x64, 0x97, 0xFB, 0xF3, 0x4D, 0x50, 0xDD }, /* Master key 00 encrypted with Master key 01. */
            { 0xDE, 0xCF, 0xEB, 0xEB, 0x10, 0xAE, 0x74, 0xD8, 0xAD, 0x7C, 0xF4, 0x9E, 0x62, 0xE0, 0xE8, 0x72 }, /* Master key 01 encrypted with Master key 02. */
            { 0x0A, 0x0D, 0xDF, 0x34, 0x22, 0x06, 0x6C, 0xA4, 0xE6, 0xB1, 0xEC, 0x71, 0x85, 0xCA, 0x4E, 0x07 }, /* Master key 02 encrypted with Master key 03. */
            { 0x6E, 0x7D, 0x2D, 0xC3, 0x0F, 0x59, 0xC8, 0xFA, 0x87, 0xA8, 0x2E, 0xD5, 0x89, 0x5E, 0xF3, 0xE9 }, /* Master key 03 encrypted with Master key 04. */
            { 0xEB, 0xF5, 0x6F, 0x83, 0x61, 0x9E, 0xF8, 0xFA, 0xE0, 0x87, 0xD7, 0xA1, 0x4E, 0x25, 0x36, 0xEE }, /* Master key 04 encrypted with Master key 05. */
            { 0x1E, 0x1E, 0x22, 0xC0, 0x5A, 0x33, 0x3C, 0xB9, 0x0B, 0xA9, 0x03, 0x04, 0xBA, 0xDB, 0x07, 0x57 }, /* Master key 05 encrypted with Master key 06. */
            { 0xA4, 0xD4, 0x52, 0x6F, 0xD1, 0xE4, 0x36, 0xAA, 0x9F, 0xCB, 0x61, 0x27, 0x1C, 0x67, 0x65, 0x1F }, /* Master key 06 encrypted with Master key 07. */
            { 0xEA, 0x60, 0xB3, 0xEA, 0xCE, 0x8F, 0x24, 0x46, 0x7D, 0x33, 0x9C, 0xD1, 0xBC, 0x24, 0x98, 0x29 }, /* Master key 07 encrypted with Master key 08. */
            { 0x4D, 0xD9, 0x98, 0x42, 0x45, 0x0D, 0xB1, 0x3C, 0x52, 0x0C, 0x9A, 0x44, 0xBB, 0xAD, 0xAF, 0x80 }, /* Master key 08 encrypted with Master key 09. */
            { 0xB8, 0x96, 0x9E, 0x4A, 0x00, 0x0D, 0xD6, 0x28, 0xB3, 0xD1, 0xDB, 0x68, 0x5F, 0xFB, 0xE1, 0x2A }, /* Master key 09 encrypted with Master key 0A. */
            { 0xC1, 0x8D, 0x16, 0xBB, 0x2A, 0xE4, 0x1D, 0xD4, 0xC2, 0xC1, 0xB6, 0x40, 0x94, 0x35, 0x63, 0x98 }, /* Master key 0A encrypted with Master key 0B. */
        };

        alignas(se::AesBlockSize) constexpr inline const u8 MasterKeySourcesDev[pkg1::KeyGeneration_Count][se::AesBlockSize] = {
            { 0x46, 0x22, 0xB4, 0x51, 0x9A, 0x7E, 0xA7, 0x7F, 0x62, 0xA1, 0x1F, 0x8F, 0xC5, 0x3A, 0xDB, 0xFE }, /* Zeroes encrypted with Master Key 00. */
            { 0x39, 0x33, 0xF9, 0x31, 0xBA, 0xE4, 0xA7, 0x21, 0x2C, 0xDD, 0xB7, 0xD8, 0xB4, 0x4E, 0x37, 0x23 }, /* Master key 00 encrypted with Master key 01. */
            { 0x97, 0x29, 0xB0, 0x32, 0x43, 0x14, 0x8C, 0xA6, 0x85, 0xE9, 0x5A, 0x94, 0x99, 0x39, 0xAC, 0x5D }, /* Master key 01 encrypted with Master key 02. */
            { 0x2C, 0xCA, 0x9C, 0x31, 0x1E, 0x07, 0xB0, 0x02, 0x97, 0x0A, 0xD8, 0x03, 0xA2, 0x76, 0x3F, 0xA3 }, /* Master key 02 encrypted with Master key 03. */
            { 0x9B, 0x84, 0x76, 0x14, 0x72, 0x94, 0x52, 0xCB, 0x54, 0x92, 0x9B, 0xC4, 0x8C, 0x5B, 0x0F, 0xBA }, /* Master key 03 encrypted with Master key 04. */
            { 0x78, 0xD5, 0xF1, 0x20, 0x3D, 0x16, 0xE9, 0x30, 0x32, 0x27, 0x34, 0x6F, 0xCF, 0xE0, 0x27, 0xDC }, /* Master key 04 encrypted with Master key 05. */
            { 0x6F, 0xD2, 0x84, 0x1D, 0x05, 0xEC, 0x40, 0x94, 0x5F, 0x18, 0xB3, 0x81, 0x09, 0x98, 0x8D, 0x4E }, /* Master key 05 encrypted with Master key 06. */
            { 0x37, 0xAF, 0xAB, 0x35, 0x79, 0x09, 0xD9, 0x48, 0x29, 0xD2, 0xDB, 0xA5, 0xA5, 0xF5, 0x30, 0x19 }, /* Master key 06 encrypted with Master key 07. */
            { 0xEC, 0xE1, 0x46, 0x89, 0x37, 0xFD, 0xD2, 0x15, 0x8C, 0x3F, 0x24, 0x82, 0xEF, 0x49, 0x68, 0x04 }, /* Master key 07 encrypted with Master key 08. */
            { 0x43, 0x3D, 0xC5, 0x3B, 0xEF, 0x91, 0x02, 0x21, 0x61, 0x54, 0x63, 0x8A, 0x35, 0xE7, 0xCA, 0xEE }, /* Master key 08 encrypted with Master key 09. */
            { 0x6C, 0x2E, 0xCD, 0xB3, 0x34, 0x61, 0x77, 0xF5, 0xF9, 0xB1, 0xDD, 0x61, 0x98, 0x19, 0x3E, 0xD4 }, /* Master key 09 encrypted with Master key 0A. */
            { 0x21, 0x88, 0x6B, 0x10, 0x9E, 0x83, 0xD6, 0x52, 0xAB, 0x08, 0xDB, 0x6D, 0x39, 0xFF, 0x1C, 0x9C }, /* Master key 0A encrypted with Master key 0B. */
        };

        alignas(se::AesBlockSize) constinit u8 MasterKeys[pkg1::OldMasterKeyCount][se::AesBlockSize] = {};
        alignas(se::AesBlockSize) constinit u8 DeviceMasterKeys[pkg1::OldDeviceMasterKeyCount][se::AesBlockSize] = {};

        void DeriveMasterKeys(bool is_prod) {
            /* Decrypt the vector chain from current generation to start. */
            int slot = pkg1::AesKeySlot_BootloaderMaster;
            for (int i = pkg1::KeyGeneration_Current; i > pkg1::KeyGeneration_1_0_0; --i) {
                /* Decrypt the old master key. */
                se::DecryptAes128(MasterKeys[i - 1], se::AesBlockSize, slot, is_prod ? MasterKeySources[i] : MasterKeySourcesDev[i], se::AesBlockSize);

                /* Set the old master key into a temporary keyslot. */
                se::SetAesKey(pkg1::AesKeySlot_BootloaderTemporary, MasterKeys[i - 1], se::AesBlockSize);

                /* Perform the next decryption with the older master key. */
                slot = pkg1::AesKeySlot_BootloaderTemporary;
            }

            /* Decrypt the final vector. */
            alignas(se::AesBlockSize) u8 test_vector[se::AesBlockSize];
            se::DecryptAes128(test_vector, se::AesBlockSize, pkg1::AesKeySlot_BootloaderTemporary, is_prod ? MasterKeySources[pkg1::KeyGeneration_1_0_0] : MasterKeySourcesDev[pkg1::KeyGeneration_1_0_0], se::AesBlockSize);

            /* Verify the vector chain. */
            alignas(se::AesBlockSize) constexpr u8 ZeroBlock[se::AesBlockSize] = {};
            if (!crypto::IsSameBytes(ZeroBlock, test_vector, se::AesBlockSize)) {
                ShowFatalError("Failed to derive master keys!\n");
            }
        }

        void DeriveDeviceMasterKeys(fuse::SocType soc_type, bool is_prod) {
            alignas(se::AesBlockSize) u8 work_block[se::AesBlockSize];

            /* Iterate for all generations. */
            for (int i = 0; i < pkg1::OldDeviceMasterKeyCount; ++i) {
                const int generation = pkg1::KeyGeneration_4_0_0 + i;

                /* Load the first master key into the temporary keyslot keyslot. */
                se::SetAesKey(pkg1::AesKeySlot_BootloaderTemporary, MasterKeys[pkg1::KeyGeneration_1_0_0], se::AesBlockSize);

                /* Decrypt the device master kek for the generation. */
                se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderTemporary, pkg1::AesKeySlot_BootloaderTemporary, is_prod ? DeviceMasterKekSources[i] : DeviceMasterKekSourcesDev[i], se::AesBlockSize);

                /* Decrypt the device master key source into the work block. */
                se::DecryptAes128(work_block, se::AesBlockSize, pkg1::AesKeySlot_DeviceMasterKeySourceKekErista, DeviceMasterKeySourceSources[i], se::AesBlockSize);

                if (generation == pkg1::KeyGeneration_Current) {
                    se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderDeviceMaster, pkg1::AesKeySlot_BootloaderTemporary, work_block, se::AesBlockSize);

                    /* If on erista, derive the current device master key into the DeviceMaster key slot. */
                    if (soc_type == fuse::SocType_Erista) {
                        se::SetEncryptedAesKey128(pkg1::AesKeySlot_DeviceMaster, pkg1::AesKeySlot_BootloaderTemporary, work_block, se::AesBlockSize);
                    }
                } else {
                    /* Decrypt the device master key. */
                    se::DecryptAes128(DeviceMasterKeys[i], se::AesBlockSize, pkg1::AesKeySlot_BootloaderTemporary, work_block, se::AesBlockSize);

                    /* If on mariko, ensure that we derive device key 0 here. */
                    if (soc_type == fuse::SocType_Mariko && i == 0) {
                        se::SetAesKey(pkg1::AesKeySlot_Device, DeviceMasterKeys[i], se::AesBlockSize);
                    }
                }
            }
        }

        alignas(se::AesBlockSize) constexpr inline const u8 GeneratePersonalizedAesKeyKekKekSource[se::AesBlockSize] = {
            0x4D, 0x87, 0x09, 0x86, 0xC4, 0x5D, 0x20, 0x72, 0x2F, 0xBA, 0x10, 0x53, 0xDA, 0x92, 0xE8, 0xA9
        };

        alignas(se::AesBlockSize) constexpr inline const u8 GeneratePersonalizedAesKeyKeyKekSource[se::AesBlockSize] = {
            0x89, 0x61, 0x5E, 0xE0, 0x5C, 0x31, 0xB6, 0x80, 0x5F, 0xE5, 0x8F, 0x3D, 0xA2, 0x4F, 0x7A, 0xA8
        };

        void GeneratePersonalizedAesKeyForBis(int slot, const void *kek_source, const void *key_source, int generation) {
            /* Derive kek. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderTemporary, PrepareDeviceMasterKey(generation), GeneratePersonalizedAesKeyKekKekSource, se::AesBlockSize);
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderTemporary, pkg1::AesKeySlot_BootloaderTemporary, kek_source, se::AesBlockSize);

            /* Derive key. */
            se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderTemporary, pkg1::AesKeySlot_BootloaderTemporary, GeneratePersonalizedAesKeyKeyKekSource, se::AesBlockSize);
            se::SetEncryptedAesKey128(slot, pkg1::AesKeySlot_BootloaderTemporary, key_source, se::AesBlockSize);
        }

        alignas(se::AesBlockSize) constexpr inline const u8 BisKekSource[se::AesBlockSize] = {
            0x34, 0xC1, 0xA0, 0xC4, 0x82, 0x58, 0xF8, 0xB4, 0xFA, 0x9E, 0x5E, 0x6A, 0xDA, 0xFC, 0x7E, 0x4F
        };

        alignas(se::AesBlockSize) constexpr inline const u8 BisPartitionSystemKeySources[2][se::AesBlockSize] = {
            { 0x52, 0xC2, 0xE9, 0xEB, 0x09, 0xE3, 0xEE, 0x29, 0x32, 0xA1, 0x0C, 0x1F, 0xB6, 0xA0, 0x92, 0x6C },
            { 0x4D, 0x12, 0xE1, 0x4B, 0x2A, 0x47, 0x4C, 0x1C, 0x09, 0xCB, 0x03, 0x59, 0xF0, 0x15, 0xF4, 0xE4 },
        };

        void DeriveBisPartitionSystemKeys() {
            /* Determine key generation. */
            const int key_generation = std::max<int>(0, static_cast<int>(fuse::GetDeviceUniqueKeyGeneration()) - 1);

            /* Generate desired keys. */
            GeneratePersonalizedAesKeyForBis(pkg1::AesKeySlot_BootloaderSystem0, BisKekSource, BisPartitionSystemKeySources[0], key_generation);
            GeneratePersonalizedAesKeyForBis(pkg1::AesKeySlot_BootloaderSystem1, BisKekSource, BisPartitionSystemKeySources[1], key_generation);
        }

    }

    void DeriveKeysErista() {
        /* Get work buffer. */
        alignas(se::AesBlockSize) u8 work_buffer[se::AesBlockSize];

        /* Get whether we're using dev keys. */
        const bool is_prod = fuse::GetHardwareState() == fuse::HardwareState_Production;

        /* Derive Keyblob Key. */
        se::DecryptAes128(work_buffer, se::AesBlockSize, pkg1::AesKeySlot_Tsec, KeyblobKeySource, se::AesBlockSize);
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_Device, pkg1::AesKeySlot_SecureBoot, work_buffer, se::AesBlockSize);

        /* Derive Master Kek. */
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_MasterKek, is_prod ? pkg1::AesKeySlot_TsecRoot : pkg1::AesKeySlot_TsecRootDev, EristaMasterKekSource, se::AesBlockSize);

        /* Derive Master Key. */
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderMaster, pkg1::AesKeySlot_MasterKek, MasterKeySource, se::AesBlockSize);
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_Master,           pkg1::AesKeySlot_MasterKek, MasterKeySource, se::AesBlockSize);

        /* Derive Device Master Key Source Kek, Device Key. */
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_DeviceMasterKeySourceKekErista, pkg1::AesKeySlot_Device, DeviceMasterKeySourceKekSource, se::AesBlockSize);
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_Device, pkg1::AesKeySlot_Device, DeviceKeySource, se::AesBlockSize);

        /* Derive all master keys. */
        DeriveMasterKeys(is_prod);

        /* Derive all device master keys. */
        DeriveDeviceMasterKeys(fuse::SocType_Erista, is_prod);

        /* Derive system partition keys. */
        DeriveBisPartitionSystemKeys();
    }

    void DeriveKeysMariko() {
        /* Get whether we're using dev keys. */
        const bool is_prod = fuse::GetHardwareState() == fuse::HardwareState_Production;

        /* Derive Device Master Key Source Kek, Master Key. */
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_DeviceMasterKeySourceKekErista, pkg1::AesKeySlot_SecureBoot, DeviceMasterKeySourceKekSource, se::AesBlockSize);
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderMaster, pkg1::AesKeySlot_MarikoKek, is_prod ? MarikoMasterKekSource : MarikoMasterKekSourceDev, se::AesBlockSize);
        se::SetEncryptedAesKey128(pkg1::AesKeySlot_BootloaderMaster, pkg1::AesKeySlot_BootloaderMaster, MasterKeySource, se::AesBlockSize);

        /* Derive all master keys. */
        DeriveMasterKeys(is_prod);

        /* Derive all device master keys. */
        DeriveDeviceMasterKeys(fuse::SocType_Mariko, is_prod);

        /* Derive system partition keys. */
        DeriveBisPartitionSystemKeys();
    }

    int PrepareMasterKey(int generation) {
        if (generation == pkg1::KeyGeneration_Current) {
            return pkg1::AesKeySlot_BootloaderMaster;
        }

        se::SetAesKey(pkg1::AesKeySlot_BootloaderTemporary, MasterKeys[generation], se::AesBlockSize);

        return pkg1::AesKeySlot_BootloaderTemporary;
    }

    int PrepareDeviceMasterKey(int generation) {
        if (generation == pkg1::KeyGeneration_1_0_0) {
            return pkg1::AesKeySlot_Device;
        }

        if (generation == pkg1::KeyGeneration_Current) {
            return pkg1::AesKeySlot_BootloaderDeviceMaster;
        }

        const int index = std::max(0, generation - pkg1::KeyGeneration_4_0_0);
        se::SetAesKey(pkg1::AesKeySlot_BootloaderTemporary, DeviceMasterKeys[index], se::AesBlockSize);

        return pkg1::AesKeySlot_BootloaderTemporary;
    }

}
