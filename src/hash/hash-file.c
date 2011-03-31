/*
 *   Copyright (C) 2007-2010 Tristan Heaven <tristanheaven@gmail.com>
 *
 *   This file is part of GtkHash.
 *
 *   GtkHash is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   GtkHash is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GtkHash. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
	#include "config.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <gdk/gdk.h>
#include <glib.h>

#include "hash-file.h"
#include "hash-func.h"
#include "hash-lib.h"

static bool gtkhash_hash_file(struct hash_file_s *data);

unsigned int gtkhash_hash_file_get_source(struct hash_file_s *data)
{
	g_mutex_lock(data->priv.mutex);
	unsigned int source = data->priv.source;
	g_mutex_unlock(data->priv.mutex);

	return source;
}

static void gtkhash_hash_file_set_source(struct hash_file_s *data,
	const unsigned int source)
{
	g_mutex_lock(data->priv.mutex);
	data->priv.source = source;
	g_mutex_unlock(data->priv.mutex);
}

void gtkhash_hash_file_add_source(struct hash_file_s *data)
{
	g_mutex_lock(data->priv.mutex);
	if (G_UNLIKELY(data->priv.source != 0))
		g_critical("source was already added");
	data->priv.source = gdk_threads_add_idle(
		(GSourceFunc)gtkhash_hash_file, data);
	g_mutex_unlock(data->priv.mutex);
}

static void gtkhash_hash_file_remove_source(struct hash_file_s *data)
{
	g_mutex_lock(data->priv.mutex);
	if (G_UNLIKELY(!g_source_remove(data->priv.source)))
		g_critical("failed to remove source");
	data->priv.source = 0;
	g_mutex_unlock(data->priv.mutex);
}

bool gtkhash_hash_file_get_stop(struct hash_file_s *data)
{
	g_mutex_lock(data->priv.mutex);
	bool stop = data->priv.stop;
	g_mutex_unlock(data->priv.mutex);

	return stop;
}

void gtkhash_hash_file_set_stop(struct hash_file_s *data, const bool stop)
{
	g_mutex_lock(data->priv.mutex);
	data->priv.stop = stop;
	g_mutex_unlock(data->priv.mutex);
}

enum hash_file_state_e gtkhash_hash_file_get_state(struct hash_file_s *data)
{
	g_mutex_lock(data->priv.mutex);
	enum hash_file_state_e state = data->priv.state;
	g_mutex_unlock(data->priv.mutex);

	return state;
}

void gtkhash_hash_file_set_state(struct hash_file_s *data,
	const enum hash_file_state_e state)
{
	g_mutex_lock(data->priv.mutex);
	data->priv.state = state;
	g_mutex_unlock(data->priv.mutex);
}

void gtkhash_hash_file_set_uri(struct hash_file_s *data, const char *uri)
{
	data->uri = uri;
}

static void gtkhash_hash_file_start(struct hash_file_s *data)
{
	g_assert(data->uri);

	for (int i = 0; i < HASH_FUNCS_N; i++)
		if (data->funcs[i].enabled)
			gtkhash_hash_lib_start(&data->funcs[i]);

	data->buffer = g_malloc(FILE_BUFFER_SIZE);
	data->file = g_file_new_for_uri(data->uri);
	data->current_func = 0;
	data->just_read = 0;
	data->total_read = 0;
	data->timer = g_timer_new();

	gtkhash_hash_file_set_state(data, HASH_FILE_STATE_OPEN);
}

static void gtkhash_hash_file_open_finish(
	G_GNUC_UNUSED GObject *source, GAsyncResult *res, struct hash_file_s *data)
{
	data->stream = g_file_read_finish(data->file, res, NULL);
	if (!data->stream) {
		g_warning("file_open_finish: failed to open file");
		gtkhash_hash_file_set_stop(data, true);
	}

	if (gtkhash_hash_file_get_stop(data))
		if (data->stream)
			gtkhash_hash_file_set_state(data, HASH_FILE_STATE_CLOSE);
		else
			gtkhash_hash_file_set_state(data, HASH_FILE_STATE_FINISH);
	else
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_GET_SIZE);

	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_open(struct hash_file_s *data)
{
	if (gtkhash_hash_file_get_stop(data)) {
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_FINISH);
		return;
	}

	gtkhash_hash_file_remove_source(data);
	g_file_read_async(data->file, G_PRIORITY_DEFAULT, NULL,
		(GAsyncReadyCallback)gtkhash_hash_file_open_finish, data);
}

static void gtkhash_hash_file_get_size_finish(
	G_GNUC_UNUSED GObject *source, GAsyncResult *res, struct hash_file_s *data)
{
	GFileInfo *info = g_file_input_stream_query_info_finish(
		data->stream, res, NULL);
	data->file_size = g_file_info_get_size(info);
	g_object_unref(info);

	if (gtkhash_hash_file_get_stop(data))
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_CLOSE);
	else if (data->file_size == 0)
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_HASH);
	else
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_READ);

	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_get_size(struct hash_file_s *data)
{
	if (gtkhash_hash_file_get_stop(data)) {
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_CLOSE);
		return;
	}

	gtkhash_hash_file_remove_source(data);
	g_file_input_stream_query_info_async(data->stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, G_PRIORITY_DEFAULT, NULL,
		(GAsyncReadyCallback)gtkhash_hash_file_get_size_finish, data);
}

static void gtkhash_hash_file_read_finish(
	G_GNUC_UNUSED GObject *source, GAsyncResult *res, struct hash_file_s *data)
{
	data->just_read = g_input_stream_read_finish(
		G_INPUT_STREAM(data->stream), res, NULL);

	if (G_UNLIKELY(data->just_read == -1)) {
		g_warning("file_read_finish: failed to read file");
		gtkhash_hash_file_set_stop(data, true);
	} else if (G_UNLIKELY(data->just_read == 0)) {
		g_warning("file_read_finish: unexpected EOF");
		gtkhash_hash_file_set_stop(data, true);
	} else {
		data->total_read += data->just_read;
		if (G_UNLIKELY(data->total_read > data->file_size)) {
			g_warning("file_read_finish: read %" G_GOFFSET_FORMAT
				" bytes more than expected",
				data->total_read - data->file_size);
			gtkhash_hash_file_set_stop(data, true);
		} else
			gtkhash_hash_file_set_state(data, HASH_FILE_STATE_HASH);
	}

	if (G_UNLIKELY(gtkhash_hash_file_get_stop(data)))
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_CLOSE);

	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_read(struct hash_file_s *data)
{
	if (G_UNLIKELY(gtkhash_hash_file_get_stop(data))) {
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_CLOSE);
		return;
	}

	gtkhash_hash_file_remove_source(data);
	g_input_stream_read_async(G_INPUT_STREAM(data->stream),
		data->buffer, FILE_BUFFER_SIZE, G_PRIORITY_DEFAULT, NULL,
		(GAsyncReadyCallback)gtkhash_hash_file_read_finish, data);
}

static void gtkhash_hash_file_hash(struct hash_file_s *data)
{
	if (G_UNLIKELY(gtkhash_hash_file_get_stop(data))) {
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_CLOSE);
		return;
	} else if (data->current_func < HASH_FUNCS_N) {
		if (data->funcs[data->current_func].enabled) {
			gtkhash_hash_lib_update(&data->funcs[data->current_func],
				data->buffer, data->just_read);
		}
		data->current_func++;
		return;
	}

	data->current_func = 0;

	gtkhash_hash_file_set_state(data, HASH_FILE_STATE_REPORT);
}

static void gtkhash_hash_file_report(struct hash_file_s *data)
{
	if (data->total_read >= data->file_size)
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_CLOSE);
	else
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_READ);

	if (data->file_size == 0)
		return;

	gtkhash_hash_file_report_cb(data->cb_data, data->file_size,
		data->total_read, data->timer);
}

static void gtkhash_hash_file_close_finish(
	G_GNUC_UNUSED GObject *source, GAsyncResult *res, struct hash_file_s *data)
{
	if (!g_input_stream_close_finish(G_INPUT_STREAM(data->stream),
		res, NULL))
	{
		g_warning("file_close_finish: failed to close file");
	}

	g_object_unref(data->stream);

	gtkhash_hash_file_set_state(data, HASH_FILE_STATE_FINISH);
	gtkhash_hash_file_add_source(data);
}

static void gtkhash_hash_file_close(struct hash_file_s *data)
{
	gtkhash_hash_file_remove_source(data);
	g_input_stream_close_async(G_INPUT_STREAM(data->stream),
		G_PRIORITY_DEFAULT, NULL,
		(GAsyncReadyCallback)gtkhash_hash_file_close_finish, data);
}

static void gtkhash_hash_file_finish(struct hash_file_s *data)
{
	if (gtkhash_hash_file_get_stop(data)) {
		for (int i = 0; i < HASH_FUNCS_N; i++)
			if (data->funcs[i].enabled)
				gtkhash_hash_lib_stop(&data->funcs[i]);
	} else {
		for (int i = 0; i < HASH_FUNCS_N; i++)
			if (data->funcs[i].enabled)
				gtkhash_hash_lib_finish(&data->funcs[i]);
	}

	g_timer_destroy(data->timer);
	g_free(data->buffer);
	g_object_unref(data->file);

	gtkhash_hash_file_set_source(data, 0);
	gtkhash_hash_file_set_state(data, HASH_FILE_STATE_TERM);
}

static bool gtkhash_hash_file(struct hash_file_s *data)
{
	static void (* const state_funcs[])(struct hash_file_s *) = {
		gtkhash_hash_file_start,
		gtkhash_hash_file_open,
		gtkhash_hash_file_get_size,
		gtkhash_hash_file_read,
		gtkhash_hash_file_hash,
		gtkhash_hash_file_report,
		gtkhash_hash_file_close,
		gtkhash_hash_file_finish
	};

	const enum hash_file_state_e state = gtkhash_hash_file_get_state(data);

	if (state == HASH_FILE_STATE_TERM) {
		gtkhash_hash_file_set_state(data, HASH_FILE_STATE_IDLE);
		gtkhash_hash_file_finish_cb(data->cb_data);
		return false;
	}

	// Call func
	state_funcs[state](data);

	return true;
}

void gtkhash_hash_file_init(struct hash_file_s *data, struct hash_func_s *funcs,
	void *cb_data)
{
	data->priv.mutex = g_mutex_new();
	data->priv.source = 0;
	data->priv.state = HASH_FILE_STATE_IDLE;
	data->priv.stop = false;
	data->funcs = funcs;
	data->cb_data = cb_data;
	data->uri = NULL;
}

void gtkhash_hash_file_deinit(struct hash_file_s *data)
{
	g_assert(data->priv.source == 0);

	g_mutex_free(data->priv.mutex);
}

void gtkhash_hash_file_clear_digests(struct hash_file_s *data)
{
	for (int i = 0; i < HASH_FUNCS_N; i++)
		gtkhash_hash_func_set_digest(&data->funcs[i], NULL);
}
