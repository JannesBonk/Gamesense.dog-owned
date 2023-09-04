#pragma once

#include "Globalincludes.h"

template <typename T>
class singleton2
{
public:
	static T* get()
	{
		static T* _inst = nullptr;

		if (!_inst)
			_inst = new T();

		return _inst;
	}
};

class CMenu : public singleton2<CMenu>
{
public:


	void Initialize();
	void Draw();
	D3DCOLOR MenuColor();
	int DPI();
	float GetDPINum();
	int GetDPITab();
	LPD3DXFONT GetFontDPI();
	LPD3DXFONT GetFontBold();
	LPD3DXFONT GetKeybindDPI();
	bool is_menu_opened();
	int GetTabNumber();
	void set_menu_opened(bool v);
	bool LockMenu = false;
	bool ResetMenu = false;
	int configID = -1;
	bool unload = false;

	int ThemeColor[4] = { 153,225,1,255 };
	bool* nigga;
	bool nigga2 = false;
	bool PopUpOpen = false;
	const char* Clicked = "";
	const char* popupname = "";
	int color;

private:
	bool m_bInitialized;
	bool m_bIsOpened;
	int players_section;

	int m_nCurrentTab;
	int legitsel;
	bool legit_pistol;
	bool legit_smg;
	bool legit_rifle;
	bool legit_sg;
	bool legit_heavy;
	bool legit_snipers;
};