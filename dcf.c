#include <unistd.h>
#include <string.h>
#include "dcf.h"
#include "bigint.h"

int dcf_init(struct dcf *dcf, int fd, char *v, int v_len)
{
	dcf->fd = fd;
	memset(&dcf->sha256, 0, sizeof(dcf->sha256));
	sha256_init_ctx(&dcf->sha256);
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
int dcf_magic_read(struct dcf *dcf);
int dcf_magic_read(struct dcf *dcf);
int dcf_varint_size_read(struct dcf *dcf); /* returns number bytes in varint */
int dcf_varint_value_read(struct dcf *dcf, struct bigint *i);
int dcf_meta_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_hash_read(struct dcf *dcf);
#endif

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

int dcf_magic_write(struct dcf *dcf)
{
	if(write(dcf->fd, DCF_MAGIC, 6) != 6)
		return -1;
	sha256_process_bytes(DCF_MAGIC, 6, &dcf->sha256);
	if(_dcf_recordsize_inci(dcf, 6))
		return -1;
	return 0;
}

int dcf_collectiontype_write(struct dcf *dcf)
{
	if(write(dcf->fd, dcf->collectiontype, 4) != 4)
		return -1;
	sha256_process_bytes(dcf->collectiontype, 4, &dcf->sha256);
	if(_dcf_recordsize_inci(dcf, 4))
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

static int _dcf_varint_write_cs(struct dcf *dcf, struct bigint *b, int cs)
{
	int nibs, bytes, i, totwritten = 0;
	unsigned char numbytes[1], buf[1];

	nibs = (bigint_bits(b)+3)/4;
	bytes = (nibs+1)/2;
	numbytes[0] = bytes;
	if(write(dcf->fd, numbytes, 1) != 1)
                return -1;
	if(cs) sha256_process_bytes(numbytes, 1, &dcf->sha256);
	totwritten++;
	for(i=0;i<bytes;i++) {
		if((i*2+1) >= nibs)
			buf[0] = 0;
		else
			buf[0] = b->v[i*2+1];
		buf[0] <<= 4;
		buf[0] += b->v[i*2];
		if(write(dcf->fd, buf, 1) != 1)
			return -1;
		if(cs) sha256_process_bytes(buf, 1, &dcf->sha256);
		totwritten++;
	}
	if(write(dcf->fd, numbytes, 1) != 1)
                return -1;
	if(cs) sha256_process_bytes(numbytes, 1, &dcf->sha256);
	totwritten++;
	if(_dcf_recordsize_inci(dcf, totwritten))
		return -1;
	return 0;
}

int dcf_varint_write(struct dcf *dcf, struct bigint *b)
{
	return _dcf_varint_write_cs(dcf, b, 1);
}

int dcf_meta_write(struct dcf *dcf, int identsize, const char *ident, int contentsize, const char *content)
{
	struct bigint b;
	char v[16];
	if(bigint_loadi(&b, v, sizeof(v), identsize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, ident, identsize) != identsize)
                return -1;
	sha256_process_bytes(ident, identsize, &dcf->sha256);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	if(bigint_loadi(&b, v, sizeof(v), contentsize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, content, contentsize) != contentsize)
                return -1;
	sha256_process_bytes(content, contentsize, &dcf->sha256);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	return 0;
}

static int _dcf_write_zero_cs(struct dcf *dcf, int cs) {
	char buf[2];
	buf[0] = 0;
	buf[1] = 0;
        if(write(dcf->fd, buf, 2) != 1)
                return -1;
	if(cs) sha256_process_bytes(buf, 2, &dcf->sha256);
	if(_dcf_recordsize_inci(dcf, 2))
		return -1;
	return 0;
}

static int _dcf_write_zero(struct dcf *dcf) {
	return _dcf_write_zero_cs(dcf, 1);
}

int dcf_meta_write_final(struct dcf *dcf) {
	return _dcf_write_zero(dcf);
}

int dcf_data_write(struct dcf *dcf, const char *buf, int size)
{
	struct bigint b;
	char v[16];

	if(size == 0)
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), size))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, buf, size) != size)
                return -1;
        sha256_process_bytes(buf, size, &dcf->sha256);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	
        return 0;
}

/* sigtypesize == 0 means no signature */
int dcf_signature_write(struct dcf *dcf, const char *sigtype, int sigtypesize, const char *sig, int sigsize)
{
	struct bigint b;
	char v[16];

	if(sigtypesize == 0)
		return _dcf_write_zero(dcf);
	if(sigsize == 0)
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), sigtypesize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, sigtype, sigtypesize) != sigtypesize)
                return -1;
        sha256_process_bytes(sigtype, sigtypesize, &dcf->sha256);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), sigsize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, sig, sigsize) != sigsize)
                return -1;
        sha256_process_bytes(sig, sigsize, &dcf->sha256);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	
        return 0;	
}

int dcf_data_write_final(struct dcf *dcf) {
	return _dcf_write_zero(dcf);
}

int dcf_hash_write(struct dcf *dcf, char *hash)
{
	unsigned char resbuf[SHA256_DIGEST_LENGTH];
	sha256_finish_ctx (&dcf->sha256, resbuf);
	if(write(dcf->fd, resbuf, SHA256_DIGEST_LENGTH) != SHA256_DIGEST_LENGTH)
		return -1;
	if(hash) {
		memcpy(hash, resbuf, SHA256_DIGEST_LENGTH);
	}
	sha256_init_ctx(&dcf->sha256);
	if(_dcf_recordsize_inci(dcf, SHA256_DIGEST_LENGTH))
		return -1;
	return 0;
}

/* writes recordsize and hash of recordsize */
int dcf_recordsize_write(struct dcf *dcf, char *hash)
{
	char tail_v[4];
	int tailsizei;
	struct bigint tailsize;
	
	tailsizei = _dcf_varint_size(dcf, &dcf->recordsize) + SHA256_DIGEST_LENGTH;
	if(bigint_loadi(&tailsize, tail_v, sizeof(tail_v), tailsizei))
		return -1;
	if(bigint_zero(&dcf->temp2))
		return -1;
	if(bigint_sum(&dcf->temp2, &dcf->recordsize, &tailsize))
		return -1;
	if(dcf_varint_write(dcf, &dcf->temp2))
                return -1;
	if(dcf_hash_write(dcf, hash))
		return -1;
	bigint_zero(&dcf->recordsize);
	return 0;
}
