#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

static uv_loop_t *loop;

static uv_pipe_t pipe_sp;

static void on_sp_write(uv_write_t * req, int status);
static void on_sp_read(uv_stream_t * stream, ssize_t nread,
			  const uv_buf_t * buf);

static void alloc_buf(uv_handle_t * handle, size_t suggested_size,
		      uv_buf_t * buf);
static void free_write_req(uv_write_t * req);
static void write_data(uv_stream_t * dest, size_t size, uv_buf_t buf,
		       uv_write_cb cb);

int main(int argc, char **argv)
{
	uv_fs_t req_sp;

	loop = uv_default_loop();

	uv_fs_open(loop, &req_sp, argv[1], O_RDWR, 0644, NULL);
	uv_pipe_init(loop, &pipe_sp, 0);
	uv_pipe_open(&pipe_sp, req_sp.result);

	uv_read_start((uv_stream_t *) & pipe_sp, alloc_buf,
		      on_sp_read);

	uv_run(loop, UV_RUN_DEFAULT);

	uv_loop_close(loop);

	uv_fs_req_cleanup(&req_sp);

	return 0;
}

static void alloc_buf(uv_handle_t * handle, size_t suggested_size,
		      uv_buf_t * buf)
{
	*buf =
	    uv_buf_init((char *) malloc(suggested_size), suggested_size);
}

static void on_sp_read(uv_stream_t * stream, ssize_t nread,
			  const uv_buf_t * buf)
{
	printf("on_sp_read: %ld\n", nread);
	if (nread < 0) {
		if (UV_EOF == nread) {
			uv_close((uv_handle_t *) & pipe_sp, NULL);
		}
	} else if (nread > 0) {
		write_data(stream, nread, *buf,
			   on_sp_write);
	} else if (nread == 0) {
	}

	if (buf->base) {
		free(buf->base);
	}
}

static void free_write_req(uv_write_t * req)
{
	write_req_t *wr = (write_req_t *) req;

	free(wr->buf.base);
	free(wr);
}

static void on_sp_write(uv_write_t * req, int status)
{
	free_write_req(req);
}

static void write_data(uv_stream_t * dest, size_t size, uv_buf_t buf,
		       uv_write_cb cb)
{
	write_req_t *req;

	if (NULL == (req = (write_req_t *) malloc(sizeof(write_req_t)))) {
		return;
	}

	req->buf = uv_buf_init((char *) malloc(size), size);
	memcpy(req->buf.base, buf.base, size);

	printf("uv_write\n");
	uv_write((uv_write_t *) req, (uv_stream_t *) dest, &req->buf, 1,
		 cb);
}
