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
#include "creport_scoped_file.hpp"
#include "creport_threads.hpp"

namespace ams::creport {

    class ModuleList {
        private:
            static constexpr size_t ModuleCountMax      = 0x60;
            static constexpr size_t ModuleNameLengthMax = 0x20;
            static constexpr size_t ModuleIdSize = 0x20;

            struct ModuleInfo {
                char name[ModuleNameLengthMax];
                u8   module_id[ModuleIdSize];
                u64  start_address;
                u64  end_address;
            };
        private:
            os::NativeHandle m_debug_handle;
            size_t m_num_modules;
            ModuleInfo m_modules[ModuleCountMax];

            /* For pretty-printing. */
            char m_address_str_buf[0x280];
        public:
            ModuleList() : m_debug_handle(os::InvalidNativeHandle), m_num_modules(0) {
                std::memset(m_modules, 0, sizeof(m_modules));
            }

            size_t GetModuleCount() const {
                return m_num_modules;
            }

            u64 GetModuleStartAddress(size_t i) const {
                return m_modules[i].start_address;
            }

            void FindModulesFromThreadInfo(os::NativeHandle debug_handle, const ThreadInfo &thread);
            const char *GetFormattedAddressString(uintptr_t address);
            void SaveToFile(ScopedFile &file);
        private:
            bool TryFindModule(uintptr_t *out_address, uintptr_t guess);
            void TryAddModule(uintptr_t guess);
            void GetModuleName(char *out_name, uintptr_t text_start, uintptr_t ro_start);
            void GetModuleId(u8 *out, uintptr_t ro_start);
    };

}
