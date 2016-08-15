#include <unistd.h>
#include <string.h>
#include "dcf.h"
#include "bigint.h"

int dcf_init(struct dcf *dcf, int fd, const char *collectiontype, char *v, int v_len)
{
	dcf->fd = fd;
	memcpy(dcf->collectiontype, collectiontype, 4);
	memset(&dcf->sha256, 0, sizeof(dcf->sha256));
	sha256_init_ctx(&dcf->sha256);
	if(bigint_loadi(&dcf->recordsize, v, v_len/3, 0))
		return -1;
	if(bigint_loadi(&dcf->temp, v+(v_len/3), v_len/3, 0))
		return -1;
	if(bigint_loadi(&dcf->temp2, v+(v_len/3)*2, (v_len/3)*2, 0))
		return -1;
	return 0;
}

#if 0
int dcf_magic_read(struct dcf *dcf);
int dcf_magic_read(struct dcf *dcf);
int dcf_varint_size_read(struct dcf *dcf); /* returns number bytes in varint */
int dcf_varint_value_read(struct dcf *dcf, struct bigint *i);
int dcf_meta_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_checksum_read(struct dcf *dcf);
#endif

int dcf_magic_write(struct dcf *dcf)
{
	if(write(dcf->fd, DCF_MAGIC, 6) != 6)
		return -1;
	sha256_process_bytes(DCF_MAGIC, 6, &dcf->sha256);
	// bigint_load 6
	// bigint_loadi 0 (result)
	// bigint_sum
	// copy result to recordsize
	return 0;
}

int dcf_collectiontype_write(struct dcf *dcf)
{
	if(write(dcf->fd, dcf->collectiontype, 4) != 4)
		return -1;
	sha256_process_bytes(dcf->collectiontype, 4, &dcf->sha256);
	return 0;
}

int _dcf_varint_size(struct dcf *dcf, struct bigint *b)
{
	int nibs, bytes;
	nibs = (bigint_bits(b)+3)/4;
        bytes = (nibs+1)/2;
	
	return 1 + bytes + 1;
}

int dcf_varint_write(struct dcf *dcf, struct bigint *b)
{
	int nibs, bytes, i;
	unsigned char numbytes[1], buf[1];

	nibs = (bigint_bits(b)+3)/4;
	bytes = (nibs+1)/2;
	numbytes[0] = bytes;
	if(write(dcf->fd, numbytes, 1) != 1)
                return -1;
	sha256_process_bytes(numbytes, 1, &dcf->sha256);
	for(i=0;i<bytes;i++) {
		if((i*2+1) >= nibs)
			buf[0] = 0;
		else
			buf[0] = b->v[i*2+1];
		buf[0] <<= 4;
		buf[0] += b->v[i*2];
		if(write(dcf->fd, buf, 1) != 1)
			return -1;
		sha256_process_bytes(buf, 1, &dcf->sha256);
	}
	if(write(dcf->fd, numbytes, 1) != 1)
                return -1;
	sha256_process_bytes(numbytes, 1, &dcf->sha256);
	return 0;
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
	if(bigint_loadi(&b, v, sizeof(v), contentsize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, content, contentsize) != contentsize)
                return -1;
	sha256_process_bytes(content, contentsize, &dcf->sha256);
	return 0;
}

int dcf_meta_write_final(struct dcf *dcf) {
	char buf[1];
	buf[0] = 0;
        if(write(dcf->fd, buf, 1) != 1)
                return -1;
	sha256_process_bytes(buf, 1, &dcf->sha256);
	return 0;
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
        return 0;
}

int dcf_data_write_final(struct dcf *dcf) {
	return dcf_meta_write_final(dcf);
}

int dcf_checksum_write(struct dcf *dcf, char *chksum)
{
	unsigned char resbuf[SHA256_DIGEST_LENGTH];
	sha256_finish_ctx (&dcf->sha256, resbuf);
	if(write(dcf->fd, resbuf, SHA256_DIGEST_LENGTH) != SHA256_DIGEST_LENGTH)
		return -1;
	if(chksum) {
		memcpy(chksum, resbuf, SHA256_DIGEST_LENGTH);
	}
	sha256_init_ctx(&dcf->sha256);
	return 0;
}

/* writes recordsize and checksum of recordsize */
int dcf_recordsize_write(struct dcf *dcf, char *chksum)
{
	char tail_v[4], rec_v[16]; // FIXME: rec_v should be supplied by _init
	int tailsizei;
	struct bigint recsize, tailsize;
	
	tailsizei = _dcf_varint_size(dcf, &dcf->recordsize) + SHA256_DIGEST_LENGTH;
	if(bigint_loadi(&tailsize, tail_v, sizeof(tail_v), tailsizei))
		return -1;
	if(bigint_loadi(&recsize, rec_v, sizeof(rec_v), 0))
		return -1;
	if(bigint_sum(&recsize, &dcf->recordsize, &tailsize))
		return -1;
	if(dcf_varint_write(dcf, &recsize))
                return -1;
	if(dcf_checksum_write(dcf, chksum))
		return -1;
	bigint_zero(&dcf->recordsize);
	return 0;
}
