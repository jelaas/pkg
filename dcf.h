/*

Data Collection Format

Record:
 OCTETS dcf_magic[6]
 OCTETS collectiontype[4]; Subtype that uses DCF as a container 
 VARINT dcfversion
 VARINT collectionid This way we can multiplex records from several collections with the same collectiontype
 meta: (VARINT 0|VARINT identsize, OCTETS identifier, VARINT contentsize, OCTETS content)  Repeated until identsize or contentsize is 0 which marks the end of metadata.
 OCTETS hash[32] (meta) -- SHA256 hash from magic upto and including 'meta' section
 data: (VARINT 0|VARINT datasize, OCTETS data). Repeated until datasize is 0 which marks the end of data.
 OCTETS datahash[32] (data) -- SHA256 hash of 'data' section
 signature: (VARINT 0|VARINT signaturetypesize, OCTETS signaturetype, VARINT signaturesize, OCTETS signature, OCTETS sighash[32]). Repeated until signaturetypesize is 0.
 VARINT recordsize. Size of complete record from dcf_magic upto and including recordsizehash.
 OCTETS recordsizehash[32] (data) -- SHA256 hash of recordsize
 
 VARINT = NUMINTBYTES INTBYTELOW .. INTBYTEHIGH .. NUMINTBYTES(repeated)

 */
#include "sha256.h"
#include "bigint.h"

struct dcf {
  int fd;
  char collectiontype[4];
  struct sha256_ctx sha256; /* initialized by dcf_init and dcf_hash_read|write */
  struct bigint recordsize;
  struct bigint temp, temp2;
};

#define DCF_MAGIC "%DCF_%"
#define DCF_VERSION 1

int dcf_init(struct dcf *dcf, int fd, char *workbuf, int workbufsize); /* workbuf must be big enough to handle maximum recordsize of collection times three (64 bytes should be enough) */
int dcf_collectiontype_set(struct dcf *dcf, const char *collectiontype);

int dcf_magic_read(struct dcf *dcf);
int dcf_collectiontype_read(struct dcf *dcf);
int dcf_varint_size_read(struct dcf *dcf, int *size);
int dcf_varint_value_read(struct dcf *dcf, struct bigint *i, int size);
int dcf_data_read(struct dcf *dcf, int datasize, unsigned char *buf);
int dcf_signature_read(struct dcf *dcf, int * typesize, int *sigsize, int typebufsize, char *typebuf, int sigbufsize, unsigned char *sigbuf); /* 0 ok. 1 = no-signature. < 0 on error */
int dcf_hash_read(struct dcf *dcf); /* reads and checks hash */
int dcf_recordsize_read(struct dcf *dcf, char *hash); /* reads and checks recordsize and hash of recordsize */

int dcf_magic_write(struct dcf *dcf);
int dcf_collectiontype_write(struct dcf *dcf);
int dcf_varint_write(struct dcf *dcf, struct bigint *i);
int dcf_meta_write(struct dcf *dcf, int identsize, const char *ident, int contentsize, const char *content);
int dcf_meta_write_final(struct dcf *dcf);
int dcf_data_write(struct dcf *dcf, const char *buf, int size);
int dcf_data_write_final(struct dcf *dcf);
int dcf_signature_write(struct dcf *dcf, const char *sigtype, int sigtypesize, const char *sig, int sigsize, char *hash); /* copy of hash to hash */
int dcf_signature_write_final(struct dcf *dcf);
int dcf_hash_write(struct dcf *dcf, char *hash); /* copy of hash also written to hash */
int dcf_recordsize_write(struct dcf *dcf, char *hash); /* writes recordsize and hash of recordsize. copy of hash to hash */
