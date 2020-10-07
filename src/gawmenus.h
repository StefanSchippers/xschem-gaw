#ifndef GAWMENUS_H
#define GAWMENUS_H

/*
 * gawmenus.h - gawmenus.h interface
 *
 * include LICENSE
 */
typedef struct _tooltipInfo TooltipInfo;
struct _tooltipInfo {
   gchar *name;
   gchar *label;
   gchar *tooltip;
   GtkWidget *widget;
};

/*
 * prototypes
 */
void gm_update_menu(GtkWidget *menu, TooltipInfo *tip_table);
void gm_create_layout (UserData *ud);
void gm_create_panel_popmenu (WavePanel *wp );
void gm_update_toggle_state(GSimpleActionGroup *group, const gchar *action_name,
                       gboolean is_active);
void gm_update_action_sensitive(GSimpleActionGroup *group, const gchar *action_name,
                           gboolean is_sensitive );

void gm_create_vl_menu (DataFile *wdata );
void gm_create_vw_popmenu (VisibleWave * vw );

#endif /* GAWMENUS_H */
