#include <sys/prx.h>
#include <sys/ppu_thread.h>
#include <sys/process.h>
#include <sys/event.h>
#include <sys/syscall.h>
#include <sys/memory.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sys_time.h>
#include <sys/timer.h>
#include <cell/pad.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <netinet/in.h>

#include "inc/vsh_exports.h"
#include "network.h"
#include "misc.h"
#include "mem.h"
#include "blitting.h"




SYS_MODULE_INFO(VSH_MENU, 0, 1, 0);
SYS_MODULE_START(vsh_menu_start);
SYS_MODULE_STOP(vsh_menu_stop);

#define THREAD_NAME 				"vsh_menu_thread"
#define STOP_THREAD_NAME 		"vsh_menu_stop_thread"



static sys_ppu_thread_t vsh_menu_tid = -1;
static int32_t done = 0;
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
// BLITTING
static uint8_t menu_running = 0;           // vsh menu off(0) or on(1)
static sys_ppu_thread_t drawing_tid = -1;


static uint16_t line = 0;  // current line into menu, init 0 (Menu Entry 1)
#define MAX_MENU    7

const char *entry_str[] = {"Menu Entry 1: Make a single beep",
	                         "Menu Entry 2: Make a double beep",
	                         "Menu Entry 3: Play trophy sound",
	                         "Menu Entry 4: Make screenshot",
	                         "Menu Entry 5: Make screenshot with Menu",
	                         "Menu Entry 6: Reset PS3",
	                         "Menu Entry 7: Shutdown PS3"};




/***********************************************************************
* draw a frame
***********************************************************************/
static void draw_frame(void)
{
	int32_t i;
	
	
	// all 32bit colors are ARGB, the framebuffer format
	set_background_color(0x7F0000FF);     // blue, semitransparent
	set_foreground_color(0xFFFFFFFF);     // white, opac
	
	// fill background with background color
	draw_background();
	
	// print headline string, coordinates in canvas = x(296 pixel), y(8 pixel)
	print_text(293, 8, "PS3 VSH Menu");
	
	// print all menu entries, and the current selected entry in green
	for(i = 0; i < MAX_MENU; i++)
	{
		if(line == i)
			set_foreground_color(0xFF00FF00);    // green text, selected
		else
			set_foreground_color(0xFFFFFFFF);    // white text, normal
		
	  print_text(8, 8 +(FONT_H * (i + 1)), entry_str[i]);
	}
	
	// ...
}


/***********************************************************************
* drawing thread
***********************************************************************/
static void drawing_thread(uint64_t arg)
{
	while(menu_running == 1)
	{
		draw_frame();
		
		flip_frame();
		
		sys_timer_usleep(30);
		
		if(menu_running == 0)
		{
			sys_ppu_thread_exit(0);
		}
		else
		{
			continue;
		}
	}
}









/***********************************************************************
* stop VSH Menu
***********************************************************************/
static void stop_VSH_Menu(void)
{
	// unset menu_running and stop drawing_thread
	menu_running = 0;
	sys_ppu_thread_join(drawing_tid, NULL);
	
	// free heap memory
  destroy_heap();
  
  // restart vsh pad
	start_stop_vsh_pad(1);
	
	// continue rsx rendering...
  lv1_rsx_fifo_pause(0x55555555, 0); 
}




/***********************************************************************
* execute a menu aktion, based on line(current selected menu entry)
***********************************************************************/
static void do_menu_action(void)
{
	switch(line)
	{
	  case 0:                  // "Menu Entry 1: Make a single beep"
	    buzzer(1);
	    break;
	  case 1:                  // "Menu Entry 2: Make a double beep"
	    buzzer(2);
	    break;
	  case 2:                  // "Menu Entry 3: Play trophy sound"
	    play_rco_sound("system_plugin", "snd_trophy");
	    break;
	  case 3:                  // "Menu Entry 4: Make screenshot"
	    screenshot(0);         // xmb only
	    play_rco_sound("system_plugin", "snd_system_ok");
	    break;
	   case 4:                  // "Menu Entry 5: Make screenshot"
	    screenshot(1);          // xmb + menu
	    play_rco_sound("system_plugin", "snd_system_ok");
	    break;
	  case 5:                  // "Menu Entry 6: Reset PS3"
	    stop_VSH_Menu();       // stop VSH Menu and... 
	    sys_timer_sleep(1);    // a short sleep, or unproper shutdown
	    vshmain_87BB0001(2);
	    break;
	  case 6:                  // "Menu Entry 7: Shutdown PS3"
	    stop_VSH_Menu();
	    sys_timer_sleep(1);
	    vshmain_87BB0001(1);
	    break;
  }
}






/***********************************************************************
* plugin main ppu thread
***********************************************************************/
static void vsh_menu_thread(uint64_t arg)
{
	dbg_init();
	dbg_printf("programstart:\n");
	
	uint32_t oldpad = 0, curpad = 0;
	CellPadData pdata;
	
	sys_timer_sleep(13);
	vshtask_notify("sprx running...");
	
	
	while(1)
	{
		//ps3_run_stat = vshmain_EB757101();  // get current ps3 running status
		
		// if VSH Menu is running, we get pad data over our MyPadGetData()
		// else, we use the vsh pad_data struct
		if(menu_running)            
		  MyPadGetData(0, &pdata);
		else
		  VSHPadGetData(&pdata);
		
		// if pad_data and we are in XMB(vshmain_EB757101() == 0)
		if((pdata.len > 0) && (vshmain_EB757101() == 0))
		{
			curpad = (pdata.button[2] | (pdata.button[3] << 8));
			
			if((curpad & PAD_SELECT) && (curpad != oldpad))
		  {
				switch(menu_running)
				{
					case 0:                                      // VSH Menu not running, start VSH Menu
					  
					  // 
					  pause_RSX_rendering();
					  
					  // create VSH Menu heap memory from memory container 1("app")
					  create_heap(32);       // 32 MB
					  
					  // initialize VSH Menu graphic (init drawing context, alloc buffers, blah, blah, blah...)
					  init_graphic();
					  
					  // stop vsh pad
					  start_stop_vsh_pad(0);
					  
					  // set menu_running and start drawing_thread
            menu_running = 1;
		        sys_ppu_thread_create(&drawing_tid, drawing_thread, 0, 3000, 0x1000, 1, "drawing_thread");
					  
					  break;
					case 1:                                      // VSH Menu is running, stop VSH Menu
					  stop_VSH_Menu();
					  break;
				}
				
				oldpad = 0;
				sys_timer_usleep(300000);
		  }
		  
		  // only if VSH Menu is running
		  if(menu_running)
		  {
		    if((curpad & PAD_UP) && (curpad != oldpad))
		    {
		      if(line <= 0)
		        line = 0;
			  	else
			  	{
			  	  line--;
			  	  play_rco_sound("system_plugin", "snd_cursor");
					}
		  	}
		    
		    if((curpad & PAD_DOWN) && (curpad != oldpad))
		    {
		      if(line >= MAX_MENU-1)
		        line = MAX_MENU-1;
			  	else
			  	{
			  	  line++;
			  	  play_rco_sound("system_plugin", "snd_cursor");
					}
		  	}
		    
		    if((curpad & PAD_CROSS) && (curpad != oldpad))
		    {
		      do_menu_action();
		  	}
		  	
		  	// ...
			}
			
		  oldpad = curpad;
		}
		else
		{
	    oldpad = 0;
		}
  }
	
	dbg_fini();
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
* stopt thread
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
