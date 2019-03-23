typedef struct {
	void (*info) (char *, char*);

	int start;
	int width;

	uint32_t fg_col;
	uint32_t bg_col;
	
	void (*click) (xcb_button_press_event_t *ev);

	XftColor fg;
	xcb_gcontext_t bg;

	int needs_update;
} Block;
