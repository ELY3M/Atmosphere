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
#include <stratosphere.hpp>

#define AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO(C, H)                                                                                                   \
    AMS_SF_METHOD_INFO(C, H, 0, Result, ConvertCurrentObjectToDomain, (ams::sf::Out<ams::sf::cmif::DomainObjectId> out),                     (out))            \
    AMS_SF_METHOD_INFO(C, H, 1, Result, CopyFromCurrentDomain,        (ams::sf::OutMoveHandle out, ams::sf::cmif::DomainObjectId object_id), (out, object_id)) \
    AMS_SF_METHOD_INFO(C, H, 2, Result, CloneCurrentObject,           (ams::sf::OutMoveHandle out),                                          (out))            \
    AMS_SF_METHOD_INFO(C, H, 3, void,   QueryPointerBufferSize,       (ams::sf::Out<u16> out),                                               (out))            \
    AMS_SF_METHOD_INFO(C, H, 4, Result, CloneCurrentObjectEx,         (ams::sf::OutMoveHandle out, u32 tag),                                 (out, tag))

AMS_SF_DEFINE_INTERFACE(ams::sf::hipc::impl, IHipcManager, AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO)

namespace ams::sf::hipc {

    namespace impl {

        class HipcManagerImpl {
            private:
                ServerDomainSessionManager *manager;
                ServerSession *session;
                bool is_mitm_session;
            private:
                Result CloneCurrentObjectImpl(sf::OutMoveHandle &out_client_handle, ServerSessionManager *tagged_manager) {
                    /* Clone the object. */
                    cmif::ServiceObjectHolder &&clone = this->session->srv_obj_holder.Clone();
                    R_UNLESS(clone, sf::hipc::ResultDomainObjectNotFound());

                    /* Create new session handles. */
                    os::NativeHandle server_handle, client_handle;
                    R_ABORT_UNLESS(hipc::CreateSession(std::addressof(server_handle), std::addressof(client_handle)));

                    /* Register with manager. */
                    if (!is_mitm_session) {
                        R_ABORT_UNLESS(tagged_manager->RegisterSession(server_handle, std::move(clone)));
                    } else {
                        /* Clone the forward service. */
                        std::shared_ptr<::Service> new_forward_service = std::move(ServerSession::CreateForwardService());
                        R_ABORT_UNLESS(serviceClone(this->session->forward_service.get(), new_forward_service.get()));
                        R_ABORT_UNLESS(tagged_manager->RegisterMitmSession(server_handle, std::move(clone), std::move(new_forward_service)));
                    }

                    /* Set output client handle. */
                    out_client_handle.SetValue(client_handle, false);
                    return ResultSuccess();
                }
            public:
                explicit HipcManagerImpl(ServerDomainSessionManager *m, ServerSession *s) : manager(m), session(s), is_mitm_session(s->forward_service != nullptr) {
                    /* ... */
                }

                Result ConvertCurrentObjectToDomain(sf::Out<cmif::DomainObjectId> out) {
                    /* Allocate a domain. */
                    auto domain = this->manager->AllocateDomainServiceObject();
                    R_UNLESS(domain, sf::hipc::ResultOutOfDomains());

                    /* Set up the new domain object. */
                    cmif::DomainObjectId object_id = cmif::InvalidDomainObjectId;
                    if (this->is_mitm_session) {
                        /* Make a new shared pointer to manage the allocated domain. */
                        SharedPointer<cmif::MitmDomainServiceObject> cmif_domain(static_cast<cmif::MitmDomainServiceObject *>(domain), false);

                        /* Convert the remote session to domain. */
                        AMS_ABORT_UNLESS(session->forward_service->own_handle);
                        R_TRY(serviceConvertToDomain(session->forward_service.get()));

                        /* The object ID reservation cannot fail here, as that would cause desynchronization from target domain. */
                        object_id = cmif::DomainObjectId{session->forward_service->object_id};
                        domain->ReserveSpecificIds(&object_id, 1);

                        /* Register the object. */
                        domain->RegisterObject(object_id, std::move(session->srv_obj_holder));

                        /* Set the new object holder. */
                        session->srv_obj_holder = cmif::ServiceObjectHolder(std::move(cmif_domain));
                    } else {
                        /* Make a new shared pointer to manage the allocated domain. */
                        SharedPointer<cmif::DomainServiceObject> cmif_domain(domain, false);

                        /* Reserve a new object in the domain. */
                        R_TRY(domain->ReserveIds(&object_id, 1));

                        /* Register the object. */
                        domain->RegisterObject(object_id, std::move(session->srv_obj_holder));

                        /* Set the new object holder. */
                        session->srv_obj_holder = cmif::ServiceObjectHolder(std::move(cmif_domain));
                    }

                    /* Return the allocated id. */
                    AMS_ABORT_UNLESS(object_id != cmif::InvalidDomainObjectId);
                    *out = object_id;
                    return ResultSuccess();
                }

                Result CopyFromCurrentDomain(sf::OutMoveHandle out, cmif::DomainObjectId object_id) {
                    /* Get domain. */
                    auto domain = this->session->srv_obj_holder.GetServiceObject<cmif::DomainServiceObject>();
                    R_UNLESS(domain != nullptr, sf::hipc::ResultTargetNotDomain());

                    /* Get domain object. */
                    auto &&object = domain->GetObject(object_id);
                    if (!object) {
                        R_UNLESS(this->is_mitm_session, sf::hipc::ResultDomainObjectNotFound());

                        os::NativeHandle handle;
                        R_TRY(cmifCopyFromCurrentDomain(this->session->forward_service->session, object_id.value, std::addressof(handle)));

                        out.SetValue(handle, false);
                        return ResultSuccess();
                    }

                    if (!this->is_mitm_session || object_id.value != serviceGetObjectId(this->session->forward_service.get())) {
                        /* Create new session handles. */
                        os::NativeHandle server_handle, client_handle;
                        R_ABORT_UNLESS(hipc::CreateSession(std::addressof(server_handle), std::addressof(client_handle)));

                        /* Register. */
                        R_ABORT_UNLESS(this->manager->RegisterSession(server_handle, std::move(object)));

                        /* Set output client handle. */
                        out.SetValue(client_handle, false);
                    } else {
                        /* Copy from the target domain. */
                        os::NativeHandle new_forward_target;
                        R_TRY(cmifCopyFromCurrentDomain(this->session->forward_service->session, object_id.value, &new_forward_target));

                        /* Create new session handles. */
                        os::NativeHandle server_handle, client_handle;
                        R_ABORT_UNLESS(hipc::CreateSession(std::addressof(server_handle), std::addressof(client_handle)));

                        /* Register. */
                        std::shared_ptr<::Service> new_forward_service = std::move(ServerSession::CreateForwardService());
                        serviceCreate(new_forward_service.get(), new_forward_target);
                        R_ABORT_UNLESS(this->manager->RegisterMitmSession(server_handle, std::move(object), std::move(new_forward_service)));

                        /* Set output client handle. */
                        out.SetValue(client_handle, false);
                    }

                    return ResultSuccess();
                }

                Result CloneCurrentObject(sf::OutMoveHandle out) {
                    return this->CloneCurrentObjectImpl(out, this->manager);
                }

                void QueryPointerBufferSize(sf::Out<u16> out) {
                    out.SetValue(this->session->pointer_buffer.GetSize());
                }

                Result CloneCurrentObjectEx(sf::OutMoveHandle out, u32 tag) {
                    return this->CloneCurrentObjectImpl(out, this->manager->GetSessionManagerByTag(tag));
                }
        };
        static_assert(IsIHipcManager<HipcManagerImpl>);

    }

    Result ServerDomainSessionManager::DispatchManagerRequest(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) {
        /* Make a stack object, and pass a shared pointer to it to DispatchRequest. */
        /* Note: This is safe, as no additional references to the hipc manager can ever be stored. */
        /* The shared pointer to stack object is definitely gross, though. */
        UnmanagedServiceObject<impl::IHipcManager, impl::HipcManagerImpl> hipc_manager(this, session);
        return this->DispatchRequest(cmif::ServiceObjectHolder(hipc_manager.GetShared()), session, in_message, out_message);
    }

}
