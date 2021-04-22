// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (R) 2019 Intel Corporation
 */

#include "crypto-host.h"

#include <stdarg.h>

#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

/* Struct to unify the loading of public and private keys. */
struct key_type_hlpr {
	const char *key_type;

	EVP_PKEY *(*engine_load_key)(ENGINE *e, const char *key_id,
				     UI_METHOD *ui_method, void *callback_data);
	EVP_PKEY *(*pem_read_key)(FILE *fp, EVP_PKEY **x, pem_password_cb *cb,
				  void *u);
};

/* Helper functions for loading private keys. */
static const struct key_type_hlpr privk_hlpr = {
	.key_type = "private",
	.engine_load_key = ENGINE_load_private_key,
	.pem_read_key = PEM_read_PrivateKey,
};

/* Helper functions for loading public keys. */
static const struct key_type_hlpr pubk_hlpr = {
	.key_type = "public",
	.engine_load_key = ENGINE_load_public_key,
	.pem_read_key = PEM_read_PUBKEY,
};

/* Function to be called to notify the user of an OpenSSL-related error. */
int crypto_host_err(const char *fmt, ...)
{
	unsigned long sslErr = ERR_get_error();
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, ": %s\n",
		ERR_error_string(sslErr, 0));
	va_end(args);

	return -1;
}

static int crypto_host_init(void)
{
	int ret;

#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
		(defined(LIBRESSL_VERSION_NUMBER) && \
		 LIBRESSL_VERSION_NUMBER < 0x02070000fL)
	ret = SSL_library_init();
#else
	ret = OPENSSL_init_ssl(0, NULL);
#endif
	if (!ret) {
		fprintf(stderr, "Failure to init SSL library\n");
		return -1;
	}
#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
		(defined(LIBRESSL_VERSION_NUMBER) && \
		 LIBRESSL_VERSION_NUMBER < 0x02070000fL)
	SSL_load_error_strings();

	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_digests();
	OpenSSL_add_all_ciphers();
#endif

	return 0;
}

static void crypto_host_deinit(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
		(defined(LIBRESSL_VERSION_NUMBER) && \
		 LIBRESSL_VERSION_NUMBER < 0x02070000fL)
	CRYPTO_cleanup_all_ex_data();
	ERR_free_strings();
#ifdef HAVE_ERR_REMOVE_THREAD_STATE
	ERR_remove_thread_state(NULL);
#else
	ERR_remove_state(0);
#endif
	EVP_cleanup();
#endif
}

/**
 * ecdsa_engine_get_key() - Read a key from the given engine.
 * @keydir:	Key prefix.
 * @name:	Name of key.
 * @engine:	Engine to use.
 * @evp_keyp:	Returns EVP_PKEY object, or NULL on failure.
 * @hlpr:	Either 'privk_hlpr' or 'pubk_hlpr', depending on which key
 *		(private or public) should be read.
 *
 * Return:	0 on success, -ve on error (in which case *evp_keyp is set to
 *		NULL)
 */
static int crypto_host_engine_get_key(const char *keydir, const char *name,
				      ENGINE *engine, EVP_PKEY **evp_keyp,
				      const struct key_type_hlpr *hlpr)
{
	const char *engine_id;
	char key_id[1024];

	engine_id = ENGINE_get_id(engine);

	if (!engine_id || strcmp(engine_id, "pkcs11")) {
		fprintf(stderr, "Engine not supported\n");
		return -ENOTSUP;
	}

	if (keydir)
		snprintf(key_id, sizeof(key_id),
			 "pkcs11:%s;object=%s;type=%s", keydir, name,
			 hlpr->key_type);
	else
		snprintf(key_id, sizeof(key_id),
			 "pkcs11:object=%s;type=%s", name, hlpr->key_type);

	*evp_keyp = hlpr->engine_load_key(engine, key_id, NULL, NULL);
	if (!evp_keyp)
		return crypto_host_err("Failure loading %s key from engine",
				  hlpr->key_type);

	return 0;
}

/**
 * ecdsa_pem_get_key() - Read a private key from a PEM file.
 * @keydir:	Directory containing the key.
 * @name:	Name of key file (will have a .key extension).
 * @evp_keyp:	Used to store the read key, or NULL on failure.
 * @hlpr:	Either 'privk_hlpr' or 'pubk_hlpr', depending on which key
 *		(private or public) should be read.
 *
 * Return:	0 on success, -ve on error (in which case *evp_keyp will be
 *		set to NULL).
 */
static int crypto_host_pem_get_key(const char *keydir, const char *name,
				   EVP_PKEY **evp_keyp,
				   const struct key_type_hlpr *type_hlpr)
{
	char path[1024];
	FILE *f;
	int rc = 0;

	*evp_keyp = NULL;

	snprintf(path, sizeof(path), "%s/%s.key", keydir, name);
	f = fopen(path, "r");
	if (!f) {
		fprintf(stderr, "Couldn't open %s key: '%s': %s\n",
			type_hlpr->key_type, path, strerror(errno));
		return -ENOENT;
	}

	*evp_keyp = PEM_read_PrivateKey(f, NULL, NULL, NULL);
	if (!*evp_keyp) {
		crypto_host_err("Failure reading %s key", type_hlpr->key_type);
		rc = -EPROTO;
	}
	fclose(f);
	return rc;
}

/**
 * crypto_host_get_key() - Read a key from an engine or a PEM file.
 *
 * @keydir:	Directory containing the key (PEM file) or key prefix (engine).
 * @name	Name of key.
 * @engine	Engine to use for reading the key; if NULL, the key will be
 *		read from a PEM file.
 * @evp_keyp	Returns an EVP key object, or NULL on failure.
 * @hlpr:	Either 'privk_hlpr' or 'pubk_hlpr', depending on which key
 *		(private or public) should be read.
 *
 * Return:	0 on success, -ve on error (in which case *evp_keyp will be
 *		set to NULL).
 */
static int crypto_host_get_key(const char *keydir, const char *name,
			       ENGINE *engine, EVP_PKEY **evp_keyp,
			       const struct key_type_hlpr *key_hlpr)
{
	const struct key_type_hlpr *type_hlpr = &privk_hlpr;
	/* Engine case. */
	if (engine)
		return crypto_host_engine_get_key(keydir, name, engine,
						  evp_keyp, type_hlpr);
	/* PEM file case. */
	return crypto_host_pem_get_key(keydir, name, evp_keyp, type_hlpr);
}

/**
 * crypto_host_get_priv_key() - Read a private key from an engine or a PEM file.
 *
 * @keydir:	Directory containing the key (PEM file) or key prefix (engine).
 * @name	Name of key.
 * @engine	Engine to use for reading the key; if NULL, it means that the
 *		key must be read from a PEM file.
 * @evp_privkp	Returns an EVP key object, or NULL on failure.
 *
 * Return:	0 on success, -ve on error (in which case *evp_privkp will be
 *		set to NULL).
 */
static int crypto_host_get_priv_key(const char *keydir, const char *name,
				    ENGINE *engine, EVP_PKEY **evp_privkp)
{
	return crypto_host_get_key(keydir, name, engine, evp_privkp,
				   &privk_hlpr);
}

/**
 * crypto_host_get_pub_key() - Read a public key.
 *
 * @keydir:	Directory containing the key (PEM file) or key prefix (engine).
 * @name	Name of key.
 * @engine	Engine to use for reading the key; if NULL, it means that the
 *		key must be read from a PEM file.
 * @evp_pubkp	Returns an EVP key object, or NULL on failure.
 *
 * Return:	0 on success, -ve on error (in which case *evp_pubkp will be
 *		set to NULL).
 */
int crypto_host_get_pub_key(const char *keydir, const char *name,
			    ENGINE *engine, EVP_PKEY **evp_pubkp)
{
	return crypto_host_get_key(keydir, name, engine, evp_pubkp, &pubk_hlpr);
}

/* Generate signature using the specified EVP (private) key. */
static int crypto_host_sign_with_key(EVP_PKEY *priv_key,
				     struct checksum_algo *checksum_algo,
				     const struct image_region region[],
				     int region_count, uint8_t **sigp,
				     uint *sig_size)
{
	EVP_MD_CTX *context;
	int size, ret = 0;
	uint8_t *sig;
	int i;

	/* Get the maximum size of the signature (in bytes). */
	size = EVP_PKEY_size(priv_key);
	sig = malloc(EVP_PKEY_size(priv_key));
	if (!sig) {
		fprintf(stderr, "Out of memory for signature (%d bytes)\n",
			size);
		return -ENOMEM;
	}
	/* Create the EVP context (needed for the signign process). */
	context = EVP_MD_CTX_create();
	if (!context) {
		ret = crypto_host_err("EVP context creation failed");
		goto err_ctx_create;
	}
	EVP_MD_CTX_init(context);
	/* Compute the signature for the different image regions. */
	if (!EVP_SignInit(context, checksum_algo->calculate_sign())) {
		ret = crypto_host_err("Signer setup failed");
		goto err_sign;
	}
	for (i = 0; i < region_count; i++) {
		if (!EVP_SignUpdate(context, region[i].data, region[i].size)) {
			ret = crypto_host_err("Signing data failed");
			goto err_sign;
		}
	}
	if (!EVP_SignFinal(context, sig, sig_size, priv_key)) {
		ret = crypto_host_err("Could not obtain signature");
		goto err_sign;
	}
	*sigp = sig;
	debug("Got signature: %d bytes, maximum allowed %d\n", *sig_size, size);

#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
		(defined(LIBRESSL_VERSION_NUMBER) && \
		 LIBRESSL_VERSION_NUMBER < 0x02070000fL)
	EVP_MD_CTX_cleanup(context);
#else
	EVP_MD_CTX_reset(context);
#endif
	EVP_MD_CTX_destroy(context);

	return 0;

err_sign:
	/* TODO: should we do the CTX clean-up as well? */
	EVP_MD_CTX_destroy(context);
err_ctx_create:
	free(sig);
	return ret;
}

int crypto_host_sign(struct image_sign_info *info,
		     const struct image_region region[], int region_count,
		     uint8_t **sigp, uint *sig_len, int key_type,
		     int engine_flag)
{
	ENGINE *e = NULL;
	int ret;
	EVP_PKEY *privk;

	ret = crypto_host_init();
	if (ret)
		return ret;

	if (info->engine_id) {
		ret = crypto_host_engine_init(info->engine_id, &e, engine_flag);
		if (ret)
			goto err_engine;
	}
	/* Get private key (key memory is allocated). */
	ret = crypto_host_get_priv_key(info->keydir, info->keyname, e, &privk);
	if (ret)
		goto err_privk;
	/* Check if key is of the expected type. */
	if (EVP_PKEY_base_id(privk) != key_type) {
		fprintf(stderr, "Private key is of invalid type\n");
		ret = -EINVAL;
		goto err_sign;
	}
	/* Generate the signature (signature memory is allocated). */
	ret = crypto_host_sign_with_key(privk, info->checksum, region,
					region_count, sigp, sig_len);
	if (ret)
		goto err_sign;
err_sign:
	EVP_PKEY_free(privk);
err_privk:
	if (info->engine_id)
		crypto_host_engine_deinit(e);
err_engine:
	crypto_host_deinit();
	return ret;
}

int crypto_host_engine_init(const char *engine_id, ENGINE **pe, int engine_flag)
{
	ENGINE *e;
	int ret;

	ENGINE_load_builtin_engines();

	e = ENGINE_by_id(engine_id);
	if (!e) {
		fprintf(stderr, "Engine isn't available\n");
		ret = -1;
		goto err_engine_by_id;
	}

	if (!ENGINE_init(e)) {
		fprintf(stderr, "Couldn't initialize engine\n");
		ret = -1;
		goto err_engine_init;
	}

	if (!ENGINE_set_default(e, engine_flag)) {
		fprintf(stderr, "Couldn't set default engine\n");
		ret = -1;
		goto err_set_default;
	}

	*pe = e;

	return 0;

err_set_default:
	ENGINE_finish(e);
err_engine_init:
	ENGINE_free(e);
err_engine_by_id:
#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
		(defined(LIBRESSL_VERSION_NUMBER) && \
		 LIBRESSL_VERSION_NUMBER < 0x02070000fL)
	ENGINE_cleanup();
#endif
	return ret;
}

void crypto_host_engine_deinit(ENGINE *e)
{
	if (e) {
		ENGINE_finish(e);
		ENGINE_free(e);
	}
}

int crypto_host_add_verify_data(struct image_sign_info *info, void *keydest,
				const struct crypto_host_algo_hlpr *algo)
{
	int parent, node;
	char name[100];
	int ret;
	EVP_PKEY *evp_pubk;
	ENGINE *e = NULL;

	debug("%s: Getting verification data\n", __func__);
	if (info->engine_id) {
		ret = crypto_host_engine_init(info->engine_id, &e,
					      algo->engine_flag);
		if (ret)
			goto err_init_engine;
	}
	ret = crypto_host_get_pub_key(info->keydir, info->keyname, e,
				      &evp_pubk);
	if (ret)
		goto err_get_pub_key;

	/* Find/create '/signature' node. */
	parent = fdt_subnode_offset(keydest, 0, FIT_SIG_NODENAME);
	if (parent == -FDT_ERR_NOTFOUND)
		parent = fdt_add_subnode(keydest, 0, FIT_SIG_NODENAME);
	if (parent < 0) {
		ret = parent;
		fprintf(stderr, "Couldn't find/create signature node: %s\n",
			fdt_strerror(parent));
		goto exit;
	}

	/* Either create or overwrite the named key node */
	snprintf(name, sizeof(name), "key-%s", info->keyname);
	node = fdt_subnode_offset(keydest, parent, name);
	if (node == -FDT_ERR_NOTFOUND)
		node = fdt_add_subnode(keydest, parent, name);
	if (node < 0) {
		ret = node;
		fprintf(stderr,	"Couldn't find/create key subnode: %s\n",
			fdt_strerror(node));
		goto exit;
	}
	/* Add algo-specific public key properties to node. */
	ret = algo->add_pubk_data_to_node(evp_pubk, keydest, node);
	if (ret)
		goto exit;
	/* Add algo-independent properties. */
	ret = fdt_setprop_string(keydest, node, FIT_ALGO_PROP, info->name);
	if (ret)
		goto exit;
	if (info->require_keys)
		ret = fdt_setprop_string(keydest, node, "required",
					 info->require_keys);
exit:
	if (ret)
		ret = ret == -FDT_ERR_NOSPACE ? -ENOSPC : -EIO;
	EVP_PKEY_free(evp_pubk);
err_get_pub_key:
	if (info->engine_id)
		crypto_host_engine_deinit(e);
err_init_engine:
	return ret;
}
