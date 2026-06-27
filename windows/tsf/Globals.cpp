#include "Globals.h"

HINSTANCE g_hInst   = nullptr;
LONG      g_cRefDll = 0;

// {C4720D47-5015-4041-886F-54A92AD69BE2}
const CLSID c_clsidBanglaTextService =
    {0xC4720D47,0x5015,0x4041,{0x88,0x6F,0x54,0xA9,0x2A,0xD6,0x9B,0xE2}};
// {460DB25E-A6DC-4978-8F3E-3B37AA1152A0}
const GUID c_guidBanglaProfile =
    {0x460DB25E,0xA6DC,0x4978,{0x8F,0x3E,0x3B,0x37,0xAA,0x11,0x52,0xA0}};
// {BE758843-2F63-46B1-ABCC-6A7CDEB7F13E}
const GUID c_guidDisplayAttribute =
    {0xBE758843,0x2F63,0x46B1,{0xAB,0xCC,0x6A,0x7C,0xDE,0xB7,0xF1,0x3E}};

const WCHAR c_szServiceDescription[] = L"Bangla Keyboard";
const WCHAR c_szProfileDescription[] = L"Bangla Keyboard (Unicode)";
