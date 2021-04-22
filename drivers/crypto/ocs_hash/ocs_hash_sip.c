// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019, Intel Corporation
 */

#include <hw_sha.h>
#include <asm/arch/sip_svc.h>
#include <asm/arch/kmb_shared_mem.h>
#include <dm.h>

/*
 * Retrieve an area that can be shared with the secure world for
 * communication.
 */
static void __iomem *get_shmem_ptr(const char *name, size_t min_size)
{
	int offset = 0;
	fdt_addr_t shmem_addr;
	fdt_size_t shmem_size;

	offset = fdt_subnode_offset(gd->fdt_blob, 0, name);
	if (offset < 0) {
		pr_err("Couldn't find %s shmem.\n", name);
		return NULL;
	}

	offset = fdtdec_lookup_phandle(gd->fdt_blob, offset, "shmem");
	if (offset < 0) {
		pr_err("No shared memory reserved.\n");
		return NULL;
	}

	shmem_addr = fdtdec_get_addr_size_auto_parent(gd->fdt_blob,
						      0, offset, "reg",
						      0, &shmem_size, true);
	if (shmem_addr == FDT_ADDR_T_NONE) {
		pr_err("No memory found.\n");
		return NULL;
	}

	if (min_size > shmem_size) {
		return NULL;
	}

	return (void __iomem *)shmem_addr;
}

void hw_sha256(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size)
{
	unsigned char *result = get_shmem_ptr("ocs_hash_result", SHA256_SIZE);

	if (!result) {
		printf("Getting shmem for result buffer failed.\n");
		return;
	}

	if (sip_svc_hash_simple(OCS_HASH_SHA256, (uintptr_t)pbuf,
				buf_len,  (uintptr_t)result, SHA256_SIZE)) {
		printf("Secure OCS hash failed.\n");
		return;
	}

	memcpy(pout, result, SHA256_SIZE);
	memset(result, 0x0, SHA256_SIZE);
}

void hw_sha384(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size)
{
	unsigned char *result = get_shmem_ptr("ocs_hash_result", SHA384_SIZE);

	if (!result) {
		printf("Getting shmem for result buffer failed.\n");
		return;
	}

	if (sip_svc_hash_simple(OCS_HASH_SHA384, (uintptr_t)pbuf,
				buf_len,  (uintptr_t)result, SHA384_SIZE)) {
		printf("Secure OCS hash failed.\n");
		return;
	}

	memcpy(pout, result, SHA384_SIZE);
	memset(result, 0x0, SHA384_SIZE);
}

void hw_sha512(const unsigned char *pbuf, unsigned int buf_len,
	       unsigned char *pout, unsigned int chunk_size)
{
	unsigned char *result = get_shmem_ptr("ocs_hash_result", SHA512_SIZE);

	if (!result) {
		printf("Getting shmem for result buffer failed.\n");
		return;
	}

	if (sip_svc_hash_simple(OCS_HASH_SHA512, (uintptr_t)pbuf,
				buf_len,  (uintptr_t)result, SHA512_SIZE)) {
		printf("Secure OCS hash failed.\n");
		return;
	}

	memcpy(pout, result, SHA512_SIZE);
	memset(result, 0x0, SHA512_SIZE);
}

int hw_sha_init(struct hash_algo *algo, void **ctxp)
{
	unsigned char *ctx = get_shmem_ptr("ocs_hash_ctx",
					   sizeof(struct
						  ocs_hash_resume_context_t));

	if (!ctx) {
		printf("Getting shmem for context failed.\n");
		return -EINVAL;
	}

	memset(ctx, 0x0, sizeof(struct ocs_hash_resume_context_t));
	*ctxp = (void *)ctx;

	return 0;
}

static int get_ocs_hash_algo(struct hash_algo *algo, ocs_hash_alg_t *ocs_alg)
{
	if (!strcmp(algo->name, "sha256")) {
		*ocs_alg = OCS_HASH_SHA256;
		return 0;
	}

	if (!strcmp(algo->name, "sha384")) {
		*ocs_alg = OCS_HASH_SHA384;
		return 0;
	}

	if (!strcmp(algo->name, "sha512")) {
		*ocs_alg = OCS_HASH_SHA512;
		return 0;
	}

	return -EINVAL;
}

static int get_sha_block_size(struct hash_algo *algo)
{
	if (!strcmp(algo->name, "sha256")) {
		return SHA1_BLOCK_SIZE;
	}

	if (!strcmp(algo->name, "sha384")) {
		return SHA512_BLOCK_SIZE;
	}

	if (!strcmp(algo->name, "sha512")) {
		return SHA512_BLOCK_SIZE;
	}

	return 0;
}

/*
 * Hash middle update function,
 *
 * If this function is called, we know that the current residue is empty,
 * so call sip_svc_hash_resume_middle on as many blocks of the data as we
 * can, and store off the remainder, if any, in the residue buffer.
 */
static int do_hash_middle_update(ocs_hash_alg_t algo,
				 struct ocs_hash_resume_context_t *ctxp,
				 u8 *buf, unsigned int size,
				 int ocs_block_size)
{
	int len = size % ocs_block_size;
	int loop = size / ocs_block_size;
	u8 *data;
	int rc = 0;

	if (loop) {
		rc = sip_svc_hash_resume_middle(algo, (uintptr_t)buf,
						(ocs_block_size * loop),
						(uintptr_t)ctxp);
		if (rc) {
			return rc;
		}
	}

	data = buf + (ocs_block_size * loop);

	/* the residue buffer has already been cleared, so just directly
	 * memcpy to buffer.
	 */
	if (len) {
		memcpy(ctxp->residue.buffer, data, len);
	}

	ctxp->residue.len = len;

	return rc;
}

/*
 * The OCS HW resume_first() and resume_middle() require input of multiple
 * of block size to be operated.
 * e.g.
 *	SHA256: block size 64 bytes is expected,
 *	SHA384 & SHA512: block size 128 bytes is expected.
 *
 * 1. If input size is less than a block size, store it into ctx buffer.
 * 2. If input size is more than a block size:
 *	- if there is data in ctx buffer, amend new input data to ctx residue
 *	buffer, call resume_first() on the first block size msg and
 *	process resume_middle() for the rest.
 *	- if there no data in ctx buffer, call resume_first() with multiple of
 *	block size, and store the rest into ctx residue buffer.
 */
static int do_hash_first(ocs_hash_alg_t algo,
			 struct ocs_hash_resume_context_t *ctxp,
			 u8 *buf, unsigned int size, int ocs_block_size)
{
	int rc = 0;
	unsigned char *data;
	int len, loop;

	/* Calculate the total size of data need to be processed */
	int msg_size = ctxp->residue.len + size;

	/* Total input data size smaller than a block size */
	if (msg_size < ocs_block_size) {
		/* copy to the ctx temp residue buffer */
		memcpy(&ctxp->residue.buffer[ctxp->residue.len], buf, size);
		ctxp->residue.len += size;

		return 0;
	}

	/* Total input data size bigger than a block size and ctx residue buffer
	 * lenghth is not zero.
	 */
	if (ctxp->residue.len) {
		int update_size = ocs_block_size - ctxp->residue.len;

		memcpy(&ctxp->residue.buffer[ctxp->residue.len], buf,
		       update_size);

		rc = sip_svc_hash_resume_first(algo,
					       (uintptr_t)ctxp->residue.buffer,
					       ocs_block_size,
					       (uintptr_t)ctxp);
		if (rc) {
			printf("%s: sip_svc_hash_resume_first failed\n",
			       __func__);
			return rc;
		}

		size -= update_size;
		data = (unsigned char *)buf + update_size;

		if (size) {
			/* Do hash middle update for the rest msg size */
			rc = do_hash_middle_update(algo, ctxp,
						   (u8 *)data, size,
						   ocs_block_size);
		}

		return rc;
	}

	/* Total input data size bigger than a block size and ctx residue buffer
	 * length is 0
	 */
	len = msg_size % ocs_block_size;
	loop = msg_size / ocs_block_size;

	if (loop) {
		rc = sip_svc_hash_resume_first(algo, (uintptr_t)buf,
					       (ocs_block_size * loop),
					       (uintptr_t)ctxp);
		if (rc) {
			printf("%s: hash resume first func failed\n",
			       __func__);
			return rc;
		}
	}

	data = (unsigned char *)buf + (ocs_block_size * loop);
	memcpy(ctxp->residue.buffer, data, len);
	ctxp->residue.len = len;

	return rc;
}

/*
 * Update ctx temp residue buffer with newly input message.
 *
 * 1. Store all input data in residue buffer if newly input length
 * less than the offset between residue buffer len and block size.
 * Return 0 as no data has been input into hash middle function.
 *
 * 2. Otherwise fill up the residue buffer to the block size with
 * newly input data and run middle update. Return the length of msg
 * offset that hash been input into hash middle function.
 */
static int update_ctx_residue(ocs_hash_alg_t algo,
			      struct ocs_hash_resume_context_t *ctxp,
			      u8 *buf, unsigned int size,
			      int ocs_block_size)
{
	int rc = 0;
	int msg_buff_offset = ocs_block_size - (ctxp->residue.len);

	/* Store the input message to residue buffer if length fits in offset */
	if (size < msg_buff_offset) {
		memcpy(&ctxp->residue.buffer[ctxp->residue.len], buf, size);
		ctxp->residue.len += size;
		return 0;
	}

	memcpy(&ctxp->residue.buffer[ctxp->residue.len], buf, msg_buff_offset);
	/* residue.len will be set to zero in do_hash_middle_update() */
	ctxp->residue.len += msg_buff_offset;
	/* do middle update on residue buffer */
	rc = do_hash_middle_update(algo, ctxp, ctxp->residue.buffer,
				   ocs_block_size,
				   ocs_block_size);
	if (rc) {
		printf("Hash middle update failed!\n");
		return 0;
	}
	return msg_buff_offset;
}

int hw_sha_update(struct hash_algo *algo, void *ctx, const void *buf,
		  unsigned int size, int is_last)
{
	int rc = 0;
	int len_updated = 0;
	u8 *data;
	ocs_hash_alg_t ocs_alg;
	struct ocs_hash_resume_context_t *ctxp;

	int ocs_block_size = get_sha_block_size(algo);

	if (!ocs_block_size) {
		printf("Algorithm is not supported.\n");
		return -EINVAL;
	}

	rc = get_ocs_hash_algo(algo, &ocs_alg);
	if (rc) {
		printf("Algorithm is not supported.\n");
		return -EINVAL;
	}

	ctxp = (struct ocs_hash_resume_context_t *)ctx;

	if (!ctxp->inter.msg_len_lo) {
		rc = do_hash_first(ocs_alg, ctxp, (u8 *)buf,
				   size, ocs_block_size);
		return rc;
	}

	/* do hash middle directly if no data in residue buffer */
	if (!ctxp->residue.len) {
		rc = do_hash_middle_update(ocs_alg, ctxp, (u8 *)buf,
					   size, ocs_block_size);
		if (rc) {
			printf("Hash middle update failed!\n");
		}

		return rc;
	}

	/* update_ctx_msg will call hash middle if input size + ctx
	 * residue length greater than ocs_block_size, otherwise return zero
	 */
	len_updated = update_ctx_residue(ocs_alg, ctxp, (u8 *)buf,
					 size, ocs_block_size);

	if (len_updated) {
		data = (u8 *)buf + len_updated;
		/* Check to see if more data need to be updated */
		rc = do_hash_middle_update(ocs_alg, ctxp, data,
					   (size - len_updated),
					   ocs_block_size);
	}

	return rc;
}

int hw_sha_finish(struct hash_algo *algo, void *ctx, void *dest_buf, int size)
{
	int rc = 0;
	struct ocs_hash_resume_context_t *ctxp;
	unsigned char *result;
	ocs_hash_alg_t ocs_alg;

	rc = get_ocs_hash_algo(algo, &ocs_alg);
	if (rc) {
		printf("Algorithm is not supported.\n");
		return -EINVAL;
	}

	ctxp = (struct ocs_hash_resume_context_t *)ctx;

	result = get_shmem_ptr("ocs_hash_result", size);
	if (!result) {
		printf("Getting shmem for result buffer failed.\n");
		return -EINVAL;
	}

	/* No other hashing done, just call simple hash */
	if (!ctxp->inter.msg_len_lo) {
		rc = sip_svc_hash_simple(ocs_alg,
					 (uintptr_t)ctxp->residue.buffer,
					 ctxp->residue.len,
					 (uintptr_t)result,
					 size);
	} else {
		rc = sip_svc_hash_resume_last(ocs_alg, (uintptr_t)result,
					      (uintptr_t)ctxp, size);
	}

	if (!rc) {
		memcpy(dest_buf, result, size);
	} else {
		printf("Hash finish failed!\n");
	}

	memset(result, 0x0, size);
	/* Zero the ctx and result, as they reserved in shmem */
	memset(ctxp, 0x0, sizeof(struct ocs_hash_resume_context_t));

	return rc;
}
