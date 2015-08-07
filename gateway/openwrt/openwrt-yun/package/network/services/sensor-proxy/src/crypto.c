/*
 *  Created by Jun Fang on 15-7-24.
 *  Copyright (c) 2015年 Jun Fang. All rights reserved.
 */

#include <openssl/evp.h>
#include <openssl/aes.h>
#include <string.h>
#include "crypto.h"

uint8_t g_network_shared_key[DEVICE_KEY_SIZE];
uint8_t g_network_shared_key_version;

uint16_t g_seq_num;

static void generate_network_shared_key()
{
    RAND_bytes(g_network_shared_key, sizeof(g_network_shared_key));
    
    g_network_shared_key_version++;
    if (g_network_shared_key_version == 0) {
        g_network_shared_key_version++;
    }
    
    return;
}

uint8_t generate_master_key(sensor_session *session)
{
    uint8_t i = 0;
    
    if (session) {
        for (i = 0; i < DEVICE_PWD_SIZE; i++) {
            session->master_key[i] = session->pwd[i]^session->random[i];
        }
        return 1;
    }
    
    return 0;
}

uint8_t *get_network_shared_key()
{
    return g_network_shared_key;
}

uint8_t get_current_network_shared_key_version()
{
    return g_network_shared_key_version;
}

int32_t crypto_init()
{
    generate_network_shared_key();
    
    return 1;
}

uint32_t encrypt(sensor_session *session, uint8_t *plaintext, uint32_t plaintext_len, uint8_t *ciphertext)
{
    uint32_t len;
    uint32_t ciphertext_len;
    EVP_CIPHER_CTX ctx;
    
    if(!session) {
        return 0;
    }
    
    EVP_CIPHER_CTX_init(&ctx);
    
    EVP_EncryptInit_ex(&ctx, EVP_aes_128_cbc(), NULL, g_network_shared_key, session->random);
    
    /* Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(&ctx, ciphertext, &len, plaintext, plaintext_len)) {
        printf("EVP_EncryptUpdate error!\n");
        return 0;
    }
    
    ciphertext_len = len;
    
    /* Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(&ctx, ciphertext + len, &len)) {
        printf("EVP_EncryptFinal_ex error!\n");
        return 0;
    }
    
    ciphertext_len += len;
    
    return ciphertext_len;
}

uint32_t decrypt(sensor_session *session, uint8_t *ciphertext, uint32_t ciphertext_len, uint8_t *plaintext)
{
    uint32_t len;
    uint32_t plaintext_len;
    EVP_CIPHER_CTX ctx;
    
    if(!session) {
        return 0;
    }
    
    EVP_CIPHER_CTX_init(&ctx);
    
    EVP_DecryptInit_ex(&ctx, EVP_aes_128_cbc(), NULL, g_network_shared_key, session->random);
    
    if(1 != EVP_DecryptUpdate(&ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        printf("EVP_DecryptUpdate error!\n");
        return 0;
    }

    plaintext_len = len;
    
    /* Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(&ctx, plaintext + len, &len)) {
        printf("EVP_DecryptFinal_ex error!\n");
        return 0;
    }
    
    plaintext_len += len;
    
    return plaintext_len;
}

uint32_t create_security_error_msg(uint8_t *buf, uint32_t error_code, uint8_t key_version, uint16_t seq_num)
{
    security_error_msg_t *error_msg = (security_error_msg_t *)buf;
    
    error_msg->security_header.content_type = SECURITY_ERROR;
    error_msg->security_header.version = SECURITY_VERSION;
    error_msg->security_header.key_version = key_version;
    error_msg->security_header.seq = g_seq_num++;
    error_msg->security_header.len = sizeof(security_error_msg_t);
    error_msg->error_packet_seq = seq_num;
    error_msg->error_code = error_code;
    
    return sizeof(security_error_msg_t);
}

uint32_t create_security_server_hello_msg(uint8_t *buf, sensor_session *session)
{
    security_server_hello_msg_t *msg = (security_server_hello_msg_t *)buf;
    EVP_CIPHER_CTX ctx;
    uint32_t len;
    uint32_t ciphertext_len;
    
    if (!session) {
        return 0;
    }
    
    msg->security_header.content_type = SECURITY_SERVER_HELLO;
    msg->security_header.version = SECURITY_VERSION;
    msg->security_header.key_version = get_current_network_shared_key_version();
    msg->security_header.seq = g_seq_num++;
    msg->master_key_version = session->master_key_version;
    
    if(1 != EVP_EncryptInit_ex(&ctx, EVP_aes_256_cbc(), NULL, session->master_key, NULL)) {
        printf("EVP_EncryptInit_ex error\n");
        return 0;
    }
    
    if(1 != EVP_EncryptUpdate(&ctx, msg->data, &len, get_network_shared_key(), DEVICE_KEY_SIZE)) {
        printf("EVP_EncryptUpdate error\n");
        return 0;
    }
    
    ciphertext_len = len;
    
    /* Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(&ctx, msg->data + len, &len)) {
        printf("EVP_EncryptFinal_ex error\n");
        return 0;
    }
    ciphertext_len += len;
    
    msg->security_header.len = ciphertext_len + 1;
    
    return sizeof(security_server_hello_msg_t) + ciphertext_len;
}

uint32_t create_security_data_msg(uint8_t *buf, uint8_t *payload, uint32_t len)
{
    security_header_t *security_header = (security_header_t *)buf;
    
    security_header->content_type = SECURITY_DATA;
    security_header->version = SECURITY_VERSION;
    security_header->key_version = get_current_network_shared_key_version();
    security_header->len = len;
    security_header->seq = g_seq_num++;
    memcpy(buf + sizeof(security_header), payload, len);
    
    return sizeof(security_header_t) + len;
}

