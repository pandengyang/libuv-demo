#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "uv.h"

/* netcat 127.0.0.1 9000 */

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

static uv_loop_t *loop;

static void alloc_buf(uv_handle_t * handle, size_t suggested_size,
		      uv_buf_t * buf);
static void echo(uv_stream_t * stream, ssize_t nread,
		 const uv_buf_t * buf);

static void on_connection(uv_stream_t * server, int status);
static void on_echo(uv_write_t * req, int status);

static void free_write_req(uv_write_t * req);
static void write_data(uv_stream_t * dest, size_t size, uv_buf_t buf,
		       uv_write_cb cb);

int main(int argc, char **argv)
{
	int res;
	struct sockaddr_in addr;
	uv_tcp_t server;

	loop = uv_default_loop();

	/* int uv_tcp_init(uv_loop_t *loop,
	 *                 uv_tcp_t *handle);
	 *
	 * 将 TCP Server 与 loop 关联起来
	 */
	uv_tcp_init(loop, &server);

	/* int uv_ip4_addr(const char *ip,
	 *                 int port,
	 *                 struct sockaddr_in *addr);
	 *
	 * 将 ip & port 转换为 sockaddr_in 结构
	 */
	uv_ip4_addr("0.0.0.0", 9000, &addr);
	/*  int uv_tcp_bind(uv_tcp_t *handle,
	 *                  const struct sockaddr *addr,
	 *                  unsigned int flags);
	 */
	uv_tcp_bind(&server, (const struct sockaddr *) &addr, 0);

	/* int uv_listen(uv_stream_t* stream,
	 *               int backlog, 积压
	 *               uv_connection_cb cb);
	 */
	if (res = uv_listen((uv_stream_t *) & server, 128, on_connection)) {
		fprintf(stderr, "Listen error %s\n", uv_strerror(res));
		return -1;
	}

	uv_run(loop, UV_RUN_DEFAULT);

	uv_loop_close(loop);

	return 0;
}

static void on_connection(uv_stream_t * server, int status)
{
	uv_tcp_t *client;

	if (status < 0) {
		fprintf(stderr, "connection error %s\n",
			uv_strerror(status));

		return;
	}

	client = (uv_tcp_t *) malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);

	if (0 == uv_accept(server, (uv_stream_t *) client)) {
		uv_read_start((uv_stream_t *) client, alloc_buf, echo);

		return;
	}

	uv_close((uv_handle_t *) client, NULL);

	return;
}

static void alloc_buf(uv_handle_t * handle, size_t suggested_size,
		      uv_buf_t * buf)
{
	*buf =
	    uv_buf_init((char *) malloc(suggested_size), suggested_size);

	return;
}

static void echo(uv_stream_t * stream, ssize_t nread, const uv_buf_t * buf)
{
	if (nread < 0) {
		/* nread < 0 表示错误发生了
		 * 其中一种错误表示读到文件尾 UV_EOF
		 */
		if (UV_EOF == nread) {
			/* void uv_close(uv_handle_t* handle,
			 *               uv_close_cb close_cb);
			 */
			uv_close((uv_handle_t *) stream, NULL);
		}
	} else if (nread > 0) {
		write_data(stream, nread, *buf, on_echo);
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

static void on_echo(uv_write_t * req, int status)
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
