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
// TIMING
static clock_t startm, stopm;
static uint16_t tick = 0;
static double fps = 0;


////////////////////////////////////////////////////////////////////////
// BLITTING
static bool   menu_running = 0; // vsh menu off(0) or on(1)
static int8_t line = 0;         // current line into menu, init 0 (entry 1:)
static int8_t view = 0;         // menu view, init 0 (main view)
static int8_t col  = 0;         // current coloumn into menu, init 0 (entry 1:)

// Every view have its dedicated Bg/Fg/Fg2 color default combination
static uint32_t menu_colors[4][3] __attribute__((aligned(16))) = {
{   // Bg_colors[0][0-2]
    0x7F0000FF,     // blue, semitransparent
    0x70000000,     // black, semitransparent
    0x7F00FF00      // green, semitransparent
},
{  // Fg_colors[1][0-2]
    0xFFB0B0B0,     // blue, opac
    0xFFA0A0A0,     // black, opac
    0xFFFFFFFF      // white, opac
},
{   // Fg_2_colors[2] to use better gradient color, foreach view
    0xFF600090,
    0xFF6060A0,
    0xFF303030
},
{   // unused at the moment
    0xFF303030,     // shadows can be the same across view
    0x00000000,
    0x00000000
}
};

// max menu entries per view
static int8_t max_menu[] = {9, 7, 3};

// menu entry strings
const char *entry_str[3][9] __attribute__((aligned(4))) = {
{
    "1: Make a single beep",
    "2: Make a double beep",
    "3: Enter second menu view",
    "4: Setup colors menu view",
    "5: Play trophy sound",
    "6: Make screenshot",
    "7: Make screenshot with Menu",
    "8: Reset PS3",
    "9: Shutdown PS3"
},
{   // red menu
    "1: test",
    "2: screenshot",
    "3: Alpha",
    "4: Red",
    "5: Green",
    "6: Blue",
    "7: test"
},
{
    "1:bg,fg,f2",
    "2:bg,fg,f2",
    "3:bg,fg,f2",
}
};

/***********************************************************************
* draw a frame
***********************************************************************/
static void draw_frame(CellPadData *data)
{
    char tmp_ln[8 * (4 + 1)] __attribute__((aligned(16)));
    uint16_t tx, ty;
    int8_t i;

    /* all 32bit colors are ARGB, the framebuffer format */

    // set the right colors for current view
    set_background_color(menu_colors[0][view]);
    set_foreground_color(menu_colors[1][view]);

    draw_background();
    
    #ifdef HAVE_STARFIELD
    draw_stars();       // to keep them under text lines
    #endif

    if(view != 1) 
        draw_png(0, 100, 104, 0, 0, 163, 296);

    // print headline string, coordinates in canvas
    print_text(BORD_D, BORD_D, "PS3 VSH Menu");

    // print all menu entries for view, and the current selected entry in green
    for(i = 0; i < max_menu[view]; i++)
    {
        i == line ? set_foreground_color((uint32_t)rand())        // blink
                  : set_foreground_color(menu_colors[1][view]);

        ty = 8 + ((FONT_H + FONT_D) * (i + 1));
        print_text(BORD_D, ty, entry_str[view][i]);
    }

    // (re)set back after draw last line
    set_foreground_color(menu_colors[1][view]);

    // ...

    // second menu, realtime pad data dump:
    if(view == 1)
    {   // used in text position
        uint16_t ty = 180;
        uint8_t x = 0;

        sprintf(tmp_ln, "*%p, %d bytes:", data, data->len * sizeof(uint16_t));
        tx = get_aligned_x(tmp_ln, CENTER);
        print_text(tx, ty, tmp_ln);

        /* hexdump pad data: store in text 8 buttons in a row
           in "%.4x" format plus 1 for ':', last one will be
           replaced by terminator, no need to add 1
           char tmp_ln[8 * (4 + 1)]; */

        tx = 0, ty = 200;
        for(i = 0; i < 32; i++)
        {
            sprintf(&tmp_ln[x], "%.4x:", data->button[i]);
            x += 5;
            tmp_ln[x] = '\0';

            if(x %8 == 0)
            {   // overwrite last ':' with terminator
                tmp_ln[x -1] = '\0';
                if(tx == 0)
                    tx = get_aligned_x(tmp_ln, CENTER);   // only once

                print_text(tx, ty, tmp_ln);
                x = 0, ty += (FONT_H + FONT_D);      // additional px to next line
            }
        }

    }
    else if(view == 2)   // only on third view
    {
        uint32_t *color = NULL;
        uint8_t x, g;  // grounds
        // bg[0][0-2]   [(col -1) /4][line]
        // fg[1][0-2]   [(col -1) /4][line]

        // print all color entries, and the selected entry in green
        for(g = 0; g < 3; g++)  // grounds, Bg | Fg | Fg2 ...
        {
            tx = 180 + ((8 + 1 /* chars of distance */) * FONT_W) * g;
            for(i = 0; i < 3; i++)  // first 3 lines/view: 0-2
            {
                ty = 8 + ((FONT_H + FONT_D) * (i + 1));
                color = &menu_colors[g][i];
                set_foreground_color(*color);

                sprintf(tmp_ln, "%.8x", *color);
                print_text(tx, ty, tmp_ln);

                if(i == line 
                && col > 0 
                && (col -1) /4 /* ARGB */ == g)  // mark in green selected color component
                {
                    set_foreground_color((uint32_t)rand());   // blink

                    // put a terminator and print one of AA:RR:GG:BB
                    x = ((col -1) %4) *2 /*chars*/;
                    tmp_ln[x +2] = '\0';
                    print_text(tx + (x * FONT_W), ty, &tmp_ln[x]);

                    // (re)set back after marked text
                    set_foreground_color(*color);
                }
            }
        }
    } //end if(view == 2)

    // ...

    // (re)set back after draw last line
    set_foreground_color(menu_colors[1][view]);

    #ifdef HAVE_SSCROLLER
    // testing sine in a scroller
    draw_text(330);
    move_text();
    #endif

    // position footer info
    ty = CANVAS_H - (FONT_H + FONT_D) - BORD_D;     // additional px from bottom

    // sys memory stats
    read_meminfo(tmp_ln);
    tx = get_aligned_x(tmp_ln, RIGHT) - BORD_D;     // additional px from R margin
    print_text(tx, ty, tmp_ln);


    // get timing, eventually compute fps
    stopm = clock();
    if(((double)stopm-startm) > 4000000)
    {   
        fps = (double)(tick / (((double)stopm-startm) / CLOCKS_PER_SEC));
        startm = clock(), tick = 0;                 // reset counter
    }
    sprintf(tmp_ln, "%2.1f fps", fps);
    print_text(BORD_D, ty, tmp_ln);

    // keep track of drawn frames
    tick++;

} // end draw_frame()


/***********************************************************************
* stop VSH Menu
***********************************************************************/
static void stop_VSH_Menu(void)
{
    // menu off
    menu_running = 0;

    #ifdef HAVE_SYS_FONT
    // unbind renderer and kill font-instance
    font_finalize();
    #endif

    // free heap memory
    destroy_heap();

    // continue rsx rendering
    rsx_fifo_pause(0);
}


/***********************************************************************
* execute a menu action, based on coloumn(current selected menu entry)
***********************************************************************/
static void do_updown_action(uint16_t curpad)
{
    bool flag = 0;
    uint8_t *value, step, max, color_comp[4];
    uint32_t *color = NULL;

    // setup common bounds for selected ground color(col, line)
    if(col)
    {   // A, R, G, B: 0x00-0xFF, can loop
        // bg [0][0-2]   [(col -1)/4][line]
        // fg [1][0-2]   [(col -1)/4][line]
        value = &color_comp[(col -1) %4];
        step  = -2, max = 0xFF;
        color = &menu_colors[(col -1) /4][line];

        color_comp[0] = GET_A(*color), color_comp[1] = GET_R(*color);
        color_comp[2] = GET_G(*color), color_comp[3] = GET_B(*color);
    }
    else
    {   // line: 0-max_menu[view], bounded: no loop
        value = (uint8_t*)&line;
        step  = 1, max = max_menu[view] -1;
    }

    // update value
    if(curpad & PAD_UP)
    {
        if((*value == 0 && col)
        || *value >  0) { *value -= step, flag = 1; }
    }
    else   // & PAD_DOWN
    {
        if((*value == max && col)
        || *value <  max) { *value += step, flag = 1; }
    }

    if(flag)
    {   // update selected color
        if(col) *color = ARGB(color_comp[0], color_comp[1],
                              color_comp[2], color_comp[3]);

        play_rco_sound("system_plugin", "snd_cursor");
    }
}


/***********************************************************************
* execute a menu action, based on line(current selected menu entry)
***********************************************************************/
static void do_leftright_action(uint16_t curpad)
{
  if((view == 2)    // only on third view
  && (line < 3))    // only for 3 bg_color
  {
      bool flag = 0;
      if(curpad & PAD_LEFT)
      {
          if(col > 0) { col--, flag = 1; }
      }
      else   // & PAD_RIGHT
      {   //0-argb-argb-argb, 1+4+4+4, 
          if(col < 4 *3) { col++, flag = 1; }
      }

      if(flag) play_rco_sound("system_plugin", "snd_cursor");
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
            line = col = 0;        // on start entry, for colors
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
* execute a back action, unconditionally
***********************************************************************/
static void do_back_action(void)
{
    if(view) view = line = col = 0;
}


/***********************************************************************
* execute a screenshot with menu, unconditionally
***********************************************************************/
static void do_screenshot_action(void)
{
    screenshot(1);
    play_rco_sound("system_plugin", "snd_system_ok");
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

    // wait for XMB feedback
    sys_timer_sleep(13);

    //vshtask_notify("sprx running...");

    play_rco_sound("system_plugin", "snd_trophy");

    #ifdef HAVE_STARFIELD
    init_once(/* stars */);
    #endif

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

            if((curpad != oldpad)
            && (curpad & PAD_L3))        // Hotkey menu
            {
                switch(menu_running)
                {
                    // VSH Menu not running, start VSH Menu
                    case 0:
                      // main view and start on first entry 
                      view = line = col = 0;

                      //
                      pause_RSX_rendering();

                      // create VSH Menu heap memory from memory container 1("app")
                      create_heap(64);       // 64 MB

                      // initialize VSH Menu graphic (init drawing context, alloc buffers, blah, blah, blah...)
                      init_graphic();

                      #ifdef HAVE_SYS_FONT
                      // set font(char w/h = 20 pxl, line-weight = 1 pxl, distance between chars = 1 pxl)
                      set_font(FONT_W, FONT_H, 1, FONT_D);
                      #endif


                      // load background image
                      load_png_bitmap(0, "/dev_hdd0/tentacle.png");

                      // stop vsh pad
                      start_stop_vsh_pad(0);

                      // set menu_running
                      menu_running = 1;

                      // timer start
                      startm = clock();

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
                    if(curpad & (PAD_UP | PAD_DOWN)) do_updown_action(curpad);

                    if(curpad & (PAD_LEFT | PAD_RIGHT)) do_leftright_action(curpad);

                    if(curpad & PAD_CROSS) do_menu_action();

                    if(curpad & PAD_CIRCLE) do_back_action();

                    if(curpad & PAD_START) do_screenshot_action();
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
