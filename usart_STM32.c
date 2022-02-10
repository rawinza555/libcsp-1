#include <csp/drivers/usart.h>

#include <csp/csp_debug.h>
#include <cmsis_os.h>

#include <csp/csp.h>


typedef struct {
	csp_usart_callback_t rx_callback;
	void * user_data;
	UART_HandleTypeDef uartHandle;
	osThreadId_t rx_thread;
} usart_context_t;

const osMutexAttr_t mutexAttr= {.name = "uartMutex"};
/* Linux is fast, so we keep it simple by having a single lock */
osMutexId_t lock = osMutexNew(&mutexAttr);

void csp_usart_lock(void * driver_data) {
	osMutexAcquire(&lock);
}

void csp_usart_unlock(void * driver_data) {
	osMutexRelease(&lock);
}

static void * usart_rx_thread(void * arg) {

	usart_context_t * ctx = arg;
	const unsigned int CBUF_SIZE = 400;
	uint8_t * cbuf = malloc(CBUF_SIZE);

	// Receive loop
	while (1) {
		HAL_StatusTypeDef status = HAL_UART_Receive(ctx->uartHandle, cbuf, CBUF_SIZE, 500);
		ctx->rx_callback(ctx->user_data, cbuf, length, NULL);
	}
	return NULL;
}

int csp_usart_write(UART_HandleTypeDef uartHandle, const void * data, size_t data_length) {

	HAL_StatusTypeDef res = write(fd, data, data_length);
	if (res == HAL_OK) {
			return HAL_OK;
	}
	return CSP_ERR_TX;  // best matching CSP error code.
}

int csp_usart_open(const csp_usart_conf_t * conf, csp_usart_callback_t rx_callback, void * user_data, csp_usart_fd_t * return_fd) {


	usart_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}
	ctx->rx_callback = rx_callback;
	ctx->user_data = user_data;
	ctx->fd = fd;

	if (rx_callback) {
		osThreadId_t handle;
		const osThreadAttr_t rxTask_attributes = {
  .name = "rxThread",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

	/* no need to join with thread to free its resources */

		handle = osThreadNew(&handle, &rxTask_attributes, usart_rx_thread, NULL);

	if (handle == NULL) {
		return CSP_ERR_NOMEM;
	}
	}


	return CSP_ERR_NONE;
}
