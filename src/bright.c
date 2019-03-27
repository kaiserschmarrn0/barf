#include "barf.h"

float bright_get_frac(component *this) {
	xcb_randr_get_output_property_cookie_t cookie =
			xcb_randr_get_output_property_unchecked(this->data.conn,
			this->data.monitor, this->data.backlight_atom, XCB_ATOM_NONE, 0, 4, 0, 0);
	xcb_randr_get_output_property_reply_t *reply =
			xcb_randr_get_output_property_reply(this->data.conn, cookie, NULL);

	if (!reply || XCB_ATOM_INTEGER != reply->type || 1 != reply->num_items ||
			32 != reply->format) {
		fprintf(stderr, "Bright: backlight atom not supported.\n");
		free(reply);
		return -1;
	}

	uint32_t value = *(uint32_t *)xcb_randr_get_output_property_data(reply);
	float min_bright = this->data.min_bright;
	return ((float)value - min_bright) / ((float)this->data.max_bright - min_bright);
}

static void draw_bright_icon(component *this) {
	float frac = bright_get_frac(this);

	if (frac < 0) {
		return;
	}
			
	char icon[ICON_MAX];
	if (frac > 0.7f) {
		snprintf(icon, ICON_MAX, "\uf005");
	} else if (frac > 0.3f) {
		snprintf(icon, ICON_MAX, "\uf123");
	} else {
		snprintf(icon, ICON_MAX, "\uf006");
	}

	draw_block(this, icon, "\0");
}

static void draw_bright_text(component *this) {
	float frac = bright_get_frac(this);
	
	if (frac < 0) {
		return;
	}

	char text[TEXT_MAX];
	snprintf(text, TEXT_MAX, "%.0f", frac * (float)100);

	draw_block(this, "\0", text);
}

int bright_init(component *this) {
	xcb_connection_t *conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		fprintf(stderr, "Bright: couldn't connect to X.\n");
		return 1;
	}

	xcb_screen_t *scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	if (!scr) {
		fprintf(stderr, "Bright: couldn't get X screen.\n");
		xcb_disconnect(conn);
		return 1;
	}

	const xcb_query_extension_reply_t *qer = xcb_get_extension_data(conn, &xcb_randr_id);
	if (!qer || !qer->present) {
		fprintf(stderr, "Bright: couldn't get xrandr.\n");
		xcb_disconnect(conn);
		return 1;
	}

	xcb_randr_get_screen_resources_current_cookie_t mon_cookie =
			xcb_randr_get_screen_resources_current(conn, scr->root);
	xcb_randr_get_screen_resources_current_reply_t *mon_reply =
			xcb_randr_get_screen_resources_current_reply(conn, mon_cookie, NULL);

	if (!mon_reply) {
		fprintf(stderr, "Bright: couldn't get xrandr screen resources.\n");
		xcb_disconnect(conn);
		return 1;
	}

	int num_monitors = xcb_randr_get_screen_resources_current_outputs_length(mon_reply);
	xcb_randr_output_t *monitors = xcb_randr_get_screen_resources_current_outputs(mon_reply);

	if (num_monitors < 1) {
		fprintf(stderr, "Bright: couldn't get xrandr monitors.\n");
		xcb_disconnect(conn);
		free(mon_reply);
		return 1;
	}

	/* just grab the first monitor */
	this->data.monitor = *monitors;
	free(mon_reply);

	xcb_intern_atom_cookie_t cookie = xcb_intern_atom(conn, 0, sizeof("Backlight") - 1,
			"Backlight");

	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(conn, cookie, NULL);
	if (!reply) {
		fprintf(stderr, "Bright: couldn't get backlight atom.\n");
		xcb_disconnect(conn);
		return 1;
	} else if (reply->atom == XCB_ATOM_NONE) {
		fprintf(stderr, "Bright: backlight atom not supported.\n");
		xcb_disconnect(conn);
		free(reply);
		return 1;
	}
	this->data.backlight_atom = reply->atom;
	free(reply);
	
	xcb_randr_query_output_property_cookie_t range_cookie =
			xcb_randr_query_output_property_unchecked(conn, this->data.monitor,
			this->data.backlight_atom);
	xcb_randr_query_output_property_reply_t *range_reply =
			xcb_randr_query_output_property_reply(conn, range_cookie, NULL);

	uint32_t *scale = xcb_randr_query_output_property_valid_values(range_reply);
	this->data.min_bright = scale[0];
	this->data.max_bright = scale[1];

	free(range_reply);

	xcb_randr_select_input(conn, scr->root, XCB_RANDR_NOTIFY_MASK_OUTPUT_PROPERTY);

	this->data.conn = conn;
	this->fd = xcb_get_file_descriptor(conn);
	
	this->fg = xft_color(0xff3b4252);
	change_gc(this->bg, 0xff81a1c1);

	draw_bright_icon(this);

	return 0;
}

static int bright_event(component *this) {
	xcb_randr_notify_event_t *ev =
			(xcb_randr_notify_event_t *)xcb_poll_for_event(this->data.conn);

	if (ev->subCode != XCB_RANDR_NOTIFY_OUTPUT_PROPERTY ||
			ev->u.op.status != XCB_PROPERTY_NEW_VALUE ||
			ev->u.op.atom != this->data.backlight_atom) {
		free(ev);
		return 1;
	}

	free(ev);
	return 0;
}

void bright_run_icon(component *this) {
	if (bright_event(this)) {
		return;
	}

	float frac = bright_get_frac(this);

	if (frac < 0) {
		return;
	}
			
	char icon[ICON_MAX];
	if (frac > 0.7f) {
		snprintf(icon, ICON_MAX, "\uf005");
	} else if (frac > 0.3f) {
		snprintf(icon, ICON_MAX, "\uf123");
	} else {
		snprintf(icon, ICON_MAX, "\uf006");
	}

	draw_block(this, icon, "\0");
}

void bright_run_text(component *this) {
	if (bright_event(this)) {
		return;
	}

	float frac = bright_get_frac(this);
	
	if (frac < 0) {
		return;
	}

	char text[TEXT_MAX];
	snprintf(text, TEXT_MAX, "%.0f", frac * (float)100);

	draw_block(this, "\0", text);
}

void bright_clean(component *this) {
	xcb_disconnect(this->data.conn);
}

void bright_click(component *this) {
	if (this->run == bright_run_icon) {
		draw_bright_text(this);
		this->run = bright_run_text;
	} else {
		draw_bright_icon(this);
		this->run = bright_run_icon;
	}
}
