

#include <csp/arch/csp_semaphore.h>
#include <csp/csp.h>
#include <csp/csp_debug.h>

int csp_mutex_create(csp_mutex_t * mutex) {
	csp_log_lock("Mutex init: %p", mutex);
	if (pthread_mutex_init(mutex, NULL) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

void csp_mutex_create_static(csp_mutex_t * handle, csp_mutex_buffer_t * buffer) {
	csp_mutex_create(handle);
}

int csp_mutex_remove(csp_mutex_t * mutex) {
	if (pthread_mutex_destroy(mutex) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_mutex_lock(csp_mutex_t * mutex, uint32_t timeout) {

	int ret;

	csp_log_lock("Wait: %p timeout %" PRIu32, mutex, timeout);

	if (timeout == CSP_MAX_TIMEOUT) {
		ret = pthread_mutex_lock(mutex);
	} else {
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			return CSP_SEMAPHORE_ERROR;
		}

		uint32_t sec = timeout / 1000;
		uint32_t nsec = (timeout - 1000 * sec) * 1000000;

		ts.tv_sec += sec;

		if (ts.tv_nsec + nsec >= 1000000000) {
			ts.tv_sec++;
		}

		ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

		ret = pthread_mutex_timedlock(mutex, &ts);
	}

	if (ret != 0)
		return CSP_SEMAPHORE_ERROR;

	return CSP_SEMAPHORE_OK;
}

int csp_mutex_unlock(csp_mutex_t * mutex) {
	if (pthread_mutex_unlock(mutex) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_create(csp_bin_sem_handle_t * sem) {
	csp_log_lock("Semaphore init: %p", sem);
	if (sem_init(sem, 0, 1) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

void csp_bin_sem_create_static(csp_bin_sem_handle_t * handle, csp_bin_sem_t * buffer) {
	csp_log_lock("Semaphore init: %p", handle);

	/* Ignore static allocation for now on posix */
	sem_init(handle, 0, 1);
}

int csp_bin_sem_remove(csp_bin_sem_handle_t * sem) {
	if (sem_destroy(sem) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}

int csp_bin_sem_wait(csp_bin_sem_handle_t * sem, uint32_t timeout) {

	int ret;

	csp_log_lock("Wait: %p timeout %" PRIu32, sem, timeout);

	if (timeout == CSP_MAX_TIMEOUT) {
		ret = sem_wait(sem);
	} else {
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts)) {
			return CSP_SEMAPHORE_ERROR;
		}

		uint32_t sec = timeout / 1000;
		uint32_t nsec = (timeout - 1000 * sec) * 1000000;

		ts.tv_sec += sec;

		if (ts.tv_nsec + nsec >= 1000000000) {
			ts.tv_sec++;
		}

		ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

		ret = sem_timedwait(sem, &ts);
	}

	if (ret != 0)
		return CSP_SEMAPHORE_ERROR;

	return CSP_SEMAPHORE_OK;
}

int csp_bin_sem_post_isr(csp_bin_sem_handle_t * sem, int * task_woken) {
	if (task_woken) {
		*task_woken = 0;
	}
	return csp_bin_sem_post(sem);
}

int csp_bin_sem_post(csp_bin_sem_handle_t * sem) {
	csp_log_lock("Post: %p", sem);

	int value;
	sem_getvalue(sem, &value);
	if (value > 0) {
		return CSP_SEMAPHORE_OK;
	}

	if (sem_post(sem) == 0) {
		return CSP_SEMAPHORE_OK;
	}

	return CSP_SEMAPHORE_ERROR;
}
