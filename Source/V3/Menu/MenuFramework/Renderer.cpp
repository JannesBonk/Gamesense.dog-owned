#include "Renderer.h"
#include "RawData.h"
#include "..\..\cheats\menu_alpha.h"
#include "..\..\cheats\lagcompensation\animation_system.h"
#include "..\..\cheats\visuals\player_esp.h"
#include "..\..\cheats\visuals\other_esp.h"
#include "..\..\cheats\misc\logs.h"
#include "..\..\cheats\visuals\world_esp.h"
#include "..\..\cheats\misc\misc.h"
#include "..\..\cheats\visuals\GrenadePrediction.h"
#include "..\..\cheats\visuals\bullet_tracers.h"
#include "..\..\cheats\visuals\dormant_esp.h"
#include "..\..\cheats\lagcompensation\local_animations.h"
#include "../../Menu/MenuFramework/Framework.h"
#include "../../Menu/MenuFramework/Renderer.h"
#include "../../Menu/Menu.h"

std::unique_ptr<Render::CDraw> Render::Draw = std::make_unique<Render::CDraw>();

using namespace Render;
using namespace IdaLovesMe;

namespace Render
{
    namespace Fonts
    {
        LPD3DXFONT LBIcons;
        LPD3DXFONT TabIcons;
        LPD3DXFONT TabIcons2;
        LPD3DXFONT TabIcons3;
        LPD3DXFONT TabIcons4;

        LPD3DXFONT Verdana;
        LPD3DXFONT Tahombd;
        LPD3DXFONT Keybind;
        LPD3DXFONT Logs;
        LPD3DXFONT Indicator;
        LPD3DXFONT IndicatorShadow;
        LPD3DXFONT Verdana25;
        LPD3DXFONT Verdana50;
        LPD3DXFONT Verdana75;
        LPD3DXFONT Verdana200;

        LPD3DXFONT Tahombd25;
        LPD3DXFONT Tahombd50;
        LPD3DXFONT Tahombd75;
        LPD3DXFONT Tahombd200;

        LPD3DXFONT Keybind15;
        LPD3DXFONT Keybind35;
        LPD3DXFONT Keybind75;
        LPD3DXFONT Keybind200;
    }
}

void CDraw::CreateObjects()
{
    D3DXCreateSprite(m_Device, &m_Sprite);

    D3DPRESENT_PARAMETERS pp = {};
    DWORD nFonts = 0;
    DWORD nFontss = 1;
    DWORD nFonts2 = 2;
    DWORD nFonts3 = 3;
    DWORD nFonts4 = 4;
    DWORD nFonts5 = 5;
    DWORD* niggaballs;

    D3DXCreateFontA(m_Device, 12, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Verdana", &Fonts::Verdana);
    D3DXCreateFontA(m_Device, 12, 0, FW_BOLD, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, FIXED_PITCH, "Verdana", &Fonts::Tahombd);

    this->m_TabFont = AddFontMemResourceEx((void*)(RawData::TabIcons), (DWORD)5192, nullptr, &nFonts);
    this->m_TabFont2 = AddFontMemResourceEx((void*)(RawData::TabIcons2), (DWORD)5192, nullptr, &nFontss);
    this->m_TabFont3 = AddFontMemResourceEx((void*)(RawData::TabIcons3), (DWORD)5192, nullptr, &nFonts3);
    this->m_TabFont4 = AddFontMemResourceEx((void*)(RawData::TabIcons4), (DWORD)5192, nullptr, &nFonts4);

    D3DXCreateFontA(m_Device, 47, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, "gamesense", &Fonts::TabIcons);
    D3DXCreateFontA(m_Device, 47, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, "menu_font", &Fonts::TabIcons2);
    D3DXCreateFontA(m_Device, 50, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, "gs_icon", &Fonts::TabIcons3);
    D3DXCreateFontA(m_Device, 100, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, "gamsens", &Fonts::TabIcons4);

    this->m_LBFont = AddFontMemResourceEx((void*)(RawData::LBIcons), (DWORD)/*5192*/10384, nullptr, &nFonts2);
    D3DXCreateFontA(m_Device, 30, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DRAFT_QUALITY, DEFAULT_PITCH, "undefeated", &Fonts::LBIcons);

    D3DXCreateTextureFromFileInMemoryEx(m_Device, RawData::BgTexture, 424852, 4096, 4096, D3DX_DEFAULT, NULL,
        pp.BackBufferFormat, D3DPOOL_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, NULL, NULL, NULL, &this->m_BgTexture);

    D3DXCreateFontA(m_Device, 8, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, FIXED_PITCH, "Small Fonts", &Fonts::Keybind);

    D3DXCreateFontA(m_Device, 10, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Lucida console", &Fonts::Logs);
    D3DXCreateFontA(m_Device, 30, 0, 600, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, "Calibri", &Fonts::Indicator);
    // Dpi Fonts
    D3DXCreateFontA(m_Device, 13, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Verdana", &Fonts::Verdana25);
    D3DXCreateFontA(m_Device, 15, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Verdana", &Fonts::Verdana50);
    D3DXCreateFontA(m_Device, 19, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Verdana", &Fonts::Verdana75);
    D3DXCreateFontA(m_Device, 21, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Verdana", &Fonts::Verdana200);
    // Dpi Fonts Bold
    D3DXCreateFontA(m_Device, 13, 0, FW_BOLD, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Tahoma", &Fonts::Tahombd25);
    D3DXCreateFontA(m_Device, 15, 0, FW_BOLD, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Tahoma", &Fonts::Tahombd50);
    D3DXCreateFontA(m_Device, 19, 0, FW_BOLD, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Tahoma", &Fonts::Tahombd75);
    D3DXCreateFontA(m_Device, 21, 0, FW_BOLD, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, DEFAULT_PITCH, "Tahoma", &Fonts::Tahombd200);


    D3DXCreateFontA(m_Device, 9, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, FIXED_PITCH, "Verdana", &Fonts::Keybind15);
    D3DXCreateFontA(m_Device, 11, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, FIXED_PITCH, "Verdana", &Fonts::Keybind35);
    D3DXCreateFontA(m_Device, 15, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, FIXED_PITCH, "Verdana", &Fonts::Keybind75);
    D3DXCreateFontA(m_Device, 19, 0, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLEARTYPE_NATURAL_QUALITY, FIXED_PITCH, "Verdana", &Fonts::Keybind200);

}

void CDraw::ReleaseObjects()
{
    if (!Initialized)
        return;

    if (m_Sprite)
        m_Sprite->Release();
    if (Fonts::Verdana)
        Fonts::Verdana->Release();
    if (Fonts::Tahombd)
        Fonts::Tahombd->Release();
    if (Fonts::Logs)
        Fonts::Logs->Release();
    if (Fonts::Indicator)
        Fonts::Indicator->Release();
    if (Fonts::IndicatorShadow)
        Fonts::IndicatorShadow->Release();

    if (Fonts::LBIcons) {
        Fonts::LBIcons->Release();
        RemoveFontMemResourceEx(this->m_LBFont);
    }
    if (Fonts::TabIcons) {
        Fonts::TabIcons->Release();
        RemoveFontMemResourceEx(this->m_TabFont);
    }
    if (Fonts::TabIcons2) {
        Fonts::TabIcons2->Release();
        RemoveFontMemResourceEx(this->m_TabFont2);
    }
    if (Fonts::TabIcons3) {
        Fonts::TabIcons3->Release();
        RemoveFontMemResourceEx(this->m_TabFont3);
    }
    if (Fonts::TabIcons4) {
        Fonts::TabIcons4->Release();
        RemoveFontMemResourceEx(this->m_TabFont4);
    }
}

void CDraw::Init(LPDIRECT3DDEVICE9 D3dDevice)
{
    if (!Initialized) {
        m_Device = D3dDevice;
        CreateObjects();
        Initialized = true;
    }
}

void CDraw::Reset()
{
    D3DVIEWPORT9 screen;
    m_Device->GetViewport(&screen);

    Screen.Width = (float)screen.Width;
    Screen.Height = (float)screen.Height;
    Screen.x_center = Screen.Width / 2;
    Screen.y_center = Screen.Height / 2;

    m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void CDraw::Text(const char* text, float x_, float y_, int Orientation, LPD3DXFONT Font, bool Bordered, D3DCOLOR Color, Vec2 TextClipSize)
{

    if (!Font)
        return;
    int x((int)x_), y((int)y_);

    bool NoClipRect = (TextClipSize.x == 0 && TextClipSize.y == 0) ? true : false;
    DWORD TextFlags = 0x0;

    RECT rect;
    switch (Orientation)
    {
    case LEFT: TextFlags = NoClipRect ? DT_LEFT | DT_NOCLIP : DT_LEFT; break;
    case CENTER: TextFlags = NoClipRect ? DT_CENTER | DT_NOCLIP : DT_CENTER;  break;
    case RIGHT: TextFlags = NoClipRect ? DT_RIGHT | DT_NOCLIP : DT_RIGHT; break;
    }

    if (Bordered)
    {
        NoClipRect ? SetRect(&rect, x - 1, y, x - 1, y) : SetRect(&rect, x - 1, y, (int)TextClipSize.x - 1, (int)TextClipSize.y);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));
        NoClipRect ? SetRect(&rect, x + 1, y, x + 1, y) : SetRect(&rect, x + 1, (int)TextClipSize.y, (int)TextClipSize.x + 1, (int)TextClipSize.y);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));
        NoClipRect ? SetRect(&rect, x, y - 1, x, y - 1) : SetRect(&rect, x, y - 1, (int)TextClipSize.x, (int)TextClipSize.y - 1);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));
        NoClipRect ? SetRect(&rect, x, y + 1, x, y + 1) : SetRect(&rect, x, y + 1, (int)TextClipSize.x, (int)TextClipSize.y + 1);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));

        NoClipRect ? SetRect(&rect, x - 1, y - 1, x - 1, y - 1) : SetRect(&rect, x - 1, y, (int)TextClipSize.x - 1, (int)TextClipSize.y);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));
        NoClipRect ? SetRect(&rect, x + 1, y + 1, x + 1, y + 1) : SetRect(&rect, x + 1, (int)TextClipSize.y, (int)TextClipSize.x + 1, (int)TextClipSize.y);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));
        NoClipRect ? SetRect(&rect, x - 1, y + 1, x - 1, y + 1) : SetRect(&rect, x, y - 1, (int)TextClipSize.x, (int)TextClipSize.y - 1);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));
        NoClipRect ? SetRect(&rect, x + 1, y - 1, x + 1, y - 1) : SetRect(&rect, x, y + 1, (int)TextClipSize.x, (int)TextClipSize.y + 1);
        Font->DrawTextA(NULL, text, -1, &rect, TextFlags, D3DCOLOR_RGBA(15, 15, 15, get_a(Color)));
    }

    NoClipRect ? SetRect(&rect, x, y, x, y) : SetRect(&rect, x, y, (int)TextClipSize.x, (int)TextClipSize.y);
    Font->DrawTextA(NULL, text, -1, &rect, TextFlags, Color);
}

void CDraw::Line(Vec2 Pos, Vec2 Pos2, D3DCOLOR Color)
{
    vertex vertices[2] = {
        { round(Pos.x), round(Pos.y), 0.0f, 1.0f, Color },
        { round(Pos2.x), round(Pos2.y), 0.0f, 1.0f, Color },
    };
    m_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    m_Device->SetTexture(0, nullptr);

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);

    m_Device->DrawPrimitiveUP(D3DPT_LINELIST, 1, vertices, sizeof(vertex));

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
}

void CDraw::Rect(Vec2 Pos, Vec2 Size, float LineWidth, D3DCOLOR Color, bool Antialias)
{
    if (LineWidth == 0 || LineWidth == 1)
    {
        FilledRect(Pos, Vec2(Size.x, 1), Color, Antialias);             // Top
        FilledRect(Vec2(Pos.x, Pos.y + Size.y - 1), Vec2(Size.x, 1), Color, Antialias);         // Bottom
        FilledRect(Vec2(Pos.x, Pos.y + 1), Vec2(1, Size.y - 2 * 1), Color, Antialias);       // Left
        FilledRect(Vec2(Pos.x + Size.x - 1, Pos.y + 1), Vec2(1, Size.y - 2 * 1), Color, Antialias);   // Right
    }
    else
    {
        FilledRect(Pos, Vec2(Size.x, LineWidth), Color, Antialias);                                     // Top
        FilledRect(Vec2(Pos.x, Pos.y + Size.y - LineWidth), Vec2(Size.x, LineWidth), Color, Antialias);                         // Bottom
        FilledRect(Vec2(Pos.x, Pos.y + LineWidth), Vec2(LineWidth, Size.x - 2 * LineWidth), Color, Antialias);               // Left
        FilledRect(Vec2(Pos.x + Size.x - LineWidth, Pos.y + LineWidth), Vec2(LineWidth, Size.y - 2 * LineWidth), Color, Antialias);   // Right
    }
}

void CDraw::FilledRect(Vec2 Pos, Vec2 Size, D3DCOLOR Color, bool Antialias)
{
    vertex vertices[4] = {
        { Pos.x, Pos.y + Size.y, 0.0f, 1.0f, Color },
        { Pos.x, Pos.y, 0.0f, 1.0f, Color },
        { Pos.x + Size.x, Pos.y + Size.y, 0.0f, 1.0f, Color },
        { Pos.x + Size.x, Pos.y, 0.0f, 1.0f, Color }
    };

    m_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    m_Device->SetTexture(0, nullptr);

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, Antialias ? TRUE : FALSE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, Antialias ? TRUE : FALSE);

    m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(vertex));

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
}

void CDraw::Gradient(Vec2 Pos, Vec2 Size, D3DCOLOR LColor, D3DCOLOR ROtherColor, bool Vertical, bool Antialias)
{
    vertex vertices[4] = {
        { Pos.x, Pos.y, 0.0f, 1.0f, LColor },
        { Pos.x + Size.x, Pos.y, 0.0f, 1.0f, Vertical ? LColor : ROtherColor },
        { Pos.x, Pos.y + Size.y, 0.0f, 1.0f, Vertical ? ROtherColor : LColor },
        { Pos.x + Size.x, Pos.y + Size.y, 0.0f, 1.0f, ROtherColor }
    };

    m_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    m_Device->SetTexture(0, nullptr);

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, Antialias ? TRUE : FALSE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, Antialias ? TRUE : FALSE);

    m_Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vertices, sizeof(vertex));

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);
}

void CDraw::Triangle(Vec2 Top, Vec2 Left, Vec2 Right, D3DCOLOR Color, bool antialias)
{
    vertex vertices[3] =
    {
        { Top.x, Top.y, 0.0f, 1.0f, Color, },
        { Right.x, Right.y, 0.0f, 1.0f, Color, },
        { Left.x, Left.y, 0.0f, 1.0f, Color, },
    };

    m_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    m_Device->SetTexture(0, nullptr);

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, antialias ? TRUE : FALSE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, antialias ? TRUE : FALSE);

    m_Device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 1, vertices, sizeof(vertex));

    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, antialias ? TRUE : FALSE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, antialias ? TRUE : FALSE);

}


void CDraw::Sprite(LPDIRECT3DTEXTURE9 Texture, Vec2 Pos, Vec2 Size, D3DCOLOR Color)
{
    D3DXVECTOR3 pos = D3DXVECTOR3(Pos.x, Pos.y, 0.0f);
    RECT DrawArea = { (LONG)pos.x, (LONG)pos.y, LONG(pos.x + Size.x), LONG(pos.y + Size.y) };

    m_Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
    m_Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    m_Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);

    m_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    m_Device->SetPixelShader(NULL);
    m_Sprite->Begin(D3DXSPRITE_ALPHABLEND);
    m_Sprite->Draw(Texture, &DrawArea, NULL, &pos, D3DCOLOR_RGBA(255, 255, 255, get_a(Color)));
    m_Sprite->End();
}



IDirect3DTexture9* CDraw::GetBgTexture()
{
    return this->m_BgTexture;
}

Vec2 CDraw::GetTextSize(ID3DXFont* Font, const char* Text)
{
    auto rect = RECT();
    if (!Font)
        return Vec2(0,0);

    Font->DrawTextA(0, Text, strlen(Text), &rect, DT_CALCRECT, D3DCOLOR_ARGB(0, 0, 0, 0));

    return Vec2(float(rect.right - rect.left), float(rect.bottom - rect.top));
}

void CDraw::GradientColor(Vec2 Pos, Vec2 Size, D3DCOLOR color)
{
    struct D3DVERTEX
    {
        float x, y, z, rhw;
        DWORD color;
    };

    D3DVERTEX vertices[4];

    vertices[0].x = Pos.x;
    vertices[0].y = Pos.y;
    vertices[0].z = 0.f;
    vertices[0].rhw = 1.f;
    vertices[0].color = 0xffffffff;

    vertices[1].x = Pos.x + Size.x;
    vertices[1].y = Pos.y;
    vertices[1].z = 0.f;
    vertices[1].rhw = 1.f;
    vertices[1].color = color;

    vertices[2].x = Pos.x;
    vertices[2].y = Pos.y + Size.y;
    vertices[2].z = 0.f;
    vertices[2].rhw = 1.f;
    vertices[2].color = 0xff000000;

    vertices[3].x = Pos.x + Size.x;
    vertices[3].y = Pos.y + Size.y;
    vertices[3].z = 0.f;
    vertices[3].rhw = 1.f;
    vertices[3].color = 0xff000000;


    static LPDIRECT3DVERTEXBUFFER9 pVertexObject = NULL;
    static void* pVertexBuffer = NULL;

    if (!pVertexObject) {
        if (FAILED(m_Device->CreateVertexBuffer(sizeof(vertices), 0,
            D3DFVF_XYZRHW | D3DFVF_DIFFUSE, D3DPOOL_DEFAULT, &pVertexObject, NULL)))
            return;
    }
    if (FAILED(pVertexObject->Lock(0, sizeof(vertices), &pVertexBuffer, 0)))
        return;

    memcpy(pVertexBuffer, vertices, sizeof(vertices));
    pVertexObject->Unlock();

    m_Device->SetStreamSource(0, pVertexObject, 0, sizeof(D3DVERTEX));
    m_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
    m_Device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    m_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    m_Device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

}

LPDIRECT3DDEVICE9 CDraw::GetD3dDevice()
{
    return this->m_Device;
}

