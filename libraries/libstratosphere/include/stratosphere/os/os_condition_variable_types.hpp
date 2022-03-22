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
#include <vapours.hpp>
#include <stratosphere/os/impl/os_internal_condition_variable.hpp>

namespace ams::os {

    struct ConditionVariableType {
        enum State {
            State_NotInitialized = 0,
            State_Initialized    = 1,
        };

        u8 state;
        union {
            s32 _arr[sizeof(impl::InternalConditionVariableStorage) / sizeof(s32)];
            impl::InternalConditionVariableStorage _storage;
            impl::InternalConditionVariableStorageTypeForConstantInitialize _storage_for_constant_initialize;
        };
    };
    static_assert(std::is_trivial<ConditionVariableType>::value);

}
