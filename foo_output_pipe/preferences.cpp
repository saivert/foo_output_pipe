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

static advconfig_branch_factory g_advconfigBranch("Output pipe", guid_advconfig_branch, advconfig_branch::guid_branch_tools, 0);

advconfig_checkbox_factory cfg_enable("Enable", guid_cfg_enable, guid_advconfig_branch, 0, true);

advconfig_string_factory cfg_cmdline("Command line", guid_cfg_cmdline, guid_advconfig_branch, 0,
	 "d:\\ffmpeg\\bin\\ffmpeg.exe -f wav -i - -y d:\\out.mp3");

advconfig_checkbox_factory cfg_showconsolewindow("Show console window", guid_cfg_showconsolewindow, guid_advconfig_branch, 0, false);
