#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/event.h>
#include <sys/process.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/memory.h>
#include <sys/types.h>
#include <sys/sys_time.h>
#include <sys/timer.h>
#include <cell/pad.h>
#include <cell/cell_fs.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>

#include <time.h>

#include "inc/vsh_exports.h"
#include "misc.h"
#include "mem.h"
#include "blitting.h"


#ifdef DEBUG
#include "network.h"
#endif

#ifdef HAVE_SSCROLLER
#include "scroller.h"
#endif

#ifdef HAVE_STARFIELD
#include "starfield.h"
#endif


SYS_MODULE_INFO (VSH_MENU, 0, 1, 0);
SYS_MODULE_START(vsh_menu_start);
SYS_MODULE_STOP (vsh_menu_stop);

#define THREAD_NAME         "vsh_menu_thread"
#define STOP_THREAD_NAME    "vsh_menu_stop_thread"

static sys_ppu_thread_t vsh_menu_tid = -1;
static int8_t done = 0;
int32_t vsh_menu_start(uint64_t arg);
int32_t vsh_menu_stop(void);


// a temporary color, to preview text in realtime
uint8_t a, r, g, b;

/***********************************************************************
* sys_ppu_thread_exit, direct over syscall
***********************************************************************/
static inline void _sys_ppu_thread_exit(uint64_t val)
{
    system_call_1(41, val);
}


/***********************************************************************
* 
***********************************************************************/
static inline sys_prx_id_t prx_get_module_id_by_address(void *addr)
{
    system_call_1(461, (uint64_t)(uint32_t)addr);
    return (int32_t)p1;
}


////////////////////////////////////////////////////////////////////////
// BLITTING
static int8_t menu_running = 0; // vsh menu off(0) or on(1)
static int8_t line = 0;         // current line into menu, init 0 (entry 1:)
static int8_t view = 0;         // menu view, init 0 (main view)

// max menu entries per view
static int8_t max_menu[] = {9, 7, 5};

static uint32_t bg_color_menu[] =
{
    0x7F0000FF,     // blue, semitransparent
    0x7FFF0000,     // red, semitransparent
    0x7F00FF00      // green, semitransparent
};

// menu entry strings
const char *entry_str[3][9] = {
{
    "1: Make a single beep",
    "2: Make a double beep",
    "3: Enter second menu view",
    "4: Enter third menu view",
    "5: Play trophy sound",
    "6: Make screenshot",
    "7: Make screenshot with Menu",
    "8: Reset PS3",
    "9: Shutdown PS3"
},
{   // red menu
    "1: Back to main view",
    "2: screenshot",
    "3: Alpha",
    "4: Red",
    "5: Green",
    "6: Blue",
    "7: test"
},
{
    "1: bg 1",
    "2: bg 2",
    "3: bg 3",
    "4: fg 1",
    "5: Back to main view"
}
};

/***********************************************************************
* draw a frame
***********************************************************************/
static void draw_frame(CellPadData *data)
{
    int8_t i;
    uint16_t tx, ty;

    // all 32bit colors are ARGB, the framebuffer format
    set_foreground_color(0xFFFFFFFF);     // white, opac

    // set the right background color for current view
    set_background_color(bg_color_menu[view]);

    draw_background();

    #ifdef HAVE_STARFIELD
    draw_stars();       // to keep them under text lines
    #endif

    // print headline string, coordinates in canvas
    print_text(BORD_D, BORD_D, "PS3 VSH Menu");

    // print all menu entries for view, and the current selected entry in green
    for(i = 0; i < max_menu[view]; i++)
    {
        i == line ? set_foreground_color(0xFF00FF00) : set_foreground_color(0xFFFFFFFF);

        print_text(BORD_D, 8 + ((FONT_H + FONT_D) * (i + 1)), entry_str[view][i]);
    }

    // (re)set back after draw last line
    set_foreground_color(0xFFFFFFFF);

    // ...

    // second menu, red one:
    if(view == 1)
    {
        /* hexdump pad data: store in text 8 buttons in a row
           in "%.4x" format plus 1 for ':', last one will be
           replaced by terminator, no need to add 1 */
        char tmp_ln[8 * (4 + 1)];
        
        // test text alignment
        sprintf(tmp_ln, "*%p, %d bytes;", data, data->len * sizeof(uint16_t));
        tx = get_aligned_x(tmp_ln, CENTER);
        print_text(tx, 180, tmp_ln);

        // hexdump first 32 buttons
        uint16_t x = 0, ty = 200;
        for(i = 0; i < 32; i++)
        {
            sprintf(&tmp_ln[x], "%.4x:", data->button[i]);
            x += 5;
            tmp_ln[x] = '\0';

            if(x %8 == 0)
            {
                // overwrite last ':' with terminator
                tmp_ln[x -1] = '\0';
                tx = get_aligned_x(tmp_ln, CENTER);
                print_text(tx, ty, tmp_ln);
                x = 0, ty += (FONT_H + FONT_D);            // additional px to next line
            }
        }

        // sys memory stats
        read_meminfo(tmp_ln);
        tx = get_aligned_x(tmp_ln, RIGHT) - BORD_D;    // additional px from R margin
        print_text(tx, CANVAS_H - (FONT_H + FONT_D) - BORD_D, tmp_ln); // from bottom

    }
    else
    if(view == 2)   // only on third view
    {
        char tmp_ln[8 + 1];

        // print all bg_color entries, and the current selected entry in green
        tx = CANVAS_W - (8 * FONT_W) - BORD_D;   // additional px from R margin
        for(i = 0; i < 3; i++)
        {
            sprintf(tmp_ln, "%.8x", bg_color_menu[i]);
            ty = 8 + ((FONT_H + FONT_D) * (i + 1));
            print_text(tx, ty, tmp_ln);

        // testing sine
        float amp = f_sinf(10);
        sprintf(templn, "%.4f", amp);
        tmp_x = get_aligned_x(templn, RIGHT) -4;
        print_text(tmp_x, 4 + (FONT_H +1), templn);

                // (re)set back after marked text
                set_foreground_color(0xFFFFFFFF);
            }
        }
        
    } //end if(view == 2)

    // ...

    #ifdef HAVE_SSCROLLER
    // testing sine in a scroller
    draw_text(330);
    move_text();
    #endif

}



/***********************************************************************
* stop VSH Menu
***********************************************************************/
static void stop_VSH_Menu(void)
{
    // menu off
    menu_running = 0;

    // free heap memory
    destroy_heap();

    // continue rsx rendering
    rsx_fifo_pause(0);
}


/***********************************************************************
* execute a menu action, based on line(current selected menu entry)
***********************************************************************/
static void do_leftright_action(uint16_t curpad)
{
  if(view == 1)     // only on second screen
  {
    switch(line)
    {
      case 2:       // 3: Alpha
        (curpad & PAD_LEFT) ? a-- : a++;
        break;
      case 3:       // 4: Red
        (curpad & PAD_LEFT) ? r-- : r++;
        break;
      case 4:       // 5: Green
        (curpad & PAD_LEFT) ? g-- : g++;
        break;
      case 5:       // 6: Blue
        (curpad & PAD_LEFT) ? b-- : b++;
        break;

      default:      // do nothing
        return;
    }

    // update second screen bg_color
    bg_color_menu[view] = ARGB(a, r, g, b);

    play_rco_sound("system_plugin", "snd_cursor");
  }
}


/***********************************************************************
* execute a menu action, based on line(current selected menu entry)
***********************************************************************/
static void do_menu_action(void)
{
    switch(view)
    {
      case 0:                    // main menu view
        switch(line)
        {
          case 0:                  // "1: Make a single beep"
            buzzer(1);
            break;
          case 1:                  // "2: Make a double beep"
            buzzer(2);
            break;
          case 2:                  // "3: Enter second menu view"
            view = 1;              // change menu view
            line = 0;              // on start entry
            break;
          case 3:                  // "4: Enter third menu view"
            view = 2;              // change menu view
            line = 0;              // on start entry
            break;
          case 4:                  // "5: Play trophy sound"
            play_rco_sound("system_plugin", "snd_trophy");
            break;
          case 5:                  // "6: Make screenshot"
            screenshot(0);         // xmb only
            play_rco_sound("system_plugin", "snd_system_ok");
            break;
          case 6:                  // "7: Make screenshot with menu"
            screenshot(1);         //
            play_rco_sound("system_plugin", "snd_system_ok");
            break;
          case 7:                  // "8: Reset PS3"
            stop_VSH_Menu();
            delete_turnoff_flag();
            sys_timer_sleep(1);
            shutdown_reset(2);
            break;
          case 8:                  // "9: Shutdown PS3"
            stop_VSH_Menu();
            delete_turnoff_flag();
            sys_timer_sleep(1);
            shutdown_reset(1);
            break;
        }
        break;

      case 1:                   // second menu view
        switch(line)
        {
          case 0:               // 1: Back to main view"
            view = line = 0;
            break;
          case 1:               // 2: screenshot
            screenshot(1);
            break;
          case 2:               // 3: Alpha
            //...
            break;
          case 3:               // 4: Red
            //...
            break;
          case 4:               // 5: Green
            //...
            break;
          case 5:               // 6: Blue
            //...
            break;
          case 6:               // 7: test
            break;
        }
        break;

      case 2:                   // third menu view
        switch(line)
        {
          case 0:               // "1: bg 0"
            break;
          case 1:               // "2: test string..."
            //...
            break;
          case 2:               // "3: test string..."
            //...
            break;
          case 3:               // "4: test string..."
            //...
            break;
          case 4:               // "5: test string..."
            //...
            break;
        }
        break;
    }
}



/***********************************************************************
* plugin main ppu thread
***********************************************************************/
static void vsh_menu_thread(uint64_t arg)
{
    #ifdef DEBUG
    dbg_init();
    dbg_printf("programstart:\n");
    #endif

    uint16_t oldpad = 0, curpad = 0;
    CellPadData pdata;

    // wait for XMB, feedback
    sys_timer_sleep(13);

    //vshtask_notify("sprx running...");

    play_rco_sound("system_plugin", "snd_trophy");

    #ifdef HAVE_STARFIELD
    init_once(/* stars */);
    #endif

    // custom bg_color init
    a = GET_A(bg_color_menu[1]);
    r = GET_R(bg_color_menu[1]);
    g = GET_G(bg_color_menu[1]);
    b = GET_B(bg_color_menu[1]);

    while(1)
    {
        // if VSH Menu is running, we get pad data over our MyPadGetData()
        // else, we use the vsh pad_data struct
        if(menu_running)
            MyPadGetData(0, &pdata);
        else
            VSHPadGetData(&pdata);

        // if pad_data and we are in XMB(vshmain_EB757101() == 0)
        if((pdata.len > 0)
        && (vshmain_EB757101() == 0)
        )
        {
            curpad = (pdata.button[2] | (pdata.button[3] << 8));

            if((curpad & PAD_SELECT) && (curpad != oldpad))
            {
                switch(menu_running)
                {
                    // VSH Menu not running, start VSH Menu
                    case 0:
                      // main view and start on first entry 
                      view = line = 0;

                      //
                      pause_RSX_rendering();

                      // create VSH Menu heap memory from memory container 1("app")
                      create_heap(64);       // 64 MB

                      // initialize VSH Menu graphic (init drawing context, alloc buffers, blah, blah, blah...)
                      init_graphic();

                      // stop vsh pad
                      start_stop_vsh_pad(0);

                      // set menu_running
                      menu_running = 1;

                      break;

                    // VSH Menu is running, stop VSH Menu
                    case 1:
                      stop_VSH_Menu();

                      // restart vsh pad
                      start_stop_vsh_pad(1);

                      break;
                }

                oldpad = 0;
                sys_timer_usleep(300000);
            }


          // VSH Menu is running, draw menu / check pad
          if(menu_running)
          {
                #ifdef DEBUG
                dbg_printf("%p\n", pdata);
                #endif

                draw_frame(&pdata);

                flip_frame();

                if(curpad != oldpad)
                {

                    if(curpad & PAD_UP)
                    {
                        if(line <= 0){
                            line = 0;
                        }else{
                            line--;
                            play_rco_sound("system_plugin", "snd_cursor");
                        }
                    }

                    if(curpad & PAD_DOWN)
                    {
                        if(line >= max_menu[view]-1){
                            line = max_menu[view]-1;
                        }else{
                            line++;
                            play_rco_sound("system_plugin", "snd_cursor");
                        }
                    }

                    if(curpad & PAD_LEFT
                    || curpad & PAD_RIGHT) do_leftright_action(curpad);

                    if(curpad & PAD_CROSS) do_menu_action();

                }

                // ...

                sys_timer_usleep(30);

            } // end VSH Menu is running

            oldpad = curpad;
        }else{
            oldpad = 0;
        }
    }

    #ifdef DEBUG
    dbg_fini();
    #endif
    sys_ppu_thread_exit(0);
}


/***********************************************************************
* start thread
***********************************************************************/
int32_t vsh_menu_start(uint64_t arg)
{
    sys_ppu_thread_create(&vsh_menu_tid, vsh_menu_thread, 0, 3000, 0x4000, 1, THREAD_NAME);

    _sys_ppu_thread_exit(0);
    return SYS_PRX_RESIDENT;
}


/***********************************************************************
* stop thread
***********************************************************************/
static void vsh_menu_stop_thread(uint64_t arg)
{
    done = 1;

    if(vsh_menu_tid != (sys_ppu_thread_t)-1)
    {
        uint64_t exit_code;
        sys_ppu_thread_join(vsh_menu_tid, &exit_code);
    }

    sys_ppu_thread_exit(0);
}


/***********************************************************************
* 
***********************************************************************/
static void finalize_module(void)
{
    uint64_t meminfo[5];

    sys_prx_id_t prx = prx_get_module_id_by_address(finalize_module);

    meminfo[0] = 0x28;
    meminfo[1] = 2;
    meminfo[3] = 0;

    system_call_3(482, prx, 0, (uint64_t)(uint32_t)meminfo);
}


/***********************************************************************
* 
***********************************************************************/
int vsh_menu_stop(void)
{
    sys_ppu_thread_t t;
    uint64_t exit_code;

    sys_ppu_thread_create(&t, vsh_menu_stop_thread, 0, 0, 0x2000, 1, STOP_THREAD_NAME);
    sys_ppu_thread_join(t, &exit_code);

    finalize_module();
    _sys_ppu_thread_exit(0);
    return SYS_PRX_STOP_OK;
}
