#include "ui.h"
#include "i2c.h"
#include "hid.h"
#include "timer.h"

const char *options[] = {"Top Screen", "Bottom Screen", "Both Screens", "Screen-On Length", "Screen-Off Length", "Repetition Count"};


void powerOff()
{
    i2cWriteRegister(I2C_DEV_MCU, 0x20, 1 << 0);
    while(true);
}

u8 *top_screen, *bottom_screen;
int x = 0, y = 0;
bool top_on = true, bottom_on = true;
uint64_t screen_on_length = 240; // minutes
uint64_t screen_off_length = 10; // minutes
int repetition_count = 3;
bool progress_shown = false;

void drawMainMenu()
{
	x = 0, y = 0;
	ClearScreenF(true, true, COLOR_STD_BG);
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "3Deyellower by ManiacOfHomebrew"); y+=10;
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "Top Screen / Bottom Screen: %s / %s", top_on ? "On" : "Off", bottom_on ? "On" : "Off"); y+=10;
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "Screen-On Length: %llu minutes", screen_on_length); y+=10;
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "Screen-Off Length: %llu minutes", screen_off_length); y+=10;
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "Repetition Count: %d", repetition_count); y+=20;
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "Press (A) to begin."); y+=10;
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "Press (X) for settings."); y+=10;
	DrawStringF(top_screen, x, y,  0xffffff, 0x000000, "Press (B) to power off."); y+=10;

}

void powerBacklights(bool top, bool bottom)
{
	u8 top_mask = 1u << 4;
	u8 bottom_mask = 1u << 2;
	u8 on_mask = (top ? top_mask : 0) | (bottom ? bottom_mask : 0);
	on_mask <<= 1;
	u8 off_mask = (!top ? top_mask : 0) | (!bottom ? bottom_mask : 0);
	i2cWriteRegister(I2C_DEV_MCU, 0x22u, on_mask | off_mask);
}

void main(int argc, char** argv)
{
	
	// Fetch the framebuffer addresses
	if(argc >= 2) {
		// newer entrypoints
		u8 **fb = (u8 **)(void *)argv[1];
		top_screen = fb[0];
		bottom_screen = fb[2];
	} else {
		// outdated entrypoints
		top_screen = (u8*)(*(u32*)0x23FFFE00);
		bottom_screen = (u8*)(*(u32*)0x23FFFE08);
	}

	drawMainMenu();
	
	//top
	while(true) {
        u32 pad_state = InputWait();
        if (pad_state & BUTTON_A) break;
		if (pad_state & BUTTON_X) {
			ClearScreenF(true, true, COLOR_STD_BG);
			int option = ShowSelectPrompt(6, options, "Settings:");
			if(option == 1) {
				top_on = true;
				bottom_on = false;
			} else if(option == 2) {
				top_on = false;
				bottom_on = true;
			} else if(option == 3) {
				top_on = true;
				bottom_on = true;
			} else if(option == 4) {
				screen_on_length = ShowNumberPrompt(screen_on_length, "Screen-On Length (minutes):");
			} else if(option == 5) {
				screen_off_length = ShowNumberPrompt(screen_off_length, "Screen-Off Length (minutes):");
			} else if(option == 6) {
				repetition_count = ShowNumberPrompt(repetition_count, "Repetition Count:");
			}
			drawMainMenu();
		}
		if (pad_state & BUTTON_B) {
			powerOff();
		}
	}

	powerBacklights(false, false);
	for(int count = 0; count < repetition_count; count++) {
		timer_start();
		while(timer_sec() < 1) { // wait for 1 second
			if(CheckButton(BUTTON_B)) {
				powerOff();
			}
		}
		ClearScreenF(top_on, bottom_on, COLOR_WHITE);
		powerBacklights(top_on, bottom_on);
		timer_start();
		while(timer_sec() < screen_on_length * 60) {
			if(CheckButton(BUTTON_A)) {
				if(!progress_shown) {
					powerBacklights(true, false);
					ClearScreenF(true, true, COLOR_STD_BG);
				}
				ShowProgress(timer_sec(), screen_on_length * 60, "Screen-On Progress", !progress_shown);
				progress_shown = true;
				
			} else {
				if(progress_shown) {
					powerBacklights(top_on, bottom_on);
					ClearScreenF(top_on, bottom_on, COLOR_WHITE);
					progress_shown = false;
				}
			}
			if(CheckButton(BUTTON_B)) {
				powerOff();
			}
		}
		powerBacklights(false, false);
		progress_shown = false;
		timer_start();
		while(timer_sec() < screen_off_length * 60) {
			if(CheckButton(BUTTON_A)) {
				if(!progress_shown) {
					powerBacklights(true, false);
					ClearScreenF(true, true, COLOR_STD_BG);
				}
				ShowProgress(timer_sec(), screen_off_length * 60, "Screen-Off Progress", !progress_shown);
				progress_shown = true;
			} else {
				if(progress_shown) {
					powerBacklights(false, false);
					progress_shown = false;
				}
			}
			if(CheckButton(BUTTON_B)) {
				powerOff();
			}
		}
	}
	
	powerOff();
}
