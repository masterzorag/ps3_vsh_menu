#ifndef __VSHCOMMON_H__
#define __VSHCOMMON_H__


// vshcommon_6FD850FF(void);  // custom_render_plugin IF 1: - (decrease)
// vshcommon_EC73D438(void);  // custom_render_plugin IF 1: - (increase)
// vshcommon_BF88BEE4(void);  // custom_render_plugin IF 1: - blur (decrease?)
// vshcommon_C8FFD88F(void);  // custom_render_plugin IF 1: - blur (increase?)
// vshcommon_16E29622(const char *str);  // custom_render_plugin IF 1: 
// vshcommon_5723C3C1(float x, const char * str, int y);  // ?

// vshcommon_25111EFB  // ? r3 = sprx_base, r5 = widget_name


// vshcommon_D81578DB  // ? r3 = sprx_base, r4 = ?, r5 = widget_name, r6 = ?, r7 = ?


// vshcommon_73F85259(uint8_t x);  // system_plugin interface_1: 24: anim_rectangle_show_hide
// vshcommon_E011E7D0(uint8_t x);  // system_plugin interface_1: 23: anim_triangle_show_hide
// vshcommon_9BD8429E(uint8_t x);  // system_plugin interface_1: 22: anim_cross_circle_show_hide
// vshcommon_2438F1A4(uint8_t x);  // system_plugin interface_1: 21: anim_cross_circle_show_hide
// vshcommon_F7A67D49(uint8_t x[0x1C], wchar *s);  // system_plugin interface_1: 28: rectangle_text
// vshcommon_60DEE5B3  // ?
// vshcommon_75DC9C2D  // ?
// vshcommon_79A562D5  // ?
// vshcommon_B9473E7A  // ?
// vshcommon_D1DE9F38  // ?

// vshcommon_85AD7A58  // ?

// vshcommon_AE251F8F  // ?

// vshcommon_E43C21F6  // ?

// vshcommon_8C7F3E6F  // ?

// vshcommon_55A60B1A  // ?

// vshcommon_F9E43DA2(void);  // returns localized string of "msg_ok" 
// vshcommon_CCD2C319(void);  // returns localized string of "msg_cancel" 
// vshcommon_746C5F88(void);  // returns localized string of "msg_option"
// vshcommon_F995E53F(void);  // returns localized string of "msg_back" 
// vshcommon_26F18EDF(void);  // returns localized string of "msg_enter" 
// vshcommon_0E9E8DA5(void);  // returns localized string of "msg_no" 
// vshcommon_B8E256D7(void);  // returns localized string of "msg_yes" 


// vshcommon_8D173737  // ? r3 = sprx_base, r4 = ?, r5 = widget_name, r6 = ?


// vshcommon_28549FD0  // ? r3 = sprx_base, r4 = ?, r5 = widget_name, r6 = ?, r7 = ?


// vshcommon_015D4314  // ?
// vshcommon_94F43BE7  // ?
// vshcommon_7F5C551B  // ?
// vshcommon_14FF1DDC  // ?
// vshcommon_884E740C  // ?

// vshcommon_12D25E5F  // ?
// vshcommon_61D97E3B  // ?
// vshcommon_DEC761F1  // ?

// vshcommon_D87C3834  // ?

// vshcommon_98D73408  // ?
// vshcommon_FDA63F52  // ?
// vshcommon_DFD99C55  // ?
// vshcommon_63210A1C  // ?

// vshcommon_3A8D0E1C  // ?
// vshcommon_24FF1870  // ?
// vshcommon_EA95C3F5  // ?

// vshcommon_3B634FCC  // ?

// vshcommon_38815260  // ?
// vshcommon_425277FA  // ?

// vshcommon_0EBAC3B5  // ?
// vshcommon_1851C6A6  // ?
// vshcommon_3F731CC3  // ?
// vshcommon_B045A8DC  // ?
// vshcommon_95302AE6  // ?
// vshcommon_C398D8BA  // ?
// vshcommon_50DE40CE  // ?
// vshcommon_85A3D9F3  // ?
// vshcommon_6A784AE5  // ?
// vshcommon_933A54F5  // ?
// vshcommon_5A6C0DB6  // ?

// vshcommon_7C139D36  // ?
// vshcommon_459C02B6  // ?

// vshcommon_33C12393  // ?
// vshcommon_9CB87B0D  // ?
// vshcommon_CE57965F  // ?
// vshcommon_74EAD50B  // ?
// vshcommon_F06004CD  // ?

extern void vshcommon_A20E43DB(void);  // Displays a notification in XMB with texture 


//void(*vshcommon_A20E43DB)(int32_t, const char* eventName, int32_t, int32_t* texture, int32_t*, const char*, const char*, float, const wchar_t* text, int32_t, int32_t, int32_t);
//int dummy = 0; vshcommon_A20E43DB(0, const char* eventName, int32_t, int32_t* texture /*paf_3A8454FC*/, &dummy, "", "", 0f, L"notification", 0, 0, 0) 



// vshcommon_7504447B  // ?

// vshcommon_F1918912  // ?

// vshcommon_21806775  // ?

// vshcommon_F55812AE  // ?

// vshcommon_61D17188  // ?
// vshcommon_EA790023  // ?
// vshcommon_D59AEAD0  // ?
// vshcommon_5DC484F8  // ?

// vshcommon_E21C3432  // ?

// vshcommon_00EDDB5B  // ?

// vshcommon_5CE195FA  // ?
// vshcommon_E607C2BA  // ?

// vshcommon_C499E073  // ? r3 = sprx_base, r4 = const char *str ?, r5 = ?

// vshcommon_CDF4C62A  // ?
// vshcommon_53ECE7ED  // ?
// vshcommon_1379FF05  // ?
// vshcommon_AB6951A3  // ?
// vshcommon_ABCA6F32  // ?
// vshcommon_5DD71B31  // ?
// vshcommon_4A6E257D  // ?

// vshcommon_A6525AE6  // ?
// vshcommon_DAC7FC51  // ?

// vshcommon_B2D04619  // ?
// vshcommon_B7F2EFD5  // ?

// vshcommon_9DAB12AA  // ?

// vshcommon_025EC4EE  // ?

// vshcommon_E7DFF7FE  // ?

// vshcommon_D9B63654  // ?

// vshcommon_BB2EC9CD  // ?

// vshcommon_ABEB01DA  // ?

// vshcommon_980513A4  // ?

// vshcommon_5C9F85CB  // ?
// vshcommon_E9790F7A  // ?

// vshcommon_EDB3F1F2  // ?
// vshcommon_390E5621  // ?
// vshcommon_B7517F9B  // ?
// vshcommon_73B454A2  // ?

// vshcommon_C27EF445  // ?

// vshcommon_D55C9CEE  // ?
// vshcommon_7C7F21E0  // ?
// vshcommon_1452A4D3  // ?

// vshcommon_6F5A9C38  // ?
// vshcommon_16106ACD  // ?

// vshcommon_C08C2D22  // ?
// vshcommon_98E05EDA  // ?
// vshcommon_D17A0968  // ?

// vshcommon_AA6178EE  // ?
// vshcommon_F4E3246A  // ?

// vshcommon_83E28B3C  // ?

// vshcommon_8B7F8F80  // ?
// vshcommon_7C3ACA85  // ?
// vshcommon_19535F4C  // ?
// vshcommon_B0B677BC  // ?
// vshcommon_3DBB0BB2  // ?
// vshcommon_DC712BA3  // ?
// vshcommon_940FE097  // ?
// vshcommon_EC44C999  // ?
// vshcommon_B9FDA9D4  // ?
// vshcommon_B6C9D197  // ?
// vshcommon_13F9024E  // ?

// vshcommon_FB104BD2  // ?

// vshcommon_7BD54E23  // ?
// vshcommon_3F8C6AED  // ?

// vshcommon_31573F9A  // ?

// vshcommon_4B8EE123  // ?
// vshcommon_AB7E2E69  // ?
// vshcommon_AF168DD4  // ?

// vshcommon_D996B261  // ?
// vshcommon_52692705  // ?

// vshcommon_CA3CBF5D  // ?

// vshcommon_2D2F3A6E  // ?
// vshcommon_44726825  // ?
// vshcommon_B53FD739  // ?
// vshcommon_252838CF  // ?
// vshcommon_3851B803  // ?
// vshcommon_B4BAF07E  // ?

// vshcommon_97E53BCA  // ?
// vshcommon_54D13728  // ?

// vshcommon_DB0320D8  // ?
// vshcommon_0A007C99  // ?
// vshcommon_8F0C0B33  // ?
// vshcommon_D59C7D79  // ?
// vshcommon_0BECDA92  // ?
// vshcommon_79A546A1  // ?
// vshcommon_39D01FCA  // ?

// vshcommon_34A05733  // ?

// vshcommon_A2312283  // ?

// vshcommon_CC2C67F2  // ?
// vshcommon_09A43140  // ?
// vshcommon_7EE0068F  // ?

// vshcommon_B49AF109  // ?
// vshcommon_D010D338  // ?
// vshcommon_BC200324  // ?
// vshcommon_774C200A  // ?

// vshcommon_13BAFB22  // ?
// vshcommon_6DF64AAF  // ?

// vshcommon_FAEA4EF9  // ?
// vshcommon_50CBEE73  // ?
// vshcommon_A05B2B54  // ?
// vshcommon_E2D6F3D7  // ?

// vshcommon_8B2110D5  // ?
// vshcommon_9EA67737  // ?


#endif // __VSHCOMMON_H__ 
