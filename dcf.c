#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "dcf.h"
#include "bigint.h"

static int _dcf_recordsize_inc(struct dcf *dcf, struct bigint *inc)
{
	if(bigint_zero(&dcf->temp))
		return -1;
	if(bigint_sum(&dcf->temp, &dcf->recordsize, inc))
		return -1;
	if(bigint_swap(&dcf->temp, &dcf->recordsize))
		return -1;
	return 0;
}

static int _dcf_recordsize_inci(struct dcf *dcf, int smallint)
{
	struct bigint b;
	char v[4];

	if(bigint_loadi(&b, v, sizeof(v), smallint))
		return -1;
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	return 0;
}

static int _dcf_varint_size(struct dcf *dcf, struct bigint *b)
{
	int nibs, bytes;
	nibs = (bigint_bits(b)+3)/4;
        bytes = (nibs+1)/2;
	
	return 1 + bytes + 1;
}

int dcf_varint_write(struct dcf *dcf, struct crc *crc, struct bigint *b)
{
	int nibs, i, totwritten = 0;
	unsigned int crc16;
	unsigned char buf[2];

	crc_push(crc, CRC16);
	nibs = bigint_nibbles(b);
	buf[0] = nibs;
	if(crc_write(dcf->fd, crc, buf, 1) != 1)
                return -1;
	totwritten+=1;
	for(i=0;i<nibs;i++) {
		buf[0] <<= 4;
		buf[0] |= b->v[i];
		if(i & 1) {
			if(crc_write(dcf->fd, crc, buf, 1) != 1)
				return -1;
			totwritten+=1;
		}
	}
	if(nibs & 1) {
		buf[0] <<= 4;
		if(crc_write(dcf->fd, crc, buf, 1) != 1)
			return -1;
		totwritten+=1;
	}
	buf[0] = nibs;
	if(crc_write(dcf->fd, crc, buf, 1) != 1)
                return -1;
	totwritten+=1;
	if(crc_val(crc, &crc16, CRC16))
		return -1;
	buf[0] = crc16 & 0xff00 >> 8;
	buf[1] = crc16 & 0xff;
	if(write(dcf->fd, buf, 2) != 2)
                return -1;
	totwritten+=2;
	if(_dcf_recordsize_inci(dcf, totwritten))
		return -1;
	crc_pop(crc, CRC16);
	return 0;
}

int dcf_init(struct dcf *dcf, int fd, char *v, int v_len)
{
	dcf->fd = fd;
	dcf->crc16 = 0xffff;
	dcf->crc32 = 0;
	if(bigint_loadi(&dcf->recordsize, v, v_len/3, 0))
		return -1;
	if(bigint_loadi(&dcf->temp, v+(v_len/3), v_len/3, 0))
		return -1;
	if(bigint_loadi(&dcf->temp2, v+(v_len/3)*2, v_len/3, 0))
		return -1;
	return 0;
}

int dcf_collectiontype_set(struct dcf *dcf, const char *collectiontype)
{
	memcpy(dcf->collectiontype, collectiontype, 4);
	return 0;
}
#if 0
int dcf_magic_read(struct dcf *dcf)
{
	unsigned char buf[6];
	if(read(dcf->fd, buf, 6) != 6)
		return -1;
	dcf->crc32 = crc32(dcf->crc32, buf, 6);
	dcf->crc16 = crc16(dcf->crc16, buf, 6);
	if(_dcf_recordsize_inci(dcf, 6))
		return -1;
	return memcmp(DCF_MAGIC, buf, 6);
}

int dcf_collectiontype_read(struct dcf *dcf)
{
	if(read(dcf->fd, dcf->collectiontype, 4) != 4)
		return -1;
	dcf->crc32 = crc32(dcf->crc32, dcf->collectiontype, 4);
	dcf->crc16 = crc16(dcf->crc16, dcf->collectiontype, 4);
	if(_dcf_recordsize_inci(dcf, 4))
		return -1;
	return 0;
}

int dcf_varint_size_read(struct dcf *dcf, int *size)
{
	unsigned char buf[2];
	if(read(dcf->fd, buf, 2) != 2)
                return -1;
	dcf->crc32 = crc32(dcf->crc32, buf, 2);
	dcf->crc16 = crc16(dcf->crc16, buf, 2);
	if(_dcf_recordsize_inci(dcf, 2))
		return -1;
	*size = (hamdecode[buf[0]]<<4)|hamdecode[buf[1]];
	return 0;
}

int dcf_varint_value_read(struct dcf *dcf, struct bigint *b, int size)
{
	int t;
	unsigned char buf[2];
	char *v = b->v;
	int i = 0;

	if(bigint_zero(b))
		return -1;
	while(size--) {
		if(read(dcf->fd, buf, 1) != 1)
			return -1;
		dcf->crc32 = crc32(dcf->crc32, buf, 1);
		dcf->crc16 = crc16(dcf->crc16, buf, 1);
		if(i >= b->n) return -1;
		v[i++] = hamdecode[buf[0]];
	}
	if(read(dcf->fd, buf, 2) != 2)
		return -1;
	dcf->crc32 = crc32(dcf->crc32, buf, 2);
	dcf->crc16 = crc16(dcf->crc16, buf, 2);
	if(((hamdecode[buf[0]]<<4)|hamdecode[buf[1]]) != size) return -1;
	if(_dcf_recordsize_inci(dcf, size + 2))
                return -1;
	return 0;
}

/*
 * reads data segment. meta_data_identifier, meta_data_content or data segment.
 * datasize is how much to read.
 * 0 ok.
 */
int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *buf)
{
	if(read(dcf->fd, buf, datasize) != datasize)
		return -1;
	dcf->crc32 = crc32(dcf->crc32, buf, datasize);
	dcf->crc16 = crc16(dcf->crc16, buf, datasize);
	if(_dcf_recordsize_inci(dcf, datasize))
                return -1;
	return 0;
}

/* 0 ok. 1 = no-signature. < 0 on error */
int dcf_signature_read(struct dcf *dcf, int * typesize, int *sigsize, int typebufsize, char *typebuf, int sigbufsize, unsigned char *sigbuf)
{
}

/* reads and checks crc16 */
int dcf_crc16_read(struct dcf *dcf)
{
	unsigned char buf[2];
	if(read(dcf->fd, buf, 2) != 2)
		return -1;
	if(dcf->crc16 != (buf[0] << 8) + buf[1])
		return -1;
	dcf->crc16 = 0xffff;
	dcf->crc32 = crc32(dcf->crc32, buf, 2);
	if(_dcf_recordsize_inci(dcf, 2))
                return -1;
	return 0;
}

/* reads and checks crc32 */
int dcf_crc32_read(struct dcf *dcf)
{
	unsigned char buf[4];
	if(read(dcf->fd, buf, 4) != 4)
		return -1;
	if(dcf->crc32 != (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3])
		return -1;
	dcf->crc32 = 0;
	dcf->crc16 = crc16(dcf->crc16, buf, 4);
	if(_dcf_recordsize_inci(dcf, 4))
                return -1;
	return 0;
}

/* reads and checks recordsize  */
int dcf_recordsize_read(struct dcf *dcf)
{
	int bsize;
	
	if(dcf_varint_size_read(dcf, &bsize))
		return -1;
	if(bsize > dcf->temp2.n)
		return -1;
	if(dcf_varint_value_read(dcf, &dcf->temp2, bsize))
		return -1;
	dcf->recordsize.neg = 1;
	if(bigint_zero(&dcf->temp))
                return -1;
	if(bigint_sum(&dcf->temp, &dcf->temp2, &dcf->recordsize))
		return -1;
	if(!bigint_iszero(&dcf->temp))
                return -1;
	if(bigint_zero(&dcf->recordsize))
                return -1;
	return 0;
}
#endif

int dcf_magic_write(struct dcf *dcf, struct crc *crc)
{
	if(crc_write(dcf->fd, crc, DCF_MAGIC, 6) != 6)
		return -1;
	if(_dcf_recordsize_inci(dcf, 6))
		return -1;
	return 0;
}

int dcf_data_write_magic(struct dcf *dcf)
{
	if(crc_write(dcf->fd, (void*)0, DCF_DATA_MAGIC, 4) != 4)
		return -1;
	if(_dcf_recordsize_inci(dcf, 4))
		return -1;
	return 0;
}

int dcf_collectiontype_write(struct dcf *dcf, struct crc *crc)
{
	if(crc_write(dcf->fd, crc, dcf->collectiontype, 4) != 4)
		return -1;
	if(_dcf_recordsize_inci(dcf, 4))
		return -1;
	return 0;
}

int dcf_meta_write(struct dcf *dcf, int identsize, const char *ident, int contentsize, const char *content)
{
	struct bigint b;
	char v[16];
	struct crc crc;
	
	crc_init(&crc);
	
	crc_push(&crc, CRC16);
	if(bigint_loadi(&b, v, sizeof(v), identsize))
		return -1;
	if(dcf_varint_write(dcf, &crc, &b))
		return -1;
	if(write(dcf->fd, ident, identsize) != identsize)
                return -1;
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	if(bigint_loadi(&b, v, sizeof(v), contentsize))
		return -1;
	if(dcf_varint_write(dcf, &crc, &b))
		return -1;
	if(write(dcf->fd, content, contentsize) != contentsize)
                return -1;
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	crc_pop(&crc, CRC16);
	return 0;
}

static int _dcf_write_zero(struct dcf *dcf, struct crc *crc) {
	char buf[2];
	buf[0] = 0;
	buf[1] = 0;
        if(crc_write(dcf->fd, crc, buf, 2) != 2)
                return -1;
	if(_dcf_recordsize_inci(dcf, 2))
		return -1;
	return 0;
}

int dcf_meta_write_final(struct dcf *dcf) {
	return _dcf_write_zero(dcf, (void*)0);
}

int dcf_data_write(struct dcf *dcf, struct crc *crc, const char *buf, int size, int padsize)
{
	struct bigint b;
	char v[16];

	if(size == 0)
		return -1;
	if(padsize >= size)
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), size))
		return -1;
	if(dcf_varint_write(dcf, crc, &b))
		return -1;
	if(bigint_loadi(&b, v, sizeof(v), padsize))
		return -1;
	if(dcf_varint_write(dcf, crc, &b))
		return -1;
	if(crc_write(dcf->fd, crc, buf, size) != size)
                return -1;
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	
        return 0;
}

int dcf_signature_write(struct dcf *dcf, const char *sigtype, int sigtypesize, const char *sig, int sigsize)
{
	struct bigint b;
	char v[16];
	struct crc crc;
	
	crc_init(&crc);
	crc_push(&crc, CRC16);

	if(sigtypesize == 0)
		return -1;
	if(sigsize == 0)
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), sigtypesize))
		return -1;
	if(dcf_varint_write(dcf, &crc, &b))
		return -1;
	if(crc_write(dcf->fd, &crc, sigtype, sigtypesize) != sigtypesize)
                return -1;
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), sigsize))
		return -1;
	if(dcf_varint_write(dcf, &crc, &b))
		return -1;
	if(crc_write(dcf->fd, &crc, sig, sigsize) != sigsize)
                return -1;
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	if(dcf_crc16_write(dcf, &crc))
		return -1;	
	crc_pop(&crc, CRC16);
        return 0;	
}

int dcf_signature_write_final(struct dcf *dcf) {
	return _dcf_write_zero(dcf, (void*)0);
}

int dcf_data_write_final(struct dcf *dcf, struct crc *crc) {
	return _dcf_write_zero(dcf, crc);
}

int dcf_crc16_write(struct dcf *dcf, struct crc *crc)
{
	unsigned char buf[2];
	unsigned int val;
	crc_val(crc, &val, CRC16);
	buf[0] = val & 0xff00 >> 8;
	buf[1] = val & 0xff;
	if(crc_write(dcf->fd, crc, buf, 2) != 2)
		return -1;
	if(_dcf_recordsize_inci(dcf, 2))
		return -1;
	return 0;
}

int dcf_write_zeros(struct dcf *dcf, struct crc *crc, int n)
{
	char buf[8] ="\x0\x0\x0\x0\x0\x0\x0\x0";
	
	while(n > 7) {
		if(crc_write(dcf->fd, crc, buf, 8) != 8)
			return -1;
		n-=8;
	}
	if(n) {
		if(crc_write(dcf->fd, crc, buf, n) != n)
			return -1;
	}
	return 0;
}

int dcf_crc32_write(struct dcf *dcf, struct crc *crc)
{
	unsigned char buf[4];
	unsigned int val;
	crc_val(crc, &val, CRC32);
	buf[0] = (val >> 24) & 0xff;
	buf[1] = (val >> 16) & 0xff;
	buf[2] = (val >> 8) & 0xff;
	buf[3] = val & 0xff;
	if(crc_write(dcf->fd, crc, buf, 4) != 4)
		return -1;
	if(_dcf_recordsize_inci(dcf, 4))
		return -1;
	return 0;
}

/* writes recordsize */
int dcf_recordsize_write(struct dcf *dcf)
{
	char tail_v[4];
	int tailsizei;
	struct bigint tailsize;
	
	dcf->crc16 = 0xffff;
	dcf->crc32 = 0;
	
	tailsizei = _dcf_varint_size(dcf, &dcf->recordsize);
	if(bigint_loadi(&tailsize, tail_v, sizeof(tail_v), tailsizei))
		return -1;
	if(bigint_zero(&dcf->temp2))
		return -1;
	if(bigint_sum(&dcf->temp2, &dcf->recordsize, &tailsize))
		return -1;
	if(dcf_varint_write(dcf, (void*)0, &dcf->temp2))
                return -1;
	bigint_zero(&dcf->recordsize);
	return 0;
}
