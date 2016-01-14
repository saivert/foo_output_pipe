#include "stdafx.h"
#include "resource.h"

// Sample preferences interface: two meaningless configuration settings accessible through a preferences page and one accessible through advanced preferences.


// This GUID identifies our Advanced Preferences branch (replace with your own when reusing code).
// {554E372A-41B6-4AA4-9734-561F4E149B0B}
static const GUID guid_advconfig_branch = { 0x554e372a, 0x41b6, 0x4aa4, { 0x97, 0x34, 0x56, 0x1f, 0x4e, 0x14, 0x9b, 0xb } };
// This GUID identifies our Advanced Preferences setting (replace with your own when reusing code) as well as this setting's storage within our component's configuration file.
// {16A5B94D-5B69-44B1-8809-363C8D8E7715}
static const GUID guid_cfg_cmdline = { 0x16a5b94d, 0x5b69, 0x44b1, { 0x88, 0x9, 0x36, 0x3c, 0x8d, 0x8e, 0x77, 0x15 } };

// {FC0D18C5-04AB-4353-841D-97864E64D5D4}
static const GUID guid_cfg_showconsolewindow =
{ 0xfc0d18c5, 0x4ab, 0x4353, { 0x84, 0x1d, 0x97, 0x86, 0x4e, 0x64, 0xd5, 0xd4 } };

// {4FD293C4-9720-400F-A8B8-C3754C64646B}
static const GUID guid_cfg_enable =
{ 0x4fd293c4, 0x9720, 0x400f, { 0xa8, 0xb8, 0xc3, 0x75, 0x4c, 0x64, 0x64, 0x6b } };

enum {
	default_cfg_showconsole = 0,
	default_cfg_enable = 1,
};
static auto default_cfg_cmdline = "d:\\ffmpeg\\bin\\ffmpeg.exe -f wav -i - -y d:\\out.mp3";

cfg_int cfg_enable(guid_cfg_enable, 1);
cfg_int cfg_showconsolewindow(guid_cfg_showconsolewindow, 0);
cfg_string cfg_cmdline(guid_cfg_cmdline, default_cfg_cmdline);

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	//Constructor - invoked by preferences_page_impl helpers - don't do Create() in here, preferences_page_impl does this for us
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}

	//Note that we don't bother doing anything regarding destruction of our class.
	//The host ensures that our dialog is destroyed first, then the last reference to our preferences_page_instance object is released, causing our object to be deleted.


	//dialog resource ID
	enum { IDD = IDD_PREFS };
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_ENABLE, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_SHOWCONSOLE, BN_CLICKED, OnEditChange)
		COMMAND_HANDLER_EX(IDC_CMDLINE, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};

BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	CheckDlgButton(IDC_ENABLE, cfg_enable);
	CheckDlgButton(IDC_SHOWCONSOLE, cfg_showconsolewindow);
	uSetDlgItemText(m_hWnd, IDC_CMDLINE, cfg_cmdline);
	return FALSE;
}

void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	// not much to do here
	OnChanged();
}

t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CMyPreferences::reset() {
	CheckDlgButton(IDC_ENABLE, default_cfg_enable);
	CheckDlgButton(IDC_SHOWCONSOLE, default_cfg_showconsole);
	uSetDlgItemText(m_hWnd, IDC_CMDLINE, default_cfg_cmdline);
	OnChanged();
}

void CMyPreferences::apply() {
	cfg_enable = IsDlgButtonChecked(IDC_ENABLE);
	cfg_showconsolewindow = IsDlgButtonChecked(IDC_SHOWCONSOLE);
	uGetDlgItemText(m_hWnd, IDC_CMDLINE, cfg_cmdline);
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CMyPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	return IsDlgButtonChecked(IDC_ENABLE) != cfg_enable
		|| IsDlgButtonChecked(IDC_SHOWCONSOLE) != cfg_showconsolewindow
		|| uGetDlgItemText(m_hWnd, IDC_CMDLINE) != cfg_cmdline;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() { return "Output pipe"; }
	GUID get_guid() {
		// This is our GUID. Replace with your own when reusing the code.
		// {4C12805F-86AC-4C35-A670-2083E56E37CD}
		static const GUID guid =
		{ 0x4c12805f, 0x86ac, 0x4c35, { 0xa6, 0x70, 0x20, 0x83, 0xe5, 0x6e, 0x37, 0xcd } };

		return guid;
	}
	GUID get_parent_guid() { return guid_tools; }
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;
