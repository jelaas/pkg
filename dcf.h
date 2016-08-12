/*

Data Collection Format

Record:
 char dcf_magic[6]
 char collectiontype[4]; Subtype that uses DCF as a container 
 VARINT dcfversion
 VARINT collectionid This way we can multiplex records from several collections with the same collectiontype
 meta: (0|VARINT identsize, OCTETS identifier, VARINT contentsize, OCTETS content)  Repeated until identsize is 0 which marks the end of metadata.
 VARINT datasize
 OCTETS checksum[32] (meta) -- SHA256 checksum from magic upto and including datasize
 [data]
 OCTETS checksum[32] (data) -- SHA256 checksum of data

 VARINT = NUMINTBYTES INTBYTELOW .. INTBYTEHIGH

 */
#include "sha256.h"
#include "bigint.h"

struct dcf {
  int fd;
  char collectiontype[4];
  struct sha256_ctx sha256; /* initialized by dcf_init and dcf_checksum_read|write */
};

#define DCF_MAGIC "%DCF_%"
#define DCF_VERSION 1

int dcf_init(struct dcf *dcf, int fd, const char *collectiontype);

int dcf_magic_read(struct dcf *dcf);
int dcf_collectiontype_read(struct dcf *dcf);
int dcf_varint_size_read(struct dcf *dcf); /* returns number bytes in varint */
int dcf_varint_value_read(struct dcf *dcf, struct bigint *i);
int dcf_meta_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *data);
int dcf_checksum_read(struct dcf *dcf);

int dcf_magic_write(struct dcf *dcf);
int dcf_collectiontype_write(struct dcf *dcf);
int dcf_varint_write(struct dcf *dcf, struct bigint *i);
int dcf_meta_write(struct dcf *dcf, int identsize, const char *ident, int contentsize, const char *content);
int dcf_meta_write_final(struct dcf *dcf);
int dcf_data_write(struct dcf *dcf, const char *buf, int size);
int dcf_checksum_write(struct dcf *dcf, char *chksum); /* copy of checksum also written to chksum */
