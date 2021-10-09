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
#include <stratosphere.hpp>

namespace ams::pgl::srv {

    class IShellEventObserver {
        public:
            virtual void Notify(const pm::ProcessEventInfo &info) = 0;
    };

    class ShellEventObserverHolder : public util::IntrusiveListBaseNode<ShellEventObserverHolder> {
        private:
            IShellEventObserver *observer;
        public:
            explicit ShellEventObserverHolder(IShellEventObserver *observer) : observer(observer) { /* ... */ }

            void Notify(const pm::ProcessEventInfo &info) {
                this->observer->Notify(info);
            }
    };

    class ShellEventObserverImpl : public IShellEventObserver {
        private:
            static constexpr size_t QueueCapacity = 0x20;
        private:
            os::MessageQueue message_queue;
            uintptr_t queue_buffer[QueueCapacity];
            os::SystemEvent event;
            util::TypedStorage<lmem::HeapCommonHead> heap_head;
            lmem::HeapHandle heap_handle;
            pm::ProcessEventInfo event_info_data[QueueCapacity];
            util::TypedStorage<ShellEventObserverHolder> holder;
        public:
            ShellEventObserverImpl();
            ~ShellEventObserverImpl();

            os::SystemEvent &GetEvent() {
                return this->event;
            }

            Result PopEventInfo(pm::ProcessEventInfo *out);

            virtual void Notify(const pm::ProcessEventInfo &info) override final;
    };

    class ShellEventObserverCmif : public ShellEventObserverImpl {
        public:
            Result GetProcessEventHandle(ams::sf::OutCopyHandle out);
            Result GetProcessEventInfo(ams::sf::Out<pm::ProcessEventInfo> out);
    };
    static_assert(pgl::sf::IsIEventObserver<ShellEventObserverCmif>);

    class ShellEventObserverTipc : public ShellEventObserverImpl {
        public:
            Result GetProcessEventHandle(ams::tipc::OutCopyHandle out);
            Result GetProcessEventInfo(ams::tipc::Out<pm::ProcessEventInfo> out);
    };
    static_assert(pgl::tipc::IsIEventObserver<ShellEventObserverTipc>);

}
