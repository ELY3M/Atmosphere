/*
 * Copyright (c) Atmosphère-NX
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

#pragma once
#include <vapours/crypto/crypto_sha256_generator.hpp>
#include <vapours/crypto/crypto_hmac_generator.hpp>

namespace ams::crypto {

    using HmacSha256Generator = HmacGenerator<Sha256Generator>;

    void GenerateHmacSha256(void *dst, size_t dst_size, const void *data, size_t data_size, const void *key, size_t key_size);

    ALWAYS_INLINE void GenerateHmacSha256Mac(void *dst, size_t dst_size, const void *data, size_t data_size, const void *key, size_t key_size) {
        return GenerateHmacSha256(dst, dst_size, data, data_size, key, key_size);
    }

}
