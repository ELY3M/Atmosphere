#pragma once
#include <switch.h>

#include "iserviceobject.hpp"

enum DebugMonitorServiceCmd {
    Dmnt_Cmd_AddTitleToLaunchQueue = 0,
    Dmnt_Cmd_ClearLaunchQueue = 1,
    Dmnt_Cmd_GetNsoInfo = 2
};

class DebugMonitorService : IServiceObject {
    public:
        Result dispatch(IpcParsedCommand *r, IpcCommand *out_c, u32 *cmd_buf, u32 cmd_id, u32 *in_rawdata, u32 in_rawdata_size, u32 *out_rawdata, u32 *out_raw_data_count);
        
    private:
        /* Actual commands. */
        Result add_title_to_launch_queue(u64 tid, const char *args, size_t args_size);
        Result clear_launch_queue();
        Result get_nso_info(u64 pid, void *out, size_t out_size, u32 *out_num_nsos);
};