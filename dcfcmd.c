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
	struct bigint collectionid;
	char collectionid_v[8];
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
	if(dcf_meta_write(dcf, strlen(conf.meta.name)+1, conf.meta.name, strlen(conf.meta.content)+1, conf.meta.content))
		return -1;
	fprintf(stderr, "dcf_meta_write_final\n");
	if(dcf_meta_write_final(dcf))
		return -1;

	fprintf(stderr, "padsize\n");
	if(dcf_uint16_write(dcf, &crc, conf.padsize))
		return -1;

	fprintf(stderr, "padding\n");
	if(dcf_write_zeros(dcf, (void*)0, conf.padsize))
		return -1;

	fprintf(stderr, "data magic\n");
	if(dcf_data_write_magic(dcf))
		return -1;

	{
		int pos;
		bigint_toint(&pos, &dcf->recordsize);
		fprintf(stderr, "pos = %d\n", pos);
	}

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

	if(dcf_uint16_write(dcf, &crc, conf.recpadsize))
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
	int rc;

loop:
	crc_init(&crc);
	crc_push(&crc, CRC16);

	fprintf(stderr, "Listing\n");
	if(dcf_magic_read(dcf, &crc)) {
		fprintf(stderr, "Wrong magic!\n");
		return -1;
	}

	if(dcf_collectiontype_read(dcf, &crc)) {
		fprintf(stderr, "Failed to read collection type\n");
		return -1;
	}

	/* version */
	{
		int size, val;
		char *v;
		struct bigint version;
		
		if((rc = dcf_varint_size_read(dcf, &crc, &size))) {
			fprintf(stderr, "Failed to read varint size of version: %d\n", rc);
			return -1;
		}
		fprintf(stderr, "varint size of version is %d\n", size);
		v = malloc(size);
		if(!v) return -1;
		bigint_load(&version, v, size, 0);
		if((rc = dcf_varint_value_read(dcf, &crc, &version, size))) {
			fprintf(stderr, "dcf_varint_value_read failed: %d\n", rc);
			return -1;
		}
		if(bigint_toint(&val, &version)) {
			fprintf(stderr, "bigint_toint failed\n");
			return -1;
		}
		printf("dcf_version = %d\n", val);
		if(val != DCF_VERSION)
			return -1;
		free(v);
	}

	/* collectionid */
	{
		int size, val;
                char *v;
                struct bigint id;
		
		if(dcf_varint_size_read(dcf, &crc, &size))
                        return -1;
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&id, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &id, size))
                        return -1;
                if(bigint_toint(&val, &id))
			return -1;
		printf("collectionid = %d\n", val);
		free(v);
	}
	
	/* headcrc */
	if((rc = dcf_crc16_read(dcf, &crc))) {
		fprintf(stderr, "dcf_crc16_read failed: %d\n", rc);
		return -1;
	}
	if(crc_pop(&crc, CRC16))
		return -1;

	fprintf(stderr, "headcrc matched\n");
	
	/* meta */
	while(1) {
		int size;
                char *v;
		unsigned char *buf;
                struct bigint b_identsize, b_contentsize;
		int identsize, contentsize;

		crc_push(&crc, CRC16);
		
		/* identsize */
		if(dcf_varint_size_read(dcf, &crc, &size))
                        return -1;
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&b_identsize, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &b_identsize, size))
                        return -1;
                if(bigint_toint(&identsize, &b_identsize))
			return -1;
		printf("identsize = %d\n", identsize);
		free(v);

		if(identsize == 0) {
			if(crc_pop(&crc, CRC16))
				return -1;
			break;
		}

		/* identifier */
		buf = malloc(identsize);
		dcf_data_read(dcf, &crc, identsize, buf);
		fprintf(stderr, "identifier: '%s'\n", buf);
		
		/* contentsize */
		if(dcf_varint_size_read(dcf, &crc, &size))
                        return -1;
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&b_contentsize, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &b_contentsize, size))
                        return -1;
                if(bigint_toint(&contentsize, &b_contentsize))
			return -1;
		printf("contentsize = %d\n", contentsize);
		free(v);

		/* content */
		buf = malloc(contentsize);
		if(dcf_data_read(dcf, &crc, contentsize, buf))
			return -1;
		fprintf(stderr, "content: '%s'\n", buf);

		/* metaitemcrc */
		if((rc = dcf_crc16_read(dcf, &crc))) {
			fprintf(stderr, "dcf_crc16_read failed: %d\n", rc);
			return -1;
		}
		if(crc_pop(&crc, CRC16))
			return -1;
	}

	{
		unsigned int val;
		
		/* padsize */
		if(dcf_uint16_read(dcf, &crc, &val))
			return -1;
		fprintf(stderr, "padsize = %u\n", val);
		/* padding */
		if(dcf_data_read(dcf, &crc, val, (void*)0))
			return -1;
	}

	/* data_magic */
	{
		unsigned char buf[4];
		if(dcf_data_read(dcf, &crc, 4, buf))
			return -1;
		if(memcmp(buf, DCF_DATA_MAGIC, 4))
			return -1;
		fprintf(stderr, "Datamagic: OK\n");
	}

	/* data_section_pos. */
	{
		int size, pos, rpos;
                char *v;
                struct bigint bi;

		if(dcf_varint_size_read(dcf, &crc, &size)) {
			fprintf(stderr, "dcf_varint_size_read failed\n");
                        return -1;
		}
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&bi, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &bi, size)) {
			fprintf(stderr,"dcf_varint_value_read(%d) failed\n", size);
                        return -1;
		}
                if(bigint_toint(&rpos, &bi))
			return -1;
		printf("read pos = %d\n", rpos);
		bigint_toint(&pos, &dcf->recordsize);
		printf("my pos = %d\n", pos);
		if(pos != rpos)
			return -1;
		free(v);
	}

	/* data */
	crc_push(&crc, CRC32);
	
	while(1) {
		int size, datasize, segmentid, padsize;
                char *v;
                struct bigint bi;

		crc_push(&crc, CRC32);
		
		/* datasize */
		if(dcf_varint_size_read(dcf, &crc, &size)) {
			fprintf(stderr, "dcf_varint_size_read failed\n");
                        return -1;
		}
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&bi, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &bi, size)) {
			fprintf(stderr,"dcf_varint_value_read(%d) failed\n", size);
                        return -1;
		}
                if(bigint_toint(&datasize, &bi))
			return -1;
		printf("datasize = %d\n", datasize);
		free(v);
		
		if(datasize == 0) {
			if(crc_pop(&crc, CRC32))
				return -1;
			break;
		}

		/* segmentid */
		if(dcf_varint_size_read(dcf, &crc, &size)) {
			fprintf(stderr, "dcf_varint_size_read failed\n");
                        return -1;
		}
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&bi, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &bi, size)) {
			fprintf(stderr,"dcf_varint_value_read(%d) failed\n", size);
                        return -1;
		}
                if(bigint_toint(&segmentid, &bi))
			return -1;
		printf("segmentid = %d\n", segmentid);
		free(v);

		/* padsize */
		if(dcf_varint_size_read(dcf, &crc, &size)) {
			fprintf(stderr, "dcf_varint_size_read failed\n");
                        return -1;
		}
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&bi, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &bi, size)) {
			fprintf(stderr,"dcf_varint_value_read(%d) failed\n", size);
                        return -1;
		}
                if(bigint_toint(&padsize, &bi))
			return -1;
		printf("padsize = %d\n", padsize);
		free(v);
		
		/*  datasegment */
		{
			unsigned char *buf;
			buf = malloc(datasize);
			if(dcf_data_read(dcf, &crc, datasize, buf))
				return -1;
			fprintf(stderr, "datasegment: '%s'\n", buf);
			free(buf);
		}

		/* datasegmentcrc */
		if((rc = dcf_crc32_read(dcf, &crc))) {
			fprintf(stderr, "datasegmentcrc dcf_crc32_read failed: %d\n", rc);
			return -1;
		}
		if(crc_pop(&crc, CRC32))
			return -1;
	}

	/* datacrc */
	if((rc = dcf_crc32_read(dcf, &crc))) {
		fprintf(stderr, "datacrc dcf_crc32_read failed: %d\n", rc);
		return -1;
	}
	if(crc_pop(&crc, CRC32))
		return -1;

	/* signature */
	while(1) {
		int size;
                char *v;
		unsigned char *buf;
                struct bigint b_signaturetypesize, b_signaturesize;
		int signaturetypesize, signaturesize;

		crc_push(&crc, CRC16);
		
		/* signaturetypesize */
		if(dcf_varint_size_read(dcf, &crc, &size))
                        return -1;
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&b_signaturetypesize, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &b_signaturetypesize, size))
                        return -1;
                if(bigint_toint(&signaturetypesize, &b_signaturetypesize))
			return -1;
		printf("signaturetypesize = %d\n", signaturetypesize);
		free(v);

		if(signaturetypesize == 0) {
			if(crc_pop(&crc, CRC16))
				return -1;
			break;
		}

		/* signaturetype */
		buf = malloc(signaturetypesize);
		dcf_data_read(dcf, &crc, signaturetypesize, buf);
		fprintf(stderr, "signaturetype: '%s'\n", buf);
		
		/* signaturesize */
		if(dcf_varint_size_read(dcf, &crc, &size))
                        return -1;
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&b_signaturesize, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &b_signaturesize, size))
                        return -1;
                if(bigint_toint(&signaturesize, &b_signaturesize))
			return -1;
		printf("signaturesize = %d\n", signaturesize);
		free(v);

		/* signature */
		buf = malloc(signaturesize);
		if(dcf_data_read(dcf, &crc, signaturesize, buf))
			return -1;
		fprintf(stderr, "signature: '%s'\n", buf);

		/* sigcrc */
		if((rc = dcf_crc16_read(dcf, &crc))) {
			fprintf(stderr, "dcf_crc16_read failed: %d\n", rc);
			return -1;
		}
		if(crc_pop(&crc, CRC16))
			return -1;
	}


	/* padding */
	{
		unsigned int val;
		
		/* padsize */
		if(dcf_uint16_read(dcf, &crc, &val))
			return -1;
		fprintf(stderr, "padsize = %u\n", val);
		/* padding */
		if(dcf_data_read(dcf, &crc, val, (void*)0))
			return -1;
	}

	{
		int size, pos, rpos;
                char *v;
                struct bigint bi;

		if(dcf_varint_size_read(dcf, &crc, &size)) {
			fprintf(stderr, "dcf_varint_size_read failed\n");
                        return -1;
		}
                v = malloc(size);
		if(!v) return -1;
                bigint_load(&bi, v, size, 0);
                if(dcf_varint_value_read(dcf, &crc, &bi, size)) {
			fprintf(stderr,"dcf_varint_value_read(%d) failed\n", size);
                        return -1;
		}
                if(bigint_toint(&rpos, &bi))
			return -1;
		printf("read pos = %d\n", rpos);
		bigint_toint(&pos, &dcf->recordsize);
		printf("my pos = %d\n", pos);
		if(pos != rpos)
			return -1;
		free(v);
	}
	bigint_zero(&dcf->recordsize);
	goto loop;
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
	
	if(argc > 1 && !strcmp(argv[1], "-l")) {
		if(dcf_init(&dcf, 0, v, sizeof(v)))
			return 1;
		if(list(&dcf))
			return 1;
		return 0;
	}
	
	printf("dcfcmd -a|-l\n");
	return 0;
}
