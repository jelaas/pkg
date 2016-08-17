#include <stdio.h>
#include <string.h>
#include "dcf.h"

struct {
	int collectionid;
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
	int n;
	char buf[512];
	
	if(dcf_magic_write(dcf))
		return -1;
	if(dcf_collectiontype_write(dcf))
		return -1;
	
	if(bigint_loadi(&version, version_v, sizeof(version_v), DCF_VERSION))
		return -1;
	if(dcf_varint_write(dcf, &version))
		return -1;
	
	if(bigint_loadi(&collectionid, collectionid_v, sizeof(collectionid_v), conf.collectionid))
		return -1;
	if(dcf_varint_write(dcf, &collectionid))
		return -1;

	fprintf(stderr, "dcf_meta_write\n");
	if(dcf_meta_write(dcf, strlen(conf.meta.name), conf.meta.name, strlen(conf.meta.content), conf.meta.content))
		return -1;
	if(dcf_meta_write_final(dcf))
		return -1;

	if(dcf_checksum_write(dcf, (void*)0))
		return -1;
	
	while((n=read(0, buf, sizeof(buf)))>0) {
		if(dcf_data_write(dcf, buf, n))
			return -1;
	}
	
	if(dcf_data_write_final(dcf))
		return -1;
	if(dcf_checksum_write(dcf, (void*)0))
		return -1;
	if(dcf_signature_write(dcf, "sigge", 5, "STARDUST", 8))
		return -1;
	if(dcf_recordsize_write(dcf, (void*)0))
		return -1;
	
	return 0;
}

int list(struct dcf *dcf)
{
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
		return append(&dcf);
	}
	
	if(dcf_init(&dcf, 0, v, sizeof(v)))
		return 1;
	
	list(&dcf);
	
	return 2;
}
