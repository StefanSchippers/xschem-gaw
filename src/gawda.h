#ifndef GAWDA_H
#define GAWDA_H


typedef struct _SelRange SelRange;
typedef enum _SelRangeType SelRangeType;
typedef enum _MouseState MouseState;
typedef struct _GawSegment GawSegment;

/*
 * Global structures
 */

/*
 * state of mouse for drag operations
 */
enum _MouseState {
   M_NONE,
      M_CURSOR_DRAG,
      M_SELRANGE_ARMED,
      M_SELRANGE_ACTIVE,
      M_DRAW_TEXT,
      M_TEXT_DRAG,
};

enum _SelRangeType {
   SR_X = 1,
      SR_Y,
      SR_XY
};

/*
 * state related to selecting ranges/regions in X, Y, and XY
 * pixel space of a wavepanel.
 */

struct _SelRange {
   int drawn;
   SelRangeType type;
   WavePanel *wp;
   GdkRGBA *color;
   int y1;
   int y2;
   int x1;
   int x2;
   int x1_root;
   int y1_root;
};

/*
 * a temporary replacement for GdkSegment
 */
struct _GawSegment {
  gint x1;
  gint y1;
  gint x2;
  gint y2;
};

/*
 * Global functions
 */

void da_drawing_redraw(GtkWidget *w);
GtkWidget *da_drawing_create(WavePanel *wp);
void da_set_gdk_cursor(GtkWidget *w, int cursorType);
void da_drawing_set_size_request(GtkWidget *drawing, int w, int h);

#endif /* GAWDA_H */
