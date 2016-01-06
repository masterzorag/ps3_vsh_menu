#include "misc.h"
#include "inc/vsh_exports.h"


static void *vsh_pdata_addr = NULL;


/***********************************************************************
* search and return vsh_process toc
* Not the best way, but it work, it's generic and it is fast enough...
***********************************************************************/
static int32_t get_vsh_toc(void)
{
    int32_t pm_start  = 0x10000;
    int32_t v0 = 0, v1 = 0, v2 = 0;

    while(pm_start < 0x700000)
    {
        v0 = *(int32_t*)(pm_start +0x00);
        v1 = *(int32_t*)(pm_start +0x04);
        v2 = *(int32_t*)(pm_start +0x0C);

        if((v0 == 0x10200 /* .init_proc() */ ) && (v1 == v2)) break;

        pm_start += 4;
    }

    return (int32_t)v1; 
}

/***********************************************************************
* get vsh io_pad_object
***********************************************************************/
static int32_t get_vsh_pad_obj(void)
{
    int16_t idx = *(int32_t*)(*(int32_t*)((void*)sys_io_3733EA3C)) & 0x0000FFFF;  // get got_entry idx from first instruction,  
    int32_t got_entry = (idx + get_vsh_toc());                                    // get got_entry of io_pad_object
    return (int32_t)(*(int32_t*)got_entry);                                       // return io_pad_object address
}

/***********************************************************************
* get pad data struct of vsh process
* 
* CellPadData *data = data struct for holding pad_data
***********************************************************************/
void VSHPadGetData(CellPadData *data)
{
    int32_t pm_start = 0x10000;
    uint64_t pat[2]  = {0x380000077D3F4B78ULL, 0x7D6C5B787C0903A6ULL};

    if(!vsh_pdata_addr)        // first time, get address
    {
        while(pm_start < 0x700000)
        {
            if((*(uint64_t*) pm_start     == pat[0])
            && (*(uint64_t*)(pm_start +8) == pat[1]))
            {
                vsh_pdata_addr = (void*)(int32_t)(
                    (int32_t)((*(int32_t*)(pm_start +0x234) & 0x0000FFFF) <<16) +
                    (int16_t)( *(int32_t*)(pm_start +0x244) & 0x0000FFFF));

                break;
            }

            pm_start += 4;
        }
    }

    memcpy(data, vsh_pdata_addr, sizeof(CellPadData));
}

/***********************************************************************
* set/unset io_pad_library init flag
* 
* uint8_t flag = 0(unset) or 1(set)
* 
* To prevent vsh pad events during vsh-menu, we set this flag to 0
* (pad_library not init). Each try of vsh to get pad_data leads intern
* to error 0x80121104(CELL_PAD_ERROR_UNINITIALIZED) and nothing will
* happen. The lib is of course not deinit, there are pad(s) mapped and we
* can get pad_data direct over syscall sys_hid_manager_read() in our
* own function MyPadGetData().
***********************************************************************/
void start_stop_vsh_pad(uint8_t flag)
{
    int32_t lib_init_flag = get_vsh_pad_obj();
    *(uint8_t*)lib_init_flag = flag;
}

/***********************************************************************
* get pad data direct over syscall 
* (very simple, no error-handling, no synchronization...)
* 
* int32_t port_no   = pad port number (0 - 7)
* CellPadData *data = data struct for holding pad_data
***********************************************************************/
void MyPadGetData(int32_t port_no, CellPadData *data)
{
    int32_t port = *(int32_t*)(*(int32_t*)(get_vsh_pad_obj() +4) + 0x104 + port_no * 0xE8);

    // sys_hid_manager_read()
    system_call_4(0x1F6, (uint64_t)port, /*0x02*//*0x82*/0xFF, (uint64_t)(uint32_t)data+4, 0x80);

    data->len = (int32_t)p1;
}

/***********************************************************************
* pause/continue rsx fifo
* 
* uint8_t pause    = pause fifo (1), continue fifo (0)
***********************************************************************/
int32_t rsx_fifo_pause(uint8_t pause)
{
    // lv2 sys_rsx_context_attribute()
    system_call_6(0x2A2, 0x55555555ULL, (uint64_t)(pause ? 2 : 3), 0, 0, 0, 0);

    return (int32_t)p1;
}

/***********************************************************************
* 
***********************************************************************/
void delete_turnoff_flag()
{
    unlink((char*)"/dev_hdd0/tmp/turnoff");
}

/***********************************************************************
* play a rco sound file
* 
* const char *plugin = plugin name own the rco file we would play a sound from
* const char *sound  = sound recource file in rco we would play
***********************************************************************/
void play_rco_sound(const char *plugin, const char *sound)
{
    paf_B93AFE7E((paf_F21655F3(plugin)), sound, 1, 0);
}

/***********************************************************************
* ring buzzer
***********************************************************************/
void buzzer(uint8_t mode)
{
    uint16_t param = 0;

    switch(mode)
    {
        case 1: param = 0x0006; break;    // single beep
        case 2: param = 0x0036; break;    // double beep
        case 3: param = 0x01B6; break;    // triple beep
        case 4: param = 0x0FFF; break;    // continuous beep, gruesome!!!
    }

    system_call_3(392, 0x1007, 0xA, param);
}

/***********************************************************************
* #define SC_GET_TEMPERATURE                (383)
***********************************************************************/
static void get_temperature(uint32_t _dev, uint32_t *_temp)
{
    system_call_2(383, (uint64_t)(uint32_t) _dev, (uint64_t)(uint32_t) _temp);
}

/***********************************************************************
* a wrapper to read CPU and RSX temperature data
***********************************************************************/
void read_temperature(uint32_t *t1, uint32_t *t2)
{
    get_temperature(0, t1); // 3E030000 -> 3E.03°C -> 62.(03/256)°C
    get_temperature(1, t2);
    *t1 >>= 24, *t2 >>= 24;
}

/***********************************************************************
* a wrapper to read sys_memory data and compose text string from
***********************************************************************/
void read_meminfo(char *data)
{
    struct {
        uint32_t total;
        uint32_t avail;
    } meminfo;
    system_call_1(352, (uint64_t) &meminfo);

    sprintf(data, "freemem:%i/%ikb", meminfo.avail /1024, meminfo.total /1024);
}

/***********************************************************************
* write data to binary file, minimal error checking
***********************************************************************/
size_t write_bin(const char *path, const uint8_t *data, const size_t len)
{
    FILE *fd = fopen(path, "wb");
    if(!fd)
        return -1;

    size_t written = fwrite(data, sizeof(uint8_t), len, fd);

    fclose(fd);

    if(written == len) buzzer(1); // feedback

    return written;
}

/***********************************************************************
* peek & poke
***********************************************************************/
uint64_t lv2peek(uint64_t addr)
{
    system_call_1(6, addr);
    return_to_user_prog(uint64_t);
}

uint64_t lv2poke(uint64_t addr, uint64_t value)
{
    system_call_2(7, addr, value);
    return_to_user_prog(uint64_t);
}

uint64_t lv1peek(uint64_t addr)
{
    system_call_1(8, addr);
    return_to_user_prog(uint64_t);
}

uint64_t lv1poke(uint64_t addr, uint64_t value)
{
    system_call_2(9, addr, value);
    return_to_user_prog(uint64_t);
}
