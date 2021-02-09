#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

static uv_loop_t *loop;

static uv_pipe_t pipe_stdin;
static uv_pipe_t pipe_stdout;
static uv_pipe_t pipe_file;

static void on_stdout_write(uv_write_t * req, int status);
static void on_file_write(uv_write_t * req, int status);
static void on_stdin_read(uv_stream_t * stream, ssize_t nread,
			  const uv_buf_t * buf);

static void alloc_buf(uv_handle_t * handle, size_t suggested_size,
		      uv_buf_t * buf);
static void free_write_req(uv_write_t * req);
static void write_data(uv_stream_t * dest, size_t size, uv_buf_t buf,
		       uv_write_cb cb);

int main(int argc, char **argv)
{
	uv_fs_t req_file;

	loop = uv_default_loop();

	/* int uv_pipe_init(uv_loop_t *loop,
	 *                  uv_pipe_t *handle,
	 *                  int ipc); 匿名管道为 0，具名管道为 1
	 *
	 * 将管道与 loop 关联起来
	 */
	uv_pipe_init(loop, &pipe_stdin, 0);
	/* int uv_pipe_open(uv_pipe_t *handle,
	 *                  uv_os_fd_t file);
	 *
	 * 将管道与文件关联起来
	 */
	uv_pipe_open(&pipe_stdin, 0);

	uv_pipe_init(loop, &pipe_stdout, 0);
	uv_pipe_open(&pipe_stdout, 1);

	uv_fs_open(loop, &req_file, argv[1], O_CREAT | O_RDWR, 0644, NULL);
	uv_pipe_init(loop, &pipe_file, 0);
	uv_pipe_open(&pipe_file, req_file.result);

	/* int uv_read_start(uv_stream_t *stream,
	 *                   uv_alloc_cb alloc_cb, 分配缓存的函数
	 *                   uv_read_cb read_cb);
	 *
	 * 分配的缓存会在 read_cb 中释放掉
	 */
	uv_read_start((uv_stream_t *) & pipe_stdin, alloc_buf,
		      on_stdin_read);

	uv_run(loop, UV_RUN_DEFAULT);

	uv_loop_close(loop);

	uv_fs_req_cleanup(&req_file);

	return 0;
}

static void alloc_buf(uv_handle_t * handle, size_t suggested_size,
		      uv_buf_t * buf)
{
	*buf =
	    uv_buf_init((char *) malloc(suggested_size), suggested_size);
}

static void on_stdin_read(uv_stream_t * stream, ssize_t nread,
			  const uv_buf_t * buf)
{
	if (nread < 0) {
		/* nread < 0 表示错误发生了
		 * 其中一种错误表示读到文件尾 UV_EOF
		 */
		if (UV_EOF == nread) {
			/* void uv_close(uv_handle_t* handle,
			 *               uv_close_cb close_cb);
			 */
			uv_close((uv_handle_t *) & pipe_stdin, NULL);
			uv_close((uv_handle_t *) & pipe_stdout, NULL);
			uv_close((uv_handle_t *) & pipe_file, NULL);
		}
	} else if (nread > 0) {
		write_data((uv_stream_t *) & pipe_stdout, nread, *buf,
			   on_stdout_write);
		write_data((uv_stream_t *) & pipe_file, nread, *buf,
			   on_file_write);
	} else if (nread == 0) {
		/* nread == 0 表示没有数据了 */
	}

	/* 释放 alloc_buf 分配的缓存 */
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

static void on_stdout_write(uv_write_t * req, int status)
{
	free_write_req(req);
}

static void on_file_write(uv_write_t * req, int status)
{
	free_write_req(req);
}

static void write_data(uv_stream_t * dest, size_t size, uv_buf_t buf,
		       uv_write_cb cb)
{
	/* 每次写完毕，需要释放安全释放 req */
	write_req_t *req;

	if (NULL == (req = (write_req_t *) malloc(sizeof(write_req_t)))) {
		return;
	}

	/* 将数据写入请求中 */
	req->buf = uv_buf_init((char *) malloc(size), size);
	memcpy(req->buf.base, buf.base, size);

	/* 将请求中的数据写入 stream 中 */
	uv_write((uv_write_t *) req, (uv_stream_t *) dest, &req->buf, 1,
		 cb);
}
