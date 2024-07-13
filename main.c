/*
 * TODO:
 * - line breaking algorithm
 *   - generate list of line breaks given glyph list
 *   - https://en.wikipedia.org/wiki/Line_breaking_rules_in_East_Asian_languages
 *   - Knuth-Plass?
 * - create window with GTK
 * - render with Cairo
 * - read text from file
 * - render file name
 * - parse simple markup language
 * - color links
 * - proper alpha compositing
 * - mouse click support
 * - parse directory
 * - navigate upon clicking link
 * - detect mouse scroll
 * - move page up and down upon scroll
 * - image file parsing
 * - image rendering
 * - split client and server with wire protocol (capn?)
 * - communicate over socket
 */

#include <stdlib.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <hb-ft.h>
#include <hb.h>

unsigned char bmout[3000][3000] = {0};

typedef struct {
  hb_position_t x;
  hb_position_t y;
} Position;

void draw_bitmap(FT_Bitmap *bitmap, Position pos) {
  unsigned char *p = bitmap->buffer;
  for (int yy = 0; yy < bitmap->rows; ++yy) {
    for (int xx = 0; xx < bitmap->width; ++xx) {
      bmout[pos.y + yy][pos.x + xx] = p[xx] > bmout[pos.y + yy][pos.x + xx]
                                          ? p[xx]
                                          : bmout[pos.y + yy][pos.x + xx];
    }
    p += bitmap->pitch;
  }
}

void draw_glyph(FT_Face face, hb_codepoint_t glyphid, Position pos) {
  FT_Load_Glyph(face, glyphid, FT_LOAD_DEFAULT);
  FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
  printf("%u: %d,%d\n", glyphid, pos.x, pos.y);
  pos.x += face->glyph->bitmap_left;
  pos.y -= face->glyph->bitmap_top;
  draw_bitmap(&face->glyph->bitmap, pos);
}

int main() {

  FILE *out = fopen("out.pgm", "wb");
  fprintf(out, "P5\n3000 3000\n255\n");

  FT_Library library;
  FT_Error error;
  if ((error = FT_Init_FreeType(&library))) {
    exit(error);
  }

  FT_Face face;
  if ((error = FT_New_Face(library, "/System/Library/Fonts/Helvetica.ttc", 0,
                           &face))) {
    exit(error);
  }
  FT_Set_Char_Size(face, 0, 64 * 64, 0, 0);
  hb_buffer_t *buf = hb_buffer_create();
  hb_buffer_add_utf8(buf, "Hello world", -1, 0, -1);

  hb_buffer_guess_segment_properties(buf);

  hb_font_t *font = hb_ft_font_create_referenced(face);

  hb_font_extents_t extents;
  hb_font_get_v_extents(font, &extents);
  printf("Extents = %d\n", extents.line_gap);

  hb_shape(font, buf, NULL, 0);

  unsigned int glyph_count;
  hb_glyph_info_t *glyph_infos = hb_buffer_get_glyph_infos(buf, &glyph_count);
  hb_glyph_position_t *glyph_positions =
      hb_buffer_get_glyph_positions(buf, &glyph_count);

  Position cursor = {.x = 0, .y = 1500 * 64};
  for (unsigned int i = 0; i < glyph_count; i++) {
    if (cursor.x > 2000 * 64) {
      cursor.x = 0;
      cursor.y += 64 * 64;
    }
    hb_codepoint_t glyphid = glyph_infos[i].codepoint;
    hb_position_t x_offset = glyph_positions[i].x_offset;
    hb_position_t y_offset = glyph_positions[i].y_offset;
    hb_position_t x_advance = glyph_positions[i].x_advance;
    hb_position_t y_advance = glyph_positions[i].y_advance;
    Position pos = {.x = (cursor.x + x_offset) / 64,
                    .y = (cursor.y + y_offset) / 64};
    draw_glyph(face, glyphid, pos);
    cursor.x += x_advance;
    cursor.y += y_advance;
  }

  hb_buffer_destroy(buf);
  hb_font_destroy(font);

  for (int y = 0; y < 3000; ++y) {
    fwrite(bmout[y], 3000, 1, out);
  }

  fclose(out);
}
