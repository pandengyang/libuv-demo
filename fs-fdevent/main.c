#include <stdio.h>
#include <stdlib.h>

#include "uv.h"

static uv_loop_t *loop;

static uv_fs_t req_open;
static uv_fs_t req_read;
static uv_fs_t req_write;

static void on_open(uv_fs_t * req);
static void on_read(uv_fs_t * req);
static void on_write(uv_fs_t * req);

static char buf[8];
static uv_buf_t iov;

int main(int argc, char **argv)
{
	loop = uv_default_loop();

	/* int uv_fs_open(uv_loop_t *loop,
	 *                uv_fs_t *req, req->resutl 保存了打开的文件描述符，req 同时也是 cb 的参数
	 *                const char *path,
	 *                int flags,
	 *                int mode, mode 与 flags 同 UNIX 系统
	 *                uv_fs_cb cb); void callback(uv_fs_t* req)
	 */
	uv_fs_open(loop, &req_open, argv[1], O_RDONLY, 0, on_open);

	uv_run(loop, UV_RUN_DEFAULT);

	uv_loop_close(loop);

	uv_fs_req_cleanup(&req_open);
	uv_fs_req_cleanup(&req_read);
	uv_fs_req_cleanup(&req_write);

	return 0;
}

static void on_open(uv_fs_t * req)
{
	if (req->result < 0) {
		fprintf(stderr, "error opening file: %s\n",
			uv_strerror((int) req->result));
		return;
	}

	iov = uv_buf_init(buf, sizeof buf);

	/* int uv_fs_read(uv_loop_t* loop,
	 *                uv_fs_t* req, req->result 保存了读取的字节数
	 *                uv_file file,
	 *                const uv_buf_t bufs[],
	 *                unsigned int nbufs,
	 *                int64_t offset,
	 *                uv_fs_cb cb);
	 */
	uv_fs_read(loop, &req_read, req->result, &iov, 1, -1, on_read);
}

static void on_read(uv_fs_t * req)
{
	if (req->result < 0) {
		fprintf(stderr, "Read error: %s\n",
			uv_strerror((int) req->result));
		return;
	}

	/* 0 表示读取到 EOF */
	if (req->result == 0) {
		/* 因为 uv_fs_close 同步调用，所以 req_close 可为栈变量 */
		uv_fs_t req_close;

		/* int uv_fs_close(uv_loop_t* loop,
		 *                 uv_fs_t* req,
		 *                 uv_file file,
		 *                 uv_fs_cb cb) cb 为 NULL，表示同步调用
		 */
		uv_fs_close(loop, &req_close, req_open.result, NULL);

		return;
	}

	iov.len = req->result;
	/* int uv_fs_write(uv_loop_t* loop,
	 *                 uv_fs_t* req, req->result 为写的字节数
	 *                 uv_file file,
	 *                 const uv_buf_t bufs[],
	 *                 unsigned int nbufs,
	 *                 int64_t offset,
	 *                 uv_fs_cb cb);
	 */
	uv_fs_write(loop, &req_write, 1, &iov, 1, -1, on_write);
}

static void on_write(uv_fs_t * req)
{
	if (req->result < 0) {
		fprintf(stderr, "read error: %s\n",
			uv_strerror((int) req->result));
		return;
	}

	uv_fs_read(loop, &req_read, req_open.result, &iov, 1, -1, on_read);
}
