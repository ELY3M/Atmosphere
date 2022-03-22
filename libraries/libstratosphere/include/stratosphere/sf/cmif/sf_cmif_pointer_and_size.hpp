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
#include <stratosphere/sf/sf_common.hpp>

namespace ams::sf::cmif {

    class PointerAndSize {
        private:
            uintptr_t m_pointer;
            size_t m_size;
        public:
            constexpr PointerAndSize() : m_pointer(0), m_size(0) { /* ... */ }
            constexpr PointerAndSize(uintptr_t ptr, size_t sz) : m_pointer(ptr), m_size(sz) { /* ... */ }
            PointerAndSize(void *ptr, size_t sz) : PointerAndSize(reinterpret_cast<uintptr_t>(ptr), sz) { /* ... */ }

            void *GetPointer() const {
                return reinterpret_cast<void *>(m_pointer);
            }

            constexpr uintptr_t GetAddress() const {
                return m_pointer;
            }

            constexpr size_t GetSize() const {
                return m_size;
            }
    };

}
