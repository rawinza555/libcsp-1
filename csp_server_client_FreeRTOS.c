#include <csp/csp.h>
#include <csp/csp_debug.h>
#include <cmsis_os.h>

void server(void);
void client(void);

static int csp_thread_create(void * (*routine)(void *),osThreadAttr_t* attribute) {

	osThreadId_t handle;

	/* no need to join with thread to free its resources */

	handle = osThreadNew(&handle, &attributes, routine, NULL);

	if (handle == NULL) {
		return CSP_ERR_NOMEM;
	}

	return CSP_ERR_NONE;
}

static void * task_router(void * param) {

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	return NULL;
}

static void * task_server(void * param) {
	server();
	return NULL;
}

static void * task_client(void * param) {
	client();
	return NULL;
}

int router_start(void) {.
	const osThreadAttr_t routerTask_attributes = {
  .name = "routerTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
	return csp_thread_create(task_router,routerTask_attributes);
}

int server_start(void) {
	const osThreadAttr_t serverTask_attributes = {
  .name = "serverTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
	return csp_thread_create(task_server,serverTask_attributes);
}

int client_start(void) {
	const osThreadAttr_t clientTask_attributes = {
  .name = "clientTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
	return csp_thread_create(task_client,clientTask_attributes);
}
