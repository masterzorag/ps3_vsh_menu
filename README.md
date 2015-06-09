# ps3_vsh_menu

or better, a way to create a GUI + XMB-independent pad handling for a VSH plugin.

Done by pausing the RSX rendering immediately after the RSX flip a finished frame
and then use this paused framebuffer to blit in our own stuff.
