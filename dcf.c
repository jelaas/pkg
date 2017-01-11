#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include "dcf.h"
#include "bigint.h"

static const unsigned char hamcode[] = { 0x00, 0x71, 0x62, 0x13, 0x54, 0x25, 0x36, 0x47, 0x38, 0x49, 0x5A, 0x2B, 0x6C, 0x1D, 0x0E, 0x7F };
static const unsigned char hamdecode[] =
{
	0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0E, 0x07,
	0x00, 0x09, 0x0E, 0x0B, 0x0E, 0x0D, 0x0E, 0x0E,
	0x00, 0x03, 0x03, 0x03, 0x04, 0x0D, 0x06, 0x03,
	0x08, 0x0D, 0x0A, 0x03, 0x0D, 0x0D, 0x0E, 0x0D,
	0x00, 0x05, 0x02, 0x0B, 0x05, 0x05, 0x06, 0x05,
	0x08, 0x0B, 0x0B, 0x0B, 0x0C, 0x05, 0x0E, 0x0B,
	0x08, 0x01, 0x06, 0x03, 0x06, 0x05, 0x06, 0x06,
	0x08, 0x08, 0x08, 0x0B, 0x08, 0x0D, 0x06, 0x0F,
	0x00, 0x09, 0x02, 0x07, 0x04, 0x07, 0x07, 0x07,
	0x09, 0x09, 0x0A, 0x09, 0x0C, 0x09, 0x0E, 0x07,
	0x04, 0x01, 0x0A, 0x03, 0x04, 0x04, 0x04, 0x07,
	0x0A, 0x09, 0x0A, 0x0A, 0x04, 0x0D, 0x0A, 0x0F,
	0x02, 0x01, 0x02, 0x02, 0x0C, 0x05, 0x02, 0x07,
	0x0C, 0x09, 0x02, 0x0B, 0x0C, 0x0C, 0x0C, 0x0F,
	0x01, 0x01, 0x02, 0x01, 0x04, 0x01, 0x06, 0x0F,
	0x08, 0x01, 0x0A, 0x0F, 0x0C, 0x0F, 0x0F, 0x0F
};
static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t crc32(uint32_t crc, const void *buf, size_t size)
{
	const uint8_t *p;

	p = buf;
	crc = crc ^ ~0U;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

/* 32-bit parity */
static int parity(unsigned int v)
{
        v ^= v >> 1;
        v ^= v >> 2;
        v = (v & 0x11111111U) * 0x11111111U;
        return (v >> 28) & 1;
}

/* initial crc value should be 0xFFFF */
static unsigned short crc16(unsigned short crc, const void *data, unsigned char length){
	unsigned char x;
	const uint8_t *p;

	p = data;

	while (length--){
		x = crc >> 8 ^ *p++;
		x ^= x>>4;
		crc = (crc << 8) ^ (x << 12) ^ (x <<5) ^ x;
	}
	return crc & 0xffff;
}

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

static void mkham(unsigned char *buf, unsigned int byte)
{
	buf[0] = hamcode[(byte & 0xf0) >> 4];
	buf[0] |= parity(buf[0]) << 7;
	buf[1] = hamcode[(byte & 0xf)];
	buf[1] |= parity(buf[1]) << 7;	
}

static int _dcf_varint_write_cs(struct dcf *dcf, struct bigint *b, int cs)
{
	int nibs, bytes, i, totwritten = 0;
	unsigned char numbytes[2], ham[2], buf[2];
	unsigned short crc = 0xffff;

	nibs = (bigint_bits(b)+3)/4;
	bytes = (nibs+1)/2;
	mkham(numbytes, bytes);
	if(write(dcf->fd, numbytes, 2) != 2)
                return -1;
	totwritten+=2;
	crc = crc16(crc, numbytes, 2);
	if(cs) {
		dcf->crc32 = crc32(dcf->crc32, numbytes, 2);
		dcf->crc16 = crc16(dcf->crc16, numbytes, 2);
	}
	for(i=0;i<bytes;i++) {
		if((i*2+1) >= nibs)
			buf[0] = 0;
		else
			buf[0] = b->v[i*2+1];
		buf[0] <<= 4;
		buf[0] += b->v[i*2];
		mkham(ham, buf[0]);
		if(write(dcf->fd, ham, 2) != 2)
			return -1;
		totwritten+=2;
		crc = crc16(crc, ham, 2);
		if(cs) {
			dcf->crc32 = crc32(dcf->crc32, ham, 2);
			dcf->crc16 = crc16(dcf->crc16, ham, 2);
		}
	}
	if(write(dcf->fd, numbytes, 2) != 2)
                return -1;
	totwritten+=2;
	crc = crc16(crc, numbytes, 2);
	if(cs) {
		dcf->crc32 = crc32(dcf->crc32, numbytes, 2);
		dcf->crc16 = crc16(dcf->crc16, numbytes, 2);
	}
	buf[0] = crc & 0xff00 >> 8;
	buf[1] = crc & 0xff;
	if(write(dcf->fd, buf, 2) != 2)
                return -1;
	totwritten+=2;
	if(cs) {
		dcf->crc32 = crc32(dcf->crc32, buf, 2);
		dcf->crc16 = crc16(dcf->crc16, buf, 2);
	}
	if(_dcf_recordsize_inci(dcf, totwritten))
		return -1;
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
	unsigned char buf[1];
	if(read(dcf->fd, buf, 1) != 1)
                return -1;
	dcf->crc32 = crc32(dcf->crc32, buf, 1);
	dcf->crc16 = crc16(dcf->crc16, buf, 1);
	if(_dcf_recordsize_inci(dcf, 1))
		return -1;
	*size = buf[0];
	return 0;
}

int dcf_varint_value_read(struct dcf *dcf, struct bigint *b, int size)
{
	int t;
	unsigned char buf[1];
	char *v = b->v;
	int i = 0;

	if(bigint_zero(b))
		return -1;
	while(size--) {
		if(read(dcf->fd, buf, 1) != 1)
			return -1;
		dcf->crc32 = crc32(dcf->crc32, buf, 1);
		dcf->crc16 = crc16(dcf->crc16, buf, 1);
		t = buf[0];
		if(i >= b->n) return -1;
		v[i++] = t & 0xf;
		if(size && (t >> 4)) {
			if(i >= b->n) return -1;
			v[i++] = t >> 4;
		}
	}
	if(read(dcf->fd, buf, 1) != 1)
		return -1;
	dcf->crc32 = crc32(dcf->crc32, buf, 1);
	dcf->crc16 = crc16(dcf->crc16, buf, 1);
	if(buf[0] != size) return -1;
	if(_dcf_recordsize_inci(dcf, size + 1))
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

int dcf_magic_write(struct dcf *dcf)
{
	if(write(dcf->fd, DCF_MAGIC, 6) != 6)
		return -1;
	dcf->crc32 = crc32(dcf->crc32, DCF_MAGIC, 6);
	dcf->crc16 = crc16(dcf->crc16, DCF_MAGIC, 6);
	if(_dcf_recordsize_inci(dcf, 6))
		return -1;
	return 0;
}

int dcf_collectiontype_write(struct dcf *dcf)
{
	if(write(dcf->fd, dcf->collectiontype, 4) != 4)
		return -1;
	dcf->crc32 = crc32(dcf->crc32, dcf->collectiontype, 4);
	dcf->crc16 = crc16(dcf->crc16, dcf->collectiontype, 4);
	if(_dcf_recordsize_inci(dcf, 4))
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
	dcf->crc32 = crc32(dcf->crc32, ident, identsize);
	dcf->crc16 = crc16(dcf->crc16, ident, identsize);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	if(bigint_loadi(&b, v, sizeof(v), contentsize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, content, contentsize) != contentsize)
                return -1;
	dcf->crc32 = crc32(dcf->crc32, content, contentsize);
	dcf->crc16 = crc16(dcf->crc16, content, contentsize);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	return 0;
}

static int _dcf_write_zero_cs(struct dcf *dcf, int cs) {
	char buf[2];
	buf[0] = 0;
	buf[1] = 0;
        if(write(dcf->fd, buf, 2) != 2)
                return -1;
	if(cs) {
		dcf->crc32 = crc32(dcf->crc32, buf, 2);
		dcf->crc16 = crc16(dcf->crc16, buf, 2);
	}
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

int dcf_data_write(struct dcf *dcf, const char *buf, int size, int padsize)
{
	struct bigint b;
	char v[16];

	if(size == 0)
		return -1;
	if(padsize >= size)
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), size))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(bigint_loadi(&b, v, sizeof(v), padsize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, buf, size) != size)
                return -1;
	dcf->crc32 = crc32(dcf->crc32, buf, size);
	dcf->crc16 = crc16(dcf->crc16, buf, size);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	
        return 0;
}

int dcf_signature_write(struct dcf *dcf, const char *sigtype, int sigtypesize, const char *sig, int sigsize)
{
	struct bigint b;
	char v[16];

	if(sigtypesize == 0)
		return -1;
	if(sigsize == 0)
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), sigtypesize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, sigtype, sigtypesize) != sigtypesize)
                return -1;
	dcf->crc32 = crc32(dcf->crc32, sigtype, sigtypesize);
	dcf->crc16 = crc16(dcf->crc16, sigtype, sigtypesize);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;
	
	if(bigint_loadi(&b, v, sizeof(v), sigsize))
		return -1;
	if(dcf_varint_write(dcf, &b))
		return -1;
	if(write(dcf->fd, sig, sigsize) != sigsize)
                return -1;
	dcf->crc32 = crc32(dcf->crc32, sig, sigsize);
	dcf->crc16 = crc16(dcf->crc16, sig, sigsize);
	if(_dcf_recordsize_inc(dcf, &b))
		return -1;

	if(dcf_crc16_write(dcf))
		return -1;	
        return 0;	
}

int dcf_signature_write_final(struct dcf *dcf) {
	return _dcf_write_zero(dcf);
}

int dcf_data_write_final(struct dcf *dcf) {
	return _dcf_write_zero(dcf);
}

int dcf_crc16_write(struct dcf *dcf)
{
	unsigned char buf[2];
	buf[0] = (dcf->crc16 & 0xff00) >> 8;
	buf[1] = dcf->crc16 & 0xff;
	if(write(dcf->fd, buf, 2) != 2)
		return -1;
	if(_dcf_recordsize_inci(dcf, 2))
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
	if(dcf_varint_write(dcf, &dcf->temp2))
                return -1;
	bigint_zero(&dcf->recordsize);
	return 0;
}
