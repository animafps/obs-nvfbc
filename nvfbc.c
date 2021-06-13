/*
 * obs-nvfbc. OBS Studio source plugin.
 *
 * Copyright (C) 2019 Florian Zwoch <fzwoch@gmail.com>
 *
 * This file is part of obs-nvfbc.
 *
 * obs-nvfbc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * obs-nvfbc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with obs-nvfbc. If not, see <http://www.gnu.org/licenses/>.
 */

#include <obs/obs-module.h>
#include <obs/util/threading.h>

#include "NvFBC.h"
#include <dlfcn.h>

OBS_DECLARE_MODULE()

static void *nvfbc_lib = NULL;

static NVFBC_API_FUNCTION_LIST nvFBC = {
	.dwVersion = NVFBC_VERSION
};

typedef struct {
	obs_source_t *source;
	obs_data_t *settings;
	pthread_t thread;
	volatile bool thread_shutdown;
	bool thread_is_running;
	int64_t ts_offset;
} data_t;

static const char* get_name(void *type_data)
{
	return "NvFBC Source";
}

static void nvfbc_get_status(NVFBC_GET_STATUS_PARAMS *status_params)
{
	NVFBC_SESSION_HANDLE session;

	NVFBC_CREATE_HANDLE_PARAMS params = {
		.dwVersion = NVFBC_CREATE_HANDLE_PARAMS_VER
	};

	NVFBCSTATUS ret = nvFBC.nvFBCCreateHandle(&session, &params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
	}

	ret = nvFBC.nvFBCGetStatus(session, status_params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
	}

	NVFBC_DESTROY_HANDLE_PARAMS destroy_params = {
		.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER
	};

	ret = nvFBC.nvFBCDestroyHandle(session, &destroy_params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
	}
}

static void* capture_thread(void *p)
{
	data_t *data = p;
	void *frame_buffer;
	bool use_timestamps = obs_data_get_bool(data->settings, "use_timestamps");

	NVFBC_SESSION_HANDLE session;

	NVFBC_CREATE_HANDLE_PARAMS params = {
		.dwVersion = NVFBC_CREATE_HANDLE_PARAMS_VER
	};

	NVFBCSTATUS ret = nvFBC.nvFBCCreateHandle(&session, &params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
		goto bail;
	}

	NVFBC_CREATE_CAPTURE_SESSION_PARAMS cap_params = {
		.dwVersion = NVFBC_CREATE_CAPTURE_SESSION_PARAMS_VER,
		.eTrackingType = NVFBC_TRACKING_OUTPUT,
		.eCaptureType = NVFBC_CAPTURE_TO_SYS,
		.bWithCursor = obs_data_get_bool(data->settings, "show_cursor") ? NVFBC_TRUE : NVFBC_FALSE,
		.dwSamplingRateMs = 1000.0 / obs_data_get_int(data->settings, "fps") + 0.5,
		.dwOutputId = obs_data_get_int(data->settings, "screen")
	};

	ret = nvFBC.nvFBCCreateCaptureSession(session, &cap_params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
		goto bail;
	}

	NVFBC_TOSYS_SETUP_PARAMS setup_params = {
		.dwVersion = NVFBC_TOSYS_SETUP_PARAMS_VER,
		.eBufferFormat = NVFBC_BUFFER_FORMAT_BGRA,
		.ppBuffer = &frame_buffer,
	};

	ret = nvFBC.nvFBCToSysSetUp(session, &setup_params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
		goto bail;
	}

	NVFBC_FRAME_GRAB_INFO frame_info;

	NVFBC_TOSYS_GRAB_FRAME_PARAMS grab_params = {
		.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER,
		.dwFlags = NVFBC_TOSYS_GRAB_FLAGS_NOFLAGS,
		.pFrameGrabInfo = &frame_info,
	};

	data->ts_offset = INT64_MIN;

	while (data->thread_shutdown == false) {
		NVFBCSTATUS ret = nvFBC.nvFBCToSysGrabFrame(session, &grab_params);
		if (ret != NVFBC_SUCCESS) {
			blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
			goto bail;
		}

		struct obs_source_frame frame = {
			.width = frame_info.dwWidth,
			.height = frame_info.dwHeight,
			.format = VIDEO_FORMAT_BGRX,
			.full_range = true,
			// Basically just a frame counter. Actual TS would be (* 1000).
			// But this would just increase some latency on the compositing layer(?).
			// Since this is a live signal it is probably best to render as fast as
			// it is received.
			.timestamp = frame_info.ulTimestampUs,
			.linesize[0] = frame_info.dwWidth * 4,
			.data[0] = frame_buffer,
		};

		// If timestamp option is set carry on the API's timestamp.
		// Handle initial offset and convert to nanoseconds.
		if (use_timestamps) {
			if (data->ts_offset == INT64_MIN) {
				data->ts_offset = frame.timestamp;
			}

			frame.timestamp = (data->ts_offset - frame.timestamp) * 1000;
		}

		obs_source_output_video(data->source, &frame);
	}

bail:;
	NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroy_cap_params = {
		.dwVersion = NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER
	};

	ret = nvFBC.nvFBCDestroyCaptureSession(session, &destroy_cap_params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
	}

	NVFBC_DESTROY_HANDLE_PARAMS destroy_params = {
		.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER
	};

	ret = nvFBC.nvFBCDestroyHandle(session, &destroy_params);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", nvFBC.nvFBCGetLastErrorStr(session));
	}

	return NULL;
}

static void* create(obs_data_t *settings, obs_source_t *source)
{
	data_t *data = malloc(sizeof(data_t));

	memset(data, 0, sizeof(data_t));

	data->source = source;
	data->settings = settings;

	return data;
}

static void destroy(void *p)
{
	data_t *data = p;

	data->thread_shutdown = true;
	pthread_join(data->thread, NULL);
	data->thread_is_running = false;

	free(data);
}

static void get_defaults(obs_data_t *settings)
{
	NVFBC_GET_STATUS_PARAMS status_params = {
		.dwVersion = NVFBC_GET_STATUS_PARAMS_VER
	};

	nvfbc_get_status(&status_params);

	obs_data_set_default_int(settings, "screen", status_params.outputs[0].dwId);
	obs_data_set_default_int(settings, "fps", 60);
	obs_data_set_default_bool(settings, "show_cursor", true);
	obs_data_set_default_bool(settings, "use_timestamps", false);
}

static obs_properties_t* get_properties(void *data)
{
	obs_properties_t *props = obs_properties_create();

	NVFBC_GET_STATUS_PARAMS status_params = {
		.dwVersion = NVFBC_GET_STATUS_PARAMS_VER
	};

	nvfbc_get_status(&status_params);

	obs_property_t *prop = obs_properties_add_list(props, "screen", "Screen", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	for (int i = 0; i < status_params.dwOutputNum; i++) {
		obs_property_list_add_int(prop, status_params.outputs[i].name, status_params.outputs[i].dwId);
	}

	obs_properties_add_int(props, "fps", "FPS", 1, 120, 1);
	obs_properties_add_bool(props, "show_cursor", "Cursor");
	obs_properties_add_bool(props, "use_timestamps", "Use Timestamps (smoother but increases latency)");

	return props;
}

static void show(void *p)
{
	data_t *data = p;

	if (data->thread_is_running == true) {
		return;
	}

	data->thread_shutdown = false;
	pthread_create(&data->thread, NULL, capture_thread, data);
	data->thread_is_running = true;
}

static void hide(void *p)
{
	data_t *data = p;

	if (data->thread_is_running == false) {
		return;
	}

	data->thread_shutdown = true;
	pthread_join(data->thread, NULL);
	data->thread_is_running = false;
}

static void update(void *p, obs_data_t *settings)
{
	data_t *data = p;

	// prevent starting the thread if it is not running
	if (data->thread_is_running == false) {
		return;
	}

	hide(data);
	show(data);
}

struct obs_source_info nvfbc_source = {
	.id = "nvfbc-source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE,

	.get_name = get_name,
	.create = create,
	.destroy = destroy,

	.get_defaults = get_defaults,
	.get_properties = get_properties,
	.show = show,
	.hide = hide,
	.update = update,
};

bool obs_module_load(void)
{
	PNVFBCCREATEINSTANCE NvFBCCreateInstance = NULL;

	nvfbc_lib = dlopen("libnvidia-fbc.so.1", RTLD_NOW);
	if (nvfbc_lib == NULL) {
		blog(LOG_ERROR, "%s", "Unable to load NvFBC library");
		return false;
	}

	NvFBCCreateInstance = (PNVFBCCREATEINSTANCE)dlsym(nvfbc_lib, "NvFBCCreateInstance");
	if (NvFBCCreateInstance == NULL) {
		blog(LOG_ERROR, "%s", "Unable to find NvFBCCreateInstance symbol in NvFBC library");
		return false;
	}

	NVFBCSTATUS ret = NvFBCCreateInstance(&nvFBC);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "%s", "Unable to create NvFBC instance");
		return false;
	}

	obs_register_source(&nvfbc_source);

	return true;
}

void obs_module_unload(void)
{
	if (nvfbc_lib != NULL) {
		dlclose(nvfbc_lib);
	}
}
