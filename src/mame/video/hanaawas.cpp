// license:BSD-3-Clause
// copyright-holders:Zsolt Vasvari
/***************************************************************************

  video.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "emu.h"
#include "includes/hanaawas.h"

/***************************************************************************

  Convert the color PROMs into a more useable format.

***************************************************************************/

void hanaawas_state::hanaawas_palette(palette_device &palette) const
{
	uint8_t const *color_prom = memregion("proms")->base();

	// create a lookup table for the palette
	for (int i = 0; i < 0x10; i++)
	{
		int bit0, bit1, bit2;

		/* red component */
		bit0 = BIT(color_prom[i], 0);
		bit1 = BIT(color_prom[i], 1);
		bit2 = BIT(color_prom[i], 2);
		int const r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* green component */
		bit0 = BIT(color_prom[i], 3);
		bit1 = BIT(color_prom[i], 4);
		bit2 = BIT(color_prom[i], 5);
		int const g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		/* blue component */
		bit0 = 0;
		bit1 = BIT(color_prom[i], 6);
		bit2 = BIT(color_prom[i], 7);
		int const b = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;

		palette.set_indirect_color(i, rgb_t(r, g, b));
	}

	// color_prom now points to the beginning of the lookup table
	color_prom += 0x20;

	// character lookup table.  The 1bpp tiles really only use colors 0-0x0f and the 3bpp ones 0x10-0x1f
	for (int i = 0; i < 0x100; i++)
	{
		int const swapped_i = bitswap<8>(i, 2,7,6,5,4,3,1,0);
		uint8_t const ctabentry = color_prom[swapped_i] & 0x0f;
		palette.set_pen_indirect(i, ctabentry);
	}
}

WRITE8_MEMBER(hanaawas_state::hanaawas_videoram_w)
{
	m_videoram[offset] = data;
	m_bg_tilemap->mark_tile_dirty(offset);
}

WRITE8_MEMBER(hanaawas_state::hanaawas_colorram_w)
{
	m_colorram[offset] = data;

	/* dirty both current and next offsets */
	m_bg_tilemap->mark_tile_dirty(offset);
	m_bg_tilemap->mark_tile_dirty((offset + (flip_screen() ? -1 : 1)) & 0x03ff);
}

WRITE8_MEMBER(hanaawas_state::hanaawas_portB_w)
{
	/* bit 7 is flip screen */
	if (flip_screen() != (~data & 0x80))
	{
		flip_screen_set(~data & 0x80);
		machine().tilemap().mark_all_dirty();
	}
}

TILE_GET_INFO_MEMBER(hanaawas_state::get_bg_tile_info)
{
	/* the color is determined by the current color byte, but the bank is via the previous one!!! */
	int offset = (tile_index + (flip_screen() ? 1 : -1)) & 0x3ff;
	int attr = m_colorram[offset];
	int gfxbank = (attr & 0x40) >> 6;
	int code = m_videoram[tile_index] + ((attr & 0x20) << 3);
	int color = m_colorram[tile_index] & 0x1f;

	tileinfo.set(gfxbank, code, color, 0);
}

void hanaawas_state::video_start()
{
	m_bg_tilemap = &machine().tilemap().create(*m_gfxdecode, tilemap_get_info_delegate(*this, FUNC(hanaawas_state::get_bg_tile_info)), TILEMAP_SCAN_ROWS, 8, 8, 32, 32);
}

uint32_t hanaawas_state::screen_update_hanaawas(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect)
{
	m_bg_tilemap->draw(screen, bitmap, cliprect, 0, 0);
	return 0;
}
