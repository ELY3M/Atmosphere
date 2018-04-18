#pragma once
#include <switch.h>
#include <type_traits>

#include "iserviceobject.hpp"
#include "iwaitable.hpp"
#include "serviceserver.hpp"

template <typename T>
class ServiceServer;

template <typename T>
class ServiceSession : public IWaitable {
    static_assert(std::is_base_of<IServiceObject, T>::value, "Service Objects must derive from IServiceObject");
    
    T *service_object;
    ServiceServer<T> *server;
    Handle server_handle;
    Handle client_handle;
    public:
        ServiceSession<T>(ServiceServer<T> *s, Handle s_h, Handle c_h) : server(s), server_handle(s_h), client_handle(c_h) {
            this->service_object = new T();
        }
        
        virtual ~ServiceSession() {
            delete this->service_object;
            if (server_handle) {
                svcCloseHandle(server_handle);
            }
            if (client_handle) {
                svcCloseHandle(client_handle);
            }
        }

        T *get_service_object() { return this->service_object; }
        Handle get_server_handle() { return this->server_handle; }
        Handle get_client_handle() { return this->client_handle; }
        
        /* IWaitable */
        virtual unsigned int get_num_waitables() {
            return 1;
        }
        
        virtual void get_waitables(IWaitable **dst) {
            dst[0] = this;
        }
        
        virtual void delete_child(IWaitable *child) {
            /* TODO: Panic, because we can never have any children. */
        }
        
        virtual Handle get_handle() {
            return this->server_handle;
        }
        
        virtual Result handle_signaled(u64 timeout) {
            Result rc;
            int handle_index;
            if (R_SUCCEEDED(rc = svcReplyAndReceive(&handle_index, &this->server_handle, 1, 0, timeout))) {
                if (handle_index != 0) {
                    /* TODO: Panic? */
                }
                u32 *cmdbuf = (u32 *)armGetTls();
                u32 out_words = 4;
                u32 extra_rawdata_count = 0;
                u32 wordcount = 0;
                Result retval = 0;
                u32 *rawdata_start = cmdbuf;
                
                IpcParsedCommand r;
                IpcCommand c;
                
                ipcInitialize(&c);
                
                retval = ipcParse(&r);
                
                if (R_SUCCEEDED(retval)) {
                    rawdata_start = (u32 *)r.Raw;
                    wordcount = r.RawSize;
                    retval = this->service_object->dispatch(&r, &c, cmdbuf, rawdata_start[2], &rawdata_start[4], wordcount - 6, &cmdbuf[8], &extra_rawdata_count);
                    out_words += extra_rawdata_count;
                }
                
                struct {
                    u64 magic;
                    u64 retval;
                } *raw;

                raw = (decltype(raw))ipcPrepareHeader(&c, out_words);

                raw->magic = SFCO_MAGIC;
                raw->retval = retval;
                
                rc = svcReplyAndReceive(&handle_index, &this->server_handle, 1, this->server_handle, 0);
            }
              
            return rc;
        }
};