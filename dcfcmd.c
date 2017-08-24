#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include "dcf.h"
#include "crc.h"

struct {
	int collectionid;
	int padsize;
	int recpadsize;
	struct {
		char *name;
		char *content;
	} meta;
} conf;

int append(struct dcf *dcf)
{
	struct bigint version;
	char version_v[8];
	struct bigint collectionid, padsize, recpadsize;
	char collectionid_v[8], padsize_v[8], recpadsize_v[8];
	struct crc crc;
	int n;
	char buf[512];
	
	crc_init(&crc);
	crc_push(&crc, CRC16);
	if(dcf_magic_write(dcf, &crc))
		return -1;
	if(dcf_collectiontype_write(dcf, &crc))
		return -1;

	if(bigint_loadi(&version, version_v, sizeof(version_v), DCF_VERSION))
		return -1;
	if(dcf_varint_write(dcf, &crc, &version))
		return -1;
	
	if(bigint_loadi(&collectionid, collectionid_v, sizeof(collectionid_v), conf.collectionid))
		return -1;
	if(dcf_varint_write(dcf, &crc, &collectionid))
		return -1;

	fprintf(stderr, "dcf_crc16_write\n");
	if(dcf_crc16_write(dcf, &crc))
		return -1;
	crc_pop(&crc, CRC16);
	
	fprintf(stderr, "dcf_meta_write\n");
	if(dcf_meta_write(dcf, strlen(conf.meta.name), conf.meta.name, strlen(conf.meta.content), conf.meta.content))
		return -1;
	fprintf(stderr, "dcf_meta_write_final\n");
	if(dcf_meta_write_final(dcf))
		return -1;

	fprintf(stderr, "padsize\n");
	if(bigint_loadi(&padsize, padsize_v, sizeof(padsize_v), conf.padsize))
		return -1;
	if(dcf_varint_write(dcf, &crc, &padsize))
		return -1;

	fprintf(stderr, "padding\n");
	if(dcf_write_zeros(dcf, (void*)0, conf.padsize))
		return -1;

	fprintf(stderr, "data magic\n");
	if(dcf_data_write_magic(dcf))
		return -1;

	fprintf(stderr, "data pos\n");
	if(dcf_pos_write(dcf, &crc))
		return -1;

	fprintf(stderr, "data segments\n");
	crc_push(&crc, CRC32);
	while((n=read(0, buf, sizeof(buf)))>0) {
		if(dcf_data_write(dcf, &crc, buf, n, 0))
			return -1;
	}
	
	fprintf(stderr, "data final\n");
	if(dcf_data_write_final(dcf, &crc))
		return -1;
	if(dcf_crc32_write(dcf, &crc))
		return -1;
	crc_pop(&crc, CRC32);

	if(dcf_signature_write(dcf, "sigge", 5, "STARDUST", 8))
		return -1;
	if(dcf_signature_write_final(dcf))
		return -1;

	if(bigint_loadi(&recpadsize, recpadsize_v, sizeof(recpadsize_v), conf.recpadsize))
		return -1;
	if(dcf_varint_write(dcf, &crc, &recpadsize))
		return -1;

	if(dcf_write_zeros(dcf, (void*)0, conf.recpadsize))
		return -1;
	if(dcf_recordsize_write(dcf, &crc))
		return -1;
	
	return 0;
}

int list(struct dcf *dcf)
{
	struct crc crc;

	crc_init(&crc);
	crc_push(&crc, CRC16);

	if(dcf_magic_read(dcf, &crc))
		return -1;

	dcf_collectiontype_read(dcf, &crc);

	/* version */
	{
		int size, val;
		char *v;
		struct bigint version;
		
		if(dcf_varint_size_read(dcf, &crc, &size))
			return -1;
		v = malloc(size);
		if(!v) return -1;
		bigint_load(&version, v, size, 0);
		if(dcf_varint_value_read(dcf, &crc, &version, size))
			return -1;
		if(bigint_toint(&val, &version))
			return -1;
		if(val != DCF_VERSION)
			return -1;
		free(v);
	}
#if 0
	
	/* collectionid */
	int dcf_varint_size_read(dcf, int *size);
	int dcf_varint_value_read(dcf, struct bigint *i, int size);
	
	/* hash so far */
	int dcf_hash_read(struct dcf *dcf);
	
	/* meta */
	{
		/* identsize */
		int dcf_varint_size_read(struct dcf *dcf, int *size);
		int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);

		/* identifier */
		int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *buf);
		
		/* contentsize */
		int dcf_varint_size_read(struct dcf *dcf, int *size);
		int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);

		/* content */
		int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *buf);
	}

	/* data */
	{
		/* datasize */
		int dcf_varint_size_read(struct dcf *dcf, int *size);
                int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);

		/* padsize */
		int dcf_varint_size_read(struct dcf *dcf, int *size);
                int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);
		
		/*  data */
		int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *buf);
	}

	/* data hash */
	int dcf_hash_read(struct dcf *dcf);

	/* signature */
	{
		/* signaturetypesize */
		int dcf_varint_size_read(struct dcf *dcf, int *size);
                int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);

		/* signaturetype */
		int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *buf);

		/* signaturesize */
		int dcf_varint_size_read(struct dcf *dcf, int *size);
                int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);

		/* signature */
		int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *buf);

		/* sighash */
		int dcf_hash_read(struct dcf *dcf);
	}
	
	/* record size */
	int dcf_varint_size_read(struct dcf *dcf, int *size);
	int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);
	
	/* recordsizehash */
	int dcf_hash_read(struct dcf *dcf);
#endif
	return 0;
}

int main(int argc, char **argv)
{
	struct dcf dcf;
	char v[64];
	
	conf.meta.name = "dummy";
	conf.meta.content = "content";

	if(argc > 1 && !strcmp(argv[1], "-a")) {
		if(dcf_init(&dcf, 1, v, sizeof(v)))
			return 1;
		dcf_collectiontype_set(&dcf, "TYPE");
		if(append(&dcf))
			return 1;
		return 0;
	}
	
	if(dcf_init(&dcf, 0, v, sizeof(v)))
		return 1;
	
	list(&dcf);
	
	return 2;
}
