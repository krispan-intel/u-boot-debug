// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2018-2019, Intel
 *
 * This driver interfaces with OPTEE client API to communicate with
 * a TPM2 Trusted Service within OP-TEE Secure World OS.
 */

#include <common.h>
#include <dm.h>
#include <log.h>
#include <tee.h>
#include <tpm-v2.h>

/* Size of external transmit buffer */
#define TPM_BUFSIZE 4096
#define TPM_DESC_STR "TPM fTPM2.x"

#define TA_FTPM_UUID		      \
	{ 0xbc50d971, 0xd4c9, 0x42c4, \
	  { 0x82, 0xcb, 0x34, 0x3F, 0xb7, 0xf3, 0x78, 0x96 } }

enum fwtpm_optee_cmd {
	FTPM_HANDLE_CMD_SUBMIT  = 0,
	FTPM_HANDLE_PPI         = 1,
	FTPM_HANDLE_PM          = 2,
};

struct tpm_output_header {
	__be16 tag;
	__be32 length;
	__be32 return_code;
} __packed;

struct fwtpm_device_priv {
	struct udevice *tee;
	u32 session;
	struct tee_shm *cmd_buf_shm;
	struct tee_shm *resp_buf_shm;
	char *cmd_buf;
	char *resp_buf;
	size_t cmd_len;
	size_t resp_len;
};

/**
 * struct fwtpm_chip_data - Non-discoverable TPM information
 *
 * @pcr_count:          Number of PCR per bank
 * @pcr_select_min:     Size in octets of the pcrSelect array
 */
struct fwtpm_chip_data {
	unsigned int pcr_count;
	unsigned int pcr_select_min;
	unsigned int time_before_first_cmd_ms;
};

static int get_open_session(struct fwtpm_device_priv *tpm)
{
	struct udevice *tee = NULL;

	if (!tpm->tee) {
		const struct tee_optee_ta_uuid uuid = TA_FTPM_UUID;
		struct tee_open_session_arg arg;
		int rc;

		tee = tee_find_device(tee, NULL, NULL, NULL);
		if (!tee) {
			return -ENODEV;
		}

		memset(&arg, 0, sizeof(arg));
		tee_optee_ta_uuid_to_octets(arg.uuid, &uuid);

		rc = tee_open_session(tee, &arg, 0, NULL);
		if (!rc) {
			tpm->tee = tee;
			tpm->session = arg.session;
		} else {
			return rc;
		}

		rc = tee_shm_alloc(tee, TPM_BUFSIZE,
				   TEE_SHM_ALLOC,
				   &tpm->cmd_buf_shm);
		if (!rc) {
			tpm->cmd_buf = tpm->cmd_buf_shm->addr;
		} else {
			return -ENOMEM;
		}

		rc = tee_shm_alloc(tee, TPM_BUFSIZE,
				   TEE_SHM_ALLOC,
				   &tpm->resp_buf_shm);
		if (!rc) {
			tpm->resp_buf = tpm->resp_buf_shm->addr;
		} else {
			return -ENOMEM;
		}

		return rc;
	}

	return 0;
}

static int invoke_func(struct fwtpm_device_priv *tpm, u32 func,
		       ulong num_param, struct tee_param *param)
{
	struct tee_invoke_arg arg;

	memset(&arg, 0, sizeof(arg));
	arg.func = func;
	arg.session = tpm->session;

	if (tee_invoke_func(tpm->tee, &arg, num_param, param)) {
		return -1;
	}

	return 0;
}

static int fwtpm_xfer(struct udevice *dev, const u8 *sendbuf,
		      size_t send_size, u8 *recvbuf,
		      size_t *recv_len)
{
	int rc;
	struct fwtpm_device_priv *tpm = dev_get_priv(dev);
	struct tee_param param[2];
	__be32 len;

	if (!tpm) {
		return -ENOMEM;
	}

	log(LOGC_NONE, LOGL_DEBUG,
	    "%s: Sending Command Buffer to fTPM device\n", __func__);

	if (get_open_session(tpm)) {
		return -1;
	}

	log(LOGC_NONE, LOGL_DEBUG,
	    "%s: send_size= %d, recv_len=%d\n", __func__, send_size, *recv_len);

	if (send_size > TPM_BUFSIZE) {
		debug("Send TPM2 CMD buffer size larger than TPM_BUFSIZE\n");
		return -E2BIG;
	}

	tpm->cmd_len = send_size;
	memset(param, 0, sizeof(param));
	memcpy(tpm->cmd_buf, sendbuf, send_size);

	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = tpm->cmd_buf_shm;
	param[0].u.memref.size = TPM_BUFSIZE;
	param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INOUT;
	param[1].u.memref.shm = tpm->resp_buf_shm;
	param[1].u.memref.size = TPM_BUFSIZE;

	rc = invoke_func(tpm, FTPM_HANDLE_CMD_SUBMIT, ARRAY_SIZE(param), param);
	if (rc) {
		debug("Invoke TEE function failed\n");
		return rc;
	}

	len = ((struct tpm_output_header *)tpm->resp_buf)->length;
	tpm->resp_len = be32_to_cpu(len);

	if (tpm->resp_len > *recv_len) {
		debug("Response Buffer too large\n");
		return -E2BIG;
	}

	memcpy(recvbuf, tpm->resp_buf, tpm->resp_len);
	*recv_len = tpm->resp_len;
	tpm->resp_len = 0;

	return 0;
}

static int fwtpm_get_desc(struct udevice *dev, char *buf, int size)
{
	if (size < sizeof(TPM_DESC_STR)) {
		return -ENOSPC;
	}

	return snprintf(buf, size, TPM_DESC_STR);
}

static int fwtpm_open(struct udevice *dev)
{
	struct fwtpm_device_priv *tpm = dev_get_priv(dev);

	if (!tpm) {
		return -ENOMEM;
	}

	if (get_open_session(tpm)) {
		return -1;
	}

	return 0;
}

static int fwtpm_probe(struct udevice *dev)
{
	struct fwtpm_chip_data *drv_data = (void *)dev_get_driver_data(dev);
	struct fwtpm_device_priv *tpm = dev_get_priv(dev);
	struct tpm_chip_priv *priv = dev_get_uclass_priv(dev);

	if (!drv_data || !tpm || !priv) {
		return -ENOMEM;
	}

	/* Use the TPM v2 stack */
	priv->version = TPM_V2;

	priv->pcr_count = drv_data->pcr_count;
	priv->pcr_select_min = drv_data->pcr_select_min;

	memset(tpm, 0, sizeof(*tpm));

	log(LOGC_NONE, LOGL_DEBUG,
	    "%s: Probing fTPM device\n", __func__);

	if (get_open_session(tpm)) {
		return -1;
	}

	return 0;
}

static int fwtpm_close(struct udevice *dev)
{
	struct fwtpm_device_priv *tpm = dev_get_priv(dev);

	if (!tpm) {
		return -ENOMEM;
	}

	log(LOGC_NONE, LOGL_DEBUG,
	    "%s: Closing connection to fTPM\n", __func__);

	if (!tpm->tee) {
		return -ENODEV;
	}

	if (tee_close_session(tpm->tee, tpm->session)) {
		return -1;
	}

	return 0;
}

static const struct fwtpm_chip_data fwtpm_std_chip_data = {
	.pcr_count = 24,
	.pcr_select_min = 3,
	.time_before_first_cmd_ms = 30,
};

static const struct tpm_ops fwtpm_ops = {
	.open = fwtpm_open,
	.close = fwtpm_close,
	.get_desc = fwtpm_get_desc,
	.xfer = fwtpm_xfer,
};

static const struct udevice_id fwtpm_ids[] = {
	{ .compatible = "microsoft,ftpm",
	  .data = (ulong) & fwtpm_std_chip_data, },
	{ }
};

U_BOOT_DRIVER(optee_fwtpm) = {
	.name = "optee_fwtpm",
	.id = UCLASS_TPM,
	.of_match = fwtpm_ids,
	.ops = &fwtpm_ops,
	.probe = fwtpm_probe,
	.priv_auto = sizeof(struct fwtpm_device_priv),
};
