/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015-2016 Philippe Proulx <pproulx@efficios.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *
 * The following C code was generated by barectf 2.2.1
 * on 2017-01-17T13:54:01.241881.
 *
 * For more details, see <http://barectf.org>.
 */

#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "barectf.h"

#define _ALIGN(_at, _align)					\
	do {							\
		(_at) = ((_at) + ((_align) - 1)) & -(_align);	\
	} while (0)

#ifdef __cplusplus
# define TO_VOID_PTR(_value)		static_cast<void *>(_value)
# define FROM_VOID_PTR(_type, _value)	static_cast<_type *>(_value)
#else
# define TO_VOID_PTR(_value)		((void *) (_value))
# define FROM_VOID_PTR(_type, _value)	((_type *) (_value))
#endif

#define _BITS_TO_BYTES(_x)	((_x) >> 3)
#define _BYTES_TO_BITS(_x)	((_x) << 3)

union f2u {
	float f;
	uint32_t u;
};

union d2u {
	double f;
	uint64_t u;
};

uint32_t barectf_packet_size(void *ctx)
{
	return FROM_VOID_PTR(struct barectf_ctx, ctx)->packet_size;
}

int barectf_packet_is_full(void *ctx)
{
	struct barectf_ctx *cctx = FROM_VOID_PTR(struct barectf_ctx, ctx);

	return cctx->at == cctx->packet_size;
}

int barectf_packet_is_empty(void *ctx)
{
	struct barectf_ctx *cctx = FROM_VOID_PTR(struct barectf_ctx, ctx);

	return cctx->at <= cctx->off_content;
}

uint32_t barectf_packet_events_discarded(void *ctx)
{
	return FROM_VOID_PTR(struct barectf_ctx, ctx)->events_discarded;
}

uint8_t *barectf_packet_buf(void *ctx)
{
	return FROM_VOID_PTR(struct barectf_ctx, ctx)->buf;
}

uint32_t barectf_packet_buf_size(void *ctx)
{
	struct barectf_ctx *cctx = FROM_VOID_PTR(struct barectf_ctx, ctx);

	return _BITS_TO_BYTES(cctx->packet_size);
}

void barectf_packet_set_buf(void *ctx, uint8_t *buf, uint32_t buf_size)
{
	struct barectf_ctx *cctx = FROM_VOID_PTR(struct barectf_ctx, ctx);

	cctx->buf = buf;
	cctx->packet_size = _BYTES_TO_BITS(buf_size);
}

int barectf_packet_is_open(void *ctx)
{
	return FROM_VOID_PTR(struct barectf_ctx, ctx)->packet_is_open;
}

static
void _write_cstring(struct barectf_ctx *ctx, const char *src)
{
	uint32_t sz = strlen(src) + 1;

	memcpy(&ctx->buf[_BITS_TO_BYTES(ctx->at)], src, sz);
	ctx->at += _BYTES_TO_BITS(sz);
}

static
int _reserve_event_space(void *vctx, uint32_t ev_size)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);

	/* event _cannot_ fit? */
	if (ev_size > (ctx->packet_size - ctx->off_content)) {
		ctx->events_discarded++;

		return 0;
	}

	/* packet is full? */
	if (barectf_packet_is_full(ctx)) {
		/* yes: is back-end full? */
		if (ctx->cbs.is_backend_full(ctx->data)) {
			/* yes: discard event */
			ctx->events_discarded++;

			return 0;
		}

		/* back-end is not full: open new packet */
		ctx->cbs.open_packet(ctx->data);
	}

	/* event fits the current packet? */
	if (ev_size > (ctx->packet_size - ctx->at)) {
		/* no: close packet now */
		ctx->cbs.close_packet(ctx->data);

		/* is back-end full? */
		if (ctx->cbs.is_backend_full(ctx->data)) {
			/* yes: discard event */
			ctx->events_discarded++;

			return 0;
		}

		/* back-end is not full: open new packet */
		ctx->cbs.open_packet(ctx->data);
		assert(ev_size <= (ctx->packet_size - ctx->at));
	}

	return 1;
}

static
void _commit_event(void *vctx)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);

	/* is packet full? */
	if (barectf_packet_is_full(ctx)) {
		/* yes: close it now */
		ctx->cbs.close_packet(ctx->data);
	}
}

/* initialize context */
void barectf_init(
	void *vctx,
	uint8_t *buf,
	uint32_t buf_size,
	struct barectf_platform_callbacks cbs,
	void *data
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	ctx->cbs = cbs;
	ctx->data = data;
	ctx->buf = buf;
	ctx->packet_size = _BYTES_TO_BITS(buf_size);
	ctx->at = 0;
	ctx->events_discarded = 0;
	ctx->packet_is_open = 0;
}

/* open packet for stream "default" */
void barectf_default_open_packet(
	struct barectf_default_ctx *ctx
)
{
	uint64_t ts = ctx->parent.cbs.default_clock_get_value(ctx->parent.data);

	/* do not open a packet that is already open */
	if (ctx->parent.packet_is_open) {
		return;
	}

	ctx->parent.at = 0;

	/* trace packet header */
	{
		/* align structure */
		_ALIGN(ctx->parent.at, 32);

		/* "magic" field */
		_ALIGN(ctx->parent.at, 32);
		barectf_bt_bitfield_write_le(&(&ctx->parent)->buf[_BITS_TO_BYTES((&ctx->parent)->at)], uint8_t, 0, 32, uint32_t, (uint32_t) 0xc1fc1fc1UL);
		(&ctx->parent)->at += 32;

		/* "uuid" field */
		{
			static uint8_t uuid[] = {
				79,
				68,
				36,
				206,
				220,
				230,
				17,
				230,
				146,
				193,
				184,
				39,
				235,
				60,
				74,
				250,
			};

			_ALIGN(ctx->parent.at, 8);
			memcpy(&ctx->parent.buf[_BITS_TO_BYTES(ctx->parent.at)], uuid, 16);
			ctx->parent.at += _BYTES_TO_BITS(16);
		}

		/* "stream_id" field */
		_ALIGN(ctx->parent.at, 8);
		barectf_bt_bitfield_write_le(&(&ctx->parent)->buf[_BITS_TO_BYTES((&ctx->parent)->at)], uint8_t, 0, 8, uint8_t, (uint8_t) (uint8_t) 0);
		(&ctx->parent)->at += 8;
	}

	/* stream packet context */
	{
		/* align structure */
		_ALIGN(ctx->parent.at, 64);

		/* "timestamp_begin" field */
		_ALIGN(ctx->parent.at, 64);
		barectf_bt_bitfield_write_le(&(&ctx->parent)->buf[_BITS_TO_BYTES((&ctx->parent)->at)], uint8_t, 0, 64, uint64_t, (uint64_t) (uint64_t) ts);
		(&ctx->parent)->at += 64;

		/* "timestamp_end" field */
		_ALIGN(ctx->parent.at, 64);
		ctx->off_spc_timestamp_end = ctx->parent.at;
		ctx->parent.at += 64;

		/* "packet_size" field */
		_ALIGN(ctx->parent.at, 32);
		barectf_bt_bitfield_write_le(&(&ctx->parent)->buf[_BITS_TO_BYTES((&ctx->parent)->at)], uint8_t, 0, 32, uint32_t, (uint32_t) (uint32_t) ctx->parent.packet_size);
		(&ctx->parent)->at += 32;

		/* "content_size" field */
		_ALIGN(ctx->parent.at, 32);
		ctx->off_spc_content_size = ctx->parent.at;
		ctx->parent.at += 32;

		/* "events_discarded" field */
		_ALIGN(ctx->parent.at, 32);
		ctx->off_spc_events_discarded = ctx->parent.at;
		ctx->parent.at += 32;
	}

	ctx->parent.off_content = ctx->parent.at;

	/* mark current packet as open */
	ctx->parent.packet_is_open = 1;
}

/* close packet for stream "default" */
void barectf_default_close_packet(struct barectf_default_ctx *ctx)
{
	uint64_t ts = ctx->parent.cbs.default_clock_get_value(ctx->parent.data);

	/* do not close a packet that is not open */
	if (!ctx->parent.packet_is_open) {
		return;
	}

	/* save content size */
	ctx->parent.content_size = ctx->parent.at;

	/* "timestamp_end" field */
	ctx->parent.at = ctx->off_spc_timestamp_end;
	barectf_bt_bitfield_write_le(&(&ctx->parent)->buf[_BITS_TO_BYTES((&ctx->parent)->at)], uint8_t, 0, 64, uint64_t, (uint64_t) (uint64_t) ts);
	(&ctx->parent)->at += 64;

	/* "content_size" field */
	ctx->parent.at = ctx->off_spc_content_size;
	barectf_bt_bitfield_write_le(&(&ctx->parent)->buf[_BITS_TO_BYTES((&ctx->parent)->at)], uint8_t, 0, 32, uint32_t, (uint32_t) (uint32_t) ctx->parent.content_size);
	(&ctx->parent)->at += 32;

	/* "events_discarded" field */
	ctx->parent.at = ctx->off_spc_events_discarded;
	barectf_bt_bitfield_write_le(&(&ctx->parent)->buf[_BITS_TO_BYTES((&ctx->parent)->at)], uint8_t, 0, 32, uint32_t, (uint32_t) (uint32_t) ctx->parent.events_discarded);
	(&ctx->parent)->at += 32;

	/* go back to end of packet */
	ctx->parent.at = ctx->parent.packet_size;

	/* mark packet as closed */
	ctx->parent.packet_is_open = 0;
}

static void _serialize_stream_event_header_default(
	void *vctx,
	uint32_t event_id
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint64_t ts = ctx->cbs.default_clock_get_value(ctx->data);

	{
		/* align structure */
		_ALIGN(ctx->at, 64);

		/* "timestamp" field */
		_ALIGN(ctx->at, 64);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 64, uint64_t, (uint64_t) (uint64_t) ts);
		ctx->at += 64;

		/* "id" field */
		_ALIGN(ctx->at, 16);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 16, uint16_t, (uint16_t) (uint16_t) event_id);
		ctx->at += 16;
	}

}

static uint32_t _get_event_size_default_simple_uint32(
	void *vctx,
	uint32_t ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event payload */
	{

		/* "value" field */
		/* field size: 32 (partial total so far: 128) */
	}

	at += 128;

	return at - ctx->at;
}

static void _serialize_event_default_simple_uint32(
	void *vctx,
	uint32_t ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 0);

	/* event payload */
	{
		/* align structure */
		_ALIGN(ctx->at, 32);

		/* "value" field */
		_ALIGN(ctx->at, 32);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 32, uint32_t, (uint32_t) ep_value);
		ctx->at += 32;
	}

}

/* trace (stream "default", event "simple_uint32") */
void barectf_default_trace_simple_uint32(
	struct barectf_default_ctx *ctx,
	uint32_t ep_value
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_simple_uint32(TO_VOID_PTR(ctx), ep_value);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_simple_uint32(TO_VOID_PTR(ctx), ep_value);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_simple_int16(
	void *vctx,
	int16_t ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event payload */
	{

		/* "value" field */
		/* field size: 16 (partial total so far: 96) */
	}

	at += 96;

	return at - ctx->at;
}

static void _serialize_event_default_simple_int16(
	void *vctx,
	int16_t ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 1);

	/* event payload */
	{
		/* align structure */
		_ALIGN(ctx->at, 16);

		/* "value" field */
		_ALIGN(ctx->at, 16);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 16, int16_t, (int16_t) ep_value);
		ctx->at += 16;
	}

}

/* trace (stream "default", event "simple_int16") */
void barectf_default_trace_simple_int16(
	struct barectf_default_ctx *ctx,
	int16_t ep_value
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_simple_int16(TO_VOID_PTR(ctx), ep_value);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_simple_int16(TO_VOID_PTR(ctx), ep_value);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_simple_float(
	void *vctx,
	float ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event payload */
	{

		/* "value" field */
		/* field size: 32 (partial total so far: 128) */
	}

	at += 128;

	return at - ctx->at;
}

static void _serialize_event_default_simple_float(
	void *vctx,
	float ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 2);

	/* event payload */
	{
		/* align structure */
		_ALIGN(ctx->at, 32);

		/* "value" field */
		_ALIGN(ctx->at, 32);

		{
			union f2u f2u;

			f2u.f = ep_value;
			barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 32, uint32_t, (uint32_t) f2u.u);
		}

		ctx->at += 32;
	}

}

/* trace (stream "default", event "simple_float") */
void barectf_default_trace_simple_float(
	struct barectf_default_ctx *ctx,
	float ep_value
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_simple_float(TO_VOID_PTR(ctx), ep_value);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_simple_float(TO_VOID_PTR(ctx), ep_value);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_trace_sensor(
	void *vctx,
	const char * ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event payload */
	{

		/* "value" field */
		at += 80;
		at += _BYTES_TO_BITS(strlen(ep_value) + 1);
	}

	return at - ctx->at;
}

static void _serialize_event_default_trace_sensor(
	void *vctx,
	const char * ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 3);

	/* event payload */
	{
		/* align structure */
		_ALIGN(ctx->at, 8);

		/* "value" field */
		_ALIGN(ctx->at, 8);
		_write_cstring(ctx, ep_value);
	}

}

/* trace (stream "default", event "trace_sensor") */
void barectf_default_trace_trace_sensor(
	struct barectf_default_ctx *ctx,
	const char * ep_value
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_trace_sensor(TO_VOID_PTR(ctx), ep_value);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_trace_sensor(TO_VOID_PTR(ctx), ep_value);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_simple_enum(
	void *vctx,
	uint8_t ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event payload */
	{

		/* "value" field */
		/* field size: 8 (partial total so far: 88) */
	}

	at += 88;

	return at - ctx->at;
}

static void _serialize_event_default_simple_enum(
	void *vctx,
	uint8_t ep_value
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 4);

	/* event payload */
	{
		/* align structure */
		_ALIGN(ctx->at, 8);

		/* "value" field */
		_ALIGN(ctx->at, 8);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 8, uint8_t, (uint8_t) ep_value);
		ctx->at += 8;
	}

}

/* trace (stream "default", event "simple_enum") */
void barectf_default_trace_simple_enum(
	struct barectf_default_ctx *ctx,
	uint8_t ep_value
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_simple_enum(TO_VOID_PTR(ctx), ep_value);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_simple_enum(TO_VOID_PTR(ctx), ep_value);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_context_no_payload(
	void *vctx,
	uint32_t ec_a,
	const char * ec_b
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event context */
	{

		/* "a" field */
		/* field size: 32 (partial total so far: 128) */

		/* "b" field */
		at += 128;
		at += _BYTES_TO_BITS(strlen(ec_b) + 1);
	}

	return at - ctx->at;
}

static void _serialize_event_default_context_no_payload(
	void *vctx,
	uint32_t ec_a,
	const char * ec_b
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 5);

	/* event context */
	{
		/* align structure */
		_ALIGN(ctx->at, 32);

		/* "a" field */
		_ALIGN(ctx->at, 32);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 32, uint32_t, (uint32_t) ec_a);
		ctx->at += 32;

		/* "b" field */
		_ALIGN(ctx->at, 8);
		_write_cstring(ctx, ec_b);
	}

}

/* trace (stream "default", event "context_no_payload") */
void barectf_default_trace_context_no_payload(
	struct barectf_default_ctx *ctx,
	uint32_t ec_a,
	const char * ec_b
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_context_no_payload(TO_VOID_PTR(ctx), ec_a, ec_b);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_context_no_payload(TO_VOID_PTR(ctx), ec_a, ec_b);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_no_context_no_payload(
	void *vctx
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	at += 80;

	return at - ctx->at;
}

static void _serialize_event_default_no_context_no_payload(
	void *vctx
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 6);

}

/* trace (stream "default", event "no_context_no_payload") */
void barectf_default_trace_no_context_no_payload(
	struct barectf_default_ctx *ctx
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_no_context_no_payload(TO_VOID_PTR(ctx));

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_no_context_no_payload(TO_VOID_PTR(ctx));

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_sensor_readings(
	void *vctx,
	int32_t ep_temperature,
	uint16_t ep_humidity,
	const char * ep_other_readings,
	uint8_t ep_state
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event payload */
	{

		/* "temperature" field */
		/* field size: 32 (partial total so far: 128) */

		/* "humidity" field */
		/* field size: 16 (partial total so far: 144) */

		/* "other_readings" field */
		at += 144;
		at += _BYTES_TO_BITS(strlen(ep_other_readings) + 1);

		/* "state" field */
		/* field size: 8 (partial total so far: 8) */
	}

	at += 8;

	return at - ctx->at;
}

static void _serialize_event_default_sensor_readings(
	void *vctx,
	int32_t ep_temperature,
	uint16_t ep_humidity,
	const char * ep_other_readings,
	uint8_t ep_state
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 7);

	/* event payload */
	{
		/* align structure */
		_ALIGN(ctx->at, 32);

		/* "temperature" field */
		_ALIGN(ctx->at, 32);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 32, int32_t, (int32_t) ep_temperature);
		ctx->at += 32;

		/* "humidity" field */
		_ALIGN(ctx->at, 16);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 16, uint16_t, (uint16_t) ep_humidity);
		ctx->at += 16;

		/* "other_readings" field */
		_ALIGN(ctx->at, 8);
		_write_cstring(ctx, ep_other_readings);

		/* "state" field */
		_ALIGN(ctx->at, 8);
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 8, uint8_t, (uint8_t) ep_state);
		ctx->at += 8;
	}

}

/* trace (stream "default", event "sensor_readings") */
void barectf_default_trace_sensor_readings(
	struct barectf_default_ctx *ctx,
	int32_t ep_temperature,
	uint16_t ep_humidity,
	const char * ep_other_readings,
	uint8_t ep_state
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_sensor_readings(TO_VOID_PTR(ctx), ep_temperature, ep_humidity, ep_other_readings, ep_state);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_sensor_readings(TO_VOID_PTR(ctx), ep_temperature, ep_humidity, ep_other_readings, ep_state);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}

static uint32_t _get_event_size_default_bit_packed_integers(
	void *vctx,
	uint8_t ep_uint1,
	int8_t ep_int1,
	uint8_t ep_uint2,
	int8_t ep_int3,
	uint8_t ep_uint4,
	int8_t ep_int5,
	uint8_t ep_uint6,
	int8_t ep_int7,
	uint8_t ep_uint8
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	uint32_t at = ctx->at;

	/* byte-align entity */
	_ALIGN(at, 8);

	/* stream event header */
	{
		/* align structure */
		_ALIGN(at, 64);

		/* "timestamp" field */
		/* field size: 64 (partial total so far: 64) */

		/* "id" field */
		/* field size: 16 (partial total so far: 80) */
	}

	/* event payload */
	{

		/* "uint1" field */
		/* field size: 1 (partial total so far: 81) */

		/* "int1" field */
		/* field size: 1 (partial total so far: 82) */

		/* "uint2" field */
		/* field size: 2 (partial total so far: 84) */

		/* "int3" field */
		/* field size: 3 (partial total so far: 87) */

		/* "uint4" field */
		/* field size: 4 (partial total so far: 91) */

		/* "int5" field */
		/* field size: 5 (partial total so far: 96) */

		/* "uint6" field */
		/* field size: 6 (partial total so far: 102) */

		/* "int7" field */
		/* field size: 7 (partial total so far: 109) */

		/* "uint8" field */
		/* field size: 8 (partial total so far: 117) */
	}

	at += 117;

	return at - ctx->at;
}

static void _serialize_event_default_bit_packed_integers(
	void *vctx,
	uint8_t ep_uint1,
	int8_t ep_int1,
	uint8_t ep_uint2,
	int8_t ep_int3,
	uint8_t ep_uint4,
	int8_t ep_int5,
	uint8_t ep_uint6,
	int8_t ep_int7,
	uint8_t ep_uint8
)
{
	struct barectf_ctx *ctx = FROM_VOID_PTR(struct barectf_ctx, vctx);
	/* stream event header */
	_serialize_stream_event_header_default(ctx, 8);

	/* event payload */
	{
		/* align structure */
		_ALIGN(ctx->at, 8);

		/* "uint1" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 1, uint8_t, (uint8_t) ep_uint1);
		ctx->at += 1;

		/* "int1" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 1, 1, int8_t, (int8_t) ep_int1);
		ctx->at += 1;

		/* "uint2" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 2, 2, uint8_t, (uint8_t) ep_uint2);
		ctx->at += 2;

		/* "int3" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 4, 3, int8_t, (int8_t) ep_int3);
		ctx->at += 3;

		/* "uint4" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 7, 4, uint8_t, (uint8_t) ep_uint4);
		ctx->at += 4;

		/* "int5" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 3, 5, int8_t, (int8_t) ep_int5);
		ctx->at += 5;

		/* "uint6" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 0, 6, uint8_t, (uint8_t) ep_uint6);
		ctx->at += 6;

		/* "int7" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 6, 7, int8_t, (int8_t) ep_int7);
		ctx->at += 7;

		/* "uint8" field */
		barectf_bt_bitfield_write_le(&ctx->buf[_BITS_TO_BYTES(ctx->at)], uint8_t, 5, 8, uint8_t, (uint8_t) ep_uint8);
		ctx->at += 8;
	}

}

/* trace (stream "default", event "bit_packed_integers") */
void barectf_default_trace_bit_packed_integers(
	struct barectf_default_ctx *ctx,
	uint8_t ep_uint1,
	int8_t ep_int1,
	uint8_t ep_uint2,
	int8_t ep_int3,
	uint8_t ep_uint4,
	int8_t ep_int5,
	uint8_t ep_uint6,
	int8_t ep_int7,
	uint8_t ep_uint8
)
{
	uint32_t ev_size;

	/* get event size */
	ev_size = _get_event_size_default_bit_packed_integers(TO_VOID_PTR(ctx), ep_uint1, ep_int1, ep_uint2, ep_int3, ep_uint4, ep_int5, ep_uint6, ep_int7, ep_uint8);

	/* do we have enough space to serialize? */
	if (!_reserve_event_space(TO_VOID_PTR(ctx), ev_size)) {
		/* no: forget this */
		return;
	}

	/* serialize event */
	_serialize_event_default_bit_packed_integers(TO_VOID_PTR(ctx), ep_uint1, ep_int1, ep_uint2, ep_int3, ep_uint4, ep_int5, ep_uint6, ep_int7, ep_uint8);

	/* commit event */
	_commit_event(TO_VOID_PTR(ctx));
}
