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
static bool done = 0;
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
// TIMING, TEMPERATURES, GAMES
static clock_t startm, stopm;
static uint16_t tick = 0;
static double fps = 0;

static uint32_t t1 = 0, t2 = 0; // keep track of temperatures

#include "games.h"
struct game_entry *games = NULL;
static int16_t gmc = 0, stride = 0;


////////////////////////////////////////////////////////////////////////
// BLITTING
static bool   menu_running = 0; // vsh menu off(0) or on(1)
static int8_t line = 0;         // current line into menu, init 0 (entry 1:)
static int8_t linb = 1;         // current line backup
static int8_t view = 0;         // menu view, init 0 (main view)
static int8_t col  = 0;         // current coloumn into menu, init 0 (entry 1:)

static menu_palette_ctx palette[VIEWS], *p = NULL; // different colors combination, per view

// menu entry strings
const char *entry_str[3][10] __attribute__((aligned(4))) = {
{
    "1: Make a single beep",
    "2: Make a double beep",
    "3: Dump pad data",
    "4: Setup colors menu",
    "5: Play trophy sound",
    "6: Make screenshot",
    "7: Make screenshot with Menu",
    "8: Reset PS3",
    "9: Shutdown PS3",
    "A: Browse GAMES"
},
{   // Dump pad data
    "1: connect socket",
    "2: screenshot",
    "3: Alpha",
    "4: Red",
    "5: Green",
    "6: Blue",
    "7: test"
},
{   // Setup colors menu
    "1:bg,fg,f2",
    "2:bg,fg,f2",
    "3:bg,fg,f2",
    "4:bg,fg,f2",
    "5:write config"
}
};


/***********************************************************************
* draw a frame
***********************************************************************/
static void draw_frame(CellPadData *data)
{
    char tmp_ln[8 * (4 + 1)] __attribute__((aligned(16)));
    uint16_t tx, ty; // text coordinates
    int8_t i;

    /* all 32bit colors are ARGB, the framebuffer format */
    uint32_t blink_color = (uint32_t)rand();

    // set the right colors for current view
    p = &palette[view];

    #ifdef HAVE_PNG_FONT
    set_foreground_color(p->c[1]);
    #else
    update_gradient(&p->c[1], &p->c[2]);
    #endif

    /* background */
    set_background_color(p->c[0]);

    if(view == 3) // draw background from selected folder game icon
    {
        bool flag = 1;
        if(linb != (line + stride)) // only on line change
        {
            linb = line + stride;

            // build folder path
            sprintf(tmp_ln, "%s%s", USERLIST_PATH, (games + line + stride)->path);
            strcat(tmp_ln, "/PS3_GAME/ICON0.PNG"); // icon path

            if(load_png_bitmap(0, tmp_ln)) flag = 0; // load ICON0.PNG

            #ifdef DEBUG
            sprintf(tmp_ln, "%s v%s",
                   (games + line + stride)->path,
                   (games + line + stride)->version);
            //print_text(BORD_D, (FONT_H + FONT_D) *16, tmp_ln);
            dbg_printf("%s\n", tmp_ln);
            #endif

            sys_timer_usleep(200 *1000); /* 200msec */
        }

        if(flag)
            draw_png_2x(0, 20, 16, 0, 0, 320, 176);

        blend_canvas();
    }
    else // default
    {
        draw_background(); // alpha-blended background

        #ifdef HAVE_STARFIELD
        draw_stars(); // now, just to keep them under text lines
        #endif
    }

    /* print all menu entries for view, and blink the current selected entry */
    for(i = 0; i < p->max_lines; i++)
    {
        uint32_t *tc = NULL;
        ty = 8 + ((FONT_H + FONT_D) * (i + 1));

        if(i == line)                   // selected entry, blink
        {
            tc = &blink_color;

            #ifdef HAVE_PNG_FONT
            set_foreground_color(*tc);
            #else
            update_gradient(tc, tc);    // full color
            #endif
        }

        // print menu lines
        if(view == 3)  // Browse GAMES view
        {
            if(i < gmc ) // && gmc > 0
            {
                strcpy(tmp_ln, (games + i + stride)->title);
                tx = get_aligned_x(tmp_ln, CENTER);
                print_text(tx, ty, tmp_ln);
            }
            else
            {
                if(!i) // or gmc < 1, (shortcut) only once on error
                {
                    sprintf(tmp_ln, "(EE) 0x%p, %.2d", games, gmc);
                    tmp_ln[strlen(tmp_ln)] = '\0';
                    tx = get_aligned_x(tmp_ln, CENTER);
                    print_text(tx, ty, tmp_ln);
                }
            }
        }
        else // default
            print_text(BORD_D, ty, entry_str[view][i]);

        // (re)set back color, if just draw selected line!
        if(tc)
        {
            #ifdef HAVE_PNG_FONT
            set_foreground_color(p->c[1]);
            #else
            update_gradient(&p->c[1], &p->c[2]);
            #endif
        }
    }
    // we ends with default colors ready, per view

    if(0) // a couple of debug strings
    {
        sprintf(tmp_ln, "0x%p, lb%d le%d stride:%d (%d) %db", games, linb, line, stride, (line + stride), sizeof(menu_palette_ctx));
        #ifdef DEBUG
        print_text(BORD_D, ty + (FONT_H + FONT_D) *2, tmp_ln);
        dbg_printf("%s\n", tmp_ln);
        #endif
    }

    /* print headline string, coordinates in canvas */
    switch(view)
    {
      case 1:
      case 2:
        strcpy(tmp_ln, (char*)(entry_str[0][view +1] +3 /*strip leading number*/ ));
        break;

      case 3:
        sprintf(tmp_ln, "listing %d folder GAMES on HDD0", gmc);  // a fixed title
        break;

      default:
        strcpy(tmp_ln, "PS3 VSH Menu");
        break;
    }
    tx = get_aligned_x(tmp_ln, CENTER); // center over width
    print_text(tx, BORD_D, tmp_ln);     // minimum border from top

    // ...

    if(view == 1)  // Dump pad data view
    {
        ty = 180;
        uint8_t  x  = 0;

        sprintf(tmp_ln, "0x%p, %d bytes:", data, data->len * sizeof(uint16_t));
        tx = get_aligned_x(tmp_ln, CENTER);
        print_text(tx, ty, tmp_ln);

        /* hexdump pad data: store in text 8 buttons in a row in "%.4x" format
           plus 1 for ':', last one is replaced by terminator, no need to add 1:
           [8 * (4 + 1)]; */

        tx = 0, ty = 200;
        for(i = 0; i < 32; i++)
        {
            sprintf(&tmp_ln[x], "%.4X:", data->button[i]);
            x += 5;
            tmp_ln[x] = '\0';

            if(x %8 == 0)
            {   // overwrite last ':' with terminator
                tmp_ln[x -1] = '\0';
                if(tx == 0)
                    tx = get_aligned_x(tmp_ln, CENTER); // only once

                print_text(tx, ty, tmp_ln);
                x = 0, ty += (FONT_H + FONT_D); // additional px to next line
            }
        }

    }
    else if(view == 2)  // Setup colors menu view
    {
        uint32_t *tc = NULL;
        uint8_t x, g;  // grounds

        // print all color entries: Bg, Fg, Fg2, per line
        for(g = 0; g < 3; g++)  // grounds as coloumns
        {
            tx = 180 + ((8 + 1 /*chars of distance*/) * FONT_W) * g;

            for(i = 0; i < VIEWS; i++) // first 4 lines: view[0-3]
            {
                ty = 8 + ((FONT_H + FONT_D) * (i + 1));
                tc = &palette[i].c[g];    // address the temp color

                #ifdef HAVE_PNG_FONT
                set_foreground_color(*tc);
                #else
                update_gradient(tc, &palette[i].c[2]); // fade to: selected Fg2 or same color?
                #endif

                sprintf(tmp_ln, "%.8X", *tc);        // print color
                print_text(tx, ty, tmp_ln);

                if(i == line                         // selected entry
                && col > 0 
                && (col -1) /4 /*ARGB*/ == g)        // selected color component
                {
                    tc = &blink_color;               // blink

                    #ifdef HAVE_PNG_FONT
                    set_foreground_color(*tc);
                    #else
                    update_gradient(tc, tc);         // full color
                    #endif

                    // put a terminator and print one of AA:RR:GG:BB\0
                    x = ((col -1) %4) *2 /*chars*/;  // [0, 2, 4, 6, 8]
                    uint16_t add_x = 0;

                    if(x) // 2, 4, 6
                    {
                        #ifdef HAVE_SYS_FONT
                        tmp_ln[x] = '\0';
                        add_x = get_render_length(tmp_ln);
                        sprintf(tmp_ln, "%.8X", palette[i].c[g]); // need to restore value (cutted by terminating)

                        #elif HAVE_XBM_FONT
                        add_x = (x * FONT_W);         // xbm_font is monospace
                        #endif
                    }

                    // terminate and print current selected color component
                    tmp_ln[x +2] = '\0';
                    print_text(tx + add_x, ty, &tmp_ln[x]);
                }
            }
        }
    } // end (view == 2)

    // ...

    // (re)set back after draw last line
    #ifdef HAVE_PNG_FONT
    set_foreground_color(p->c[1]);
    #else
    update_gradient(&p->c[1], &p->c[2]);
    #endif

    #ifdef HAVE_SSCROLLER
    // testing sine in a scroller
    draw_text(330);
    move_text();
    #endif


    /* position footer info */
    ty = CANVAS_H - (FONT_H + FONT_D) - BORD_D - SHADOW_PX;  // additional px from bottom

    // sys memory stats, on the right
    read_meminfo(tmp_ln);
    tx = get_aligned_x(tmp_ln, RIGHT) - BORD_D - SHADOW_PX;  // additional px from R margin
    print_text(tx, ty, tmp_ln);


    // get timing, eventually compute fps and read temperature data
    stopm = clock();
    if(((double)stopm-startm) > 3500000)
    {
        fps = (double)(tick / (((double)stopm-startm) / CLOCKS_PER_SEC));

        startm = clock(), tick = 0;              // reset fps counters

        read_temperature(&t1, &t2);              // update temperature data
    }

    // report stats, on the left
    sprintf(tmp_ln, "%2.1ffps, C:%iC, R:%iC", fps, t1, t2);
    print_text(BORD_D, ty, tmp_ln);

    tick++;               // keep track of drawn frame

} // end draw_frame()


/***********************************************************************
* stop VSH Menu
***********************************************************************/
static void stop_VSH_Menu(void)
{
    menu_running = 0;               // menu off

    #ifdef HAVE_SYS_FONT
    font_finalize();                // unbind renderer and kill font-instance
    #endif

    destroy_heap();                 // free heap memory

    games = NULL, stride = 0;       // set refresh flag for next init

    rsx_fifo_pause(0);              // continue rsx rendering

    start_stop_vsh_pad(1);          // restart vsh pad

    sys_timer_usleep(100000);
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
* execute a menu action, based on coloumn(current selected menu entry)
***********************************************************************/
static void do_updown_action(uint16_t curpad)
{
    bool flag = 0;
    uint8_t  *value = NULL, step, max, c_comp[4]; // single color components
    uint32_t *color = NULL;

    /* 1. setup variables, common bounds */
    if(col)
    {   // change selected color(col, line)
        // A, R, G, B: 0x00-0xFF, no bound: can loop
        value = &c_comp[(col -1) %4];
        step  = -2, max = 0xFF;
        color = &palette[line].c[(col -1) /4];

        c_comp[0] = GET_A(*color), c_comp[1] = GET_R(*color),
        c_comp[2] = GET_G(*color), c_comp[3] = GET_B(*color);
    }
    else
    {   // line: 0-max_menu[view], bounded: can't loop
        value = (uint8_t*)&line;
        step  = 1, max = p->max_lines -1;
    }

    /* 2. check for bounds, eventually update value */
    if(curpad & PAD_UP)
    {
        if( (*value == 0 && col)  // loop
          || *value >  0)/* bound */{ *value -= step, flag = 1; }

        else // deal with scrolling lines
        if(*value == 0 && view == 3
        && stride > 0) { stride--, flag = 1; }
    }
    else // & PAD_DOWN
    {
        if( (*value == max && col)  // loop
          || *value <  max)/* bound */{ *value += step, flag = 1; }

        else // deal with scrolling lines
        if(*value == max && view == 3
        && stride < gmc - p->max_lines) { stride++, flag = 1; }
    }

    /* 3. update and feedback */
    if(flag)
    {   // selected color with updated single components
        if(col)
          *color = ARGB(c_comp[0], c_comp[1], c_comp[2], c_comp[3]);

        play_rco_sound("system_plugin", "snd_cursor");
    }
}


/***********************************************************************
* execute a menu action, based on line(current selected menu entry)
***********************************************************************/
static void do_leftright_action(uint16_t curpad)
{
  if((view == 2)     // only on third view
  && (line < VIEWS)) // only for all color palettes
  {
      bool flag = 0;
      if(curpad & PAD_LEFT)
      {
          if(col > 0)
          { col--, flag = 1; }
      }
      else   // & PAD_RIGHT
      {
          if(col < 4 *3)          //0-argb-argb-argb, 1+4+4+4
          { col++, flag = 1; }
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
            do_screenshot_action();
            break;
          case 7:                  // "8: Reset PS3"
            delete_turnoff_flag();
            stop_VSH_Menu();
            sys_timer_sleep(1);
            shutdown_reset(2);
            break;
          case 8:                  // "9: Shutdown PS3"
            delete_turnoff_flag();
            stop_VSH_Menu();
            sys_timer_sleep(1);
            shutdown_reset(1);
            break;
          case 9: // Browse GAMES
            if(!games)
                games = ReadUserList(&gmc); // refresh list
            if(games)
            {
                view = 3;          // change menu view
                line = stride = 0; // on start entry
                linb = 1;          // flag for icon loading
            }
            break;
        }
        break;

      case 1:                   // second menu view
        switch(line)
        {
          case 0:               // 1: Start UDP debug"
            #ifdef DEBUG
            //dbg_fini();
            //dbg_init();
            dbg_printf("program restart:\n");
            //dbg_printf("%s:%s\n", DB_IP, DB_PORT);
            #endif
            view = line = 0;
            break;
          case 1:               // 2: screenshot
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
          case 4:               // "5: write config"
            store_palette((menu_palette_ctx*)&palette, sizeof(menu_palette_ctx) * VIEWS);
            break;
        }
        break;

      case 3:                   // Browse GAMES view
        do_mount((games + line + stride)->path);
        sys_timer_usleep(500 *1000); /* 500msec */
        stop_VSH_Menu();
        break;

    }
}


/***********************************************************************
* execute a back action, unconditionally
***********************************************************************/
static void do_back_action(void)
{
    if(view) view = line = col = 0;

    if(view == 0
    && line == 9)
    {
        do_umount();
        sys_timer_usleep(500 *1000); /* 500msec */
        stop_VSH_Menu();
    }
}


/***********************************************************************
* plugin main ppu thread
***********************************************************************/
static void vsh_menu_thread(uint64_t arg)
{
    uint16_t oldpad = 0, curpad = 0;
    CellPadData pdata;

    // wait for XMB feedback
    sys_timer_sleep(13);

    #ifdef DEBUG
    dbg_init();
    dbg_printf("programstart:\n");
    #endif

    play_rco_sound("system_plugin", "snd_trophy");

    #ifdef HAVE_STARFIELD
    init_once(/* stars */);
    #endif

    init_menu_palette((menu_palette_ctx*)&palette);

    while(1)
    {
        if(menu_running)
            MyPadGetData(0, &pdata);       // get pad data over our MyPadGetData()
        else
            VSHPadGetData(&pdata);         // else use the vsh pad_data struct

        if((pdata.len > 0)
        && (GetCurrentRunningMode() == 0)) // we are in XMB
        {
            curpad = (pdata.button[2] | (pdata.button[3] << 8));

            if((curpad != oldpad)
            && (curpad & PAD_L3))          // Hotkey menu
            {
                switch(menu_running)
                {
                    // VSH Menu not running, start VSH Menu
                    case 0:
                      view = line = col = 0;        // main view and start on first entry

                      pause_RSX_rendering();

                      create_heap(16);              // create VSH Menu heap memory from memory container 1("app")

                      init_graphic();               // initialize VSH Menu graphic

                      #ifdef HAVE_SYS_FONT
                      set_font(FONT_W, FONT_H, 1, FONT_D); // line-weight = 1 pxl, distance between chars)
                      #endif

                      load_png_bitmap(0, "/dev_hdd0/tentacle.png"); // load an image

                      start_stop_vsh_pad(0);                        // stop vsh pad

                      startm = clock(), tick = 0;                   // reset fps counters

                      menu_running = 1;         // set menu_running

                      break;

                    // VSH Menu is running, stop VSH Menu
                    case 1:
                      stop_VSH_Menu();

                      break;
                }

                oldpad = 0;
                sys_timer_usleep(300 *1000); /* 300msec */
            }

          // VSH Menu is running, draw menu / check pad
          if(menu_running)
          {
                draw_frame(&pdata);

                flip_frame();

                if(curpad != oldpad)
                {
                    #ifdef DEBUG
                    dbg_printf("0x%p\n", (uint32_t*)&pdata);
                    dbg_printf("0x%p, lb%d le%d stride:%d (%d) %db\n", games, linb, line, stride, (line + stride), sizeof(menu_palette_ctx));
                    #endif

                    if(curpad & (PAD_UP | PAD_DOWN)) do_updown_action(curpad);

                    if(curpad & (PAD_LEFT | PAD_RIGHT)) do_leftright_action(curpad);

                    if(curpad & PAD_CROSS) do_menu_action();

                    if(curpad & PAD_CIRCLE) do_back_action();

                    if(curpad & PAD_START) do_screenshot_action();
                }

                // ...

                // sys_timer_usleep(2 *1000); /* 2msec */

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

    if(menu_running) stop_VSH_Menu();

    vshtask_notify("unloading");

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
