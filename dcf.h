/*

Data Collection Format

Record:
 char dcf_magic[6]
 char collectiontype[4]; Subtype that uses DCF as a container 
 VARINT dcfversion
 VARINT collectionid This way we can multiplex records from several collections with the same collectiontype
 meta: (VARINT 0|VARINT identsize, OCTETS identifier, VARINT contentsize, OCTETS content)  Repeated until identsize or contentsize is 0 which marks the end of metadata.
 OCTETS checksum[32] (meta) -- SHA256 checksum from magic upto and including meta
 data: (VARINT 0|VARINT datasize, OCTETS data). Repeated until datasize is 0 which marks the end of data.
 signature: (VARINT 0|VARINT signaturetypesize, OCTETS signaturetype, VARINT signaturesize, OCTETS signature)
 OCTETS datachecksum[32] (data) -- SHA256 checksum of 'data' and 'signature' sections
 VARINT recordsize. Size of complete record from dcf_magic upto and including recordsizechecksum.
 OCTETS recordsizechecksum[32] (data) -- SHA256 checksum of recordsize
 
 VARINT = NUMINTBYTES INTBYTELOW .. INTBYTEHIGH .. NUMINTBYTES(repeated)

 */
#include "sha256.h"
#include "bigint.h"

struct dcf {
  int fd;
  char collectiontype[4];
  struct sha256_ctx sha256; /* initialized by dcf_init and dcf_checksum_read|write */
  struct bigint recordsize;
  struct bigint temp, temp2;
};

#define DCF_MAGIC "%DCF_%"
#define DCF_VERSION 1

int dcf_init(struct dcf *dcf, int fd, char *workbuf, int workbufsize); /* workbuf must be big enough to handle maximum recordsize of collection times three (64 bytes should be enough) */
int dcf_collectiontype_set(struct dcf *dcf, const char *collectiontype);

int dcf_magic_read(struct dcf *dcf);
int dcf_collectiontype_read(struct dcf *dcf);
int dcf_varint_size_read(struct dcf *dcf); /* returns number bytes in varint */
int dcf_varint_value_read(struct dcf *dcf, struct bigint *i);
int dcf_meta_data_read(struct dcf *dcf, int *datasize, int bufsize, char *buf); /* 0 ok. 1 = end-of-meta-data. < 0 on error */
int dcf_data_read(struct dcf *dcf, int *datasize, int bufsize, unsigned char *buf); /* 0 ok. 1 = end-of-data. < 0 on error */
int dcf_signature_read(struct dcf *dcf, int * typesize, int *sigsize, int typebufsize, char *typebuf, int sigbufsize, unsigned char *sigbuf); /* 0 ok. 1 = no-signature. < 0 on error */
int dcf_checksum_read(struct dcf *dcf); /* reads and checks checksum */
int dcf_recordsize_read(struct dcf *dcf, char *chksum); /* reads and checks recordsize and checksum of recordsize */

int dcf_magic_write(struct dcf *dcf);
int dcf_collectiontype_write(struct dcf *dcf);
int dcf_varint_write(struct dcf *dcf, struct bigint *i);
int dcf_meta_write(struct dcf *dcf, int identsize, const char *ident, int contentsize, const char *content);
int dcf_meta_write_final(struct dcf *dcf);
int dcf_data_write(struct dcf *dcf, const char *buf, int size);
int dcf_data_write_final(struct dcf *dcf);
int dcf_signature_write(struct dcf *dcf, const char *sigtype, int sigtypesize, const char *sig, int sigsize); /* sigtypesize == 0 means no signature */
int dcf_checksum_write(struct dcf *dcf, char *chksum); /* copy of checksum also written to chksum */
int dcf_recordsize_write(struct dcf *dcf, char *chksum); /* writes recordsize and checksum of recordsize. copy of checksum to chksum */
