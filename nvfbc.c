#include <obs/obs-module.h>
#include "NvFBC.h"

#if defined __linux__
#include <dlfcn.h>
#endif

OBS_DECLARE_MODULE()

static NVFBC_API_FUNCTION_LIST nvFBC = {};

typedef struct {
	obs_source_t* source;
	obs_data_t* settings;
	NVFBC_SESSION_HANDLE session;
	NVFBC_FRAME_GRAB_INFO frame_info;
	void *frame;
} data_t;

static const char* get_name(void* type_data)
{
	return "NvFBC Source";
}

static void* create(obs_data_t* settings, obs_source_t* source)
{
	data_t* data = malloc(sizeof(data_t));

	memset(data, 0, sizeof(data_t));

	data->source = source;
	data->settings = settings;

	NVFBC_CREATE_HANDLE_PARAMS params = {};

	params.dwVersion = NVFBC_CREATE_HANDLE_PARAMS_VER;

	NVFBCSTATUS ret = nvFBC.nvFBCCreateHandle(&data->session, &params);
	if (ret != NVFBC_SUCCESS)
	{
		blog(LOG_ERROR, nvFBC.nvFBCGetLastErrorStr(data->session));
	}

	NVFBC_CREATE_CAPTURE_SESSION_PARAMS cap_params = {};
	cap_params.dwVersion = NVFBC_CREATE_CAPTURE_SESSION_PARAMS_VER;
	cap_params.eCaptureType = NVFBC_CAPTURE_TO_SYS;
	cap_params.bWithCursor = NVFBC_TRUE;
	cap_params.eTrackingType = NVFBC_TRACKING_SCREEN;

	ret = nvFBC.nvFBCCreateCaptureSession(data->session, &cap_params);
	if (ret != NVFBC_SUCCESS)
	{
		blog(LOG_ERROR, nvFBC.nvFBCGetLastErrorStr(data->session));
	}

	NVFBC_TOSYS_SETUP_PARAMS setup_params = {};
	setup_params.dwVersion = NVFBC_TOSYS_SETUP_PARAMS_VER;
	setup_params.eBufferFormat = NVFBC_BUFFER_FORMAT_RGB;
	setup_params.ppBuffer = &data->frame;
	setup_params.bWithDiffMap = NVFBC_FALSE;

	ret = nvFBC.nvFBCToSysSetUp(data->session, &setup_params);
	if (ret != NVFBC_SUCCESS)
	{
		blog(LOG_ERROR, nvFBC.nvFBCGetLastErrorStr(data->session));
	}

	return data;
}

static void destroy(void* p)
{
	data_t *data = p;

	NVFBC_DESTROY_CAPTURE_SESSION_PARAMS destroy_cap_params = {};

	destroy_cap_params.dwVersion = NVFBC_DESTROY_CAPTURE_SESSION_PARAMS_VER;

	NVFBCSTATUS ret = nvFBC.nvFBCDestroyCaptureSession(data->session, &destroy_cap_params);
	if (ret != NVFBC_SUCCESS)
	{
		blog(LOG_ERROR, nvFBC.nvFBCGetLastErrorStr(data->session));
	}

	NVFBC_DESTROY_HANDLE_PARAMS destroy_params = {};
	destroy_params.dwVersion = NVFBC_DESTROY_HANDLE_PARAMS_VER;

	ret = nvFBC.nvFBCDestroyHandle(data->session, &destroy_params);
	if (ret != NVFBC_SUCCESS)
	{
		blog(LOG_ERROR, nvFBC.nvFBCGetLastErrorStr(data->session));
	}

	free(data);
}

static void video_render(void *p, gs_effect_t *effect)
{
	data_t *data = p;

	NVFBC_TOSYS_GRAB_FRAME_PARAMS params = {};

	params.dwVersion = NVFBC_TOSYS_GRAB_FRAME_PARAMS_VER;
	params.dwFlags = NVFBC_TOSYS_GRAB_FLAGS_NOFLAGS;
	params.pFrameGrabInfo = &data->frame_info;

	NVFBCSTATUS ret = nvFBC.nvFBCToSysGrabFrame(data->session, &params);
	if (ret != NVFBC_SUCCESS)
	{
		blog(LOG_ERROR, nvFBC.nvFBCGetLastErrorStr(data->session));
	}
}

static uint32_t get_width(void *data)
{
	return ((data_t*)data)->frame_info.dwWidth;
}

static uint32_t get_height(void *data)
{
	return ((data_t*)data)->frame_info.dwHeight;
}

bool obs_module_load(void)
{
	PNVFBCCREATEINSTANCE NvFBCCreateInstance = NULL;

#if defined __linux__
	void *dll = dlopen("libnvidia-fbc.so.1", RTLD_NOW);
	if (dll == NULL)
	{
		blog(LOG_ERROR, "Unable to load NvFCB library");
		return false;
	}

	NvFBCCreateInstance = (PNVFBCCREATEINSTANCE)dlsym(dll, "NvFBCCreateInstance");
#endif

	if (NvFBCCreateInstance == NULL)
	{
		blog(LOG_ERROR, "Unable to find NvFBCCreateInstance symbol in NvFCB library");
		return false;
	}

	nvFBC.dwVersion = NVFBC_VERSION;

	NVFBCSTATUS ret = NvFBCCreateInstance(&nvFBC);
	if (ret != NVFBC_SUCCESS) {
		blog(LOG_ERROR, "Unable to create NvFBC instance");
		return false;
	}

	struct obs_source_info info = {
		.id = "nvfbc-source",
		.type = OBS_SOURCE_TYPE_INPUT,
		.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_DO_NOT_DUPLICATE,

		.get_name = get_name,
		.create = create,
		.destroy = destroy,
		.video_render = video_render,
		.get_width = get_width,
		.get_height = get_height,

//		.get_defaults = get_defaults,
//		.get_properties = get_properties,
//		.update = update,
//		.show = show,
//		.hide = hide,
	};

	obs_register_source(&info);

	return true;
}
